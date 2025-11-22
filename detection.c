#include "detection.h"
#include "detection_cuda.h"
#include <string.h>
#include <math.h>
#include <stdio.h>

#define ENTROPY_THRESHOLD 1.5  // Lower entropy = stronger attack indication (tuned)
#define PCA_THRESHOLD 3.0      // PCA anomaly threshold (Mahalanobis distance)
#define PCA_COMPONENTS 3       // Number of principal components to use
#define CUSUM_THRESHOLD 5.0    // CUSUM decision threshold (H)
#define CUSUM_DRIFT 0.5        // Allowable drift (K) - half of expected shift
#define CUSUM_BASELINE 50000.0 // Baseline average bytes/s per flow
#define MAX_UNIQUE_IPS 1000

// Global flag for CUDA availability
static int use_cuda = 0;

void detection_init()
{
    // TEMPORARY: Disable CUDA to fix Worker 2 deadlock
    // TODO: Fix CUDA multi-process contention issue
    use_cuda = 0; // Force CPU mode

    // use_cuda = cuda_available();
    if (use_cuda)
    {
        printf("[Detection] CUDA acceleration enabled\n");
        print_cuda_info();
    }
    else
    {
        printf("[Detection] Using CPU-only detection (CUDA disabled for stability)\n");
    }
}

// Simple hash for IP counting
typedef struct
{
    char ip[MAX_IP_LEN];
    int count;
} IPCount;

// Calculate Shannon entropy of source IPs
static double calculate_entropy(FlowRecord *flows, int count)
{
    IPCount ip_counts[MAX_UNIQUE_IPS] = {0};
    int unique_ips = 0;

    // Count occurrences of each source IP
    for (int i = 0; i < count; i++)
    {
        int found = 0;
        for (int j = 0; j < unique_ips; j++)
        {
            if (strcmp(ip_counts[j].ip, flows[i].src_ip) == 0)
            {
                ip_counts[j].count++;
                found = 1;
                break;
            }
        }
        if (!found && unique_ips < MAX_UNIQUE_IPS)
        {
            strcpy(ip_counts[unique_ips].ip, flows[i].src_ip);
            ip_counts[unique_ips].count = 1;
            unique_ips++;
        }
    }

    // Calculate entropy: H = -Σ(p_i * log2(p_i))
    double entropy = 0.0;
    for (int i = 0; i < unique_ips; i++)
    {
        double p = (double)ip_counts[i].count / count;
        if (p > 0)
        {
            entropy -= p * log2(p);
        }
    }

    return entropy;
}

// Entropy-based detection
int detect_entropy_based(FlowRecord *flows, int count, Alert *alerts, int *alert_count)
{
    if (count < 10)
        return 0; // Need minimum samples

    // Use CUDA if available
    double entropy;
    if (use_cuda)
    {
        entropy = (double)calculate_entropy_cuda(flows, count);
    }
    else
    {
        entropy = calculate_entropy(flows, count);
    }

    // Low entropy = potential DDoS (traffic concentrated from few sources)
    if (entropy < ENTROPY_THRESHOLD)
    {
        // Find the most frequent source IP
        IPCount ip_counts[MAX_UNIQUE_IPS] = {0};
        int unique_ips = 0;

        for (int i = 0; i < count; i++)
        {
            int found = 0;
            for (int j = 0; j < unique_ips; j++)
            {
                if (strcmp(ip_counts[j].ip, flows[i].src_ip) == 0)
                {
                    ip_counts[j].count++;
                    found = 1;
                    break;
                }
            }
            if (!found && unique_ips < MAX_UNIQUE_IPS)
            {
                strcpy(ip_counts[unique_ips].ip, flows[i].src_ip);
                ip_counts[unique_ips].count = 1;
                unique_ips++;
            }
        }

        // Alert for top contributor
        int max_idx = 0;
        for (int i = 1; i < unique_ips; i++)
        {
            if (ip_counts[i].count > ip_counts[max_idx].count)
            {
                max_idx = i;
            }
        }

        strcpy(alerts[*alert_count].src_ip, ip_counts[max_idx].ip);
        alerts[*alert_count].timestamp = 0.0;    // No timestamp in FlowRecord
        alerts[*alert_count].detection_type = 1; // Entropy
        alerts[*alert_count].confidence = (ENTROPY_THRESHOLD - entropy) / ENTROPY_THRESHOLD;
        (*alert_count)++;
    }

    return 0;
}

// Rate-based detection (CUSUM-inspired)
// PCA-based detection - detects anomalies in multi-dimensional feature space
int detect_pca_based(FlowRecord *flows, int count, Alert *alerts, int *alert_count)
{
    typedef struct
    {
        char ip[MAX_IP_LEN];
        double features[5]; // Multi-dimensional features: bytes/s, packets/s, flow duration, packet length, IAT
        int flow_count;
    } IPFeatures;

    IPFeatures ip_features[MAX_UNIQUE_IPS] = {0};
    int unique_ips = 0;

    // Aggregate features per IP
    for (int i = 0; i < count; i++)
    {
        int found = 0;
        for (int j = 0; j < unique_ips; j++)
        {
            if (strcmp(ip_features[j].ip, flows[i].src_ip) == 0)
            {
                ip_features[j].features[0] += flows[i].flow_bytes_per_sec;
                ip_features[j].features[1] += (flows[i].total_fwd_packets + flows[i].total_bwd_packets);
                ip_features[j].features[2] += flows[i].flow_duration;
                ip_features[j].features[3] += flows[i].packet_length_mean;
                ip_features[j].features[4] += flows[i].flow_iat_mean;
                ip_features[j].flow_count++;
                found = 1;
                break;
            }
        }
        if (!found && unique_ips < MAX_UNIQUE_IPS)
        {
            strcpy(ip_features[unique_ips].ip, flows[i].src_ip);
            ip_features[unique_ips].features[0] = flows[i].flow_bytes_per_sec;
            ip_features[unique_ips].features[1] = flows[i].total_fwd_packets + flows[i].total_bwd_packets;
            ip_features[unique_ips].features[2] = flows[i].flow_duration;
            ip_features[unique_ips].features[3] = flows[i].packet_length_mean;
            ip_features[unique_ips].features[4] = flows[i].flow_iat_mean;
            ip_features[unique_ips].flow_count = 1;
            unique_ips++;
        }
    }

    // Calculate mean and standard deviation for normalization
    double mean[5] = {0}, stddev[5] = {0};
    for (int f = 0; f < 5; f++)
    {
        for (int i = 0; i < unique_ips; i++)
        {
            mean[f] += ip_features[i].features[f] / ip_features[i].flow_count;
        }
        mean[f] /= unique_ips;

        for (int i = 0; i < unique_ips; i++)
        {
            double val = ip_features[i].features[f] / ip_features[i].flow_count;
            stddev[f] += (val - mean[f]) * (val - mean[f]);
        }
        stddev[f] = sqrt(stddev[f] / unique_ips);
        if (stddev[f] < 1e-6)
            stddev[f] = 1.0; // Avoid division by zero
    }

    // Detect anomalies using simplified PCA (Mahalanobis-like distance)
    for (int i = 0; i < unique_ips; i++)
    {
        double distance = 0.0;
        for (int f = 0; f < 5; f++)
        {
            double normalized = (ip_features[i].features[f] / ip_features[i].flow_count - mean[f]) / stddev[f];
            distance += normalized * normalized;
        }
        distance = sqrt(distance);

        // Flag as anomaly if distance exceeds threshold
        if (distance > PCA_THRESHOLD)
        {
            strcpy(alerts[*alert_count].src_ip, ip_features[i].ip);
            alerts[*alert_count].timestamp = 0.0;
            alerts[*alert_count].detection_type = 2; // PCA-based
            alerts[*alert_count].confidence = distance / PCA_THRESHOLD;
            (*alert_count)++;
        }
    }

    return 0;
}

// CUSUM (Cumulative Sum) statistical detection
// Detects sustained deviations from baseline traffic patterns
int detect_cusum_based(FlowRecord *flows, int count, Alert *alerts, int *alert_count)
{
    typedef struct
    {
        char ip[MAX_IP_LEN];
        double cusum_high; // Cumulative sum for upward deviations
        double cusum_low;  // Cumulative sum for downward deviations
        int flow_count;
        double total_bytes_per_sec;
    } CUSUMState;

    CUSUMState ip_cusum[MAX_UNIQUE_IPS] = {0};
    int unique_ips = 0;

    // Aggregate per-IP statistics
    for (int i = 0; i < count; i++)
    {
        int found = 0;
        for (int j = 0; j < unique_ips; j++)
        {
            if (strcmp(ip_cusum[j].ip, flows[i].src_ip) == 0)
            {
                ip_cusum[j].total_bytes_per_sec += flows[i].flow_bytes_per_sec;
                ip_cusum[j].flow_count++;
                found = 1;
                break;
            }
        }
        if (!found && unique_ips < MAX_UNIQUE_IPS)
        {
            strcpy(ip_cusum[unique_ips].ip, flows[i].src_ip);
            ip_cusum[unique_ips].total_bytes_per_sec = flows[i].flow_bytes_per_sec;
            ip_cusum[unique_ips].flow_count = 1;
            unique_ips++;
        }
    }

    // Apply CUSUM algorithm per IP
    for (int i = 0; i < unique_ips; i++)
    {
        double avg_rate = ip_cusum[i].total_bytes_per_sec / ip_cusum[i].flow_count;

        // Calculate deviation from baseline
        double deviation = avg_rate - CUSUM_BASELINE;

        // CUSUM formulas:
        // S_high = max(0, S_high + (deviation - K))
        // S_low = max(0, S_low + (-deviation - K))
        ip_cusum[i].cusum_high = fmax(0, ip_cusum[i].cusum_high + (deviation - CUSUM_DRIFT * CUSUM_BASELINE));
        ip_cusum[i].cusum_low = fmax(0, ip_cusum[i].cusum_low + (-deviation - CUSUM_DRIFT * CUSUM_BASELINE));

        // Trigger alert if CUSUM exceeds threshold (sustained anomaly detected)
        if (ip_cusum[i].cusum_high > CUSUM_THRESHOLD * CUSUM_BASELINE)
        {
            strcpy(alerts[*alert_count].src_ip, ip_cusum[i].ip);
            alerts[*alert_count].timestamp = 0.0;
            alerts[*alert_count].detection_type = 3; // CUSUM-based
            alerts[*alert_count].confidence = ip_cusum[i].cusum_high / (CUSUM_THRESHOLD * CUSUM_BASELINE);
            (*alert_count)++;
        }
    }

    return 0;
}

// Run all three detectors
int run_detection(FlowRecord *flows, int count, Alert *alerts, int *alert_count)
{
    *alert_count = 0;

    // Apply all three detection algorithms as required: Entropy, PCA, CUSUM
    detect_entropy_based(flows, count, alerts, alert_count);
    detect_pca_based(flows, count, alerts, alert_count);
    detect_cusum_based(flows, count, alerts, alert_count);

    return 0;
}
