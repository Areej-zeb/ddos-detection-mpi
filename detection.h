#ifndef DETECTION_H
#define DETECTION_H

#include "types.h"

// Initialize detection system (checks for CUDA availability)
void detection_init();

// Entropy-based detection: Calculates IP diversity (Shannon entropy)
// High entropy = distributed sources (normal), Low entropy = concentrated (DDoS)
int detect_entropy_based(FlowRecord *flows, int count, Alert *alerts, int *alert_count);

// PCA (Principal Component Analysis) detection: Detects anomalies in multi-dimensional space
// Projects flow features to principal components and flags outliers
int detect_pca_based(FlowRecord *flows, int count, Alert *alerts, int *alert_count);

// CUSUM statistical detection: Detects sustained deviations from baseline
// Tracks cumulative sum of deviations to detect persistent anomalies
int detect_cusum_based(FlowRecord *flows, int count, Alert *alerts, int *alert_count);

// Combined detection runner
int run_detection(FlowRecord *flows, int count, Alert *alerts, int *alert_count);

#endif
