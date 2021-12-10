#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "UDP.h"
#define PORT "58001"

void udp(char *ds_ip, int ds_port, char *message){
  int fd,errcode;
  ssize_t n;
  socklen_t addrlen;
  struct addrinfo hints,*res;
  struct sockaddr_in addr;
  char buffer[128];

  fd = socket(AF_INET, SOCK_DGRAM, 0);
  if(fd == -1) exit(1);

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_DGRAM;

  if(gethostname(buffer, 128) == -1)
    fprint(stderr, "error: %s\n", stderror(errno));

  errcode = getaddrinfo(ds_ip, ds_port, &hints, &res);
  if(errcode != 0) exit(1);


  n = sendto(fd, message, sizeof(message), 0, res->ai_addr, res->ai_addrlen);
  if(n ==-1) exit(1);

  addrlen = sizeof(addr);
  n = recvfrom(fd, buffer, 128, 0,(struct sockaddr*) &addr, &addrlen);
  if(n == -1) exit(1);


  printf("receive from server: %s", buffer);

  /*write(1, "echo: ", 6); write(1, buffer, n);*/

  freeaddrinfo(res);
  close(fd);



}
