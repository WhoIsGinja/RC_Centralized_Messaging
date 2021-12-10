#include "TCP.h"


char *tcpClient(char *ds_ip, int ds_port, char*message) {
    int fd,errcode;
    ssize_t n;
    socklen_t addrlen;
    struct addrinfo hints,*res;
    struct sockaddr_in addr;
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

    freeaddrinfo(res);
    close(fd);
    return buffer;
}