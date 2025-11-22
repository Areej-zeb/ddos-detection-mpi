#include "csv_parser.h"
#include <string.h>
#include <stdlib.h>

// Extract field by index from CSV line (optimized)
static char *get_field(char *line, int field_num)
{
    static char field[256];
    char *ptr = line;
    int current = 0;

    // Fast-forward to target field
    while (*ptr && current < field_num)
    {
        if (*ptr == ',')
            current++;
        ptr++;
    }

    // Fast copy until delimiter (optimized tight loop)
    char *start = ptr;
    char *end = ptr;
    while (*end && *end != ',' && *end != '\n' && *end != '\r')
        end++;

    int len = end - start;
    if (len > 255)
        len = 255;
    memcpy(field, start, len);
    field[len] = '\0';
    return field;
}

// Parse CSV line - extract only essential columns
// This parser assumes the *processed* CSV layout produced by scripts/preprocess_all.sh:
// Columns (0-indexed after preprocessing):
// 0: Source IP
// 1: Source Port
// 2: Destination IP
// 3: Destination Port
// 4: Protocol
// 5: Flow Duration
// 6: Total Fwd Packets
// 7: Total Backward Packets
// 8: Total Length of Fwd Packets
// 9: Total Length of Bwd Packets
// 10: Flow Bytes/s
// 11: Flow Packets/s
// 12: Min Packet Length
// 13: Max Packet Length
// 14: Packet Length Mean
// 15: Packet Length Std
// 16: Flow IAT Mean
// 17: Flow IAT Std
// 18: Label
int parse_csv_line(char *line, FlowRecord *record)
{
    if (!line || !record)
        return -1;
    if (strlen(line) < 10)
        return -1; // Skip empty/short lines

    // Extract Source IP (column 0)
    char *field = get_field(line, 0);
    strncpy(record->src_ip, field, MAX_IP_LEN - 1);
    record->src_ip[MAX_IP_LEN - 1] = '\0';

    // Extract Source Port (column 1)
    field = get_field(line, 1);
    record->src_port = atoi(field);

    // Extract Destination IP (column 2)
    field = get_field(line, 2);
    strncpy(record->dst_ip, field, MAX_IP_LEN - 1);
    record->dst_ip[MAX_IP_LEN - 1] = '\0';

    // Extract Destination Port (column 3)
    field = get_field(line, 3);
    record->dst_port = atoi(field);

    // Extract Protocol (column 4)
    field = get_field(line, 4);
    record->protocol = atoi(field);

    // Flow duration (column 5)
    field = get_field(line, 5);
    record->flow_duration = atof(field);

    // Total Fwd/Back packets (6,7)
    field = get_field(line, 6);
    record->total_fwd_packets = atof(field);
    field = get_field(line, 7);
    record->total_bwd_packets = atof(field);

    // Total Fwd/Back bytes (8,9)
    field = get_field(line, 8);
    record->total_fwd_bytes = atof(field);
    field = get_field(line, 9);
    record->total_bwd_bytes = atof(field);

    // Flow Bytes/s (10)
    field = get_field(line, 10);
    record->flow_bytes_per_sec = atof(field);

    // Flow Packets/s (11)
    field = get_field(line, 11);
    record->flow_packets_per_sec = atof(field);

    // Min/Max/Mean/Std packet length (12-15)
    field = get_field(line, 12);
    record->min_packet_length = atof(field);
    field = get_field(line, 13);
    record->max_packet_length = atof(field);
    field = get_field(line, 14);
    record->packet_length_mean = atof(field);
    field = get_field(line, 15);
    record->packet_length_std = atof(field);

    // Flow IAT Mean/Std (16-17)
    field = get_field(line, 16);
    record->flow_iat_mean = atof(field);
    field = get_field(line, 17);
    record->flow_iat_std = atof(field);

    // Label (column 18)
    field = get_field(line, 18);
    // If label contains "DrDoS" or "DDoS", it's an attack
    record->is_attack = (strstr(field, "DrDoS") != NULL || strstr(field, "DDoS") != NULL) ? 1 : 0;

    return 0;
}

// Count lines in CSV file
int count_csv_lines(const char *filename)
{
    FILE *fp = fopen(filename, "r");
    if (!fp)
        return -1;

    int count = 0;
    char line[MAX_LINE_LEN];

    while (fgets(line, sizeof(line), fp))
    {
        if (count > 0 || line[0] != 'U')
        { // Skip header starting with "Unnamed"
            count++;
        }
    }

    fclose(fp);
    return count;
}

// Read a chunk of flows from CSV (optimized with buffering)
int read_csv_chunk(const char *filename, int start_line, int count, FlowRecord *records, int *actual_read)
{
    FILE *fp = fopen(filename, "r");
    if (!fp)
    {
        *actual_read = 0;
        return -1;
    }

    // Use larger buffer for faster I/O
    setvbuf(fp, NULL, _IOFBF, 65536);

    char line[MAX_LINE_LEN];
    int current_line = -1; // -1 for header
    int records_read = 0;
    int end_line = start_line + count;

    // Fast skip to start_line
    while (current_line < start_line && fgets(line, sizeof(line), fp))
    {
        current_line++;
    }

    // Read target chunk
    while (records_read < count && current_line < end_line && fgets(line, sizeof(line), fp))
    {
        if (parse_csv_line(line, &records[records_read]) == 0)
        {
            records_read++;
        }
        current_line++;
    }

    fclose(fp);
    *actual_read = records_read;
    return 0;
}
