# Contributing to DDoS Detection System

Thank you for your interest in contributing to this project! This document provides guidelines for contributing.

## 🚀 Getting Started

1. Fork the repository
2. Clone your fork: `git clone https://github.com/yourusername/ddos-detection-mpi.git`
3. Create a branch: `git checkout -b feature/your-feature-name`
4. Make your changes
5. Test your changes thoroughly
6. Commit with clear messages
7. Push to your fork
8. Open a Pull Request

## 💡 How to Contribute

### Reporting Bugs
- Use GitHub Issues
- Describe the bug clearly
- Include steps to reproduce
- Provide system information (OS, GCC version, MPI version)
- Include error messages and logs

### Suggesting Features
- Open a GitHub Issue with the "enhancement" label
- Describe the feature and its benefits
- Explain the use case
- Discuss potential implementation approaches

### Submitting Code
1. Follow the existing code style
2. Add comments for complex logic
3. Update documentation if needed
4. Test on multiple configurations
5. Ensure no compilation warnings

## 📋 Code Standards

### C Code Style
```c
// Use meaningful variable names
int flow_count;  // Good
int fc;          // Avoid

// Add function documentation
/**
 * Detects DDoS attacks using entropy analysis
 * @param flows: Array of network flows
 * @param count: Number of flows
 * @return: Number of attacks detected
 */
int detect_entropy_based(FlowRecord *flows, int count);

// Use consistent indentation (4 spaces)
if (condition) {
    // code here
}

// Error checking
if (ptr == NULL) {
    fprintf(stderr, "Error: Memory allocation failed\n");
    return -1;
}
```

### Commit Messages
```
feat: Add GPU acceleration for PCA detection
fix: Correct memory leak in CSV parser
docs: Update installation instructions
perf: Optimize entropy calculation
refactor: Simplify blocklist management
test: Add unit tests for detection algorithms
```

## 🧪 Testing

### Before Submitting
1. Build the project: `./step2_build.sh`
2. Run tests: `./step3_run.sh datasets/test_data.csv 4`
3. Check for memory leaks: `valgrind ./ddos_detector test.csv`
4. Verify no warnings: `mpicc -Wall -Wextra ...`

### Test Cases
- Test with 2, 4, 8 workers
- Test with small and large datasets
- Test CPU and GPU versions
- Test error conditions

## 📝 Documentation

- Update README.md for new features
- Add inline comments for complex code
- Document API changes
- Include usage examples

## 🔍 Code Review Process

1. Automated checks run on PR
2. Maintainers review code
3. Address feedback
4. Get approval
5. Merge!

## 🤔 Questions?

Open an issue or reach out to the maintainers.

## 📜 License

By contributing, you agree that your contributions will be licensed under the MIT License.

Thank you for contributing! 🎉
