#ifndef BLOCKING_H
#define BLOCKING_H

#include "types.h"

// Initialize blocklist
void init_blocklist();

// Add IP to blocklist (RTBH-style)
int add_to_blocklist(const char *ip, double timestamp);

// Check if IP is blocked
int is_blocked(const char *ip);

// Get current blocklist
int get_blocklist(BlockedIP *list, int max_size);

// Clear blocklist
void clear_blocklist();

// Generate FlowSpec rules for blocked IPs
void generate_flowspec_rules(const char *filename, BlockedIP *blocklist, int count);

// Generate ACL rules for blocked IPs
void generate_acl_rules(const char *filename, BlockedIP *blocklist, int count);

#endif
