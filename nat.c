/*-----------------------------------------------------------------------------
 * File: nat.c
 * Date: 10/30/18
 * Author: Gabriele Fisher
 * Contact: gsfisher@stanford.edu
 *
 *---------------------------------------------------------------------------*/

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "nat.h"

/*
 * populate_nat_table copies entries in the nat_config input file into
 * nat_table_entry structs.
 */
void populate_nat_table(FILE *nat_file, nat_config_t *nat_config) {
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
      nat_params[count] = nat_param;
      nat_param = strtok(NULL, delim);
      count++;
    }

    nat_table_entry_t *new_entry = (nat_table_entry_t *)
      calloc(sizeof(nat_table_entry_t), 1);

    memcpy(&new_entry->int_ip, nat_params[2], strlen(nat_params[2]));
    new_entry->int_port = (uint16_t)strtoul(nat_params[3], NULL, 10);

    // Add to IP addr-port wildcard linkedlist
    if (strcmp(nat_params[0], "*") == 0 && strcmp(nat_params[1], "*") == 0) {
      memcpy(&new_entry->ext_ip, nat_params[0], strlen(nat_params[0]));
      new_entry->ext_port = 0;

      new_entry->next = nat_config->wildcard_ip_port;
      nat_config->wildcard_ip_port = new_entry;
    } // Else add to IP addr wildcard linked list
    else if (strcmp(nat_params[0], "*") == 0) { // wildcard IP
      memcpy(&new_entry->ext_ip, nat_params[0], strlen(nat_params[0]));
      new_entry->ext_port = (uint16_t)strtoul(nat_params[1], NULL, 10);

      new_entry->next = nat_config->wildcard_ip;
      nat_config->wildcard_ip = new_entry;
    } // Else add to port wildcard linked list
    else if (strcmp(nat_params[1], "*") == 0) { // wildcard port
      memcpy(&new_entry->ext_ip, nat_params[0], strlen(nat_params[0]));
      new_entry->ext_port = 0;

      new_entry->next = nat_config->wildcard_port;
      nat_config->wildcard_port = new_entry;
    }
    else { // Else exact IP addr-port pair given, add to NAT table linked list.
      memcpy(&new_entry->ext_ip, nat_params[0], strlen(nat_params[0]));
      new_entry->ext_port = (uint16_t)strtoul(nat_params[1], NULL, 10);

      new_entry->next = nat_config->nat_table;
      nat_config->nat_table = new_entry;
    }
  }
  free(line);
}

/*
 * Attempts to find exact matches for ip-port pairs in flow.txt; if
 * no direct mapping exists, sees if wildcard mapping exists
 */
int print_match(FILE *output, char *query_ip, uint16_t query_port,
                       nat_table_entry_t *nat_table) {
  nat_table_entry_t *curr = nat_table;
  while (curr != NULL) {
    // if exact IP addr-port match
    if (strcmp(query_ip, curr->ext_ip) == 0 &&
        query_port == curr->ext_port) {
      fprintf(output, "%s:%hu -> %s:%hu\n", query_ip, query_port, curr->int_ip,
        curr->int_port);
      return 1;
    }
    else if (strcmp(query_ip, curr->ext_ip) == 0 && curr->ext_port == 0) {
      // port wildcard case
      fprintf(output, "%s:%hu -> %s:%hu\n", query_ip, query_port,
        curr->int_ip, curr->int_port);
      return 1;
    }
    else if (strcmp(curr->ext_ip, "*") == 0 && query_port == curr->ext_port) {
      // IP addr wildcard case
      fprintf(output, "%s:%hu -> %s:%hu\n", query_ip, query_port,
        curr->int_ip, curr->int_port);
      return 1;
    }
    else if (strcmp(curr->ext_ip, "*") == 0 && curr->ext_port == 0) {
      // IP addr and port wildcard case
      fprintf(output, "%s:%hu -> %s:%hu\n", query_ip, query_port,
        curr->int_ip, curr->int_port);
      return 1;
    }
    curr = curr->next;
  }
  return -1;
}

/*
 * Attempts to find exact matches for ip-port pairs in flow.txt; if
 * no direct mapping exists, sees if wildcard mapping exists
 */
void nat_matching(FILE *flow, nat_config_t *nat_config){
  FILE *output = fopen("output.txt", "w");

  char *line = NULL;
  size_t len = 0;
  ssize_t read;

  const char delim[] = ",:";

  while ((read = getline(&line, &len, flow)) != -1) {
    char *query_ip = strtok(line, delim);
    uint16_t query_port = (uint16_t)strtoul(strtok(NULL, delim), NULL, 10);

    // First check if there is ip-port pair match
    if (print_match(output, query_ip, query_port, nat_config->nat_table) == 1) {
      continue; // ip-port-pair match has been found.
    } // Look for wildcard options now that we have confirmed no match.
    else if (print_match(output, query_ip, query_port,
        nat_config->wildcard_ip) == 1) {
      continue; // wildcard IP addr match has been found.
    }
    else if (print_match(output, query_ip, query_port,
        nat_config->wildcard_port) == 1) {
      continue; // wildcard port match has been found.
    }
    else if (print_match(output, query_ip, query_port,
        nat_config->wildcard_ip_port) == 1) {
      continue;
    }
    else {
      fprintf(output, "No nat match for %s:%hu\n", query_ip, query_port);
    }
  }
  free(line);
  fclose(output);
}

/*
 * Initializes empty NAT configs
 */
void nat_init(nat_config_t *config){
  config->nat_table = NULL;
  config->wildcard_port = NULL;
  config->wildcard_ip = NULL;
  config->wildcard_ip_port = NULL;
}

/*
 * Free all entries in linkedlist.
 */
void free_linkedlist(nat_table_entry_t *curr) {
  while (curr != NULL) {
    nat_table_entry_t *tmp = curr;
    curr = curr->next;
    free(tmp);
  }
}

/*
 * Frees up all heap-allocated data in nat_config.
 */
void nat_free(nat_config_t *nat_config) {
  nat_table_entry_t *curr = nat_config->nat_table;
  free_linkedlist(curr);

  curr = nat_config->wildcard_ip;
  free_linkedlist(curr);

  curr = nat_config->wildcard_port;
  free_linkedlist(curr);

  curr = nat_config->wildcard_ip_port;
  free_linkedlist(curr);
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
    populate_nat_table(config_fd, &config);

    // Now, run comparisons with inputs in FLOW file.
    FILE *flow_fd = fopen( argv[2], "r" );
    if ( flow_fd == 0 ) {
      printf("Could not open file %s\n", argv[1]);
      return 1;
    }
    nat_matching(flow_fd, &config);

    // free heap-allocated memory
    nat_free(&config);

    fclose(flow_fd);
    fclose(config_fd);

    return 0;
}
