/*
 * Stub implementation for CUDA functions when CUDA is not available
 * This allows the code to compile on systems without CUDA
 */

#include "detection_cuda.h"
#include <stdio.h>

// Stub: CUDA not available
int cuda_available()
{
    return 0;
}

// Stub: Print unavailable message
void print_cuda_info()
{
    printf("[CUDA] Not available (compiled without CUDA support)\n");
}

// Stub: Return high entropy (no detection)
float calculate_entropy_cuda(FlowRecord *flows, int count)
{
    // This should never be called if cuda_available() returns 0
    fprintf(stderr, "ERROR: calculate_entropy_cuda called without CUDA support\n");
    return 10.0f;
}

// Stub: Return no detections
int detect_high_rate_ips_cuda(FlowRecord *flows, int count, float threshold,
                              char detected_ips[][MAX_IP_LEN], float *rates, int max_detections)
{
    // This should never be called if cuda_available() returns 0
    fprintf(stderr, "ERROR: detect_high_rate_ips_cuda called without CUDA support\n");
    return 0;
}
