#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include "connection_manager.h"
#include "../../protocol_constants.h"

int fd;
char buffer[128];

int parse_status(const char* status)
{
  
  if(strncmp(status,"OK", 2) == 0)
  {
    return OK;
  }
  else if(strncmp(status,"NOK", 3) == 0)
  {
    return NOK;
  }
  else{
    return -1;
  }
}


//*UDP transmissions

int send_message_udp(const char *ds_ip, const char* ds_port, const char *message, int size)
{
  ssize_t n;
  struct addrinfo hints, *res;

  fd = socket(AF_INET, SOCK_DGRAM, 0);
  if(fd == -1)
  {
    fprintf(stderr, "Error creating socket\n");
    return NOK;
  }

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_DGRAM;

  if(getaddrinfo(ds_ip, ds_port, &hints, &res) != 0)
  {
    fprintf(stderr, "Error getting server\n");
    freeaddrinfo(res);
    return NOK;
  }

  n = sendto(fd, message, size, 0, res->ai_addr, res->ai_addrlen);
  if(n == -1)
  {
    fprintf(stderr, "Error sending to server\n");
    freeaddrinfo(res);
    return NOK;
  }

  freeaddrinfo(res);
  return OK;
}


int receive_message_udp()
{
  socklen_t addrlen;
  struct sockaddr_in addr;
  char buffer[BUFFER];
  char buf[3];
  char *token;
  int j,i, count;
  addrlen = sizeof(addr);
  ssize_t n;

  n = recvfrom(fd, buffer, sizeof(buffer), 0, (struct sockaddr*) &addr, &addrlen);
  if(n == -1)
  {
    fprintf(stderr, "Error receiving from server\n");
    return NOK;
  }
  //FIXME this is an optimistc approach, i.e. the server respects protocol
  //TODO verify everything
  if(strncmp(buffer, "RGL", 3) == 0 || strncmp(buffer, "RGM", 3) == 0)
  { 
    snprintf(buf, 3, "%s\n", buffer + 4);
    j = atoi(buf);
    token = strtok(buffer + 6, " ");
    for (i = 1; i < j; i++){
      count = 2;
      while (count > 0){
        printf("%s ", token);
        token = strtok(NULL, " ");
        count--;
      }
      token = strtok(NULL, " ");
      printf("\n");
    }
    count = 2;
    while (count > 0){
      printf("%s ", token);
      token = strtok(NULL, " ");
      count--;
    }
    printf("\n");
    //printf("display groups or my_groups response\n");
  }
  else
  {
    write(1, buffer, n);
    return parse_status(buffer + 4);
  }

  return OK;
}


int udp_send(const char *ds_ip, const char* ds_port, const char *message, int size)
{  
  int status;
  
  if(send_message_udp(ds_ip, ds_port, message, size) == NOK)
  {
    close(fd);
    return NOK;
  }
  
  status = receive_message_udp();
  close(fd);

  return status;
}


//*TCP transmissions 

/*WIP*/
int send_message_tcp(const char* ds_ip, const char* ds_port, const char *message, int size)
{
  struct addrinfo hints, *res;
  int n, errcode;

  fd = socket(AF_INET, SOCK_STREAM, 0);
  if(fd == -1)
  {
    fprintf(stderr, "Error creating socket\n");
    return NOK;
  }

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;

  errcode = getaddrinfo(ds_ip, ds_port, &hints, &res);
  if(errcode != 0)
  {
    fprintf(stderr, "Error getting the server\n");
    freeaddrinfo(res);
    return NOK;
  }

  n = connect(fd, res->ai_addr, res->ai_addrlen);
  if(n == -1)
  {
    freeaddrinfo(res);
    return NOK;
  }

  /*SIZE AQUI*/
  if((n = write(fd,message, size) == -1))
  {
    fprintf(stderr, "Error sending to server\n");
    return NOK;
  }


  return OK;

}


/*copied, needs adjustments*/
int receive_message_tcp()
{
  int n;
  char buffer[128];

  if((n = read(fd, buffer, 128)) == -1) 
  {
    fprintf(stderr, "Error receiving from server\n");
    return NOK;
  }
  
  printf("AYAYAYAYAYAYAYA BENNY LOUCO\n");

  return OK;
}

//TODO replicate similar UDP functions for TCP operations
int tcp_send(const char* ds_ip, const char* ds_port, const char* message, int size) {
  int status;

  if(send_message_tcp(ds_ip,ds_port,message,size) == NOK)
  {
    close(fd);
    return NOK;
  }

  status = receive_message_tcp();
  close(fd);

  return status;
}