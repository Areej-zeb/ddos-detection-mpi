#include "output.h"
#include "metrics.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

// Top IP tracking structure
typedef struct
{
    char ip[MAX_IP_LEN];
    int attack_count;
    double avg_confidence;
} TopAttacker;

// Print comprehensive performance report
void print_comprehensive_report(Metrics *m,
                                Alert *all_alerts, int alert_count,
                                BlockedIP *blocklist, int blocklist_count,
                                int num_ranks, double total_time_sec,
                                const char *dataset_file)
{
    // Calculate metrics
    double precision, recall, f1, fpr, accuracy;
    calculate_final_metrics(m, &precision, &recall, &f1, &fpr, &accuracy);

    // Track top attacking IPs
    TopAttacker top_ips[1000] = {0};
    int unique_attackers = 0;

    for (int i = 0; i < alert_count; i++)
    {
        int found = 0;
        for (int j = 0; j < unique_attackers; j++)
        {
            if (strcmp(top_ips[j].ip, all_alerts[i].src_ip) == 0)
            {
                top_ips[j].attack_count++;
                top_ips[j].avg_confidence += all_alerts[i].confidence;
                found = 1;
                break;
            }
        }
        if (!found && unique_attackers < 1000)
        {
            strcpy(top_ips[unique_attackers].ip, all_alerts[i].src_ip);
            top_ips[unique_attackers].attack_count = 1;
            top_ips[unique_attackers].avg_confidence = all_alerts[i].confidence;
            unique_attackers++;
        }
    }

    // Calculate average confidence and sort by attack count
    for (int i = 0; i < unique_attackers; i++)
    {
        top_ips[i].avg_confidence /= top_ips[i].attack_count;
    }

    // Simple bubble sort for top 10
    for (int i = 0; i < unique_attackers - 1; i++)
    {
        for (int j = 0; j < unique_attackers - i - 1; j++)
        {
            if (top_ips[j].attack_count < top_ips[j + 1].attack_count)
            {
                TopAttacker temp = top_ips[j];
                top_ips[j] = top_ips[j + 1];
                top_ips[j + 1] = temp;
            }
        }
    }

    // Calculate throughput metrics
    double total_bytes = (double)m->total_flows * 1500; // Estimate avg packet size
    double packets_per_sec = m->total_flows / total_time_sec;
    double mbps = (total_bytes * 8) / (total_time_sec * 1000000);
    double gbps = mbps / 1000;

    // Print formatted report
    printf("\n");
    printf("=====================================================\n");
    printf("    MPI DDoS Prediction System (OPTIMIZED)\n");
    printf("=====================================================\n");
    printf("Processes: %d (1 master + %d workers)\n", num_ranks, num_ranks - 1);
    printf("Input: %s\n", strrchr(dataset_file, '/') ? strrchr(dataset_file, '/') + 1 : dataset_file);
    printf("=====================================================\n\n");

    printf("[Master] Dataset: %d rows\n\n", m->total_flows);

    printf("===================================================================================\n");
    printf("                 COMPREHENSIVE PERFORMANCE ANALYSIS METRICS\n");
    printf("===================================================================================\n\n");

    // 1. Dataset Overview
    printf("1. DATASET OVERVIEW:\n");
    printf("   Total rows processed:                %d\n", m->total_flows);
    printf("   DDoS traffic:                        %d (%.2f%%)\n",
           m->ddos_flows, (m->ddos_flows * 100.0) / m->total_flows);
    printf("   Benign traffic:                      %d (%.2f%%)\n",
           m->benign_flows, (m->benign_flows * 100.0) / m->total_flows);
    printf("   DDoS:Benign ratio:                   %.2f:1\n",
           (double)m->ddos_flows / (m->benign_flows > 0 ? m->benign_flows : 1));
    printf("   Total alerts generated:              %d (%.2f%%)\n",
           alert_count, (alert_count * 100.0) / m->total_flows);
    printf("   Processing time (wall):              %.2f sec\n", total_time_sec);
    printf("   Number of workers:                   %d\n\n", num_ranks - 1);

    // 2. Throughput
    printf("2. THROUGHPUT (System Processing Capacity):\n");
    printf("   Packets/second:                      %.2f pps\n", packets_per_sec);
    printf("   Megabits/second:                     %.2f Mbps\n", mbps);
    printf("   Gigabits/second:                     %.4f Gbps\n", gbps);
    printf("   Total bytes processed:               %.0f bytes\n\n", total_bytes);

    // 3. Latency (Local and Global)
    double avg_latency = m->total_flows > 0 ? m->total_latency_ms / m->total_flows : 0.0;
    double avg_local = m->local_detection_time_ms / (num_ranks - 1); // Per worker
    double avg_global = m->global_detection_time_ms / (num_ranks - 1);
    double mpi_overhead_pct = (m->mpi_communication_time_ms / avg_global) * 100.0;

    printf("3. LATENCY & MPI OVERHEAD:\n");
    printf("   Per-packet latency (avg):            %.4f ms\n", avg_latency);
    printf("   Per-packet latency (P95):            %.4f ms\n", m->latency_p95_ms);
    printf("   Per-packet latency (P99):            %.4f ms\n", m->latency_p99_ms);
    printf("   Detection lead time:                 %.2f ms\n", m->detection_lead_time_ms);
    printf("   Local detection time (avg/worker):   %.2f ms\n", avg_local);
    printf("   Global detection time (avg/worker):  %.2f ms\n", avg_global);
    printf("   MPI communication overhead:          %.2f ms (%.2f%%)\n\n",
           m->mpi_communication_time_ms, mpi_overhead_pct);

    // 4. Resource Utilization
    printf("4. RESOURCE UTILIZATION:\n");
    printf("   CPU usage (avg):                     %.2f%%\n", m->cpu_usage_percent / (num_ranks - 1));
    printf("   Memory usage (total):                %.2f MB\n", m->memory_usage_mb);
    printf("   GPU:                                 CUDA Disabled (Stability)\n");
    printf("   Network I/O:                         MPI-based\n\n");

    // 5. Blocking Effectiveness & Mechanisms
    printf("5. BLOCKING EFFECTIVENESS & MECHANISMS:\n");
    printf("   Mechanisms:                          RTBH, FlowSpec, ACL\n");
    printf("   Total flows analyzed:                %d\n", m->total_flows);
    printf("   Attack traffic detected:             %d (%.2f%%)\n",
           m->true_positives, (m->true_positives * 100.0) / m->total_flows);
    printf("   Attack traffic dropped:              %.2f%%\n", m->attack_traffic_dropped_pct);
    printf("   Flows to be blocked:                 %d (%.2f%%)\n",
           m->blocked_flows, (m->blocked_flows * 100.0) / m->total_flows);
    printf("   Collateral damage:                   %d flows (%.4f%%)\n\n",
           m->legitimate_blocked, (m->legitimate_blocked * 100.0) / m->total_flows);

    // 6. Detection Accuracy
    printf("6. DETECTION ACCURACY:\n");
    printf("   True Positives (TP):                 %d\n", m->true_positives);
    printf("   False Positives (FP):                %d\n", m->false_positives);
    printf("   True Negatives (TN):                 %d\n", m->true_negatives);
    printf("   False Negatives (FN):                %d\n", m->false_negatives);
    printf("   Precision:                           %.4f (%.2f%%)\n", precision, precision * 100);
    printf("   Recall/TPR:                          %.4f (%.2f%%)\n", recall, recall * 100);
    printf("   F1-Score:                            %.4f\n", f1);
    printf("   False Positive Rate:                 %.4f (%.2f%%)\n", fpr, fpr * 100);
    printf("   Accuracy:                            %.4f (%.2f%%)\n\n", accuracy, accuracy * 100);

    // 7. Output Files & Blocking Rules
    printf("7. OUTPUT FILES & BLOCKING RULES:\n");
    printf("   Alerts CSV:                          ./output/alerts_output.csv\n");
    printf("   Performance metrics CSV:             performance_metrics.csv\n");
    printf("   RTBH Blocklist:                      blocklist_%d_ranks.txt\n", num_ranks);
    printf("   FlowSpec BGP rules:                  flowspec_rules.txt\n");
    printf("   ACL rules (Cisco):                   acl_rules.txt\n");
    printf("===================================================================================\n\n");

    // Top Attacking IPs
    printf("\n=== TOP ATTACKING IPs ===\n");
    printf("%-20s %-15s %-15s\n", "Source IP", "Attacks", "Avg Prob");
    int top_count = unique_attackers < 10 ? unique_attackers : 10;
    for (int i = 0; i < top_count; i++)
    {
        printf("%-20s %-15d %.4f\n",
               top_ips[i].ip,
               top_ips[i].attack_count,
               top_ips[i].avg_confidence);
    }
    printf("\n");

    printf("Blocklist: ./output/blocklist_alerts_output.txt\n");
    printf("Firewall rules: ./output/firewall_rules_alerts_output.sh\n");
    printf("===================================================================================\n\n");
}
