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
#include <regex.h>
#include "../protocol_constants.h"
#include "auxiliar/file_manager.h"

regex_t reg_uid;
regex_t reg_pass;
regex_t reg_gid;
regex_t reg_gname;
regex_t reg_mid;
regex_t reg_text;
regex_t reg_fname;

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
        printf("%s %s\n", uid, pass);
        snprintf(buffer,  5, "ERR\n");
        return NOK;   
    }

    //*Check uid and pass format
    if(regexec(&reg_uid, uid, 0, NULL, 0) != 0)
    {   
        return NOK;
    }
    if(regexec(&reg_pass, pass, 0, NULL, 0) != 0)
    {   
        return NOK;
    }

    return user_create(uid, pass);
}


void udp_commands(char* buffer, int n)
{
    char* cmd;
    int status;

    buffer[n-1] = '\0';

    if((cmd = strtok(buffer, " ")) == NULL)
    {   
        snprintf(buffer,  5, "ERR\n");
        return;
    }

    if(n == 19 && strncmp(cmd, "REG\n", 3) == 0)
    {   
        status = reg(buffer);

        sprintf(buffer,"RRG %d\n", status);
    }
    else if(n == 4 && strncmp(cmd, "GLS\n", 4) == 0)
    {
        fprintf(stderr,"GLS works\n");
    }
    else
    {
        snprintf(buffer, 5, "ERR\n");
    }

    return;
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

            n = sendto(fd, buffer, strlen(buffer), 0, (struct sockaddr*) &addr, addrlen);

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


void init(){
    //*Initialize regular expressions
    if(regcomp(&reg_uid, "^[0-9]{5}$", REG_EXTENDED) != 0)
    {
        fprintf(stderr, "Regular expression for uid compilation failed!");
        exit(1);
    }
    if(regcomp(&reg_pass, "^[0-9a-zA-Z]{8}$", REG_EXTENDED) != 0)
    {
        fprintf(stderr, "Regular expression for pass compilation failed!");
        exit(1);
    }
    if(regcomp(&reg_gid, "^[0-9]{2}$", REG_EXTENDED) != 0)
    {
        fprintf(stderr, "Regular expression for gid compilation failed!");
        exit(1);
    }
    if(regcomp(&reg_gname, "^[0-9a-zA-Z_-]{1,24}$", REG_EXTENDED) != 0)
    {
        fprintf(stderr, "Regular expression for gname compilation failed!");
        exit(1);
    }
    if(regcomp(&reg_mid, "^[0-9]{4}$", REG_EXTENDED) != 0)
    {
        fprintf(stderr, "Regular expression for mid compilation failed!");
        exit(1);
    }
    if(regcomp(&reg_text, "^[.]{1,240}$", REG_EXTENDED) != 0)
    {
        fprintf(stderr, "Regular expression for text compilation failed!");
        exit(1);
    }
    if(regcomp(&reg_fname, "^[0-9a-zA-Z_.-]{1,20}\\.[a-z]{3}$", REG_EXTENDED) != 0)
    {
        fprintf(stderr, "Regular expression for fname compilation failed!");
        exit(1);
    }
}


int main(int argc, char *argv[])
{   
    pid_t child_pid;
    char port[6] = "58005\0";

    init();
    
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

    regfree(&reg_uid);
    regfree(&reg_pass);
    regfree(&reg_gid);
    regfree(&reg_gname);
    regfree(&reg_mid);
    regfree(&reg_text);
    regfree(&reg_fname);

    exit(0);
}