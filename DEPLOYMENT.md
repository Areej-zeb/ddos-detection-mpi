# Deployment Guide

This guide covers deploying the DDoS Detection System in production environments.

## 📋 Prerequisites

### Minimum Requirements
- **CPU**: 4+ cores
- **RAM**: 4GB+ (8GB recommended)
- **OS**: Linux (Ubuntu 20.04+, CentOS 8+, RHEL 8+)
- **Network**: Access to network flow data sources
- **Software**: MPI, GCC 7.0+, Python 3.6+

### Recommended Setup
- **CPU**: 16+ cores
- **RAM**: 16GB+
- **GPU**: NVIDIA GPU with CUDA support (optional, 3-5x speedup)
- **Storage**: SSD with 100GB+ for logs and datasets
- **Network**: 1Gbps+ for high-throughput environments

## 🚀 Installation Steps

### 1. Prepare System

```bash
# Update system
sudo apt-get update
sudo apt-get upgrade -y

# Install dependencies
sudo apt-get install -y mpich libmpich-dev gcc make git python3 python3-pip

# Optional: Install CUDA for GPU acceleration
# Follow: https://developer.nvidia.com/cuda-downloads
```

### 2. Clone and Build

```bash
# Clone repository
git clone https://github.com/yourusername/ddos-detection-mpi.git
cd ddos-detection-mpi

# Build
./step2_build.sh

# Verify build
./ddos_detector --help 2>&1 || echo "Build successful"
```

### 3. Configure

```bash
# Create configuration directory
mkdir -p /etc/ddos-detector

# Copy example configuration (if available)
# Edit thresholds in detection.c or types.h as needed

# Create log directory
sudo mkdir -p /var/log/ddos-detector
sudo chown $USER:$USER /var/log/ddos-detector
```

## 🔧 Configuration

### Tuning Parameters

Edit `types.h`:
```c
#define WINDOW_SIZE 50000          // Adjust based on memory
#define MAX_BLOCKED_IPS 1000       // Increase for large networks
#define HASH_SIZE 10000            // Increase for better hash distribution
```

Edit `detection.c`:
```c
#define ENTROPY_THRESHOLD 2.5      // Lower = more sensitive
#define RATE_THRESHOLD 10000       // Bytes/sec threshold
#define CUSUM_THRESHOLD 5.0        // Statistical threshold
```

Rebuild after changes:
```bash
./step2_build.sh
```

## 📊 Data Sources

### Network Flow Collection

#### Option 1: NetFlow/sFlow
```bash
# Install flow collector (e.g., nfdump)
sudo apt-get install nfdump

# Configure router to send flows
# Router config example (Cisco):
# ip flow-export destination <collector-ip> 9995
# ip flow-export version 9
```

#### Option 2: Packet Capture
```bash
# Use tcpdump or similar
sudo tcpdump -i eth0 -w /data/capture.pcap

# Convert to CSV (requires custom script or tool)
```

#### Option 3: Existing Dataset
```bash
# Place CIC-DDoS2019 or similar in datasets/
# Preprocess: ./step1_preprocess.sh <input> <output> <rows>
```

## 🏃 Running in Production

### Manual Execution

```bash
# Single run with 8 workers
./step3_run.sh /data/processed_flows.csv 8

# View results
cat results.txt
```

### Automated Execution (Cron)

```bash
# Edit crontab
crontab -e

# Add entry (run every hour)
0 * * * * cd /opt/ddos-detector && ./step3_run.sh /data/flows.csv 8 >> /var/log/ddos-detector/cron.log 2>&1
```

### Systemd Service

Create `/etc/systemd/system/ddos-detector.service`:
```ini
[Unit]
Description=DDoS Detection Service
After=network.target

[Service]
Type=simple
User=ddos-detector
WorkingDirectory=/opt/ddos-detector
ExecStart=/usr/bin/mpiexec -n 8 /opt/ddos-detector/ddos_detector /data/flows.csv
Restart=on-failure
RestartSec=10
StandardOutput=append:/var/log/ddos-detector/service.log
StandardError=append:/var/log/ddos-detector/service-error.log

[Install]
WantedBy=multi-user.target
```

Enable and start:
```bash
sudo systemctl daemon-reload
sudo systemctl enable ddos-detector
sudo systemctl start ddos-detector
sudo systemctl status ddos-detector
```

## 🛡️ Mitigation Integration

### Automatic Blocklist Application

#### 1. BGP FlowSpec (Recommended)
```bash
# Deploy generated rules to BGP router
scp flowspec_rules.txt admin@router:/config/

# On router (example):
# configure
# flowspec import-policy apply-ddos-rules
# commit
```

#### 2. Firewall ACLs
```bash
# Apply ACL rules to firewall
scp acl_rules.txt admin@firewall:/tmp/

# On firewall:
# Apply ACLs via CLI or API
```

#### 3. Automated Script
```bash
#!/bin/bash
# auto_mitigate.sh

# Run detection
./step3_run.sh /data/flows.csv 8

# Apply blocklist
if [ -f blocklist_8_ranks.txt ]; then
    while read line; do
        IP=$(echo $line | awk '{print $1}')
        if [[ $IP =~ ^[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
            # Add to iptables
            sudo iptables -A INPUT -s $IP -j DROP
            echo "Blocked: $IP"
        fi
    done < blocklist_8_ranks.txt
fi
```

## 📈 Monitoring

### Real-time Monitoring

```bash
# Terminal 1: Run detector
./step3_run.sh /data/flows.csv 8

# Terminal 2: Watch results
watch -n 2 'tail -n 30 results.txt'

# Terminal 3: Monitor blocklist
watch -n 5 'wc -l blocklist_8_ranks.txt'
```

### Log Aggregation

```bash
# Forward logs to centralized system (e.g., ELK stack)
# Configure filebeat or similar
```

### Metrics Export

```bash
# Export metrics to Prometheus/Grafana
# Parse results.txt and expose via HTTP endpoint
# (Requires custom exporter script)
```

## 🔒 Security Considerations

### 1. Access Control
```bash
# Restrict access to detector
sudo chown root:ddos-group /opt/ddos-detector/ddos_detector
sudo chmod 750 /opt/ddos-detector/ddos_detector

# Limit log access
sudo chmod 750 /var/log/ddos-detector
```

### 2. Data Privacy
- Anonymize IP addresses in logs if needed
- Encrypt stored datasets
- Secure MPI communication (use SSH for remote nodes)

### 3. False Positive Handling
- Maintain whitelist of trusted IPs
- Implement manual review process
- Set up alerting for high false positive rates

## ⚖️ Scaling Strategies

### Horizontal Scaling

#### Multi-Node MPI Cluster

Create hostfile:
```
# /etc/mpi/hostfile
node1.example.com slots=8
node2.example.com slots=8
node3.example.com slots=8
```

Run across cluster:
```bash
mpiexec -n 24 -f /etc/mpi/hostfile ./ddos_detector /data/flows.csv
```

#### Load Balancing
- Distribute network segments across nodes
- Use Round-robin DNS for data collection
- Implement message queue for flow data

### Vertical Scaling
- Add more CPU cores (increase MPI processes)
- Add GPU for acceleration
- Increase RAM for larger windows

## 🧪 Testing in Production

### Gradual Rollout
1. **Week 1**: Monitor-only mode (no blocking)
2. **Week 2**: Block with manual review
3. **Week 3**: Automated blocking with alerts
4. **Week 4**: Full automation

### Validation
```bash
# Compare with existing IDS/IPS
diff existing_alerts.txt results.txt

# Measure impact
# - Network performance
# - False positive rate
# - Attack mitigation effectiveness
```

## 🚨 Troubleshooting

### High False Positives
- Increase detection thresholds
- Add whitelists for known services
- Tune CUSUM baseline parameters

### Performance Issues
- Increase WINDOW_SIZE for batch processing
- Add more worker processes
- Enable GPU acceleration
- Profile with: `mpiexec -n 8 valgrind --tool=callgrind ./ddos_detector`

### Memory Issues
- Reduce WINDOW_SIZE
- Process dataset in chunks
- Increase system swap space

## 📞 Support & Maintenance

### Regular Tasks
- **Daily**: Check logs for errors
- **Weekly**: Review blocked IPs and false positives
- **Monthly**: Update detection thresholds based on trends
- **Quarterly**: Update system and dependencies

### Backup
```bash
# Backup configuration and results
tar -czf ddos-detector-backup-$(date +%Y%m%d).tar.gz \
    /opt/ddos-detector/types.h \
    /opt/ddos-detector/detection.c \
    /var/log/ddos-detector/ \
    results.txt blocklist_*.txt
```

## 📚 Additional Resources

- [MPI Documentation](https://www.mpich.org/documentation/)
- [CUDA Programming Guide](https://docs.nvidia.com/cuda/)
- [BGP FlowSpec RFC](https://tools.ietf.org/html/rfc5575)
- [CIC-DDoS2019 Dataset](https://www.unb.ca/cic/datasets/ddos-2019.html)

---

For questions, open an issue on GitHub or contact the development team.
