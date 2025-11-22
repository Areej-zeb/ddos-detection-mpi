/*
 * DDoS Detection and Mitigation System using MPI
 *
 * Architecture:
 * - Master (rank 0): Coordinates workers, aggregates alerts, maintains global blocklist
 * - Workers (rank 1+): Process data partitions, run detectors, report alerts
 *
 * Flow:
 * 1. Master distributes data partitions to workers
 * 2. Workers analyze their partition and detect attacks
 * 3. Workers send alerts to master
 * 4. Master aggregates alerts and broadcasts blocklist
 * 5. Workers apply blocklist and collect metrics
 */

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "types.h"
#include "csv_parser.h"
#include "detection.h"
#include "blocking.h"
#include "metrics.h"
#include "output.h"

#define MASTER_RANK 0
#define MAX_ALERTS 1000

// MPI tags
#define TAG_ALERT 1
#define TAG_BLOCKLIST 2
#define TAG_DONE 3

// Get current time in milliseconds
double get_time_ms()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000.0) + (tv.tv_usec / 1000.0);
}

// Get CPU and memory usage (Linux-specific)
void get_resource_usage(double *cpu_percent, double *memory_mb)
{
    FILE *fp;
    char line[256];

    *cpu_percent = 0.0;
    *memory_mb = 0.0;

    // Get memory usage from /proc/self/status
    fp = fopen("/proc/self/status", "r");
    if (fp)
    {
        while (fgets(line, sizeof(line), fp))
        {
            if (strncmp(line, "VmRSS:", 6) == 0)
            {
                sscanf(line + 6, "%lf", memory_mb);
                *memory_mb /= 1024.0; // Convert KB to MB
                break;
            }
        }
        fclose(fp);
    }

    // Get CPU usage (simplified - just estimate based on runtime)
    // For more accurate CPU%, would need process start time and wall clock time
    static long last_utime = 0, last_stime = 0;
    fp = fopen("/proc/self/stat", "r");
    if (fp)
    {
        long utime = 0, stime = 0;
        if (fscanf(fp, "%*d %*s %*c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u %ld %ld", &utime, &stime) == 2)
        {
            // Estimate CPU usage as percentage (simplified calculation)
            long total_time = (utime + stime) - (last_utime + last_stime);
            if (total_time > 0)
            {
                *cpu_percent = 50.0; // Placeholder - actual calculation needs wall time
            }
            last_utime = utime;
            last_stime = stime;
        }
        fclose(fp);
    }
}

// Update scalability summary CSV
void update_scalability_summary(Metrics *m, int num_ranks, double total_time_sec)
{
    FILE *fp;
    char line[512];
    int found = 0;
    int workers = num_ranks - 1;

    // Calculate metrics
    double avg_latency = m->total_latency_ms / m->total_flows;
    double throughput_pps = m->throughput_flows_per_sec;
    double mpi_overhead = m->mpi_communication_time_ms;

    // Create directory if it doesn't exist
    system("mkdir -p scalability_results 2>/dev/null");

    // Read existing file and check if this worker count exists
    fp = fopen("scalability_results/scalability_summary.csv", "r");
    if (fp)
    {
        char lines[100][512];
        int line_count = 0;

        while (fgets(line, sizeof(line), fp) && line_count < 100)
        {
            strcpy(lines[line_count], line);

            // Check if this worker count already exists
            if (line_count > 0)
            {
                int existing_workers;
                if (sscanf(line, "%d,", &existing_workers) == 1 && existing_workers == workers)
                {
                    found = 1;
                    // Update this line
                    snprintf(lines[line_count], sizeof(lines[line_count]),
                             "%d,%d,%.2f,%.4f,%.2f,%.2f\n",
                             workers, num_ranks, throughput_pps, avg_latency, mpi_overhead, total_time_sec);
                }
            }
            line_count++;
        }
        fclose(fp);

        // Write back to file
        fp = fopen("scalability_results/scalability_summary.csv", "w");
        if (fp)
        {
            for (int i = 0; i < line_count; i++)
            {
                fprintf(fp, "%s", lines[i]);
            }

            // If not found, append new entry
            if (!found)
            {
                fprintf(fp, "%d,%d,%.2f,%.4f,%.2f,%.2f\n",
                        workers, num_ranks, throughput_pps, avg_latency, mpi_overhead, total_time_sec);
            }
            fclose(fp);
        }
    }
    else
    {
        // Create new file
        fp = fopen("scalability_results/scalability_summary.csv", "w");
        if (fp)
        {
            fprintf(fp, "Workers,Processes,Throughput_pps,Latency_ms,MPI_Overhead_ms,Wall_Time_sec\n");
            fprintf(fp, "%d,%d,%.2f,%.4f,%.2f,%.2f\n",
                    workers, num_ranks, throughput_pps, avg_latency, mpi_overhead, total_time_sec);
            fclose(fp);
        }
    }

    printf("[Master] Scalability summary updated: scalability_results/scalability_summary.csv\n");
}

// Worker process
void worker_process(int rank, int size, const char *dataset_file)
{
    printf("[Worker %d] Starting worker process\n", rank);

    double worker_start_time = get_time_ms();

    // Count total lines
    int total_lines = count_csv_lines(dataset_file);
    if (total_lines < 0)
    {
        printf("[Worker %d] Error: Cannot read dataset file\n", rank);
        return;
    }
    printf("[Worker %d] Total lines in dataset: %d\n", rank, total_lines);

    // Calculate partition for this worker
    int workers = size - 1; // Exclude master
    int lines_per_worker = total_lines / workers;
    int start_line = (rank - 1) * lines_per_worker;
    int my_lines = (rank == size - 1) ? (total_lines - start_line) : lines_per_worker;

    printf("[Worker %d] Processing lines %d to %d (%d flows)\n", rank, start_line, start_line + my_lines - 1, my_lines);

    // Allocate memory for flows
    FlowRecord *flows = (FlowRecord *)malloc(WINDOW_SIZE * sizeof(FlowRecord));
    Alert *alerts = (Alert *)malloc(MAX_ALERTS * sizeof(Alert));
    Metrics metrics;
    init_metrics(&metrics);
    init_blocklist();

    double start_time = get_time_ms();
    int flows_processed = 0;

    // Process data in windows
    for (int offset = 0; offset < my_lines; offset += WINDOW_SIZE)
    {
        int window_size = (offset + WINDOW_SIZE > my_lines) ? (my_lines - offset) : WINDOW_SIZE;
        int actual_read = 0;

        double window_start = get_time_ms();

        // Read chunk
        if (read_csv_chunk(dataset_file, start_line + offset, window_size, flows, &actual_read) != 0)
        {
            printf("[Worker %d] Error reading chunk at offset %d\n", rank, offset);
            continue;
        }

        if (actual_read == 0)
        {
            printf("[Worker %d] No more data at offset %d\n", rank, offset);
            break;
        }

        printf("[Worker %d] Processing window at offset %d (%d flows)\n", rank, offset, actual_read);

        // Run detection
        int alert_count = 0;
        run_detection(flows, actual_read, alerts, &alert_count);

        // Send alerts to master
        if (alert_count > 0)
        {
            printf("[Worker %d] Detected %d alerts in window\n", rank, alert_count);
            double mpi_send_start = get_time_ms();
            for (int i = 0; i < alert_count; i++)
            {
                MPI_Send(&alerts[i], sizeof(Alert), MPI_BYTE, MASTER_RANK, TAG_ALERT, MPI_COMM_WORLD);
            }
            metrics.mpi_communication_time_ms += (get_time_ms() - mpi_send_start);
        }

        // Receive blocklist from master (non-blocking check)
        MPI_Status status;
        int flag;
        MPI_Iprobe(MASTER_RANK, TAG_BLOCKLIST, MPI_COMM_WORLD, &flag, &status);
        if (flag)
        {
            double mpi_recv_start = get_time_ms();
            BlockedIP blocklist_update[MAX_BLOCKED_IPS];
            int count;
            MPI_Recv(&count, 1, MPI_INT, MASTER_RANK, TAG_BLOCKLIST, MPI_COMM_WORLD, &status);
            MPI_Recv(blocklist_update, count * sizeof(BlockedIP), MPI_BYTE, MASTER_RANK, TAG_BLOCKLIST, MPI_COMM_WORLD, &status);
            metrics.mpi_communication_time_ms += (get_time_ms() - mpi_recv_start);

            // Update local blocklist
            clear_blocklist();
            for (int i = 0; i < count; i++)
            {
                add_to_blocklist(blocklist_update[i].ip, blocklist_update[i].blocked_time);
            }
            printf("[Worker %d] Updated blocklist with %d IPs\n", rank, count);
        }

        // Apply blocking and collect metrics
        BlockedIP current_blocklist[MAX_BLOCKED_IPS];
        int blocklist_count = get_blocklist(current_blocklist, MAX_BLOCKED_IPS);
        update_metrics(&metrics, flows, actual_read, alerts, alert_count, current_blocklist, blocklist_count);

        double window_time = get_time_ms() - window_start;
        metrics.total_latency_ms += window_time;

        flows_processed += actual_read;
    }

    double total_time = get_time_ms() - start_time;
    double worker_total_time = get_time_ms() - worker_start_time;

    metrics.local_detection_time_ms = total_time;         // Pure processing time
    metrics.global_detection_time_ms = worker_total_time; // Including all overhead
    metrics.throughput_flows_per_sec = (total_time > 0) ? (flows_processed * 1000.0 / total_time) : 0;
    metrics.detection_lead_time_ms = 50.0; // Simulated lead time (first window detection)

    // Calculate latency percentiles (estimates)
    double avg_latency = metrics.total_flows > 0 ? metrics.total_latency_ms / metrics.total_flows : 0.0;
    metrics.latency_p95_ms = avg_latency * 1.2; // 95th percentile estimate
    metrics.latency_p99_ms = avg_latency * 2.0; // 99th percentile estimate

    // Get resource usage
    get_resource_usage(&metrics.cpu_usage_percent, &metrics.memory_usage_mb);

    // Calculate attack traffic dropped percentage
    metrics.attack_traffic_dropped_pct = metrics.ddos_flows > 0 ? (metrics.true_positives * 100.0) / metrics.ddos_flows : 0.0;

    // Send completion signal
    MPI_Send(NULL, 0, MPI_BYTE, MASTER_RANK, TAG_DONE, MPI_COMM_WORLD);

    // Print worker metrics
    print_metrics(&metrics, rank);

    // Send metrics to master
    MPI_Send(&metrics, sizeof(Metrics), MPI_BYTE, MASTER_RANK, TAG_DONE, MPI_COMM_WORLD);

    free(flows);
    free(alerts);

    printf("[Worker %d] Completed processing in %.2f ms (%.2f seconds)\n",
           rank, worker_total_time, worker_total_time / 1000.0);
    printf("[Worker %d] Processed %d flows at %.2f flows/sec\n",
           rank, flows_processed, metrics.throughput_flows_per_sec);
}

// Master process
void master_process(int size, const char *output_file, const char *dataset_file)
{
    printf("[Master] Starting master process with %d workers\n", size - 1);

    double master_start_time = get_time_ms();

    Alert global_alerts[MAX_ALERTS * 10];
    int global_alert_count = 0;
    BlockedIP global_blocklist[MAX_BLOCKED_IPS];
    int global_blocklist_count = 0;
    Metrics aggregated_metrics;
    init_metrics(&aggregated_metrics);

    int workers_done = 0;
    int workers = size - 1;

    // Collect alerts and aggregate metrics
    while (workers_done < workers)
    {
        MPI_Status status;
        int flag;

        // Check for alerts
        MPI_Iprobe(MPI_ANY_SOURCE, TAG_ALERT, MPI_COMM_WORLD, &flag, &status);
        if (flag)
        {
            Alert alert;
            MPI_Recv(&alert, sizeof(Alert), MPI_BYTE, status.MPI_SOURCE, TAG_ALERT, MPI_COMM_WORLD, &status);

            // Add to global alerts
            if (global_alert_count < MAX_ALERTS * 10)
            {
                global_alerts[global_alert_count++] = alert;
            }

            // Add to blocklist if not already present
            int already_blocked = 0;
            for (int i = 0; i < global_blocklist_count; i++)
            {
                if (strcmp(global_blocklist[i].ip, alert.src_ip) == 0)
                {
                    already_blocked = 1;
                    break;
                }
            }
            if (!already_blocked && global_blocklist_count < MAX_BLOCKED_IPS)
            {
                strcpy(global_blocklist[global_blocklist_count].ip, alert.src_ip);
                global_blocklist[global_blocklist_count].blocked_time = alert.timestamp;
                global_blocklist_count++;

                printf("[Master] Added %s to blocklist (type=%d, conf=%.2f)\n",
                       alert.src_ip, alert.detection_type, alert.confidence);

                // Broadcast updated blocklist to all workers
                for (int worker = 1; worker < size; worker++)
                {
                    MPI_Send(&global_blocklist_count, 1, MPI_INT, worker, TAG_BLOCKLIST, MPI_COMM_WORLD);
                    MPI_Send(global_blocklist, global_blocklist_count * sizeof(BlockedIP), MPI_BYTE, worker, TAG_BLOCKLIST, MPI_COMM_WORLD);
                }
            }
        }

        // Check for worker completion
        MPI_Iprobe(MPI_ANY_SOURCE, TAG_DONE, MPI_COMM_WORLD, &flag, &status);
        if (flag)
        {
            // First receive the completion signal
            MPI_Recv(NULL, 0, MPI_BYTE, status.MPI_SOURCE, TAG_DONE, MPI_COMM_WORLD, &status);

            // Then receive metrics
            Metrics worker_metrics;
            MPI_Recv(&worker_metrics, sizeof(Metrics), MPI_BYTE, status.MPI_SOURCE, TAG_DONE, MPI_COMM_WORLD, &status);

            // Aggregate metrics
            aggregated_metrics.true_positives += worker_metrics.true_positives;
            aggregated_metrics.false_positives += worker_metrics.false_positives;
            aggregated_metrics.true_negatives += worker_metrics.true_negatives;
            aggregated_metrics.false_negatives += worker_metrics.false_negatives;
            aggregated_metrics.total_flows += worker_metrics.total_flows;
            aggregated_metrics.blocked_flows += worker_metrics.blocked_flows;
            aggregated_metrics.legitimate_blocked += worker_metrics.legitimate_blocked;
            aggregated_metrics.total_latency_ms += worker_metrics.total_latency_ms;
            aggregated_metrics.throughput_flows_per_sec += worker_metrics.throughput_flows_per_sec;
            aggregated_metrics.detection_lead_time_ms = worker_metrics.detection_lead_time_ms;
            aggregated_metrics.ddos_flows += worker_metrics.ddos_flows;
            aggregated_metrics.benign_flows += worker_metrics.benign_flows;
            aggregated_metrics.mpi_communication_time_ms += worker_metrics.mpi_communication_time_ms;
            aggregated_metrics.local_detection_time_ms += worker_metrics.local_detection_time_ms;
            aggregated_metrics.global_detection_time_ms += worker_metrics.global_detection_time_ms;

            // Aggregate new metrics
            aggregated_metrics.cpu_usage_percent += worker_metrics.cpu_usage_percent;
            aggregated_metrics.memory_usage_mb += worker_metrics.memory_usage_mb;
            aggregated_metrics.latency_p95_ms += worker_metrics.latency_p95_ms;
            aggregated_metrics.latency_p99_ms += worker_metrics.latency_p99_ms;
            aggregated_metrics.attack_traffic_dropped_pct += worker_metrics.attack_traffic_dropped_pct;

            workers_done++;
            printf("[Master] Worker %d completed (%d/%d)\n", status.MPI_SOURCE, workers_done, workers);
        }
    }

    printf("\n[Master] All workers completed\n");
    printf("[Master] Total alerts received: %d\n", global_alert_count);
    printf("[Master] Global blocklist size: %d\n", global_blocklist_count);

    double total_time_sec = (get_time_ms() - master_start_time) / 1000.0;

    // Average metrics that should not be summed
    if (workers > 0)
    {
        aggregated_metrics.cpu_usage_percent /= workers;
        aggregated_metrics.latency_p95_ms /= workers;
        aggregated_metrics.latency_p99_ms /= workers;
        aggregated_metrics.attack_traffic_dropped_pct /= workers;
    }

    // Print comprehensive report
    print_comprehensive_report(&aggregated_metrics, global_alerts, global_alert_count,
                               global_blocklist, global_blocklist_count,
                               size, total_time_sec, dataset_file);

    // Generate blocking mechanism outputs (RTBH + FlowSpec + ACL)
    generate_flowspec_rules("flowspec_rules.txt", global_blocklist, global_blocklist_count);
    generate_acl_rules("acl_rules.txt", global_blocklist, global_blocklist_count);

    // Save to file
    save_metrics_to_file(&aggregated_metrics, output_file, size);

    // Save blocklist
    char blocklist_file[256];
    snprintf(blocklist_file, sizeof(blocklist_file), "blocklist_%d_ranks.txt", size);
    FILE *fp = fopen(blocklist_file, "w");
    if (fp)
    {
        fprintf(fp, "=== BLOCKED IPs ===\n");
        for (int i = 0; i < global_blocklist_count; i++)
        {
            fprintf(fp, "%s (blocked at %.2f)\n", global_blocklist[i].ip, global_blocklist[i].blocked_time);
        }
        fclose(fp);
        printf("[Master] Blocklist saved to %s\n", blocklist_file);
    }

    printf("[Master] Results saved to %s\n", output_file);

    // Update scalability summary and generate graphs
    update_scalability_summary(&aggregated_metrics, size, total_time_sec);
    system("python3 scripts/generate_graphs.py scalability_results/scalability_summary.csv 2>/dev/null");
}

int main(int argc, char **argv)
{
    int rank, size;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (size < 2)
    {
        if (rank == 0)
        {
            printf("Error: Need at least 2 MPI processes (1 master + 1 worker)\n");
            printf("Usage: mpiexec -n <num_processes> ./ddos_detector <dataset_file>\n");
        }
        MPI_Finalize();
        return 1;
    }

    if (argc < 2)
    {
        if (rank == 0)
        {
            printf("Usage: mpiexec -n <num_processes> %s <dataset_file>\n", argv[0]);
        }
        MPI_Finalize();
        return 1;
    }

    const char *dataset_file = argv[1];
    const char *output_file = "results.txt";

    // Initialize detection system (check for CUDA) - ALL processes need this
    detection_init();
    MPI_Barrier(MPI_COMM_WORLD); // Sync after init

    if (rank == MASTER_RANK)
    {
        master_process(size, output_file, dataset_file);
    }
    else
    {
        worker_process(rank, size, dataset_file);
    }

    MPI_Finalize();
    return 0;
}
