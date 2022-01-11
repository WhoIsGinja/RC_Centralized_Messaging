#ifndef CONNECTION_MANAGER_H
#define CONNECTION_MANAGER_H

#include <stdbool.h>


bool regex_test(const char *rule, const char *str);

int udp_send(const char *ds_ip, const char* ds_port, char *message, int size);

int tcp_send(const char* ds_ip, const char* ds_port, char* message, int size, char* filename);

int tcp_send_file(const char* ds_ip, const char* ds_port, char* message, int size);

#endif