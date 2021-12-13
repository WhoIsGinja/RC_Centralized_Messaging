#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include "TCP.h"

void tcp_send(char* ds_ip, char* ds_port, char* message) {
    int fd,errcode;
    ssize_t n;
    struct addrinfo hints,*res;
    char buffer[128];

    fd = socket(AF_INET,SOCK_STREAM,0);
    if(fd == -1) exit(1);

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    errcode = getaddrinfo(ds_ip, ds_port,&hints,&res);
    if(errcode != 0) exit(1);

    n = connect(fd, res->ai_addr,res->ai_addrlen);
    if(n == -1) exit(1);

    n = write(fd, message, sizeof(message));
    if(n == -1) exit(1);

    n = read(fd, buffer, 128);
    if(n == -1) exit(1);

    printf("Receive from server: %s", buffer);

    freeaddrinfo(res);
    close(fd);
    //return buffer;
}