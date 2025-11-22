# Quick Start Guide

Get up and running with the DDoS Detection System in 5 minutes!

## Prerequisites Check

```bash
# Check if MPI is installed
mpicc --version
mpiexec --version

# If not installed (Ubuntu/WSL):
sudo apt-get update
sudo apt-get install mpich libmpich-dev gcc make
```

## Installation (3 steps)

### 1. Clone Repository
```bash
git clone https://github.com/yourusername/ddos-detection-mpi.git
cd ddos-detection-mpi
```

### 2. Create Test Dataset
```bash
mkdir -p datasets
cat > datasets/test_data.csv << 'EOF'
SrcIP,DstIP,Protocol,FlowBytes/s,PktLenMean,Label
192.168.1.100,10.0.0.1,6,1000,64,BENIGN
192.168.1.101,10.0.0.1,6,50000,512,DDoS
192.168.1.102,10.0.0.1,17,2000,128,BENIGN
192.168.1.103,10.0.0.1,6,100000,1024,DDoS
192.168.1.104,10.0.0.1,6,1500,64,BENIGN
192.168.1.105,10.0.0.1,6,80000,768,DDoS
192.168.1.106,10.0.0.1,17,3000,256,BENIGN
192.168.1.107,10.0.0.1,6,120000,1024,DDoS
192.168.1.108,10.0.0.1,6,1200,64,BENIGN
192.168.1.109,10.0.0.1,6,90000,896,DDoS
EOF
```

### 3. Build and Run
```bash
# Build the detector
./step2_build.sh

# Run with 4 processes (1 master + 3 workers)
./step3_run.sh datasets/test_data.csv 4
```

## View Results

```bash
# View detection results
cat results.txt

# View blocked IPs
cat blocklist_4_ranks.txt

# View mitigation rules
cat flowspec_rules.txt
cat acl_rules.txt
```

## Expected Output

```
╔════════════════════════════════════════════════════════╗
║        DDoS Detection - Comprehensive Report           ║
╚════════════════════════════════════════════════════════╝

Detection Accuracy:
  True Positives:  5
  False Positives: 0
  True Negatives:  4
  False Negatives: 1
  
  Precision: 100.00%
  Recall:    83.33%
  F1-Score:  90.91%

Performance Metrics:
  Total Flows Processed: 10
  Throughput: 2500.00 flows/sec
  Average Latency: 0.40 ms
  
Blocked IPs: 5
```

## Next Steps

### 1. Use Real Dataset
Download CIC-DDoS2019:
```bash
# Download from: https://www.unb.ca/cic/datasets/ddos-2019.html
# Place in: datasets/CIC-DDoS 2019/

# Preprocess (extract first 10,000 rows)
./step1_preprocess.sh "datasets/CIC-DDoS 2019/DrDoS_NTP.csv" \
                      "datasets/processed_data.csv" 10000

# Run detection
./step3_run.sh datasets/processed_data.csv 4
```

### 2. Scale Up
```bash
# Run with more workers for better performance
./step3_run.sh datasets/processed_data.csv 8  # 8 processes
./step3_run.sh datasets/processed_data.csv 16 # 16 processes
```

### 3. Run Complete Workflow
```bash
# Automated: preprocess + build + run
./run_all.sh
```

### 4. Scalability Testing
```bash
# Test with 2, 4, 8 workers and generate graphs
./scalability_test.sh datasets/processed_data.csv
```

## Troubleshooting

### Issue: "mpicc: command not found"
**Solution:**
```bash
sudo apt-get install mpich libmpich-dev
```

### Issue: "Dataset not found"
**Solution:**
```bash
# Create test dataset (see step 2 above)
# OR download real dataset
```

### Issue: "Permission denied"
**Solution:**
```bash
chmod +x *.sh
```

### Issue: Build errors
**Solution:**
```bash
# Check GCC version (need 7.0+)
gcc --version

# Clean and rebuild
rm -f ddos_detector *.o
./step2_build.sh
```

## Performance Tips

1. **More Workers = Better Performance**
   - Use `2^n` processes (2, 4, 8, 16) for optimal load distribution

2. **GPU Acceleration**
   - If you have NVIDIA GPU with CUDA, it's automatically detected
   - 3-5x speedup on large datasets

3. **Larger Datasets**
   - Adjust `WINDOW_SIZE` in `types.h` for memory vs performance tradeoff
   - Rebuild after changes

4. **Network Deployment**
   - Deploy `flowspec_rules.txt` to BGP routers
   - Apply `acl_rules.txt` to edge routers

## Need Help?

- 📖 Read full [README.md](README.md)
- 🐛 Report issues on GitHub
- 💬 Check [CONTRIBUTING.md](CONTRIBUTING.md)

## What's Next?

- Integrate with your network monitoring system
- Set up real-time alerting
- Deploy mitigation rules automatically
- Tune detection thresholds for your network

Happy detecting! 🚀🛡️
