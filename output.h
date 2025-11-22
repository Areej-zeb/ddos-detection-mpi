#ifndef OUTPUT_H
#define OUTPUT_H

#include "types.h"

// Print comprehensive performance report with top attacking IPs
void print_comprehensive_report(Metrics *aggregated_metrics,
                                Alert *all_alerts, int alert_count,
                                BlockedIP *blocklist, int blocklist_count,
                                int num_ranks, double total_time_sec,
                                const char *dataset_file);

#endif // OUTPUT_H
