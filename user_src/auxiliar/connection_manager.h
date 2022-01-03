#ifndef CONNECTION_MANAGER_H
#define CONNECTION_MANAGER_H

int udp_send(const char *ds_ip, const char* ds_port, const char *message, int size);

int tcp_send(const char* ds_ip, const char* ds_port, const char* message, int size);

#endif