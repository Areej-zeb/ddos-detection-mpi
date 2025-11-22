#!/bin/bash
# Step 1: Preprocess the large CSV dataset
# Extracts only essential columns to make processing faster

echo "=== CSV Dataset Preprocessor ==="
echo ""

if [ $# -lt 2 ]; then
    echo "Usage: $0 <input_csv> <output_csv> [max_rows]"
    echo ""
    echo "Example:"
    echo "  $0 'datasets/CIC-DDoS 2019/DrDoS_NTP.csv' 'datasets/processed_data.csv' 10000"
    echo ""
    exit 1
fi

INPUT_FILE="$1"
OUTPUT_FILE="$2"
MAX_ROWS=${3:-10000}  # Default to 10000 rows if not specified

if [ ! -f "$INPUT_FILE" ]; then
    echo "Error: Input file not found: $INPUT_FILE"
    exit 1
fi

echo "Input:  $INPUT_FILE"
echo "Output: $OUTPUT_FILE"
echo "Extracting first $MAX_ROWS rows..."
echo ""

# Extract header + data rows
# We keep columns: 2(SrcIP), 4(DstIP), 6(Protocol), 20(FlowBytes/s), 47(PktLenMean), 85(Label)
head -n $((MAX_ROWS + 1)) "$INPUT_FILE" | awk -F',' 'BEGIN {OFS=","} {print $3, $5, $7, $21, $48, $86}' > "$OUTPUT_FILE"

if [ -f "$OUTPUT_FILE" ]; then
    line_count=$(wc -l < "$OUTPUT_FILE")
    echo "✓ Success! Created $OUTPUT_FILE with $line_count lines"
    echo ""
    echo "Preview (first 3 lines):"
    head -n 3 "$OUTPUT_FILE"
    echo ""
    echo "Next step: Build the detector"
    echo "  ./build.sh"
else
    echo "✗ Failed to create output file"
    exit 1
fi
