#!/bin/bash
#
# Run DDoS Detector with 4 workers (5 total processes)
# Uses full combined DrDoS dataset
#

echo "=========================================="
echo "  Running with 4 Workers"
echo "=========================================="
echo "Dataset: datasets/processed/all_drddos_combined.csv"
echo "Processes: 5 (1 master + 4 workers)"
echo ""

mpiexec -n 5 ./ddos_detector datasets/processed/all_drddos_combined.csv

echo ""
echo "=========================================="
echo "Results saved to:"
echo "  - results.txt"
echo "  - performance_metrics.csv"
echo "  - blocklist_5_ranks.txt"
echo "  - flowspec_rules.txt"
echo "  - acl_rules.txt"
echo "=========================================="
