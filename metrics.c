#include "metrics.h"
#include "blocking.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

// Initialize metrics
void init_metrics(Metrics *m)
{
    memset(m, 0, sizeof(Metrics));
}

// Update metrics based on ground truth vs detection/blocking
void update_metrics(Metrics *m, FlowRecord *flows, int count, Alert *alerts, int alert_count, BlockedIP *blocklist, int blocklist_count)
{
    m->total_flows += count;

    // Build a quick lookup for detected IPs
    char detected_ips[100][MAX_IP_LEN];
    int detected_count = 0;
    for (int i = 0; i < alert_count && detected_count < 100; i++)
    {
        int already_added = 0;
        for (int j = 0; j < detected_count; j++)
        {
            if (strcmp(detected_ips[j], alerts[i].src_ip) == 0)
            {
                already_added = 1;
                break;
            }
        }
        if (!already_added)
        {
            strcpy(detected_ips[detected_count], alerts[i].src_ip);
            detected_count++;
        }
    }

    // Check each flow
    for (int i = 0; i < count; i++)
    {
        int is_attack = flows[i].is_attack;
        int was_detected = 0;
        int was_blocked = 0;

        // Count DDoS vs Benign based on ground truth
        if (is_attack)
        {
            m->ddos_flows++;
        }
        else
        {
            m->benign_flows++;
        }

        // Check if this IP was detected
        for (int j = 0; j < detected_count; j++)
        {
            if (strcmp(flows[i].src_ip, detected_ips[j]) == 0)
            {
                was_detected = 1;
                break;
            }
        }

        // Check if this IP was blocked
        for (int j = 0; j < blocklist_count; j++)
        {
            if (strcmp(flows[i].src_ip, blocklist[j].ip) == 0)
            {
                was_blocked = 1;
                m->blocked_flows++;
                break;
            }
        }

        // Update confusion matrix
        if (is_attack && was_detected)
        {
            m->true_positives++;
        }
        else if (!is_attack && was_detected)
        {
            m->false_positives++;
        }
        else if (!is_attack && !was_detected)
        {
            m->true_negatives++;
        }
        else if (is_attack && !was_detected)
        {
            m->false_negatives++;
        }

        // Track collateral damage (legitimate traffic blocked)
        if (!is_attack && was_blocked)
        {
            m->legitimate_blocked++;
        }
    }
}

// Calculate precision, recall, F1, FPR, accuracy
void calculate_final_metrics(Metrics *m, double *precision, double *recall, double *f1, double *fpr, double *accuracy)
{
    int tp = m->true_positives;
    int fp = m->false_positives;
    int tn = m->true_negatives;
    int fn = m->false_negatives;

    *precision = (tp + fp > 0) ? (double)tp / (tp + fp) : 0.0;
    *recall = (tp + fn > 0) ? (double)tp / (tp + fn) : 0.0;
    *f1 = (*precision + *recall > 0) ? 2 * (*precision) * (*recall) / (*precision + *recall) : 0.0;
    *fpr = (fp + tn > 0) ? (double)fp / (fp + tn) : 0.0;
    *accuracy = (tp + tn + fp + fn > 0) ? (double)(tp + tn) / (tp + tn + fp + fn) : 0.0;
}

// Print metrics
void print_metrics(Metrics *m, int rank)
{
    double precision, recall, f1, fpr, accuracy;
    calculate_final_metrics(m, &precision, &recall, &f1, &fpr, &accuracy);

    printf("\n=== METRICS (Rank %d) ===\n", rank);
    printf("Total Flows Processed: %d\n", m->total_flows);
    printf("True Positives: %d\n", m->true_positives);
    printf("False Positives: %d\n", m->false_positives);
    printf("True Negatives: %d\n", m->true_negatives);
    printf("False Negatives: %d\n", m->false_negatives);
    printf("Precision: %.4f\n", precision);
    printf("Recall (TPR): %.4f\n", recall);
    printf("F1 Score: %.4f\n", f1);
    printf("False Positive Rate: %.4f\n", fpr);
    printf("Accuracy: %.4f\n", accuracy);
    printf("Blocked Flows: %d\n", m->blocked_flows);
    printf("Legitimate Blocked (Collateral): %d\n", m->legitimate_blocked);
    printf("Avg Latency: %.4f ms\n", m->total_flows > 0 ? m->total_latency_ms / m->total_flows : 0.0);
    printf("Throughput: %.2f flows/sec\n", m->throughput_flows_per_sec);
    printf("Detection Lead Time: %.4f ms\n", m->detection_lead_time_ms);
    printf("========================\n");
}

// Save metrics to file
void save_metrics_to_file(Metrics *m, const char *filename, int num_ranks)
{
    FILE *fp = fopen(filename, "a");
    if (!fp)
    {
        printf("Warning: Could not open metrics file %s\n", filename);
        return;
    }

    double precision, recall, f1, fpr, accuracy;
    calculate_final_metrics(m, &precision, &recall, &f1, &fpr, &accuracy);

    time_t now = time(NULL);
    fprintf(fp, "\n=== Experiment Run: %s", ctime(&now));
    fprintf(fp, "MPI Ranks: %d\n", num_ranks);
    fprintf(fp, "Total Flows: %d\n", m->total_flows);
    fprintf(fp, "TP=%d, FP=%d, TN=%d, FN=%d\n", m->true_positives, m->false_positives, m->true_negatives, m->false_negatives);
    fprintf(fp, "Precision: %.4f, Recall: %.4f, F1: %.4f\n", precision, recall, f1);
    fprintf(fp, "FPR: %.4f, Accuracy: %.4f\n", fpr, accuracy);
    fprintf(fp, "Blocked: %d, Collateral: %d\n", m->blocked_flows, m->legitimate_blocked);
    fprintf(fp, "Throughput: %.2f flows/sec\n", m->throughput_flows_per_sec);
    fprintf(fp, "Avg Latency: %.4f ms\n", m->total_flows > 0 ? m->total_latency_ms / m->total_flows : 0.0);
    fprintf(fp, "Detection Lead Time: %.4f ms\n", m->detection_lead_time_ms);
    fprintf(fp, "=====================================\n");

    fclose(fp);
}
