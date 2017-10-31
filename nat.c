/*-----------------------------------------------------------------------------
 * File: nat.c
 * Date: 10/29/18
 * Author: Gabriele Fisher
 * Contact: gsfisher@stanford.edu
 *
 *---------------------------------------------------------------------------*/

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

#include "nat.h"

#define MAX_IP_SZ 16 // maximum of 15 chars in any IP address, plus null term

typedef struct nat_table_entry {
  uint16_t ext_port;
  char ext_ip[MAX_IP_SZ];
  uint16_t int_port;
  char int_ip[MAX_IP_SZ];

  struct nat_table_entry *next;
} nat_table_entry_t;

typedef struct wildcard_port {
  char ext_ip[MAX_IP_SZ];
  uint16_t int_port;
  char int_ip[MAX_IP_SZ];
} wildcard_port_t;

typedef struct wildcard_ip {
  uint16_t ext_port;
  uint16_t int_port;
  char int_ip[MAX_IP_SZ];
} wildcard_ip_t;

typedef struct wildcard_ip_port {
  uint16_t int_port;
  char int_ip[MAX_IP_SZ];
} wildcard_ip_port_t;

typedef struct nat_config {
  nat_table_entry_t *nat_table;
  wildcard_port_t *wildcard_port;
  wildcard_ip_t *wildcard_ip;
  wildcard_ip_port_t *wildcard_ip_port;

} nat_config_t;

int add_wildcard_port(const char *nat_params[], nat_config_t *nat_config){
  // If wildcard port already designated in NAT, allow overwrite.
  wildcard_port_t *new_entry = (wildcard_port_t *)
    calloc(sizeof(wildcard_port_t), 1);

  memset(&new_entry->ext_ip, 0, sizeof(new_entry->ext_ip));
  memcpy(&new_entry->ext_ip, nat_params[0], strlen(nat_params[0]));

  memset(&new_entry->int_ip, 0, sizeof(new_entry->int_ip));
  memcpy(&new_entry->int_ip, nat_params[2], strlen(nat_params[2]));
  new_entry->int_port = (uint16_t)strtoul(nat_params[3], NULL, 10);

  nat_config->wildcard_port = new_entry;
  return 1;
}

int add_wildcard_ip(const char *nat_params[], nat_config_t *nat_config){
  // If wildcard ip addr already designated in NAT, allow overwrite.
  wildcard_ip_t *new_entry = (wildcard_ip_t *)calloc(sizeof(wildcard_ip_t), 1);

  new_entry->ext_port = (uint16_t)strtoul(nat_params[1], NULL, 10);

  memset(&new_entry->int_ip, 0, sizeof(new_entry->int_ip));
  memcpy(&new_entry->int_ip, nat_params[2], strlen(nat_params[2]));
  new_entry->int_port = (uint16_t)strtoul(nat_params[3], NULL, 10);

  nat_config->wildcard_ip = new_entry;
  return 1;
}

int add_ip_port_pair(const char *nat_params[], nat_config_t *nat_config){
  nat_table_entry_t *new_entry = (nat_table_entry_t *)
    calloc(sizeof(nat_table_entry_t), 1);

  memset(&new_entry->ext_ip, 0, sizeof(new_entry->ext_ip));
  memcpy(&new_entry->ext_ip, nat_params[0], strlen(nat_params[0]));
  new_entry->ext_port = (uint16_t)strtoul(nat_params[1], NULL, 10);

  memset(&new_entry->int_ip, 0, sizeof(new_entry->int_ip));
  memcpy(&new_entry->int_ip, nat_params[2], strlen(nat_params[2]));
  new_entry->int_port = (uint16_t)strtoul(nat_params[3], NULL, 10);

  new_entry->next = nat_config->nat_table;
  nat_config->nat_table = new_entry;
  return 1;
}

int add_wildcard_ip_port(const char *nat_params[], nat_config_t *nat_config){
  wildcard_ip_port_t *new_entry = (wildcard_ip_port_t *)
    calloc(sizeof(wildcard_ip_port_t), 1);

  memset(&new_entry->int_ip, 0, sizeof(new_entry->int_ip));
  memcpy(&new_entry->int_ip, nat_params[2], strlen(nat_params[2]));
  new_entry->int_port = (uint16_t)strtoul(nat_params[3], NULL, 10);
  nat_config->wildcard_ip_port = new_entry;
  return 1;
}

/*
 * populate_nat_table copies entries in the nat_config input file into
 * nat_table_entry structs.
 */
int populate_nat_table(FILE *nat_file, nat_config_t *nat_config) {
  char *line = NULL;
  size_t len = 0;
  ssize_t read;

  const char delim[] = ",:";

  while ((read = getline(&line, &len, nat_file)) != -1) {
    const char *nat_params[4];
    char *nat_param = strtok(line, delim);
    nat_params[0] = nat_param;
    int count = 0;
    while (nat_param != NULL)
    {
      printf ("%s\n", nat_param);
      nat_params[count] = nat_param;
      nat_param = strtok(NULL, delim);
      count++;
    }

    // Check if wildcard IP-port pair entry is present
    if (strcmp(nat_params[0], "*") == 0 && strcmp(nat_params[1], "*") == 0) {
      printf("total wildcard\n");
      if (add_wildcard_ip_port(nat_params, nat_config) != 1) return -1;
    }
    else if (strcmp(nat_params[0], "*") == 0) {
      printf("wildcard ip\n");
      if (add_wildcard_ip(nat_params, nat_config) != 1) return -1;
    }
    else if (strcmp(nat_params[1], "*") == 0) {
      printf("wildcard port\n");
      if (add_wildcard_port(nat_params, nat_config) != 1) return -1;
    }
    else {
      printf("normal entry\n");
      if (add_ip_port_pair(nat_params, nat_config) != 1) return -1;
    }

  }
  return 1;
}

int find_ip_port_match(FILE *output, char *query_ip, uint16_t query_port,
                       nat_config_t *nat_config) {
  nat_table_entry_t *curr = nat_config->nat_table;
  while (curr != NULL) {
    if (strcmp(query_ip, curr->ext_ip) == 0 &&
        query_port == curr->ext_port) {
      fprintf(output, "%s:%hu -> %s:%hu\n", query_ip, query_port, curr->int_ip,
        curr->int_port);
      return 1;
    }
    curr = curr->next;
  }
  return -1;
}

int nat_matching(FILE *flow, nat_config_t *nat_config){
  FILE *output = fopen("output.txt", "w");

  char *line = NULL;
  size_t len = 0;
  ssize_t read;

  const char delim[] = ",:";

  while ((read = getline(&line, &len, flow)) != -1) {
    char *query_ip = strtok(line, delim);
    uint16_t query_port = (uint16_t)strtoul(strtok(NULL, delim), NULL, 10);

    // First check if there is ip-port pair match
    if(find_ip_port_match(output, query_ip, query_port, nat_config) == 1) {
      continue;
    }
    // If no ip-port pair match, then look for wildcard IP match
    wildcard_port_t *wildcard_port = nat_config->wildcard_port;
    if (strcmp(wildcard_port->ext_ip, query_ip) == 0) {
      fprintf(output, "%s:%hu -> %s:%hu\n", query_ip, query_port,
        wildcard_port->int_ip, wildcard_port->int_port);
      continue;
    }
    // If no ip-port pair match, then look for wildcard port match
    wildcard_ip_t *wildcard_ip = nat_config->wildcard_ip;
    if (wildcard_ip->ext_port == query_port) {
      fprintf(output, "%s:%hu -> %s:%hu\n", query_ip, query_port,
        wildcard_ip->int_ip, wildcard_ip->int_port);
      continue;
    }

    wildcard_ip_port_t *wildcard_ip_port = nat_config->wildcard_ip_port;
    if (nat_config->wildcard_ip_port != NULL){
      fprintf(output, "%s:%hu -> %s:%hu\n", query_ip, query_port,
        wildcard_ip_port->int_ip, wildcard_ip_port->int_port);
      continue;
    }

    fprintf(output, "No NAT match for %s:%hu\n", query_ip, query_port);
  }

  fclose(output);
  return 1;
}

int nat_init(nat_config_t *config){
  config->nat_table = NULL;
  config->wildcard_port = NULL;
  config->wildcard_ip = NULL;
  config->wildcard_ip_port = NULL;

  return 1;
}

int main (int argc, char *argv[]) {
  if ( argc != 3 ) /* argc should be 2 for correct execution */
    {
        /* We print argv[0] assuming it is the program name */
        printf("usage: %s NAT_filename FLOW_filename\n", argv[0]);
        exit(1);
    }

    nat_config_t config;
    memset(&config, 0, sizeof(nat_config_t));
    nat_init(&config);

    // We assume argv[1] is a filename to open
    FILE *config_fd = fopen( argv[1], "r" );
    if ( config_fd == 0 ) {
      printf("Could not open file %s\n", argv[1]);
      return 1;
    }
    if (populate_nat_table(config_fd, &config) != 1) {
      printf("NAT table input is improperly formatted.\n");
      return 1;
    }

    // Now, run comparisons with inputs in FLOW file.
    FILE *flow_fd = fopen( argv[2], "r" );
    if ( flow_fd == 0 ) {
      printf("Could not open file %s\n", argv[1]);
      return 1;
    }
    nat_matching(flow_fd, &config);

    fclose(flow_fd);
    fclose(config_fd);

    return 0;
}
