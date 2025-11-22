#ifndef DETECTION_CUDA_H
#define DETECTION_CUDA_H

#include "types.h"

#ifdef __cplusplus
extern "C"
{
#endif

    // Check if CUDA is available on this system
    int cuda_available();

    // Print CUDA device information
    void print_cuda_info();

    // CUDA-accelerated entropy calculation
    // Returns Shannon entropy of source IP distribution
    float calculate_entropy_cuda(FlowRecord *flows, int count);

    // CUDA-accelerated rate-based detection
    // Returns number of IPs detected, fills detected_ips and rates arrays
    int detect_high_rate_ips_cuda(FlowRecord *flows, int count, float threshold,
                                  char detected_ips[][MAX_IP_LEN], float *rates, int max_detections);

#ifdef __cplusplus
}
#endif

#endif // DETECTION_CUDA_H
