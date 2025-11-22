#!/bin/bash
#
# Scalability Testing Script
# Tests DDoS detector with varying number of workers (2, 4, 8, 16)
# Collects metrics for scalability analysis
#

set -e

DATASET="${1:-datasets/processed/test_50k.csv}"
OUTPUT_DIR="./scalability_results"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)

echo "=========================================="
echo "  MPI DDoS Detector - Scalability Test"
echo "=========================================="
echo "Dataset: $DATASET"
echo "Output directory: $OUTPUT_DIR"
echo ""

# Create output directory
mkdir -p "$OUTPUT_DIR"

# Test with different worker counts
WORKER_COUNTS=(2 4 8 16)

echo "Starting scalability tests..."
echo ""

for WORKERS in "${WORKER_COUNTS[@]}"; do
    TOTAL_PROCS=$((WORKERS + 1))  # +1 for master
    
    echo "----------------------------------------"
    echo "Testing with $WORKERS workers ($TOTAL_PROCS total processes)"
    echo "----------------------------------------"
    
    # Check if we have enough processors
    AVAILABLE_CORES=$(nproc 2>/dev/null || echo 4)
    if [ $TOTAL_PROCS -gt $AVAILABLE_CORES ]; then
        echo "[WARNING] Requested $TOTAL_PROCS processes but only $AVAILABLE_CORES cores available"
        echo "[WARNING] Skipping this configuration (would cause oversubscription)"
        echo ""
        continue
    fi
    
    OUTPUT_PREFIX="${OUTPUT_DIR}/metrics_${WORKERS}workers_${TIMESTAMP}"
    
    # Run detection
    echo "[INFO] Running detection with $TOTAL_PROCS processes..."
    START_TIME=$(date +%s.%N)
    
    mpiexec -n $TOTAL_PROCS ./ddos_detector "$DATASET" > "${OUTPUT_PREFIX}.log" 2>&1
    
    END_TIME=$(date +%s.%N)
    ELAPSED=$(echo "$END_TIME - $START_TIME" | bc)
    
    echo "[DONE] Completed in ${ELAPSED}s"
    
    # Extract key metrics from results
    if [ -f "performance_metrics.csv" ]; then
        cp "performance_metrics.csv" "${OUTPUT_PREFIX}.csv"
        echo "[INFO] Metrics saved to ${OUTPUT_PREFIX}.csv"
    fi
    
    # Extract throughput and latency from log
    THROUGHPUT=$(grep "Packets/second:" "${OUTPUT_PREFIX}.log" | awk '{print $2}')
    LATENCY=$(grep "Per-packet latency (avg):" "${OUTPUT_PREFIX}.log" | awk '{print $4}')
    MPI_OVERHEAD=$(grep "MPI communication overhead:" "${OUTPUT_PREFIX}.log" | awk '{print $4}')
    
    echo "[METRICS] Throughput: $THROUGHPUT pps"
    echo "[METRICS] Latency: $LATENCY ms"
    echo "[METRICS] MPI Overhead: $MPI_OVERHEAD ms"
    echo ""
done

echo "=========================================="
echo "  Scalability Test Complete"
echo "=========================================="
echo ""
echo "Generating summary report..."

# Create summary CSV
SUMMARY_FILE="${OUTPUT_DIR}/scalability_summary_${TIMESTAMP}.csv"
echo "Workers,Processes,Throughput_pps,Latency_ms,MPI_Overhead_ms,Wall_Time_sec" > "$SUMMARY_FILE"

for WORKERS in "${WORKER_COUNTS[@]}"; do
    TOTAL_PROCS=$((WORKERS + 1))
    OUTPUT_PREFIX="${OUTPUT_DIR}/metrics_${WORKERS}workers_${TIMESTAMP}"
    
    if [ -f "${OUTPUT_PREFIX}.log" ]; then
        THROUGHPUT=$(grep "Packets/second:" "${OUTPUT_PREFIX}.log" | awk '{print $2}' || echo "N/A")
        LATENCY=$(grep "Per-packet latency (avg):" "${OUTPUT_PREFIX}.log" | awk '{print $4}' || echo "N/A")
        MPI_OVERHEAD=$(grep "MPI communication overhead:" "${OUTPUT_PREFIX}.log" | awk '{print $4}' || echo "N/A")
        WALL_TIME=$(grep "Processing time (wall):" "${OUTPUT_PREFIX}.log" | awk '{print $4}' || echo "N/A")
        
        echo "$WORKERS,$TOTAL_PROCS,$THROUGHPUT,$LATENCY,$MPI_OVERHEAD,$WALL_TIME" >> "$SUMMARY_FILE"
    fi
done

echo "[SUMMARY] Scalability summary saved to: $SUMMARY_FILE"
echo ""
echo "View summary:"
echo "  cat $SUMMARY_FILE"
echo ""
echo "Next steps:"
echo "  1. Generate graphs: python3 scripts/generate_graphs.py $SUMMARY_FILE"
echo "  2. Review individual logs in: $OUTPUT_DIR/"
echo "=========================================="
