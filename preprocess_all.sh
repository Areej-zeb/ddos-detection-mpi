#!/bin/bash
# Preprocess all 8 CIC-DDoS 2019 DrDoS CSV files
# Keeps up to 200k rows per file and only important columns.

set -e

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
RAW_DIR="$ROOT_DIR/datasets/CIC-DDoS 2019"
OUT_DIR="$ROOT_DIR/datasets/processed"
MAX_ROWS=200000

mkdir -p "$OUT_DIR"

echo "Root: $ROOT_DIR"
echo "Raw:  $RAW_DIR"
echo "Out:  $OUT_DIR"
echo "Max rows per file: $MAX_ROWS"
echo ""

# Header columns we keep (1-based positions in original CSV):
# 3: Source IP
# 4: Source Port
# 5: Destination IP
# 6: Destination Port
# 7: Protocol
# 9: Flow Duration
# 10: Total Fwd Packets
# 11: Total Backward Packets
# 12: Total Length of Fwd Packets
# 13: Total Length of Bwd Packets
# 22: Flow Bytes/s
# 23: Flow Packets/s
# 46: Min Packet Length
# 47: Max Packet Length
# 48: Packet Length Mean
# 49: Packet Length Std
# 24: Flow IAT Mean
# 25: Flow IAT Std
# 86: Label

cols="3,4,5,6,7,9,10,11,12,13,22,23,46,47,48,49,24,25,88"
process_file() {
  local in_file="$1"
  local out_file="$2"

  if [ ! -f "$in_file" ]; then
    echo "[WARN] Missing: $in_file (skipping)"
    return
  fi

  echo "[INFO] Processing: $(basename "$in_file")"
  echo "       -> $(basename "$out_file")"

  # Use head to limit rows, then cut to select columns
  # Use -d',' -f with column list.
  head -n $((MAX_ROWS + 1)) "$in_file" \
    | cut -d',' -f"$cols" > "$out_file"

  local lines
  lines=$(wc -l < "$out_file")
  echo "       Saved $lines lines to $out_file"
  echo ""
}

process_file "$RAW_DIR/DrDoS_DNS.csv"    "$OUT_DIR/DrDoS_DNS_processed.csv"
process_file "$RAW_DIR/DrDoS_LDAP.csv"   "$OUT_DIR/DrDoS_LDAP_processed.csv"
process_file "$RAW_DIR/DrDoS_MSSQL.csv"  "$OUT_DIR/DrDoS_MSSQL_processed.csv"
process_file "$RAW_DIR/DrDoS_NetBIOS.csv" "$OUT_DIR/DrDoS_NetBIOS_processed.csv"
process_file "$RAW_DIR/DrDoS_NTP.csv"    "$OUT_DIR/DrDoS_NTP_processed.csv"
process_file "$RAW_DIR/DrDoS_SNMP.csv"   "$OUT_DIR/DrDoS_SNMP_processed.csv"
process_file "$RAW_DIR/DrDoS_SSDP.csv"   "$OUT_DIR/DrDoS_SSDP_processed.csv"
process_file "$RAW_DIR/DrDoS_UDP.csv"    "$OUT_DIR/DrDoS_UDP_processed.csv"

echo "[INFO] Creating combined file all_drddos_combined.csv"
cat "$OUT_DIR"/DrDoS_*_processed.csv > "$OUT_DIR/all_drddos_combined.csv"
lines_combined=$(wc -l < "$OUT_DIR/all_drddos_combined.csv")
echo "       Combined lines: $lines_combined"

echo "[DONE] Preprocessing complete. Files in $OUT_DIR"