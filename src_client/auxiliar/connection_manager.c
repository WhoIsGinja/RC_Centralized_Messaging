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

#define BUFFER_TCP 2048
#define UL_SIZE BUFFER_TCP*2


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

	if (strcmp(rcmd, "RGL") == 0 || strcmp(rcmd, "RGM") == 0)
	{
		i = atoi(strtok(NULL, " "));

		printf("[<]\n");
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
	long long fsize;
	FILE *f;
	char buffer[BUFFER_TCP];

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
			n = fread(buffer, sizeof(char), sizeof(buffer), f);
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


//* Read n - 1 bytes or less, last byte always a null char(TCP)
int read_nbytes(char *start, ssize_t *nread, int nbytes)
{
<<<<<<< Updated upstream
	char *ptr;
	int n, nleft;

	nleft = nbytes - 1;
	ptr = start;

	while (nleft > 0)
	{
		if ((n = read(fd, ptr, nleft)) == -1)
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
=======
    char *ptr;
    int n, nleft;

    nleft = nbytes - 1;
    ptr = start;

    while (nleft > 0)
    {
        if ((n = read(fd, ptr, nleft)) == -1)
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
>>>>>>> Stashed changes
}


int receive_message_tcp()
{
	ssize_t nread;
	int status;
	char *token;
	char buffer[BUFFER_TCP];

	if((status = read_nbytes(buffer, &nread, BUFFER_TCP)) == ERR)
	{
		return NOK;
	}

	//* Ulist
	if (regex_test("^RUL (OK|NOK\\\n$)( [^ ])?", buffer))
	{	
		int ulistLeft;
		int ulistSize;
		char *ulist;

		
		strtok(buffer, " ");
		token = strtok(NULL, " ");

		if (token[strlen(token)-1] == '\n')
		{	
			printf("[<]Group doesn't exist\n");
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
			if((status = read_nbytes(buffer, &nread, BUFFER_TCP)) == ERR)
			{
				free(ulist);
				return NOK;
			}

			//* If there is more to add than space
			while (ulistLeft < nread)
			{				
				ulistSize *= 2;
				ulist = (char*) realloc(ulist, ulistSize);
				
				ulistLeft = ulistSize - strlen(ulist) + 1;
			}

			//* Copy new users
			strcat(ulist, buffer);	
			ulistLeft -= nread;
		}

		//* Check format
		if (!regex_test("^[[:alnum:]_-]{1,24}( [[:digit:]]{5})*", ulist))
		{
			fprintf(stderr,"[<]Server doesn't follow protocol\n");
			free(ulist);
			return NOK;
		}

		//* Print output
		ulist[strlen(ulist) - 1] = '\0';
		
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
		strtok(buffer, " ");
		token = strtok(NULL, " ");

		if(strncmp("NOK", token, 3) == 0)
		{
			printf("[<]Unable to post\n");
			return NOK;
		}

		printf("[<]Post sent successfully: %s", token);
		
		
	}

	//* Retrieve
	else if (regex_test("^RRT (NOK\\\n$|EOF\\\n$|OK [^ ])", buffer))
	{
		/* char *filename, *data;
		FILE *f;
		char *aux;
		long long fsize;
		int status;
		int messages, state;
		char mid[5], uid[6];
		char text[241];
		char txtsize[4], filesize[11];
		int i, tsize;
		int j = 0;

		aux = strdup(buffer);
		strtok(aux, " ");
		token = strtok(NULL, " ");
		if(strcmp("NOK", token) == 0)
		{
			printf("[-]Unable to post\n");
			return NOK;
		}
		else if(strcmp("EOF", token) == 0)
		{
			printf("[-]No messages available");
			return EOF;
		}
		
		token = strtok(NULL, " ");

		filename = (char *)malloc(sizeof(char) * 25);
		data = (char *)malloc(sizeof(char) * BUFFER_TCP);

		messages = atoi(token);

		printf("%s\n", token);
		i = 8 + strlen(token);
		state = 0;
		

		/*ver tambem se N > 0*/
		while(buffer[i] != '\n')
		{
			//printf("%d state: %d\n", j, state);
			if(state == 0 && buffer[i] == ' ')
			{
				state = 1;
				mid[j] = '\0';
				j = 0;
				printf("%s\n", mid);

			}
			else if(state == 1 && buffer[i] == ' ')
			{
				state = 2;
				uid[j] = '\0';
				j = 0;
				printf("%s\n", uid);

			}
			else if(state == 2 && buffer[i] == ' ')
			{
				tsize = atoi(txtsize);
				state = 3;
				txtsize[j] = '\0';
				j = 0;

				printf("%s\n", txtsize);

			}
			else if(state == 3 && tsize == 0 && buffer[i] == ' ')
			{
				state = 4;
				text[j] = '\0';
				j = 0;

				printf("%s\n", text);

			}
			else if(state == 4 && buffer[i] >= '0' && buffer[i] <= '9')
			{
				printf("-%s uid: %s text: %s\n", mid, uid, text);
				state = 0;
			}
			else if(state == 4 && buffer[i] == '/')
			{
				state = 5;
			}
			else if(state == 5 && buffer[i] == ' ')
			{
				state = 6;
			}
			else if(state == 6 && buffer[i] == ' ')
			{
				state = 7;
				*(filename + j) = '\0';
				j = 0;

				printf("file: %s\n", filename);
			}

			else if(state == 7 && buffer[i] == ' ')
			{
				fsize = strtoll(filesize, NULL, 0);
				filesize[j] = '\0';
				state = 8;
				j = 0;
				
				if((f = fopen(filename, "w+")) == NULL)
				{
					fprintf(stderr, "[!]Error receiving file %s\n", filename);
				}

				printf("-%s uid: %s text: %s file: %s file size: %s\n", mid, uid, text, filename, filesize);
			}
			else if(state == 8 && tsize == 0 && buffer[i] == ' ')
			{
				state = 0;
			}

			switch(state)
			{
				case 0:
					if(buffer[i] == ' ')
					{
						break;
					}
					mid[j] = buffer[i];
					j++;	

					break;
				case 1:
					if(buffer[i] == ' ' && j== 0)
					{
						bzero(uid, 5);
						break;
					}
					uid[j] = buffer[i];
					j++;

					break;
				case 2:
					if(buffer[i] == ' ' && j == 0)
					{
						bzero(txtsize,3);
						break;
					}
					txtsize[j] = buffer[i];
					j++;

					break;
				case 3:
					if(buffer[i] == ' ' && j== 0)
					{
						bzero(text, 240);
						break;
					}
					text[j] = buffer[i];
					j++;
					tsize--;	

					break;
				case 6:
					if(buffer[i] == ' ')
					{
						bzero(text, 24);
						break;
					}

					*(filename + j) = buffer[i];
					j++;

					break;
				case 7:
					if(buffer[i] == ' ')
					{
						bzero(filesize, 11);
						break;
					}

					filesize[j] = buffer[i];
					j++;

					break;
				case 8:
					if(buffer[i] == ' ' && j== 0)
					{
						bzero(data, BUFFER_TCP);
						break;
					}


					/*mandar para data e depois fazer fputs?*/
					*(data + j) = buffer[i];
					j++;
					fsize--;

					break;
				default:
					break;
			}

			i++;
<<<<<<< Updated upstream
			//When buffer is all read
			if (i == n)
=======
			//*When buffer is all read
			if (i == nread)
>>>>>>> Stashed changes
			{
				memset(buffer, 0, sizeof(buffer));
				if((status = read_nbytes(buffer, &nread, BUFFER_TCP)) == ERR)
				{
					fprintf(stderr, "[!]Error receiving from server\n");
					return NOK;
				}

				i = 0;
			}
		}

		}
		else
		{
			fprintf(stderr, "[!]Response doesn't follow protocol");
		} */

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
		}*/


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
