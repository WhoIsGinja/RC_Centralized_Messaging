#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include "../protocol_constants.h"

int fd;

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
  addrlen = sizeof(addr);
  ssize_t n;

  n = recvfrom(fd, buffer, sizeof(buffer), 0, (struct sockaddr*) &addr, &addrlen);
  if(n == -1)
  {
    fprintf(stderr, "Error receiving from server\n");
    return NOK;
  }

  write(1, buffer, n);

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
  int n;// offset, jump = 6;
  char buffer[128];
  //char end[6];

    while((n = read(fd, buffer, 128)) != 0)
    {
        write(1, buffer, n);
        write(1, "\n",1);
    }

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

int main(int argc, char *argv[])
{
    char buffer[128];
    char DSIP[128];
    char DSport[128];

    //FIXME hardcoded for testing
    strcpy(DSIP,"tejo.tecnico.ulisboa.pt");
    strcpy(DSport,"58011");

    while(true)
    {   
        system("clear");
        printf("Commands:\nREG UID PASS\nUNR UID PASS\nLOG UID PASS\nOUT UID PASS\nGLS\nGSR UID GID GName\nGUR UID GID\nGLM UID\nULS GID\nPST UID GID Tsize text [Fname Fsize data][X]\nRTV UID GID MID[X]\n\n>>");
        fgets(buffer, sizeof(buffer), stdin);
        printf("\n---->\n%s<----\n", buffer);
        //*Register user
        if(buffer[0] == 'u')
        {   
            udp_send(DSIP, DSport, buffer + 2, strlen(buffer + 2));
        }
        //*Unregister user
        else if(buffer[0] == 't')
        {
            tcp_send(DSIP, DSport, buffer + 2, strlen(buffer + 2));
        }

        fgets(buffer, sizeof(buffer), stdin);
    }

 
    exit(0);
}