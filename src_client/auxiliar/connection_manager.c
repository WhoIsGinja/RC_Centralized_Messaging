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
#include <errno.h>
#include <stdbool.h>
#include <regex.h>
#include "connection_manager.h"
#include "../../protocol_constants.h"

int fd;

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
	ssize_t n;
	char *rcmd, *gid, *gname, *mid;
	int status, i;
	char buffer[4096];

	addrlen = sizeof(addr);

	n = recvfrom(fd, buffer, sizeof(buffer), 0, (struct sockaddr *)&addr, &addrlen);
	if (n == -1)
	{
		fprintf(stderr, "Error receiving from server\n");
		return NOK;
	}

	buffer[n - 1] = '\0';

	rcmd = strtok(buffer, " ");

	//TODO verify everything ????
	if (strcmp(rcmd, "RGL") == 0 || strcmp(rcmd, "RGM") == 0)
	{
		i = atoi(strtok(NULL, " "));

		for (; i > 0; i--)
		{
			gid = strtok(NULL, " ");
			gname = strtok(NULL, " ");
			mid = strtok(NULL, " ");

			printf("%s: %s (%s)\n", gid, gname, mid);
		}
	}
	else
	{
		status = istatus(strtok(NULL, " "));
		if (status == NEW)
		{
			return atoi(strtok(NULL, " "));
		}

		return status;
	}

	return OK;
}


int udp_send(const char *ds_ip, const char *ds_port, char *message, int size)
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
int send_message_tcp(const char *ds_ip, const char *ds_port, char *message, int size, char *filename)
{
	struct addrinfo hints, *res;
	int n, errcode;
	unsigned long long fsize;
	FILE *f;
	char buffer[1024];

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

	if ((n = write(fd, message, size) == -1))
	{
		fprintf(stderr, "Error sending to server\n");
		return NOK;
	}

	//* Send file
	if (filename != NULL)
	{
		if ((f = fopen(filename, "r")) == NULL)
		{
			fprintf(stderr, "[!]: %s\n", strerror(errno));
			return NOK;
		}

		//*Get file size
		fseek(f, 0L, SEEK_END);
		fsize = ftell(f);
		rewind(f);

		//* Check file size
		if ((fsize / 10000000000) >= 1)
		{
			fprintf(stderr, "[!]File size too big\n");
			return NOK;
		}

		//* Send file name and size
		sprintf(buffer, " %s %lld ", filename, fsize);
		if ((n = write(fd, buffer, strlen(buffer)) == -1))
		{
			fprintf(stderr, "Error sending to server\n");
			return NOK;
		}

		//* Send file data
		while (!feof(f))
		{
			n = fread(buffer, 1, sizeof(buffer), f);
			if ((n = write(fd, buffer, n) == -1))
			{
				fprintf(stderr, "Error sending to server\n");
				return NOK;
			}
		}

		fclose(f);

		if ((n = write(fd, "\n", 1) == -1))
		{
			fprintf(stderr, "Error sending to server\n");
			return NOK;
		}
	}

	return OK;
}


//* Read n - 1 bytes or less(TCP)
int read_nbytes(char *buffer, int *nread, int nbytes)
{
	char *ptr;
	int n;
	int nleft;

	nleft = nbytes - 1;
	ptr = buffer;

	while (nleft > 0)
	{
		//.FIXME error
		if ((n = read(fd, ptr, nleft)) == -1)
		{
			fprintf(stderr, "[!]Error receiving from server: %s\n", strerror(errno));
			return ERR;
		}

		write(1,ptr,n );
		write(1,"\n",1);

		if (n == 0)
		{
			printf("aaaa\n");
			*nread = nbytes - 1 - nleft;

			if(buffer[*nread - 1] == '\n')
			{
				buffer[*nread - 1] = '\0';
			}
			else
			{
				return ERR;
			}
			
			return OK;
		}

		nleft -= n;
		ptr += n;

	}

	*nread = nbytes - 1 - nleft;
	buffer[*nread - 1] = '\0';

	return NOK;
}


int receive_message_tcp()
{
	int n, i, ulistLeft, ulistSize;
	char c;
	char *token;

	char buffer[4096];
	char* cmd;
	int nread;
	int status;

	status = read_nbytes(buffer, &nread, 4096);
	printf("STATUS: %s", strstatus(status));

	//* Ulist
	if (regex_test("^RUL (OK|NOK) ", buffer))
	{
		char *ulist;

		strtok(buffer, " ");
		token = strtok(NULL, " ");
		if (istatus(token) == NOK)
		{
			printf("[-]Group does not exist\n");
			return NOK;
		}

		//* Read fodido
		ulist = (char *)calloc(16, sizeof(char));
		ulistLeft = 16;
		ulistSize = 16;
	
		while (true)
		{
			printf("%d-%s\n", strlen(buffer),buffer);
			while (ulistLeft < strlen(buffer))
			{
				int ulistLen = strlen(ulist);

				printf("LEN: %d\n", ulistLen);
				printf("SIZE: %d\n", ulistSize);
				
				ulistSize *= 2;
				ulist = (char*)realloc(ulist, ulistSize);
				
				ulistLeft = ulistSize - ulistLen;
				printf("LEFT: %d\n", ulistLeft);

				memset(ulist + ulistLen, 0, ulistLeft);
			}

			strncat(ulist, buffer, strlen(buffer));
			
			if (status == OK)
			{
				break;
			}

			status = read_nbytes(buffer, &nread, 4096);
		}

		printf("u: %s\n", ulist);

		if (!regex_test("^ [[:alnum:]_-]{1,24}( [[:digit:]]{5})*", ulist))
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
	}

	//* Post
	else if (regex_test("^RPT (OK|NOK)\\\n", buffer))
	{
	}

	//* Retrieve
	else if (regex_test("^RRT (OK|NOK) ", buffer))
	{
	}
	else
	{
		fprintf(stderr, "[!]Response doesn't follow protocol");
	}

	/*if (strncmp(buffer, "RRT", 3) ==)
	{
		while ((c = buffer[i]) != '\n')
		{
			switch (state)
			{
			case 0:

				break;

			default:
				break;
			}

			i++;
			if (i == n)
			{
				memset(buffer, 0, sizeof(buffer));
				if ((n = read(fd, buffer, sizeof(buffer))) == -1)
				{
					fprintf(stderr, "Error receiving from server\n");
					return NOK;
				}

				i = 0;
			}
		}
	}*/

	//TODO retrieve

	return OK;
}


int tcp_send(const char *ds_ip, const char *ds_port, char *message, int size, char *filename)
{
	int status;

	if (send_message_tcp(ds_ip, ds_port, message, size, filename) == NOK)
	{
		close(fd);
		return NOK;
	}

	status = receive_message_tcp();
	close(fd);

	return status;
}
