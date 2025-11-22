#!/bin/bash
#
# Run DDoS Detector with 8 workers (9 total processes)
# Uses full combined DrDoS dataset
#

echo "=========================================="
echo "  Running with 8 Workers"
echo "=========================================="
echo "Dataset: datasets/processed/all_drddos_combined.csv"
echo "Processes: 9 (1 master + 8 workers)"
echo ""

# Check available cores
CORES=$(nproc 2>/dev/null || echo 4)
if [ $CORES -lt 9 ]; then
    echo "[WARNING] Only $CORES cores available, but 9 processes requested"
    echo "[WARNING] This may cause performance degradation due to oversubscription"
    echo ""
fi

mpiexec -n 9 ./ddos_detector datasets/processed/all_drddos_combined.csv

echo ""
echo "=========================================="
echo "Results saved to:"
echo "  - results.txt"
echo "  - performance_metrics.csv"
echo "  - blocklist_9_ranks.txt"
echo "  - flowspec_rules.txt"
echo "  - acl_rules.txt"
echo "=========================================="
