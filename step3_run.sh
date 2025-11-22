#!/bin/bash
# Step 3: Run the detector with MPI

echo "=== Step 3: Running DDoS Detector ==="
echo ""

if [ ! -f "ddos_detector" ]; then
    echo "Error: ddos_detector not found. Run ./step2_build.sh first"
    exit 1
fi

DATASET=${1:-"datasets/processed_data.csv"}
NUM_PROCS=${2:-4}

if [ ! -f "$DATASET" ]; then
    echo "Error: Dataset not found: $DATASET"
    echo ""
    echo "Run preprocessing first:"
    echo "  ./step1_preprocess.sh 'datasets/CIC-DDoS 2019/DrDoS_NTP.csv' 'datasets/processed_data.csv' 10000"
    exit 1
fi

echo "Dataset:   $DATASET"
echo "Processes: $NUM_PROCS (1 master + $((NUM_PROCS-1)) workers)"
echo ""
echo "Starting detection..."
echo "========================================"

mpiexec -n $NUM_PROCS ./ddos_detector "$DATASET"

echo "========================================"
echo ""
echo "✓ Detection complete!"
echo ""
echo "Results saved to:"
echo "  - results.txt"
echo "  - blocklist_${NUM_PROCS}_ranks.txt"
echo ""
echo "View results:"
echo "  cat results.txt"
echo "  cat blocklist_${NUM_PROCS}_ranks.txt"
