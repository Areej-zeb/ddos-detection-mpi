#ifndef METRICS_H
#define METRICS_H

#include "types.h"

// Initialize metrics
void init_metrics(Metrics *m);

// Update metrics based on ground truth vs detection
void update_metrics(Metrics *m, FlowRecord *flows, int count, Alert *alerts, int alert_count, BlockedIP *blocklist, int blocklist_count);

// Calculate derived metrics
void calculate_final_metrics(Metrics *m, double *precision, double *recall, double *f1, double *fpr, double *accuracy);

// Print metrics report
void print_metrics(Metrics *m, int rank);

// Save metrics to file
void save_metrics_to_file(Metrics *m, const char *filename, int num_ranks);

#endif
