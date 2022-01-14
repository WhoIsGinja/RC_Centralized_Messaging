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

#define BUFFER_2KB 2048
#define UL_SIZE BUFFER_2KB * 2

int fd;

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

//*UDP transmissions
int send_message_udp(const char *ds_ip, const char *ds_port, const char *message, int size)
{
	ssize_t n;
	struct addrinfo hints, *res;

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd == -1)
	{
		fprintf(stderr, "[!]Creating socket\n");
		return NOK;
	}

	TimerON(fd);

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;

	if (getaddrinfo(ds_ip, ds_port, &hints, &res) != 0)
	{
		fprintf(stderr, "[!]Getting server\n");
		return NOK;
	}

	n = sendto(fd, message, size, 0, res->ai_addr, res->ai_addrlen);
	if (n == -1)
	{
		fprintf(stderr, "[!]Sending to server\n");
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
	char *gid, *gname, *mid;
	int i;
	char buffer[BUFFER_2KB * 2];

	addrlen = sizeof(addr);

	n = recvfrom(fd, buffer, sizeof(buffer), 0, (struct sockaddr *)&addr, &addrlen);
	if (n == -1)
	{
		fprintf(stderr, "[!]Receiving from server\n");
		return NOK;
	}

	//* Replace '\n'
	buffer[n - 1] = '\0';

	if (regex_test("^(RGL|RGM) (0|\\b([1-9]|[1-9][0-9])\\b( [[:digit:]]{2} [[:alnum:]_-]{1,24} [[:digit:]]{4})+)$", buffer))
	{
		strtok(buffer, " ");
		i = atoi(strtok(NULL, " "));

		if (i == 0)
		{
			printf("[<]No groups found\n");
		}
		else
		{
			printf("[<]%d groups found:\n", i);
		}

		for (; i > 0; i--)
		{
			gid = strtok(NULL, " ");
			gname = strtok(NULL, " ");
			mid = strtok(NULL, " ");

			printf("%s: %s (%s)\n", gid, gname, mid);
		}
	}
	else if (regex_test("^(RRG (OK|DUP|NOK)|(RUN|RLO|ROU) (OK|NOK)|RGS (OK|NOK|E_USR|E_GRP|E_GNAME|E_FULL|NEW [[:digit:]]{2})|RGU (OK|NOK|E_USR|E_GRP))$", buffer))
	{
		int status;
		strtok(buffer, " ");
		status = istatus(strtok(NULL, " "));

		if (status == NEW)
		{
			printf("[<]New group successfully created with mid of %s\n", strtok(NULL, " "));
		}

		return status;
	}
	else if (regex_test("^ERR$", buffer))
	{
		printf("[!]Server returned error\n");
	}
	else
	{
		printf("[!]Unknown Response\n");
		return ERR;
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
	long long fsize;
	FILE *f;
	char buffer[BUFFER_2KB];

	fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd == -1)
	{
		fprintf(stderr, "[!]Creating socket\n");
		return NOK;
	}

	TimerON(fd);

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	errcode = getaddrinfo(ds_ip, ds_port, &hints, &res);
	if (errcode != 0)
	{
		fprintf(stderr, "[!]Getting the server\n");
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
		fprintf(stderr, "[!]Sending to server\n");
		return NOK;
	}

	//* Send file
	if (filename != NULL)
	{
		if ((f = fopen(filename, "r")) == NULL)
		{
			fprintf(stderr, "[!]Opening file to send: %s\n", strerror(errno));
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
			fprintf(stderr, "[!]Sending to server\n");
			return NOK;
		}

		//* Send file data
		while (!feof(f))
		{
			n = fread(buffer, sizeof(char), sizeof(buffer), f);
			if ((n = write(fd, buffer, n) == -1))
			{
				fprintf(stderr, "[!]Sending to server\n");
				return NOK;
			}
		}

		fclose(f);

		if ((n = write(fd, "\n", 1) == -1))
		{
			fprintf(stderr, "[!]Sending to server\n");
			return NOK;
		}
	}

	return OK;
}

//* Read n - 1 bytes or less, last byte always a null char(TCP)
int read_nbytes(char *start, ssize_t *nread, int nbytes)
{
	char *ptr;
	int n, nleft;

	nleft = nbytes - 1;
	ptr = start;

	while (nleft > 0)
	{
		if ((n = read(fd, ptr, nleft)) == -1)
		{
			fprintf(stderr, "[!]Receiving from server: %s\n", strerror(errno));
			return ERR;
		}

		if (ptr[n - 1] == '\n' || n == 0)
		{
			ptr[n] = '\0';
			nleft -= n;
			*nread = nbytes - 1 - nleft;
			return OK;
		}

		nleft -= n;
		ptr += n;
	}

	*nread = nbytes - 1 - nleft;
	start[*nread + 1] = '\0';

	return NOK;
}

int receive_message_tcp()
{
	ssize_t nread;
	int status;
	char *token;
	char buffer[BUFFER_2KB];

	if ((status = read_nbytes(buffer, &nread, BUFFER_2KB)) == ERR)
	{
		return NOK;
	}

	//* Ulist
	if (regex_test("^RUL (OK|NOK\\\n$) [^ ]", buffer))
	{
		int ulistLeft;
		int ulistSize;
		char *ulist;

		strtok(buffer, " ");
		token = strtok(NULL, " ");
		if (strncmp(token, "NOK", 3) == 0)
		{
			printf("[-]Group doesn't exist\n");
			return NOK;
		}

		//* Create user list
		ulist = (char *)calloc(UL_SIZE, sizeof(char));
		ulistLeft = UL_SIZE;
		ulistSize = UL_SIZE;

		token = strtok(NULL, "");

		//* Copy read users
		strcpy(ulist, token);

		ulistLeft -= strlen(ulist) + 1;

		//* If not all users have been read
		while (status != OK)
		{
			if ((status = read_nbytes(buffer, &nread, BUFFER_2KB)) == ERR)
			{
				free(ulist);
				return NOK;
			}

			//* If there is more to add than space
			while (ulistLeft < nread)
			{
				ulistSize *= 2;
				ulist = (char*)realloc(ulist, ulistSize);
				ulistLeft = ulistSize - strlen(ulist) + 1;
			}

			//* Copy new users
			strcat(ulist, buffer);
			ulistLeft -= nread;
		}

		//* Check format
		if (!regex_test("^[[:alnum:]_-]{1,24}( [[:digit:]]{5})*", ulist))
		{
			fprintf(stderr, "[<]Server doesn't follow protocol\n");
			free(ulist);
			return NOK;
		}

		ulist[strlen(ulist)-1] = '\0';
		//* Print output
		token = strtok(ulist, " ");
		printf("[<]Users of group %s:\n", token);
		while ((token = strtok(NULL, " ")) != NULL)
		{
			printf("-%s\n", token);
		}

		free(ulist);
	}

	//* Post
	else if (regex_test("^RPT ([[:digit:]]{4}|NOK)\\\n$", buffer))
	{
		buffer[strlen(buffer) - 1] = '\0';
		strtok(buffer, " ");
		token = strtok(NULL, "");
		if (istatus(token) == NOK)
		{
			printf("[<]Unable to post\n");
			return NOK;
		}

		printf("[<]Post sent successfully: %s\n", token);
	}

	//* Retrieve
	else if (regex_test("^RRT (NOK\\\n$|EOF\\\n$|OK \\b([1-9]|1[0-9]|20)\\b )", buffer))
	{

		FILE *f;
		//char *data = NULL;
		char *filename;
		char aux[BUFFER_2KB], data[BUFFER_2KB];
		long long fsize;
		//ssize_t n;
		int status;
		int messages, state;
		//char mid[4], uid[5];
		//char *txtsize, *filesize;
		int i, tsize;
		int j, tsizemax;
		int notmax = 0;
		//int ti, fi;

		strtok(buffer, " ");
		token = strtok(NULL, " ");
		if (strncmp("NOK", token, 3) == 0)
		{
			printf("[<]Retieve failed\n");
			return NOK;
		}
		else if (strncmp("EOF", token, 3) == 0)
		{
			printf("[<]No messages available\n");
			return EOF;
		}

		token = strtok(NULL, " ");
		messages = atoi(token);

		i = 8 + strlen(token);
		state = 0;
		j = 0;
		memset(aux, 0, sizeof(aux));
		while (messages > 0)
		{
			if (state == 0 && buffer[i] == ' ')
			{
				printf("[%s]\n", aux);
				memset(aux, 0, j);
				j = 0;

				state = 1;
			}
			else if (state == 1 && buffer[i] == ' ')
			{
				memset(aux, 0, j);
				j = 0;
				state = 2;
			}
			else if (state == 2 && buffer[i] == ' ')
			{
				tsize = atoi(aux) + 1;
				tsizemax = tsize;
				memset(aux, 0, j);
				j = 0;
				state = 3;
			}
			else if (state == 3 && tsize == 0)
			{
				//aux[j] = '\0';
				printf("[Text]%s\n", aux);
				memset(aux, 0, j);
				j = 0;
				state = 4;
				fsize = 0;
			}
			else if ((state == 4) && (buffer[i] >= '0') && (buffer[i] <= '9'))
			{
				j = 0;
				state = 0;
				messages--;
				printf("\n");
			}
			else if (state == 4 && buffer[i] == '/')
			{
				i++;
				j = 0;
				state = 5;
			}
			else if (state == 5 && buffer[i] == ' ')
			{
				printf("[File]%s\n", aux);
				filename = strdup(aux);
				memset(aux, 0, j);
				j = 0;
				state = 6;
			}

			else if (state == 6 && buffer[i] == ' ')
			{
				fsize = strtoll(aux, NULL, 0);
				memset(aux, 0, j);
				j = 0;
				if ((f = fopen(filename, "w")) == NULL)
				{
					fprintf(stderr, "[!]Error receiving file %s\n", filename);
				}
				state = 7;
				i++;
			}

			else if (state == 7 && fsize == 0 && buffer[i] == ' ')
			{
				fwrite(data, sizeof(char), j, f);
				bzero(data, BUFFER_2KB);
				fclose(f);

				j = 0;
				state = 0;
				messages--;
				fsize = -1;
				printf("\n");
			}

			else if (state == 7 && fsize == 0 && buffer[i] == '\n')

			{

				fwrite(data, sizeof(char), j, f);
				bzero(data, BUFFER_2KB);
				fclose(f);
				state = 0;
			}

			switch (state)
			{
			case 0:
				if (buffer[i] == ' ')
				{
					memset(aux, 0, j);
					j = 0;
					break;
				}
				aux[j] = buffer[i];
				j++;
				break;
			case 1:
				if (buffer[i] == ' ')
				{
					memset(aux, 0, j);
					j = 0;
					break;
				}
				aux[j] = buffer[i];
				j++;
				break;
			case 2:
				if (buffer[i] == ' ')
				{
					memset(aux, 0, j);
					break;
				}
				aux[j] = buffer[i];
				j++;

				break;
			case 3:
				if (tsize == 0)
				{
					aux[j] = '\0';
					break;
				}
				if (tsize == tsizemax)
				{
					tsize--;
					break;
				}
				aux[j] = buffer[i];
				j++;
				tsize--;
				break;
			/*case 4:
					if(buffer[i] == ' '){
						memset(aux, 0, j);
						break;
					}
					break;*/
			case 5:
				if (buffer[i] == ' ')
				{
					memset(aux, 0, j);
					j = 0;
					break;
				}
				aux[j] = buffer[i];
				j++;
				break;
			case 6:
				if (buffer[i] == ' ')
				{
					memset(aux, 0, j);
					j = 0;
					break;
				}
				aux[j] = buffer[i];
				j++;
				break;

			case 7:
				*(data + j) = buffer[i];

				j++;

				if (j == BUFFER_2KB)
				{
					fwrite(data, sizeof(char), j, f);
					bzero(data, BUFFER_2KB);
					j = 0;
					fsize--;
					break;
				}
				fsize--;
				break;
			default:
				break;
			}
			i++;
			//* ENDS THE LOOP
			if (i == nread && notmax == 1 && fsize == 0)
			{
				if (messages == 1)
				{
					break;
					;
				}
			}
			//*When buffer is all read
			if (i == nread)
			{
				//bzero(data, BUFFER_2KB);
				memset(buffer, 0, sizeof(buffer));
				if ((status = read_nbytes(buffer, &nread, sizeof(buffer))) == ERR)
				{
					fprintf(stderr, "[!]Error receiving from server\n");
					return NOK;
				}

				if (nread < BUFFER_2KB)
				{
					notmax = 1;
				}
				else if (nread == BUFFER_2KB)
				{
					notmax = 0;
				}

				i = 0;
			}
		}
	}

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
