/*-----------------------------------------------------------------------------
 * File: nat.h
 * Date: 10/30/18
 * Author: Gabriele Fisher
 * Contact: gsfisher@stanford.edu
 *
 *---------------------------------------------------------------------------*/

#ifndef NAT_H
#define NAT_H

#include <sys/time.h>
#include <stdio.h>
#include <stdbool.h>

#define MAX_IP_SZ 16 // maximum of 15 chars in any IP address, plus null term

typedef struct nat_table_entry {
  uint16_t ext_port;
  char ext_ip[MAX_IP_SZ];
  uint16_t int_port;
  char int_ip[MAX_IP_SZ];

  struct nat_table_entry *next;
} nat_table_entry_t;

typedef struct nat_config {
  nat_table_entry_t *nat_table;
  nat_table_entry_t *wildcard_port;
  nat_table_entry_t *wildcard_ip;
  nat_table_entry_t *wildcard_ip_port;

} nat_config_t;

#endif /* NAT_H */
