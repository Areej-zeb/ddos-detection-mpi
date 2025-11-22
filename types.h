#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>
#include <time.h>

#define MAX_IP_LEN 32
#define MAX_LINE_LEN 2048
#define HASH_SIZE 10000
#define MAX_BLOCKED_IPS 1000
#define WINDOW_SIZE 50000 // Number of flows per analysis window (increased for GPU efficiency)

// Flow record (simplified from CIC-DDoS2019)
typedef struct
{
    char src_ip[MAX_IP_LEN];
    char dst_ip[MAX_IP_LEN];
    int src_port;
    int dst_port;
    int protocol;
    double flow_duration;
    double total_fwd_packets;
    double total_bwd_packets;
    double total_fwd_bytes;
    double total_bwd_bytes;
    double flow_bytes_per_sec;
    double flow_packets_per_sec;
    double min_packet_length;
    double max_packet_length;
    double packet_length_mean;
    double packet_length_std;
    double flow_iat_mean;
    double flow_iat_std;
    int is_attack; // Ground truth from Label column
} FlowRecord;

// Detection result
typedef struct
{
    char src_ip[MAX_IP_LEN];
    double timestamp;
    int detection_type; // 1=entropy, 2=rate
    double confidence;
} Alert;

// Blocklist entry
typedef struct
{
    char ip[MAX_IP_LEN];
    double blocked_time;
} BlockedIP;

// Metrics
typedef struct
{
    int true_positives;
    int false_positives;
    int true_negatives;
    int false_negatives;
    int total_flows;
    int blocked_flows;
    int legitimate_blocked; // Collateral damage
    double total_latency_ms;
    double detection_lead_time_ms;
    double throughput_flows_per_sec;
    int ddos_flows;                    // Total DDoS traffic (ground truth)
    int benign_flows;                  // Total benign traffic (ground truth)
    double mpi_communication_time_ms;  // MPI overhead
    double local_detection_time_ms;    // Local processing time
    double global_detection_time_ms;   // End-to-end time
    double cpu_usage_percent;          // CPU utilization
    double memory_usage_mb;            // Memory consumption
    double latency_p95_ms;             // 95th percentile latency
    double latency_p99_ms;             // 99th percentile latency
    double attack_traffic_dropped_pct; // % of attack traffic blocked
} Metrics;

#endif
