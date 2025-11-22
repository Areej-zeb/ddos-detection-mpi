/*
 * CUDA-accelerated DDoS Detection
 * GPU kernels for entropy and rate-based detection
 */

#include <cuda_runtime.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "types.h"

#define CUDA_CHECK(call)                                                      \
    do                                                                        \
    {                                                                         \
        cudaError_t err = call;                                               \
        if (err != cudaSuccess)                                               \
        {                                                                     \
            fprintf(stderr, "CUDA error at %s:%d - %s\n", __FILE__, __LINE__, \
                    cudaGetErrorString(err));                                 \
            exit(EXIT_FAILURE);                                               \
        }                                                                     \
    } while (0)

#define MAX_UNIQUE_IPS_GPU 1000
#define BLOCK_SIZE 256

// Device structure for IP counting
typedef struct
{
    char ip[MAX_IP_LEN];
    int count;
    float total_rate;
} IPData;

// Hash function for IP strings (simple DJB2)
__device__ unsigned int hash_ip(const char *str)
{
    unsigned int hash = 5381;
    int c;
    while ((c = *str++))
    {
        hash = ((hash << 5) + hash) + c;
    }
    return hash % MAX_UNIQUE_IPS_GPU;
}

// Kernel: Count IP occurrences for entropy calculation
__global__ void count_ips_kernel(FlowRecord *flows, int count, IPData *ip_data, int *unique_count)
{
    int idx = blockIdx.x * blockDim.x + threadIdx.x;

    if (idx >= count)
        return;

    FlowRecord flow = flows[idx];
    unsigned int hash = hash_ip(flow.src_ip);

    // Linear probing for collision resolution
    while (true)
    {
        // Try to claim this slot atomically
        if (atomicCAS((int *)&ip_data[hash].count, 0, 1) == 0)
        {
            // First time seeing this slot - copy IP
            for (int i = 0; i < MAX_IP_LEN; i++)
            {
                ip_data[hash].ip[i] = flow.src_ip[i];
            }
            atomicAdd(unique_count, 1);
            break;
        }

        // Check if this slot has our IP
        bool match = true;
        for (int i = 0; i < MAX_IP_LEN; i++)
        {
            if (ip_data[hash].ip[i] != flow.src_ip[i])
            {
                match = false;
                break;
            }
        }

        if (match)
        {
            // Found our IP - increment count
            atomicAdd(&ip_data[hash].count, 1);
            break;
        }

        // Collision - try next slot
        hash = (hash + 1) % MAX_UNIQUE_IPS_GPU;
    }
}

// Kernel: Accumulate rates per IP for rate-based detection
__global__ void accumulate_rates_kernel(FlowRecord *flows, int count, IPData *ip_data)
{
    int idx = blockIdx.x * blockDim.x + threadIdx.x;

    if (idx >= count)
        return;

    FlowRecord flow = flows[idx];
    unsigned int hash = hash_ip(flow.src_ip);

    // Find IP in hash table and accumulate rate
    while (true)
    {
        bool match = true;
        for (int i = 0; i < MAX_IP_LEN; i++)
        {
            if (ip_data[hash].ip[i] != flow.src_ip[i])
            {
                match = false;
                break;
            }
        }

        if (match)
        {
            // Found IP - add rate atomically
            atomicAdd(&ip_data[hash].total_rate, (float)flow.flow_bytes_per_sec);
            break;
        }

        hash = (hash + 1) % MAX_UNIQUE_IPS_GPU;
    }
}

// Kernel: Calculate entropy from IP counts
__global__ void calculate_entropy_kernel(IPData *ip_data, int unique_count, int total_count, float *entropy)
{
    __shared__ float partial_entropy[BLOCK_SIZE];

    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    float local_entropy = 0.0f;

    if (idx < unique_count && ip_data[idx].count > 0)
    {
        float p = (float)ip_data[idx].count / total_count;
        if (p > 0)
        {
            local_entropy = -p * log2f(p);
        }
    }

    partial_entropy[threadIdx.x] = local_entropy;
    __syncthreads();

    // Reduction in shared memory
    for (int stride = blockDim.x / 2; stride > 0; stride >>= 1)
    {
        if (threadIdx.x < stride)
        {
            partial_entropy[threadIdx.x] += partial_entropy[threadIdx.x + stride];
        }
        __syncthreads();
    }

    // First thread writes result
    if (threadIdx.x == 0)
    {
        atomicAdd(entropy, partial_entropy[0]);
    }
}

extern "C"
{

    // CUDA-accelerated entropy calculation
    float calculate_entropy_cuda(FlowRecord *flows, int count)
    {
        if (count < 10)
            return 10.0f; // High entropy for small samples

        // Allocate device memory
        FlowRecord *d_flows;
        IPData *d_ip_data;
        int *d_unique_count;
        float *d_entropy;

        CUDA_CHECK(cudaMalloc(&d_flows, count * sizeof(FlowRecord)));
        CUDA_CHECK(cudaMalloc(&d_ip_data, MAX_UNIQUE_IPS_GPU * sizeof(IPData)));
        CUDA_CHECK(cudaMalloc(&d_unique_count, sizeof(int)));
        CUDA_CHECK(cudaMalloc(&d_entropy, sizeof(float)));

        // Initialize
        CUDA_CHECK(cudaMemset(d_ip_data, 0, MAX_UNIQUE_IPS_GPU * sizeof(IPData)));
        CUDA_CHECK(cudaMemset(d_unique_count, 0, sizeof(int)));
        CUDA_CHECK(cudaMemset(d_entropy, 0, sizeof(float)));

        // Copy flows to device
        CUDA_CHECK(cudaMemcpy(d_flows, flows, count * sizeof(FlowRecord), cudaMemcpyHostToDevice));

        // Launch IP counting kernel
        int blocks = (count + BLOCK_SIZE - 1) / BLOCK_SIZE;
        count_ips_kernel<<<blocks, BLOCK_SIZE>>>(d_flows, count, d_ip_data, d_unique_count);
        cudaError_t err = cudaGetLastError();
        if (err != cudaSuccess)
        {
            fprintf(stderr, "[CUDA] Kernel launch failed: %s\n", cudaGetErrorString(err));
        }
        CUDA_CHECK(cudaDeviceSynchronize());

        // Get unique count
        int unique_count;
        CUDA_CHECK(cudaMemcpy(&unique_count, d_unique_count, sizeof(int), cudaMemcpyDeviceToHost));

        // Calculate entropy
        int entropy_blocks = (unique_count + BLOCK_SIZE - 1) / BLOCK_SIZE;
        calculate_entropy_kernel<<<entropy_blocks, BLOCK_SIZE>>>(d_ip_data, unique_count, count, d_entropy);
        err = cudaGetLastError();
        if (err != cudaSuccess)
        {
            fprintf(stderr, "[CUDA] Entropy kernel launch failed: %s\n", cudaGetErrorString(err));
        }
        CUDA_CHECK(cudaDeviceSynchronize());

        // Get result
        float entropy;
        CUDA_CHECK(cudaMemcpy(&entropy, d_entropy, sizeof(float), cudaMemcpyDeviceToHost));

        // Cleanup
        cudaFree(d_flows);
        cudaFree(d_ip_data);
        cudaFree(d_unique_count);
        cudaFree(d_entropy);

        return entropy;
    }

    // CUDA-accelerated rate-based detection
    int detect_high_rate_ips_cuda(FlowRecord *flows, int count, float threshold,
                                  char detected_ips[][MAX_IP_LEN], float *rates, int max_detections)
    {
        if (count == 0)
            return 0;

        // Allocate device memory
        FlowRecord *d_flows;
        IPData *d_ip_data;
        int *d_unique_count;

        CUDA_CHECK(cudaMalloc(&d_flows, count * sizeof(FlowRecord)));
        CUDA_CHECK(cudaMalloc(&d_ip_data, MAX_UNIQUE_IPS_GPU * sizeof(IPData)));
        CUDA_CHECK(cudaMalloc(&d_unique_count, sizeof(int)));

        // Initialize
        CUDA_CHECK(cudaMemset(d_ip_data, 0, MAX_UNIQUE_IPS_GPU * sizeof(IPData)));
        CUDA_CHECK(cudaMemset(d_unique_count, 0, sizeof(int)));

        // Copy to device
        CUDA_CHECK(cudaMemcpy(d_flows, flows, count * sizeof(FlowRecord), cudaMemcpyHostToDevice));

        // First pass: count IPs
        int blocks = (count + BLOCK_SIZE - 1) / BLOCK_SIZE;
        count_ips_kernel<<<blocks, BLOCK_SIZE>>>(d_flows, count, d_ip_data, d_unique_count);
        cudaError_t err = cudaGetLastError();
        if (err != cudaSuccess)
        {
            fprintf(stderr, "[CUDA] Rate count kernel launch failed: %s\n", cudaGetErrorString(err));
        }
        CUDA_CHECK(cudaDeviceSynchronize());

        // Second pass: accumulate rates
        accumulate_rates_kernel<<<blocks, BLOCK_SIZE>>>(d_flows, count, d_ip_data);
        err = cudaGetLastError();
        if (err != cudaSuccess)
        {
            fprintf(stderr, "[CUDA] Rate accumulate kernel launch failed: %s\n", cudaGetErrorString(err));
        }
        CUDA_CHECK(cudaDeviceSynchronize());

        // Copy IP data back to host
        IPData *h_ip_data = (IPData *)malloc(MAX_UNIQUE_IPS_GPU * sizeof(IPData));
        CUDA_CHECK(cudaMemcpy(h_ip_data, d_ip_data, MAX_UNIQUE_IPS_GPU * sizeof(IPData),
                              cudaMemcpyDeviceToHost));

        // Find IPs exceeding threshold
        int detection_count = 0;
        for (int i = 0; i < MAX_UNIQUE_IPS_GPU && detection_count < max_detections; i++)
        {
            if (h_ip_data[i].count > 0 && h_ip_data[i].total_rate > threshold)
            {
                strncpy(detected_ips[detection_count], h_ip_data[i].ip, MAX_IP_LEN);
                rates[detection_count] = h_ip_data[i].total_rate;
                detection_count++;
            }
        }

        // Cleanup
        free(h_ip_data);
        cudaFree(d_flows);
        cudaFree(d_ip_data);
        cudaFree(d_unique_count);

        return detection_count;
    }

    // Check if CUDA is available
    int cuda_available()
    {
        int device_count = 0;
        cudaError_t err = cudaGetDeviceCount(&device_count);
        if (err == cudaSuccess && device_count > 0)
        {
            // Set device flags for multi-process sharing
            cudaSetDeviceFlags(cudaDeviceScheduleYield | cudaDeviceMapHost);
            return 1;
        }
        return 0;
    }

    // Get CUDA device info
    void print_cuda_info()
    {
        int device_count = 0;
        cudaGetDeviceCount(&device_count);

        if (device_count == 0)
        {
            printf("[CUDA] No CUDA devices found\n");
            return;
        }

        cudaDeviceProp prop;
        cudaGetDeviceProperties(&prop, 0);
        printf("[CUDA] Device: %s\n", prop.name);
        printf("[CUDA] Compute capability: %d.%d\n", prop.major, prop.minor);
        printf("[CUDA] Global memory: %.2f GB\n", prop.totalGlobalMem / (1024.0 * 1024.0 * 1024.0));
        printf("[CUDA] Multiprocessors: %d\n", prop.multiProcessorCount);
    }

} // extern "C"
