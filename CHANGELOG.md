# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.0.0] - 2024-11-22

### Added
- Initial release of DDoS Detection and Mitigation System
- MPI-based distributed detection architecture (master-worker model)
- Three detection algorithms:
  - Entropy-based detection for IP diversity analysis
  - PCA-based anomaly detection
  - CUSUM statistical detection for sustained deviations
- Optional CUDA GPU acceleration for entropy and rate-based detection
- Dynamic IP blocklist management with real-time updates
- Comprehensive metrics collection:
  - Detection accuracy (precision, recall, F1-score)
  - Processing latency (P95, P99 percentiles)
  - Resource utilization (CPU, memory)
  - MPI communication overhead
- Automated mitigation rule generation:
  - BGP FlowSpec rules
  - Router ACL configurations
  - RTBH (Remotely Triggered Black Hole) routing
- Scalability testing framework
- Performance visualization with Python graphs
- Support for CIC-DDoS2019 dataset format
- CSV parser for large datasets
- Preprocessing scripts for dataset preparation
- Automated build and run scripts
- Complete documentation (README, CONTRIBUTING)
- MIT License

### Performance
- Processes 10,000+ flows per second per worker
- Sub-millisecond detection latency per flow
- Linear scaling with additional workers
- ~95% detection accuracy on test datasets

### Supported Platforms
- Linux (Ubuntu 20.04+)
- Windows WSL2
- Any Unix-like system with MPI support

## [Unreleased]

### Planned Features
- Machine learning-based detection
- Real-time dashboard for monitoring
- Docker containerization
- REST API for integration
- Additional dataset format support
- Automated parameter tuning
- Distributed training capabilities

---

## Version History

- **v1.0.0** (2024-11-22): Initial public release
