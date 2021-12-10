#ifndef UDP
#define UDP

#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>

#include "UDP.c"

void udp_send(char *ds_ip, int ds_port, char *message);

#endif