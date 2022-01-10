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

//* Test str with rule
bool regex_test(const char *rule, const char *str)
{
	regex_t reg;

	if (regcomp(&reg, rule, REG_EXTENDED | REG_NOSUB) != 0)
	{
		fprintf(stderr, "[!]Regular expression compilation failed.");
		exit(1);
	}

	if (regexec(&reg, str, 0, NULL, 0) == 0)
	{
		return true;
	}
	else
	{
		return false;
	}
}

/*
*  _    _ _____  _____  
* | |  | |  __ \|  __ \ 
* | |  | | |  | | |__) |
* | |  | | |  | |  ___/ 
* | |__| | |__| | |     
*  \____/|_____/|_| 
*/

//* Commands that have the format of: CMD UID PASS
int cmd_uid_pass(int cmd)
{
	char *uid, *pass;

	uid = strtok(NULL, " ");
	pass = strtok(NULL, " ");

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

//* Creates a group or subscribes a user to a group
int gsr()
{
	char *uid, *gid, *gname;

	uid = strtok(NULL, " ");
	gid = strtok(NULL, " ");
	gname = strtok(NULL, " ");

	//* Check if user is logged on
	if (user_logged(uid) != OK)
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

//* Removes a user from a group
int gur()
{
	char *uid, *gid;

	uid = strtok(NULL, " ");
	gid = strtok(NULL, " ");

	//* Check if user is logged on
	if (user_logged(uid) != OK)
	{
		return E_USR;
	}

	return group_remove(uid, gid);
}

//* Returns 
int ggroups(char **groups, const char *uid)
{
	if (uid != NULL)
	{
		//* Check if user is logged on
		if (user_logged(uid) != OK)
		{
			return E_USR;
		}
	}

	if (groups_get(groups, uid) == NOK)
	{
		return ERR;
	}

	return OK;
}

//* Returns the corresponding response message to the client
//* If response smaller than BUFFER_UDP, buffer is used
char *udp_commands(char *buffer, int n)
{
	char *groups;
	int status;

	//* Register
	if (regex_test("^REG [[:digit:]]{5} [[:alnum:]]{8}$", buffer))
	{
		strtok(buffer, " ");
		status = cmd_uid_pass(REG);
		sprintf(buffer, "RRG %s\n", strstatus(status));
	}
	//* Unregister
	else if (regex_test("^UNR [[:digit:]]{5} [[:alnum:]]{8}$", buffer))
	{
		strtok(buffer, " ");
		status = cmd_uid_pass(UNR);
		sprintf(buffer, "RUN %s\n", strstatus(status));
	}
	//* Login
	else if (regex_test("^LOG [[:digit:]]{5} [[:alnum:]]{8}$", buffer))
	{
		strtok(buffer, " ");
		status = cmd_uid_pass(LOG);
		sprintf(buffer, "RLO %s\n", strstatus(status));
	}
	//* Logout
	else if (regex_test("^OUT [[:digit:]]{5} [[:alnum:]]{8}$", buffer))
	{
		strtok(buffer, " ");
		status = cmd_uid_pass(OUT);
		sprintf(buffer, "ROU %s\n", strstatus(status));
	}
	//* Subscribe
	else if (regex_test("^GSR [[:digit:]]{5} [[:digit:]]{2} [[:alnum:]_-]{1,24}$", buffer))
	{
		strtok(buffer, " ");
		status = gsr();

		if (status > NEW)
		{
			sprintf(buffer, "RGS NEW %02d\n", status - NEW);
		}
		else
		{
			sprintf(buffer, "RGS %s\n", strstatus(status));
		}
	}
	//* Unsubscribe
	else if (regex_test("^GUR [[:digit:]]{5} [[:digit:]]{2}$", buffer))
	{
		strtok(buffer, " ");
		status = gur();

		sprintf(buffer, "RGU %s\n", strstatus(status));
	}
	//* All groups
	else if (regex_test("^GLS$", buffer))
	{
		status = ggroups(&groups, NULL);
		if (status == OK)
		{
			return groups;
		}
		else if (status == E_USR)
		{
			sprintf(buffer, "RGL %s\n", strstatus(status));
		}
		else
		{
			sprintf(buffer, "%s\n", strstatus(status));
		}
	}
	//* User groups
	else if (regex_test("^GLM [[:digit:]]{5}$", buffer))
	{
		strtok(buffer, " ");
		status = ggroups(&groups, strtok(NULL, " "));
		if (status == OK)
		{
			return groups;
		}
		else if (status == E_USR)
		{
			sprintf(buffer, "RGM %s\n", strstatus(status));
		}
		else
		{
			sprintf(buffer, "%s\n", strstatus(status));
		}
	}
	else
	{
		sprintf(buffer, "%s\n", strstatus(ERR));
	}

	return buffer;
}

//* Manage udp comunication with client
void udp_communication(const char *port)
{
	int fd;
	int errcode;
	ssize_t n;
	struct addrinfo hints;
	struct addrinfo *res;
	struct sockaddr_in addr;
	char buffer[BUFFER_UDP];
	char *response;
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

			if (response != buffer)
			{
				free(response);
			}

			exit(0);
		}

		//* Clears childs from the process table
		if (childp == MAXCHILD_P)
		{
			for (; childp > 0; wait(NULL), childp--)
				;
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

void tcp_communication(const char *port)
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
		if (childp == MAXCHILD_P)
		{
			for (; childp > 0; wait(NULL), childp--)
				;
		}
	}
}

int main(int argc, char *argv[])
{
	pid_t child_pid;

	//FIXME
	char port[6] = "58005\0";
	bool verbose = true;

	if (verbose == false)
	{
		if (freopen("/dev/null", "a", stdout) == NULL)
		{
			fprintf(stderr, "[!]Error suppressing stdout: %s", strerror(errno));
			exit(1);
		}
	}

	//TODO read args

	init_server_data();

	child_pid = fork();

	if (child_pid == 0)
	{
		udp_communication(port);
		kill(getppid(), SIGKILL);
		exit(1);
	}
	else
	{
		tcp_communication(port);
		kill(child_pid, SIGKILL);
		exit(1);
	}

	exit(0);
}