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
#include <regex.h>
#include <dirent.h>
#include "../protocol_constants.h"

int fd;

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

//*UDP transmissions

int send_message_udp(const char *ds_ip, const char *ds_port, const char *message, int size)
{
	ssize_t n;
	struct addrinfo hints, *res;

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd == -1)
	{
		fprintf(stderr, "Error creating socket\n");
		return NOK;
	}

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;

	if (getaddrinfo(ds_ip, ds_port, &hints, &res) != 0)
	{
		fprintf(stderr, "Error getting server\n");
		return NOK;
	}

	n = sendto(fd, message, size, 0, res->ai_addr, res->ai_addrlen);
	if (n == -1)
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
	char buffer[4096];
	addrlen = sizeof(addr);
	ssize_t n;

	n = recvfrom(fd, buffer, sizeof(buffer), 0, (struct sockaddr *)&addr, &addrlen);
	if (n == -1)
	{
		fprintf(stderr, "Error receiving from server\n");
		return NOK;
	}

	write(1, buffer, n);

	return OK;
}

int udp_send(const char *ds_ip, const char *ds_port, const char *message, int size)
{
	int status;

	if (send_message_udp(ds_ip, ds_port, message, size) == NOK)
	{
		close(fd);
		return NOK;
	}

	status = receive_message_udp();
	close(fd);

	return status;
}

//*TCP transmissions

int send_message_tcp(const char *ds_ip, const char *ds_port, const char *message, int size)
{
	struct addrinfo hints, *res;
	char buffer[2];
	int n, errcode;

	fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd == -1)
	{
		fprintf(stderr, "Error creating socket\n");
		return NOK;
	}

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	errcode = getaddrinfo(ds_ip, ds_port, &hints, &res);
	if (errcode != 0)
	{
		fprintf(stderr, "Error getting the server\n");
		freeaddrinfo(res);
		return NOK;
	}

	n = connect(fd, res->ai_addr, res->ai_addrlen);
	if (n == -1)
	{
		freeaddrinfo(res);
		return NOK;
	}

	for (int i = 0; i < size; i++)
	{
		sprintf(buffer, "%c", message[i]);
		if ((n = write(fd, buffer, 1) == -1))
		{
			fprintf(stderr, "Error sending to server\n");
			return NOK;
		}
	}

	return OK;
}

int receive_message_tcp()
{
	int n; // offset, jump = 6;
	char buffer[128];
	//char end[6];

	while ((n = read(fd, buffer, 128)) != 0)
	{
		write(1, buffer, n);
		write(1, "\n", 1);
	}

	return OK;
}

//TODO replicate similar UDP functions for TCP operations
int tcp_send(const char *ds_ip, const char *ds_port, const char *message, int size)
{
	int status;

	if (send_message_tcp(ds_ip, ds_port, message, size) == NOK)
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
	char buffer[512];
	char DSIP[128];
	char DSport[128];
	int n;
	int nleft;
	char *ptr;
	char *token;
	struct dirent **groups;

	setvbuf(stdout, NULL, _IONBF, 0);

	if (argc > 1 && argv[1][0] == 'm')
	{
		strcpy(DSIP, "DESKTOP-HPQ1DJ7");
		strcpy(DSport, "58005");
	}
	else
	{
		strcpy(DSIP, "tejo.tecnico.ulisboa.pt");
		strcpy(DSport, "58011");
	}

	while (true)
	{
		system("clear");
		printf("Commands:\n[?]U REG UID PASS\n[?]U UNR UID PASS\n[?]U LOG UID PASS\n[?]U OUT UID PASS\n[?]U GLS\n[?]U GSR UID GID GName\n[?]U GUR UID GID\n[?]U GLM UID\n[?]T ULS GID\n[?]T PST UID GID Tsize text [Fname Fsize data]\n[?]T RTV UID GID MID\n\n[?]");
		fgets(buffer, sizeof(buffer), stdin);
		buffer[strlen(buffer) -1] ='\0';
		//*Register user
		if (buffer[0] == 'U')
		{
			printf("\n[+]%s\n[-]", buffer+2);

			udp_send(DSIP, DSport, buffer + 2, strlen(buffer + 2));
		}
		//*Unregister user
		else if (buffer[0] == 'T')
		{
			printf("\n[+]%s\n[-]", buffer+2);

			tcp_send(DSIP, DSport, buffer + 2, strlen(buffer + 2));
		}

		printf("\n...");

		/* if (regex_test("^RUL (OK|NOK) ", buffer))
		{
			char *ulist;

			strtok(buffer, " ");
			token = strtok(NULL, " ");

			ulist = token + strlen(token) + 1;

			printf("%s", ulist);

			if (!regex_test("^[[:alnum:]_-]{1,24}( [[:digit:]]{5})*\\\n", ulist))
			{
				printf("[-]Server doesn't follow protocol\n");
				return NOK;
			}

			token = strtok(ulist, " ");
			printf("[-]%s users:\n", token);

			while ((token = strtok(NULL, " ")) != NULL)
			{
				printf("-%s\n", token);
			}
		} */

		//buffer[strlen(buffer) -1] = '\0';
		printf("test: %s\n", regex_test("^(RGL|RGM) (0|\\b([1-9]|[1-9][0-9])\\b( [[:digit:]]{2} [[:alnum:]_-]{1,24} [[:digit:]]{4})+)$", buffer)? "true" : "false");

		/* if ((n = scandir(".", &groups, NULL, alphasort)) == -1)
		{
			fprintf(stderr, "[!]Getting groups\n");
		}
		while(n--)
		printf("%s\n", groups[n]->d_name); */

		fgets(buffer, sizeof(buffer), stdin);
	}

	exit(0);
}