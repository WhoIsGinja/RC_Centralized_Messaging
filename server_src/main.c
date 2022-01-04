#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include <errno.h>
#include "../protocol_constants.h"
#include "auxiliar/file_manager.h"


/*
*Check the argument format
*arg: argument name
*value: argument value
*size: string size desired (0 to not check)
*alphanum: true to check alphanumeric char, false to only check numeric
*/
bool check_arg(const char* arg, const char* value, int size, bool alphanum)
{
    int i;
    bool error = false;
  
    //*size check
    if(size != 0)
    {
        for(i = 0; value[i]!= '\0' ; i++)
        {
            switch (alphanum)
            {
                case true:
                    if(isalnum(value[i]) == 0)
                    {
                        error = true;
                    }
                    break;
                case false:
                    if(isdigit(value[i]) == 0)
                    {
                        error = true;
                    }
                    break;
            }

            if(error)
            {
                fprintf(stderr, "%s has to be %s\n", arg, alphanum? "alphanumeric":"numeric");
                break; 
            }
        }

        if(i != size)
        {
            error = true;
            fprintf(stderr, "%s must have a size of %d\n", arg, size); 
        }
    }
 
    return error;
}


/*
*  _    _ _____  _____  
* | |  | |  __ \|  __ \ 
* | |  | | |  | | |__) |
* | |  | | |  | |  ___/ 
* | |__| | |__| | |     
*  \____/|_____/|_| 
*/    
                       

int reg(char* buffer)
{
    char *uid, *pass;
    if((uid = strtok(NULL, " ")) == NULL || (pass = strtok(NULL, " ")) == NULL)
    {
        snprintf(buffer,  5, "ERR\n");
        return;   
    }
}


void udp_commands(char* buffer, int n)
{
    char* cmd;

    if((cmd = strtok(buffer, " ")) == NULL)
    {   
        snprintf(buffer,  5, "ERR\n");
        return;
    }

    if(n == 19 && strncmp(cmd, "REG", 3) == 0)
    {   
        write(1, buffer, n);
        
        
        //reg(buffer);
    }
    else if(n == 4 && strncmp(cmd, "GLS\n", 4) == 0)
    {
        fprintf(stderr,"GLS works\n");
    }
    else
    {
        snprintf(buffer,  5, "ERR\n");
    }

}


void udp_connections(const char* port)
{
    int fd, errcode;
    ssize_t n;
    struct addrinfo hints, *res;
    struct sockaddr_in addr;
    char buffer[BUFFER];
    socklen_t addrlen = sizeof(addr);

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if(fd == -1)
    {
        fprintf(stderr, "Error: %s\n", strerror(errno));
        return;
    }
    
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    if((errcode = getaddrinfo(NULL, port, &hints, &res)) != 0)
    {
        fprintf(stderr, "Error: %s\n", gai_strerror(errcode));
        close(fd);
        return;
    }

    if((n = bind(fd, res->ai_addr, res->ai_addrlen)) == -1)
    {
        fprintf(stderr, "Error: %s\n", strerror(errno));
        freeaddrinfo(res);
        close(fd);
        return;
    }

    while(true)
    {
        n = recvfrom(fd, buffer, sizeof(buffer), 0, (struct sockaddr*) &addr, &addrlen);
        if(n == -1)
        {
            fprintf(stderr, "Error receiving from client\n");
            freeaddrinfo(res);
            close(fd);
            return;
        }

        if(fork() == 0)
        {   
            udp_commands(buffer, n);

            //!FIXME message size
            n = sendto(fd, buffer, n, 0, (struct sockaddr*) &addr, addrlen);

            exit(0);
        }
    }
    //TODO
}


/*
*  _______ _____ _____  
* |__   __/ ____|  __ \ 
*    | | | |    | |__) |
*    | | | |    |  ___/ 
*    | | | |____| |     
*    |_|  \_____|_|                               
*/                     
void tcp_commands(const char* cmd){
    printf("Executes command...\n");
}


void tcp_connections(const char* port)
{   
    int fd, connfd, errcode;
    ssize_t n;
    struct addrinfo hints, *res;
    struct sockaddr_in addr;
    char buffer[BUFFER];
    socklen_t addrlen = sizeof(addr);
    
    fd = socket(AF_INET, SOCK_STREAM, 0);
    if(fd == -1)
    {
        fprintf(stderr, "Error: %s\n", strerror(errno));
        return;
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if((errcode = getaddrinfo(NULL, port, &hints, &res)) != 0)
    {
        fprintf(stderr, "Error: %s\n", gai_strerror(errcode));
        close(fd);
        return;
    }
    
    if((n = bind(fd, res->ai_addr, res->ai_addrlen)) == -1)
    {
        fprintf(stderr, "Error: %s\n", strerror(errno));
        freeaddrinfo(res);
        close(fd);
        return;
    }

    if(listen(fd, 100) == -1)
    {
        fprintf(stderr, "Error: %s\n", strerror(errno));
        freeaddrinfo(res);
        close(fd);
        return;
    }

    while(true)
    {
        if((connfd = accept(fd, (struct sockaddr*) &addr, &addrlen)) == -1)
        {
            fprintf(stderr, "Error: %s\n", strerror(errno));
            freeaddrinfo(res);
            close(fd);
            return;
        }

        if(fork() == 0)
        {
            if((n = read(connfd, buffer, sizeof(buffer))) == -1)
            {
                fprintf(stderr, "Error(receiving): %s\n", strerror(errno));
                close(connfd);
                exit(1);
            }

            tcp_commands(buffer/*, connfd*/);

            //!FIXME message size
            if((n = write(connfd, buffer, 16)) == -1)
            {
                fprintf(stderr, "Error(receiving): %s\n", strerror(errno));
                close(connfd);
                exit(1);
            }

            close(connfd);
            exit(0);
        }
        else
        {
            close(connfd);
        }
    }
}


int main(int argc, char *argv[])
{   
    pid_t child_pid;
    char port[6] = "58005\0";
    
    //TODO read args

    init_server_data();

    child_pid = fork();

    if(child_pid == 0)
    {
        udp_connections(port);
        kill(getppid(), SIGKILL);
        exit(1); 
    }
    else
    {
        tcp_connections(port);
        kill(child_pid, SIGKILL);
        exit(1);
    }

    exit(0);
}