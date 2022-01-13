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
#include <signal.h>
#include <dirent.h>
#include "../protocol_constants.h"
#include "auxiliar/data_manager.h"

#define MAXCHILD_P 100
#define BUFFER_64B 64
#define BUFFER_512B 512
#define BUFFER_2KB 2048

char port[16];

//* Sets socket timer on
int TimerON(int sd)
{
	struct timeval tmout;

	memset((char *)&tmout, 0, sizeof(tmout));
	tmout.tv_sec = 15;

	return (setsockopt(sd, SOL_SOCKET, SO_RCVTIMEO, (struct timeval *)&tmout, sizeof(struct timeval)));
}

//* Test str with rule
bool regex_test(const char *rule, const char *str)
{
	regex_t reg;

	if (str == NULL)
	{
		return false;
	}

	if (regcomp(&reg, rule, REG_EXTENDED) != 0)
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
//* UDP values
enum uid_pass_cmd
{
	REG,
	UNR,
	LOG,
	OUT
};

//* Commands that have the format of: CMD UID PASS
int cmd_uid_pass(int cmd)
{
	char *uid, *pass;

	uid = strtok(NULL, " ");
	pass = strtok(NULL, " ");

	switch (cmd)
	{
	case REG:
		printf("Register %s\n", uid);
		return user_create(uid, pass);
		break;
	case UNR:
		printf("Unregister %s\n", uid);
		return user_delete(uid, pass);
		break;
	case LOG:
		printf("Login %s\n", uid);
		return user_entry(uid, pass, true);
		break;
	case OUT:
		printf("Logout %s\n", uid);
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

	printf("Subscribe user %s to %s(%s)\n", uid, gname, gid);

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

	printf("Unsubscribe user %s to %s\n", uid, gid);

	//* Check if user is logged on
	if (user_logged(uid) != OK)
	{
		return E_USR;
	}

	return group_remove(uid, gid);
}

//* Returns
int groups(char **glist, const char *uid)
{

	if (uid != NULL)
	{
		printf("Get user %s groups\n", uid);

		//* Check if user is logged on
		if (user_logged(uid) != OK)
		{
			return E_USR;
		}
	}
	else
	{
		printf("Get all groups\n");
	}

	return groups_get(glist, uid);
}

//* Executes received udp command
char *udp_commands(char *buffer, int n)
{
	char *glist;
	char *tmp;
	int status;

	//* Register
	if (regex_test("^REG ", buffer))
	{
		if (!regex_test("^REG [[:digit:]]{5} [[:alnum:]]{8}$", buffer))
		{
			printf("Register bad request\n");
			sprintf(buffer, "RRG %s\n", strstatus(NOK));
		}
		else
		{
			strtok(buffer, " ");
			status = cmd_uid_pass(REG);
			sprintf(buffer, "RRG %s\n", strstatus(status));
		}
	}
	//* Unregister
	else if (regex_test("^UNR ", buffer))
	{
		if (!regex_test("^UNR [[:digit:]]{5} [[:alnum:]]{8}$", buffer))
		{
			printf("Unregister bad request\n");
			sprintf(buffer, "RRG %s\n", strstatus(NOK));
		}
		else
		{
			strtok(buffer, " ");
			status = cmd_uid_pass(UNR);
			sprintf(buffer, "RUN %s\n", strstatus(status));
		}
	}
	//* Login
	else if (regex_test("^LOG ", buffer))
	{
		if (!regex_test("^LOG [[:digit:]]{5} [[:alnum:]]{8}$", buffer))
		{
			printf("Login bad request\n");
			sprintf(buffer, "RLO %s\n", strstatus(NOK));
		}
		else
		{
			strtok(buffer, " ");
			status = cmd_uid_pass(LOG);
			sprintf(buffer, "RLO %s\n", strstatus(status));
		}
	}
	//* Logout
	else if (regex_test("^OUT ", buffer))
	{
		if (!regex_test("^OUT [[:digit:]]{5} [[:alnum:]]{8}$", buffer))
		{
			printf("Logout bad request\n");
			sprintf(buffer, "ROU %s\n", strstatus(NOK));
		}
		else
		{
			strtok(buffer, " ");
			status = cmd_uid_pass(OUT);
			sprintf(buffer, "ROU %s\n", strstatus(status));
		}
	}
	//* Subscribe
	else if (regex_test("^GSR ", buffer))
	{
		if (!regex_test("^GSR [[:digit:]]{5} [[:digit:]]{2} [[:alnum:]_-]{1,24}$", buffer))
		{
			printf("Subscribe bad request\n");
			sprintf(buffer, "RGS %s\n", strstatus(NOK));
		}
		else
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
	}
	//* Unsubscribe
	else if (regex_test("^GUR ", buffer))
	{
		if (!regex_test("^GUR [[:digit:]]{5} [[:digit:]]{2}$", buffer))
		{
			printf("Unsubscribe bad request\n");
			sprintf(buffer, "RGU %s\n", strstatus(NOK));
		}
		else
		{

			strtok(buffer, " ");
			status = gur();
			sprintf(buffer, "RGU %s\n", strstatus(status));
		}
	}
	//* All groups
	else if (regex_test("^GLS$", buffer))
	{
		status = groups(&glist, NULL);

		if (status == OK && glist != NULL)
		{
			tmp = (char *)malloc(sizeof(char *) * (strlen(glist) + 5));
			sprintf(tmp, "RGL %s\n", glist);
			free(glist);

			return tmp;
		}

		sprintf(buffer, "RGL 0\n");
	}
	//* User groups
	else if (regex_test("^GLM ", buffer))
	{
		if (!regex_test("^GLM [[:digit:]]{5}$", buffer))
		{
			printf("User groups bad request\n");
			sprintf(buffer, "RGM %s\n", strstatus(NOK));
		}
		else
		{
			strtok(buffer, " ");
			status = groups(&glist, strtok(NULL, " "));

			if (status == OK && glist != NULL)
			{
				tmp = (char *)malloc(sizeof(char *) * (strlen(glist) + 5));
				sprintf(tmp, "RGM %s\n", glist);
				free(glist);

				return tmp;
			}
			else if (status == E_USR)
			{
				sprintf(buffer, "RGM %s\n", strstatus(status));
			}
			else
			{
				sprintf(buffer, "RGM 0\n");
			}
		}
	}
	else
	{
		printf("Command not available\n");
		sprintf(buffer, "%s\n", strstatus(ERR));
	}

	return buffer;
}

//* Manage udp comunication with client
void udp_communication()
{
	int fd;
	int errcode;
	ssize_t n;
	struct addrinfo hints;
	struct addrinfo *res;
	struct sockaddr_in addr;
	char buffer[BUFFER_64B];
	char *response;
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

		if (fork() == 0)
		{
			printf("[?]From %s:%d->", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));

			if (buffer[n - 1] == '\n')
			{
				buffer[n - 1] = '\0';
				response = udp_commands(buffer, n);
			}
			else
			{
				sprintf(buffer, "%s\n", strstatus(ERR));
			}

			n = sendto(fd, response, strlen(response), 0, (struct sockaddr *)&addr, addrlen);

			if (response != buffer)
			{
				free(response);
			}

			exit(0);
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

//* TCP values
int connfd;

//* Reads n - 1 bytes or less, last byte always a null char(TCP)
int read_nbytes(char *start, ssize_t *nread, int nbytes)
{
	char *ptr;
	int n, nleft;

	nleft = nbytes - 1;
	ptr = start;

	while (nleft > 0)
	{
		if ((n = read(connfd, ptr, nleft)) == -1)
		{
			fprintf(stderr, "[!]Error receiving from client: %s\n", strerror(errno));
			return ERR;
		}

		if (ptr[n - 1] == '\n')
		{
			nleft -= n;
			*nread = nbytes - 1 - nleft;
			start[*nread + 1] = '\0';
			return OK;
		}

		nleft -= n;
		ptr += n;
	}

	*nread = nbytes - 1 - nleft;
	start[*nread + 1] = '\0';

	return NOK;
}

//* Gets user from the selected group
int uls(char **ulist)
{
	char *gid;

	gid = strtok(NULL, "");
	gid[strlen(gid) - 1] = '\0';

	printf("User list for %s\n", gid);

	return group_users(ulist, gid);
}

//* Create the text message
int post_msg(char *gid, char *mid, char **fileinfo)
{
	char *uid, *text;
	int tsize, status;

	*fileinfo = NULL;

	//* Get uid, gid, tsize and text
	uid = strtok(NULL, " ");
	strcpy(gid, strtok(NULL, " "));
	tsize = atoi(strtok(NULL, " "));
	text = strtok(NULL, "");

	printf("Post on %s by %s\n", gid, uid);

	//* Check if user is logged on
	if (user_logged(uid) != OK)
	{
		return NOK;
	}

	//* Test length and terminating char
	if (strlen(text) < tsize || (text[tsize] != '\n' && text[tsize] != ' '))
	{
		return NOK;
	}

	//* Message has file
	if (text[tsize] == ' ')
	{
		text[tsize] = '\0';
		*fileinfo = text + tsize + 1;
	}

	//* Create message information and text
	status = group_msg_add(uid, gid, text, mid);

	return status;
}

//* Create the file
int post_file(const char *start, const int nread, const char *gid, const char *mid, char *file)
{
	char *filename, *data, *aux;
	FILE *f;
	char buffer[BUFFER_2KB];
	long long fsize;
	ssize_t n;
	int status;

	//* Get filename and fsize
	filename = strtok(file, " ");
	aux = strtok(NULL, " ");
	fsize = strtoll(aux, NULL, 0);

	//* Create file
	status = group_msg_file(gid, mid, filename, buffer);
	if (status != OK)
	{
		group_msg_remove(gid, mid);
		return NOK;
	}

	//* Open file
	if ((f = fopen(buffer, "a")) == NULL)
	{
		fprintf(stderr, "[!]Writing %s(%s) file(%s): %s\n", gid, mid, filename, strerror(errno));
		group_msg_remove(gid, mid);
		return NOK;
	}

	//* Get pointer to start of already read data
	data = aux + strlen(aux) + 1;
	//* Get the amount of data read
	n = nread - (data - start);

	//* Write read data to file
	n = fwrite(data, sizeof(char), n, f);
	fsize -= n;

	//* Read rest of the data
	while (fsize > 0)
	{
		status = read_nbytes(buffer, &n, sizeof(buffer));

		if (status == ERR)
			break;

		//* Check if all file has been read
		if (status == OK && fsize - (n - 1) == 0)
			n--;

		//* Write read data
		n = fwrite(buffer, sizeof(char), n, f);
		fsize -= n;
	}
	fclose(f);

	if (status != OK || fsize < 0)
	{
		fprintf(stderr, "[!]Writing %s(%s) file(%s)\n", gid, mid, filename);
		group_msg_remove(gid, mid);
		return NOK;
	}

	return OK;
}

//* Retrieves up to 20 messages
int retrieve()
{
	char *uid, *gid, *mid;
	int status;
	FILE *f;
	DIR *d;
	struct dirent **mids;
	struct dirent *entry;
	int nmsg, n;
	char gpathname[BUFFER_64B];
	char current_mid[BUFFER_64B * 2];
	char tmp[BUFFER_64B];
	char buffer[BUFFER_512B];
	char message[BUFFER_2KB];

	uid = strtok(NULL, " ");
	gid = strtok(NULL, " ");
	mid = strtok(NULL, "");
	mid[strlen(mid) - 1] = '\0';

	printf("Retrieve from %s(%s) by %s\n", gid, mid, uid);

	//* Check if user is logged on
	if (user_logged(uid) != OK)
	{
		return NOK;
	}

	status = group_msgs_get(uid, gid, mid, gpathname, &mids, &nmsg);
	if (status == NOK)
	{
		return NOK;
	}
	else if (nmsg == 0)
	{
		return EOF;
	}

	sprintf(message, "RRT OK %d", nmsg);
	if ((n = write(connfd, message, strlen(message))) == -1)
	{
		fprintf(stderr, "[!]Sending to client: %s\n", strerror(errno));
		free(mids);
		return ERR;
	}

	for (int i = 0; i < nmsg; i++)
	{
		bool file = false;
		char filename[25];

		strcpy(message, " ");
		strncat(message, mids[i]->d_name, 4);

		sprintf(current_mid, "%s/", gpathname);
		strncat(current_mid, mids[i]->d_name, 4);
		if ((d = opendir(current_mid)) == NULL)
		{
			fprintf(stderr, "[!]Opening messga directory: %s\n", strerror(errno));
			free(mids);
			exit(1);
		}

		//* See if there is a file
		while ((entry = readdir(d)) != NULL)
		{
			if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0 &&
				strcmp(entry->d_name, "A U T H O R.txt") != 0 && strcmp(entry->d_name, "T E X T.txt") != 0)
			{
				file = true;
				strncpy(filename, entry->d_name, strlen(entry->d_name) + 1);
			}
		}

		//* Get author
		sprintf(buffer, "%s/A U T H O R.txt", current_mid);
		if ((f = fopen(buffer, "r")) == NULL)
		{
			fprintf(stderr, "[!]Opening author file: %s\n", strerror(errno));
			free(mids);
			return ERR;
		}
		if (fgets(buffer, sizeof(buffer), f) == NULL)
		{
			fprintf(stderr, "[!]Reading message author\n");
			fclose(f);
			free(mids);
			return ERR;
		}
		fclose(f);

		//* Write author
		strcat(message, " ");
		strcat(message, buffer);

		//* Get text
		sprintf(buffer, "%s/T E X T.txt", current_mid);
		if ((f = fopen(buffer, "r")) == NULL)
		{
			fprintf(stderr, "[!]Opening  text file: %s\n", strerror(errno));
			free(mids);
			return ERR;
		}
		if (fgets(buffer, sizeof(buffer), f) == NULL)
		{
			fprintf(stderr, "[!]Reading text\n");
			fclose(f);
			free(mids);
			return ERR;
		}
		fclose(f);

		//* Write text size
		sprintf(tmp, " %ld", strlen(buffer));
		strcat(message, tmp);
		//* Write text
		strcat(message, " ");
		strcat(message, buffer);

		//* If there is no file send
		if (!file)
		{
			//* Send
			if ((n = write(connfd, message, strlen(message)) == -1))
			{
				fprintf(stderr, "[!]Sending to client\n");
				free(mids);
				return ERR;
			}
			continue;
		}

		//* Write filename
		strcat(message, " / ");
		strcat(message, filename);

		//* Get file
		sprintf(buffer, "%s/%s", current_mid, filename);
		if ((f = fopen(buffer, "r")) == NULL)
		{
			fprintf(stderr, "[!]: %s\n", strerror(errno));
			free(mids);
			return ERR;
		}

		//*Get file size
		fseek(f, 0L, SEEK_END);
		sprintf(buffer, " %ld", ftell(f));
		rewind(f);
		strcat(message, buffer);
		strcat(message, " ");

		//* Send
		if ((n = write(connfd, buffer, strlen(buffer)) == -1))
		{
			fprintf(stderr, "[!]Sending to client\n");
			free(mids);
			return ERR;
		}

		//* Send file data
		while (!feof(f))
		{
			n = fread(buffer, sizeof(char), sizeof(buffer), f);
			if ((n = write(connfd, buffer, n) == -1))
			{
				fprintf(stderr, "[!]Sending to client\n");
				free(mids);
				return ERR;
			}
		}
		fclose(f);
	}

	if ((n = write(connfd, "\n", 1) == -1))
	{
		fprintf(stderr, "[!]Sending to server\n");
		free(mids);
		return ERR;
	}

	return OK;
}

//* Executes received tcp command
void tcp_commands(char *buffer, int nread)
{
	int status, n;

	//* Ulist
	if (regex_test("^ULS ", buffer))
	{
		char *ulist;

		if (!regex_test("^ULS [[:digit:]]{2}\\\n$", buffer))
		{
			printf("Users list bad request\n");
			sprintf(buffer, "RUL %s\n", strstatus(NOK));
			return;
		}

		strtok(buffer, " ");
		status = uls(&ulist);
		if (status == NOK)
		{
			sprintf(buffer, "RUL %s\n", strstatus(OK));
			return;
		}
		else
		{
			sprintf(buffer, "RUL %s ", strstatus(OK));
			if ((n = write(connfd, buffer, strlen(buffer))) == -1)
			{
				fprintf(stderr, "[!]Sending to client: %s\n", strerror(errno));
				close(connfd);
				free(ulist);
				exit(1);
			}

			strcat(ulist, "\n");
			if ((n = write(connfd, ulist, strlen(ulist))) == -1)
			{
				fprintf(stderr, "[!]Sending to client: %s\n", strerror(errno));
				close(connfd);
				free(ulist);
				exit(1);
			}
		}
	}
	//* Post
	else if (regex_test("^PST ", buffer))
	{
		char mid[5];
		char gid[3];
		char *file;

		//* Test message format
		if (!regex_test("^PST [[:digit:]]{5} [[:digit:]]{2} ([1-9]|[1-9][0-9]|1[0-9][0-9]|2[0-3][0-9]|240) .{1,240}", buffer))
		{
			printf("Post bad request\n");
			sprintf(buffer, "RPT %s\n", strstatus(NOK));
			return;
		}

		strtok(buffer, " ");
		status = post_msg(gid, mid, &file);

		//* Problems creating message or there isn't a file
		if (file == NULL && status == OK)
		{
			sprintf(buffer, "RPT %s\n", mid);
			return;
		}
		else if (status == NOK)
		{
			sprintf(buffer, "RPT %s\n", strstatus(status));
			return;
		}

		//* Test file format
		if (!regex_test("^[[:alnum:]_.-]{1,20}\\.[[:alnum:]]{3} [[:digit:]]{1,10} ", file))
		{
			group_msg_remove(gid, mid);
			sprintf(buffer, "RPT %s\n", strstatus(NOK));
			return;
		}

		//* Create file
		status = post_file(buffer, nread, gid, mid, file);

		if (status == OK)
		{
			sprintf(buffer, "RPT %s\n", mid);
		}
		else
		{
			sprintf(buffer, "RPT %s\n", strstatus(status));
		}

		return;
	}
	//* Retrieve
	else if (regex_test("^RTV ", buffer))
	{
		if (!regex_test("^RTV [[:digit:]]{5} [[:digit:]]{2} [[:digit:]]{4}\\\n$", buffer))
		{
			printf("Retrieve bad request\n");
			sprintf(buffer, "RRT %s\n", strstatus(NOK));
			return;
		}

		strtok(buffer, " ");
		status = retrieve();

		if (status == NOK)
		{
			sprintf(buffer, "RRT NOK\n");
			return;
		}
		else if (status == EOF)
		{
			sprintf(buffer, "RRT EOF\n");
			return;
		}
	}
	else
	{
		printf("Command not available\n");
		sprintf(buffer, "%s\n", strstatus(ERR));
		return;
	}

	close(connfd);
	exit(0);
}

//* Manage tcp comunication with client
void tcp_communication()
{
	int fd;
	int errcode;
	ssize_t n;
	struct addrinfo hints;
	struct addrinfo *res;
	struct sockaddr_in addr;
	char buffer[BUFFER_2KB];
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

		if (fork() == 0)
		{
			int status;

			TimerON(connfd);

			printf("[?]From %s:%d->", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));

			status = read_nbytes(buffer, &n, BUFFER_2KB);

			if (status != ERR)
			{
				tcp_commands(buffer, n);
			}
			else
			{
				printf("Bad request\n");
				sprintf(buffer, "%s\n", strstatus(ERR));
			}

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
	}
}

int main(int argc, char *argv[])
{
	pid_t child_pid;
	int opt;
	struct sigaction act;
	char buffer[512];

	//* Default
	strcpy(port, "58005");
	bool verbose = false;

	while ((opt = getopt(argc, argv, ":vp:")) != -1)
	{
		switch (opt)
		{
		case 'v':
			verbose = true;
			break;
		case 'p':
			strcpy(port, optarg);
			break;
		case ':':
			fprintf(stderr, "[!]Argument needs a value\n");
			exit(1);
			break;
		case '?':
			fprintf(stderr, "[!]Invalid option\n");
			exit(1);
			break;
		}
	}

	if (!verbose)
	{
		if (freopen("/dev/null", "a", stdout) == NULL)
		{
			fprintf(stderr, "[!]Enabling non-verbose: %s\n", strerror(errno));
			exit(1);
		}
	}

	fprintf(stderr, "[=]Running %s on %s\n", verbose?"verbose":"non-verbose", port);

	//* Ignore lost of connection and zombie child signals
	memset(&act, 0, sizeof act);
	act.sa_handler = SIG_IGN;
	if (sigaction(SIGPIPE, &act, NULL) == -1)
	{
		fprintf(stderr, "[!]Ignoring SIGPIPE\n");
		exit(1);
	}
	if (sigaction(SIGCHLD, &act, NULL) == -1)
	{
		fprintf(stderr, "[!]Ignoring SIGCHILD\n");
		exit(1);
	}

	//* Create server data
	init_server_data();

	setvbuf(stdout, buffer, _IOLBF, sizeof(buffer));

	child_pid = fork();
	if (child_pid == 0)
	{
		udp_communication(port);
		kill(getppid(), SIGTERM);
		exit(1);
	}
	else
	{
		tcp_communication(port);
		kill(child_pid, SIGTERM);
		exit(1);
	}

	exit(0);
}