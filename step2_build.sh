#!/bin/bash
# Step 2: Build the MPI detector

echo "=== Step 2: Building DDoS Detector ==="
echo ""

# Check for MPI
if ! command -v mpicc &> /dev/null; then
    echo "Error: mpicc not found."
    echo ""
    echo "Install MPI first:"
    echo "  sudo apt-get update"
    echo "  sudo apt-get install mpich libmpich-dev gcc make"
    echo ""
    exit 1
fi

# Check for CUDA
if command -v nvcc &> /dev/null; then
    echo "[INFO] CUDA detected - building GPU-accelerated version"
    USE_CUDA=1
else
    echo "[INFO] CUDA not found - building CPU-only version"
    USE_CUDA=0
fi

echo ""
echo "Compiling..."

# Clean old builds
rm -f ddos_detector ddos_detector_cuda *.o

if [ $USE_CUDA -eq 1 ]; then
    # Build with CUDA
    echo "  - Compiling CUDA kernels..."
    nvcc -arch=sm_50 -Xcompiler -fPIC -c src/detection_cuda.cu -o detection_cuda.o
    
    echo "  - Linking with CUDA support..."
    mpicc -Wall -O2 -std=c99 \
        src/main.c src/csv_parser.c src/detection.c src/blocking.c src/metrics.c src/output.c \
        detection_cuda.o \
        -o ddos_detector \
        -lm -L/usr/local/cuda/lib64 -lcudart
else
    # Build CPU-only with stub
    mpicc -Wall -O2 -std=c99 \
        src/main.c src/csv_parser.c src/detection.c src/blocking.c src/metrics.c src/output.c \
        src/detection_cuda_stub.c \
        -o ddos_detector \
        -lm
fi

if [ -f "ddos_detector" ]; then
    echo ""
    echo "✓ Build successful!"
    if [ $USE_CUDA -eq 1 ]; then
        echo "  Built with: GPU acceleration (CUDA)"
    else
        echo "  Built with: CPU-only"
    fi
    echo ""
    echo "Next step: Run the detector"
    echo "  ./step3_run.sh datasets/processed/all_drddos_combined.csv"
else
    echo "✗ Build failed"
    exit 1
fi
