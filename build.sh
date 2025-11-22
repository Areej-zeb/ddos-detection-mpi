#!/bin/bash
# Build script for Linux/WSL

echo "=== Building DDoS Detection System ==="
echo ""

# Check if mpicc is available
if ! command -v mpicc &> /dev/null; then
    echo "Error: mpicc not found. Please install MPICH or OpenMPI."
    echo ""
    echo "To install on Ubuntu/WSL:"
    echo "  sudo apt-get update"
    echo "  sudo apt-get install mpich libmpich-dev gcc make"
    echo ""
    exit 1
fi

echo "Found mpicc: $(which mpicc)"
echo ""

# Compile
echo "Compiling source files..."
mpicc -Wall -O2 -std=c99 src/main.c src/csv_parser.c src/detection.c src/blocking.c src/metrics.c -o ddos_detector -lm

if [ -f "ddos_detector" ]; then
    echo ""
    echo "✓ Build successful: ddos_detector"
    echo ""
    echo "To run the program:"
    echo "  mpiexec -n 4 ./ddos_detector 'datasets/CIC-DDoS 2019/DrDoS_NTP.csv'"
    echo ""
else
    echo ""
    echo "✗ Build failed - executable not created"
    exit 1
fi
