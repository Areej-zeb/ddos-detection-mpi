#ifndef CSV_PARSER_H
#define CSV_PARSER_H

#include "types.h"
#include <stdio.h>

// Parse a single CSV line into a FlowRecord
// Only extracts essential columns for performance
int parse_csv_line(char *line, FlowRecord *record);

// Count total lines in CSV (for partitioning)
int count_csv_lines(const char *filename);

// Read a chunk of flows from CSV (for MPI workers)
int read_csv_chunk(const char *filename, int start_line, int count, FlowRecord *records, int *actual_read);

#endif
