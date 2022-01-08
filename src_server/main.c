#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include <errno.h>
#include <regex.h>
#include "../protocol_constants.h"
#include "auxiliar/data_manager.h"

#define MAXCHILD_P 100
#define BUFFER_UDP 64
#define BUFFER_TCP 1024

enum uid_pass_cmd
{
	REG,
	UNR,
	LOG,
	OUT
};

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

//* Commands that have the format of: CMD UID PASS
int cmd_uid_pass(char *buffer, int cmd)
{
	char *uid, *pass, *end;

	//* Check number of arguments
	if ((uid = strtok(NULL, " ")) == NULL || (pass = strtok(NULL, " ")) == NULL || (end = strtok(NULL, " ")) != NULL)
	{
		return NOK;
	}

	//*Check uid and pass format
	if (regexec(&reg_uid, uid, 0, NULL, 0) != 0)
	{
		return NOK;
	}
	else if (regexec(&reg_pass, pass, 0, NULL, 0) != 0)
	{
		return NOK;
	}

	switch (cmd)
	{
	case REG:
		return user_create(uid, pass);
		break;
	case UNR:
		return user_delete(uid, pass);
		break;
	case LOG:
		return user_entry(uid, pass, true);
		break;
	case OUT:
		return user_entry(uid, pass, false);
		break;
	}

	return ERR;
}


int GSR(char *buffer)
{	
	char *uid, *gid, *gname, *end;

	//* Check number of arguments
	if ((uid = strtok(NULL, " ")) == NULL || (gid = strtok(NULL, " ")) == NULL || (gname = strtok(NULL, " ")) == NULL || (end = strtok(NULL, " ")) != NULL)
	{
		return NOK;
	}

	//*Check uid, gid and gname format
	if (regexec(&reg_uid, uid, 0, NULL, 0) != 0)
	{
		return NOK;
	}
	else if (regexec(&reg_gid, gid, 0, NULL, 0) != 0)
	{
		return NOK;
	}
	else if (regexec(&reg_gname, gname, 0, NULL, 0) != 0)
	{
		return NOK;
	}

	//* Check if user is logged on
	if(user_logged(uid) != OK)
	{
		return E_USR;
	}

	if (strcmp(gid, "00") == 0)
	{
		return group_create(uid, gname);
	}
	else
	{
		return group_add(uid, gid, gname);
	}

	return ERR;
}


char *GLS()
{
	return "RGL OK";
}


char *udp_commands(char *buffer, int n)
{
	char *cmd;
	//char *groups;
	int status;

	if ((cmd = strtok(buffer, " ")) == NULL)
	{
		snprintf(buffer, 5, "ERR\n");
		return buffer;
	}

	//* Register
	if (strcmp(cmd, "REG") == 0)
	{
		status = cmd_uid_pass(buffer, REG);
		sprintf(buffer, "RRG %s\n", strstatus(status));
	}
	//* Unregister
	else if (strcmp(cmd, "UNR") == 0)
	{
		status = cmd_uid_pass(buffer, UNR);
		sprintf(buffer, "RUN %s\n", strstatus(status));
	}
	//* Login
	else if (strcmp(cmd, "LOG") == 0)
	{
		status = cmd_uid_pass(buffer, LOG);
		sprintf(buffer, "RLO %s\n", strstatus(status));
	}
	//* Logout
	else if (strcmp(cmd, "OUT") == 0)
	{
		status = cmd_uid_pass(buffer, OUT);
		sprintf(buffer, "ROU %s\n", strstatus(status));
	}
	else if (n == 4 && strcmp(cmd, "GLS") == 0)
	{
		/* groups = GLS();
		buffer[0] = '\0';
		return groups; */
	}
	//* Subscribe
	else if (strcmp(cmd, "GSR") == 0)
	{
		status = GSR(buffer);

		if(status > NEW)
		{
			sprintf(buffer, "RGS NEW %02d\n", status - NEW);
		}
		else
		{
			sprintf(buffer, "RGS %s\n", strstatus(status));
		}
	}
	else
	{
		snprintf(buffer, 5, "ERR\n");
	}

	return buffer;
}


void udp_connections(const char *port)
{	
	int fd;
	int errcode;
	ssize_t n;
	struct addrinfo hints;
	struct addrinfo* res;
	struct sockaddr_in addr;
	char buffer[BUFFER_UDP];
	char* response;
	int childp = 0;
	socklen_t addrlen = sizeof(addr);

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd == -1)
	{
		fprintf(stderr, "[!]Creating socket: %s\n", strerror(errno));
		return;
	}

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE;

	if ((errcode = getaddrinfo(NULL, port, &hints, &res)) != 0)
	{
		fprintf(stderr, "[!]Getting address: %s\n", gai_strerror(errcode));
		close(fd);
		return;
	}

	if ((n = bind(fd, res->ai_addr, res->ai_addrlen)) == -1)
	{
		fprintf(stderr, "[!]Binding: %s\n", strerror(errno));
		freeaddrinfo(res);
		close(fd);
		return;
	}

	while (true)
	{	
		n = recvfrom(fd, buffer, sizeof(buffer), 0, (struct sockaddr *)&addr, &addrlen);
		if (n == -1)
		{
			fprintf(stderr, "[!]Receiving from client\n");
			freeaddrinfo(res);
			close(fd);
			return;
		}

		buffer[n - 1] = '\0';

		printf("[?]From %s - %d: %s\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port), buffer);

		childp++;

		if (fork() == 0)
		{
			response = udp_commands(buffer, n);

			n = sendto(fd, response, strlen(response), 0, (struct sockaddr *)&addr, addrlen);

			if (buffer[0] == '\0')
			{
				free(response);
			}

			exit(0);
		}

		//* Clears childs from the process table
		if(childp == MAXCHILD_P)
		{
			for(; childp > 0 ; wait(NULL), childp--);
		}
	}
}


/*
*  _______ _____ _____  
* |__   __/ ____|  __ \ 
*    | | | |    | |__) |
*    | | | |    |  ___/ 
*    | | | |____| |     
*    |_|  \_____|_|                               
*/
void tcp_commands(const char *cmd)
{
	printf("Executes command...\n");
}


void tcp_connections(const char *port)
{
	int fd;
	int connfd;
	int errcode;
	ssize_t n;
	struct addrinfo hints;
	struct addrinfo *res;
	struct sockaddr_in addr;
	char buffer[BUFFER_TCP];
	int childp = 0;
	socklen_t addrlen = sizeof(addr);

	fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd == -1)
	{
		fprintf(stderr, "[!]Creating socket: %s\n", strerror(errno));
		return;
	}

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	if ((errcode = getaddrinfo(NULL, port, &hints, &res)) != 0)
	{
		fprintf(stderr, "[!]Getting address: %s\n", gai_strerror(errcode));
		close(fd);
		return;
	}

	if ((n = bind(fd, res->ai_addr, res->ai_addrlen)) == -1)
	{
		fprintf(stderr, "[!]Binding: %s\n", strerror(errno));
		freeaddrinfo(res);
		close(fd);
		return;
	}

	if (listen(fd, 100) == -1)
	{
		fprintf(stderr, "[!]Creating queque: %s\n", strerror(errno));
		freeaddrinfo(res);
		close(fd);
		return;
	}

	while (true)
	{
		if ((connfd = accept(fd, (struct sockaddr *)&addr, &addrlen)) == -1)
		{
			fprintf(stderr, "[!]Connecting to client: %s\n", strerror(errno));
			freeaddrinfo(res);
			close(fd);
			return;
		}

		childp++;

		if (fork() == 0)
		{
			if ((n = read(connfd, buffer, sizeof(buffer))) == -1)
			{
				fprintf(stderr, "[!]Receiving from client: %s\n", strerror(errno));
				close(connfd);
				exit(1);
			}

			tcp_commands(buffer /*, connfd*/);

			sprintf(buffer, "TCP response");
			//FIXME
			if ((n = write(connfd, buffer, strlen(buffer))) == -1)
			{
				fprintf(stderr, "[!]Sending to client: %s\n", strerror(errno));
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

		//* Clears childs from the process table
		if(childp == MAXCHILD_P)
		{
			for(; childp > 0; wait(NULL), childp--);
		}
	}
}


void init()
{
	//*Initialize regular expressions
	if (regcomp(&reg_uid, "^[0-9]{5}$", REG_EXTENDED) != 0)
	{
		fprintf(stderr, "[!]Regular expression for uid compilation failed!");
		exit(1);
	}
	if (regcomp(&reg_pass, "^[0-9a-zA-Z]{8}$", REG_EXTENDED) != 0)
	{
		fprintf(stderr, "[!]Regular expression for pass compilation failed!");
		exit(1);
	}
	if (regcomp(&reg_gid, "^[0-9]{2}$", REG_EXTENDED) != 0)
	{
		fprintf(stderr, "[!]Regular expression for gid compilation failed!");
		exit(1);
	}
	if (regcomp(&reg_gname, "^[0-9a-zA-Z_-]{1,24}$", REG_EXTENDED) != 0)
	{
		fprintf(stderr, "[!]Regular expression for gname compilation failed!");
		exit(1);
	}
	if (regcomp(&reg_mid, "^[0-9]{4}$", REG_EXTENDED) != 0)
	{
		fprintf(stderr, "[!]Regular expression for mid compilation failed!");
		exit(1);
	}
	if (regcomp(&reg_text, "^[.]{1,240}$", REG_EXTENDED) != 0)
	{
		fprintf(stderr, "[!]Regular expression for text compilation failed!");
		exit(1);
	}
	if (regcomp(&reg_fname, "^[0-9a-zA-Z_.-]{1,20}\\.[a-z]{3}$", REG_EXTENDED) != 0)
	{
		fprintf(stderr, "[!]Regular expression for fname compilation failed!");
		exit(1);
	}
}


int main(int argc, char *argv[])
{
	pid_t child_pid;

	//FIXME
	char port[6] = "58005\0";
	bool verbose = true;

	if(verbose == false)
	{
		if(freopen("/dev/null", "a", stdout) == NULL)
		{
			fprintf(stderr, "[!]Error suppressing stdout: %s", strerror(errno));
			exit(1);
		}
	}

	init();

	//TODO read args

	init_server_data();

	child_pid = fork();

	if (child_pid == 0)
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