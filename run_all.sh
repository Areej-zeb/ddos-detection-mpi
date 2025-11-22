#!/bin/bash
# MASTER SCRIPT - Run Everything Step by Step

echo "╔════════════════════════════════════════════════════════╗"
echo "║   DDoS Detection System - Complete Workflow           ║"
echo "╚════════════════════════════════════════════════════════╝"
echo ""

# Configuration
INPUT_DATASET="datasets/CIC-DDoS 2019/DrDoS_NTP.csv"
PROCESSED_DATASET="datasets/processed_data.csv"
MAX_ROWS=10000  # Process first 10k rows (adjust as needed)
NUM_PROCESSES=4

echo "Configuration:"
echo "  Input:     $INPUT_DATASET"
echo "  Output:    $PROCESSED_DATASET"
echo "  Max Rows:  $MAX_ROWS"
echo "  MPI Procs: $NUM_PROCESSES"
echo ""
read -p "Press ENTER to continue or Ctrl+C to abort..."
echo ""

# ============================================================
# STEP 1: Preprocess Dataset
# ============================================================
echo ""
echo "┌─────────────────────────────────────────────────────┐"
echo "│ STEP 1: Preprocessing Dataset                       │"
echo "└─────────────────────────────────────────────────────┘"
echo ""

if [ ! -f "$INPUT_DATASET" ]; then
    echo "⚠ Warning: Input dataset not found!"
    echo "  Expected: $INPUT_DATASET"
    echo ""
    echo "Please download CIC-DDoS2019 dataset and place in:"
    echo "  datasets/CIC-DDoS 2019/"
    echo ""
    echo "For testing, you can create a dummy file:"
    echo "  mkdir -p 'datasets/CIC-DDoS 2019'"
    echo "  echo 'test' > '$INPUT_DATASET'"
    exit 1
fi

chmod +x step1_preprocess.sh
./step1_preprocess.sh "$INPUT_DATASET" "$PROCESSED_DATASET" $MAX_ROWS

if [ $? -ne 0 ]; then
    echo "✗ Preprocessing failed!"
    exit 1
fi

echo ""
read -p "Step 1 complete. Press ENTER to continue..."

# ============================================================
# STEP 2: Build the Detector
# ============================================================
echo ""
echo "┌─────────────────────────────────────────────────────┐"
echo "│ STEP 2: Building MPI Detector                       │"
echo "└─────────────────────────────────────────────────────┘"
echo ""

chmod +x step2_build.sh
./step2_build.sh

if [ $? -ne 0 ]; then
    echo "✗ Build failed!"
    exit 1
fi

echo ""
read -p "Step 2 complete. Press ENTER to continue..."

# ============================================================
# STEP 3: Run Detection
# ============================================================
echo ""
echo "┌─────────────────────────────────────────────────────┐"
echo "│ STEP 3: Running DDoS Detection                      │"
echo "└─────────────────────────────────────────────────────┘"
echo ""

chmod +x step3_run.sh
./step3_run.sh "$PROCESSED_DATASET" $NUM_PROCESSES

if [ $? -ne 0 ]; then
    echo "✗ Detection failed!"
    exit 1
fi

# ============================================================
# FINAL SUMMARY
# ============================================================
echo ""
echo "╔════════════════════════════════════════════════════════╗"
echo "║   🎉 ALL STEPS COMPLETED SUCCESSFULLY!                ║"
echo "╚════════════════════════════════════════════════════════╝"
echo ""
echo "📊 Results Summary:"
echo ""

if [ -f "results.txt" ]; then
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    tail -n 20 results.txt
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
fi

echo ""
echo "📁 Output Files:"
echo "  ✓ $PROCESSED_DATASET (preprocessed data)"
echo "  ✓ results.txt (metrics)"
echo "  ✓ blocklist_${NUM_PROCESSES}_ranks.txt (blocked IPs)"
echo ""
echo "🔄 To run again with different settings:"
echo "  1. Edit configuration at top of this script"
echo "  2. Run: ./run_all.sh"
echo ""
echo "📈 For scalability testing:"
echo "  ./step3_run.sh $PROCESSED_DATASET 2  # 2 processes"
echo "  ./step3_run.sh $PROCESSED_DATASET 4  # 4 processes"
echo "  ./step3_run.sh $PROCESSED_DATASET 8  # 8 processes"
echo ""
