#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include "UDP.h"

void udp_send(char *ds_ip, char* ds_port, char *message){
  int fd,errcode;
  ssize_t n;
  socklen_t addrlen;
  struct addrinfo hints,*res;
  struct sockaddr_in addr;
  char buffer[128];

  fd = socket(AF_INET, SOCK_DGRAM, 0);
  if(fd == -1){
    fprintf(stderr, "Error creating socket\n");
    return;
  }

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_DGRAM;

  errcode = getaddrinfo(ds_ip, ds_port, &hints, &res);

  if(errcode != 0){
    fprintf(stderr, "Error getting server\n");
    freeaddrinfo(res);
    close(fd);
    return;
  }

  n = sendto(fd, message, sizeof(message), 0, res->ai_addr, res->ai_addrlen);
  if(n == -1){
    fprintf(stderr, "Error sending to server\n");
    freeaddrinfo(res);
    close(fd);
    return;
  }

  addrlen = sizeof(addr);
  n = recvfrom(fd, buffer, 128, 0,(struct sockaddr*) &addr, &addrlen);
  if(n == -1){
    fprintf(stderr, "Error receiving from server\n");
    freeaddrinfo(res);
    close(fd);
    return;
  }

  printf("Receive from server: %s\n", buffer);

  /*write(1, "echo: ", 6); write(1, buffer, n);*/

  freeaddrinfo(res);
  close(fd);
}
