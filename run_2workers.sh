#!/bin/bash
#
# Run DDoS Detector with 2 workers (3 total processes)
# Uses full combined DrDoS dataset
#

echo "=========================================="
echo "  Running with 2 Workers"
echo "=========================================="
echo "Dataset: datasets/processed/all_drddos_combined.csv"
echo "Processes: 3 (1 master + 2 workers)"
echo ""

mpiexec -n 3 ./ddos_detector datasets/processed/all_drddos_combined.csv

echo ""
echo "=========================================="
echo "Results saved to:"
echo "  - results.txt"
echo "  - performance_metrics.csv"
echo "  - blocklist_3_ranks.txt"
echo "  - flowspec_rules.txt"
echo "  - acl_rules.txt"
echo "=========================================="
