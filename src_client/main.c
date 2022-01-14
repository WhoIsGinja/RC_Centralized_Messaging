#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <regex.h>
#include "auxiliar/connection_manager.h"
#include "../protocol_constants.h"

#define MESSAGE_SIZE 64
#define POST_SIZE 512
#define IP_SIZE 128
#define PORT_SIZE 16

struct user_info
{
    bool logged;
    char uid[6];
    char pass[9];
    char gid[3];
};

struct user_info user;
char DSIP[IP_SIZE];
char DSport[PORT_SIZE];

//* Register user
void reg()
{
    char *uid, *pass;
    int status;
    char message[MESSAGE_SIZE];

    uid = strtok(NULL, " ");
    pass = strtok(NULL, " ");

    sprintf(message, "REG %s %s\n", uid, pass);

    status = udp_send(DSIP, DSport, message, strlen(message));

    if (status == OK)
    {
        printf("[<]Successfully registered user with id of %s\n", uid);
    }
    else if (status == DUP)
    {
        printf("[<]User with id of %s already exists\n", uid);
    }
    else if (status == NOK)
    {
        printf("[<]Unable to create user with id of %s\n", uid);
    }
}

//* Unregister user
void unr()
{
    char *uid, *pass;
    int status;
    char message[MESSAGE_SIZE];

    uid = strtok(NULL, " ");
    pass = strtok(NULL, " ");

    sprintf(message, "UNR %s %s\n", uid, pass);

    status = udp_send(DSIP, DSport, message, strlen(message));

    if (status == OK)
    {
        printf("[<]Successfully unregistered user with id of %s\n", uid);
        //*If unregister user that is logged in
        if (strcmp(uid, user.uid) == 0)
        {
            user.logged = false;
            memset(user.uid, 0, 6);
            memset(user.pass, 0, 9);
            memset(user.gid, 0, 3);
        }
    }
    else if (status == NOK)
    {
        printf("[<]Unable to unregister user with id of %s\n", uid);
    }
}

//* Login user
void login()
{
    char *uid, *pass;
    int status;
    char message[MESSAGE_SIZE];

    //*Check if there is already a log in
    if (user.logged == true)
    {
        fprintf(stderr, "[!]An user is already logged in\n");
        return;
    }

    uid = strtok(NULL, " ");
    pass = strtok(NULL, " ");

    sprintf(message, "LOG %s %s\n", uid, pass);

    status = udp_send(DSIP, DSport, message, strlen(message));

    if (status == OK)
    {
        printf("[<]Successfully logged in with user %s\n", uid);
        sprintf(user.uid, "%s", uid);
        sprintf(user.pass, "%s", pass);
        user.logged = true;
    }
    else if (status == NOK)
    {
        printf("[<]Unable to log in with %s\n", uid);
    }
}

//* Logout user
void logout()
{
    int status;
    char message[MESSAGE_SIZE];

    //* Check if there is no login
    if (user.logged == false)
    {
        fprintf(stderr, "[!]No user logged in\n");
        return;
    }

    sprintf(message, "OUT %s %s\n", user.uid, user.pass);

    status = udp_send(DSIP, DSport, message, strlen(message));

    if (status == OK)
    {
        //*Localy clean user info
        printf("[<]Successfully logged out with %s\n", user.uid);
        user.logged = false;
        memset(user.uid, 0, 6);
        memset(user.pass, 0, 9);
        memset(user.gid, 0, 3);
    }
    else if (status == NOK)
    {
        printf("[<]Unable to log out with %s\n", user.uid);
    }
}

//* Show the current user id
void showuid()
{
    //* Check if there is no login
    if (user.logged == false)
    {
        fprintf(stderr, "[!]No user logged in\n");
        return;
    }

    printf("[<]Current user ID: %s\n", user.uid);
}

//* Show all groups
void groups()
{
    char message[MESSAGE_SIZE];

    sprintf(message, "GLS\n");

    udp_send(DSIP, DSport, message, strlen(message));
}

//* Enter/Create a group
void subscribe()
{
    char *gid, *gname;
    int status;
    char message[MESSAGE_SIZE];

    //* Check if there is no login
    if (user.logged == false)
    {
        fprintf(stderr, "[!]No user logged in\n");
        return;
    }

    gid = strtok(NULL, " ");
    gname = strtok(NULL, " ");

    sprintf(message, "GSR %s %s %s\n", user.uid, gid, gname);

    status = udp_send(DSIP, DSport, message, strlen(message));
    if(status == OK)
    {
        printf("[<]Successfully subscribed to group %s\n", gid);
    }
    else if(status == NOK)
    {
        printf("[<]Unable to subscribe to group %s\n", gid);
    }
    else if(status == E_USR)
    {
        printf("[<]Problems with user id %s\n", user.uid);
    }
    else if(status == E_GRP)
    {
        printf("[<]Group id of %s doesn't exist\n", gid);
    }
    else if(status == E_GNAME)
    {
        printf("[<]Group name \"%s\" doesn't match gid %s\n", gname, gid);
    }
    else if(status == E_FULL)
    {
        printf("[<]Group is full\n");
    }
}

//* Leave a group
void unsubscribe()
{
    char *gid;
    int status;
    char message[MESSAGE_SIZE];

    //* Check if no user if logged in
    if (user.logged == false)
    {
        fprintf(stderr, "[!]No user logged in\n");
        return;
    }

    gid = strtok(NULL, " ");

    sprintf(message, "GUR %s %s\n", user.uid, gid);

    status = udp_send(DSIP, DSport, message, strlen(message));

    if(status == OK)
    {
        printf("[<]Successfully unsubscribed to group %s\n", gid);
    }
    else if(status == NOK)
    {
        printf("[<]Unable to unsubscribe to group %s\n", gid);
    }
    else if(status == E_USR)
    {
        printf("[<]Problems with user id %s\n", user.uid);
    }
    else if(status == E_GRP)
    {
        printf("[<]Group id of %s doesn't exist\n", gid);
    }
}

//* Show groups the current user is subscribed
void my_groups()
{
    char message[MESSAGE_SIZE];

    //* Check if user no is logged in
    if (user.logged == false)
    {
        fprintf(stderr, "[!]No user logged in\n");
        return;
    }

    sprintf(message, "GLM %s\n", user.uid);

    udp_send(DSIP, DSport, message, strlen(message));
}

//* Select group
void sag()
{
    char *gid;

    //* Check if no user is logged in
    if (user.logged == false)
    {
        fprintf(stderr, "[!]No user logged in\n");
        return;
    }

    gid = strtok(NULL, " ");

    sprintf(user.gid, "%s", gid);

    printf("[<]Selected group %s\n", gid);
}

//* Show current selected group
void showgid()
{
    //* Check if no user is logged in
    if (user.logged == false)
    {
        fprintf(stderr, "[!]No user logged in\n");
        return;
    }

    //* Check if there is a selected group
    if (user.gid == 0)
    {
        fprintf(stderr, "[!]No group selected\n");
        return;
    }

    printf("[<]Selected group ID: %s\n", user.gid);
}

//* Show all users of current selected group
void ulist()
{
    char message[MESSAGE_SIZE];

    //* Check if no user is logged in
    if (user.logged == false)
    {
        fprintf(stderr, "[!]No user logged in\n");
        return;
    }

    //* Check if there is a selected group
    if (user.gid[0] == 0)
    {
        fprintf(stderr, "[!]No group selected\n");
        return;
    }

    sprintf(message, "ULS %s\n", user.gid);

    tcp_send(DSIP, DSport, message, strlen(message), NULL);
}

//* Sends a message to the current group
void post()
{
    char *text, *textend;
    char *filename = NULL;
    char post[POST_SIZE];

    //* Check if there is no login
    if (user.logged == false)
    {
        fprintf(stderr, "[!]No user logged in\n");
        return;
    }
    //* Check if there is a selected group
    if (user.gid[0] == 0)
    {
        fprintf(stderr, "[!]No group selected\n");
        return;
    }

    text = strtok(NULL, "");
    textend = rindex(++text, '\"');
    textend[0] = '\0';

    if (textend[1] == ' ')
    {
        filename = textend + 2;
    }

    sprintf(post, "PST %s %s %ld %s\n", user.uid, user.gid, strlen(text), text);
    if (filename == NULL)
    {
        tcp_send(DSIP, DSport, post, strlen(post), NULL);
    }
    else
    {
        tcp_send(DSIP, DSport, post, strlen(post) - 1, filename);
    }
}

//* Retrieves up to 20 messages from the current group
void retrieve()
{
    char message[MESSAGE_SIZE];
    char *mid;

    //* Check if there is no login
    if (user.logged == false)
    {
        fprintf(stderr, "[!]No user logged in\n");
        return;
    }
    //* Check if there is a selected group
    if (user.gid[0] == 0)
    {
        fprintf(stderr, "[!]No group selected\n");
        return;
    }

    mid = strtok(NULL, " ");

    sprintf(message, "RTV %s %s %s\n", user.uid, user.gid, mid);

    tcp_send(DSIP, DSport, message, strlen(message), NULL);
}

int main(int argc, char *argv[])
{
    int opt;
    char buffer[512];

    //* Initialize local user
    user.logged = false;
    memset(user.uid, 0, 6);
    memset(user.pass, 0, 9);
    memset(user.gid, 0, 3);

    //* Initialize base address
    if (gethostname(buffer, sizeof(buffer)) == -1)
    {
        fprintf(stderr, "[!]Getting host name\n");
    }
    strcpy(DSIP, buffer);
    strcpy(DSport, "58005");

    while ((opt = getopt(argc, argv, ":n:p:")) != -1)
    {
        switch (opt)
        {
        case 'n':
            strcpy(DSIP, optarg);
            break;
        case 'p':
            strcpy(DSport, optarg);
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

    printf("[=]Connection with server at %s:%s\n\n", DSIP, DSport);

    while (true)
    {
        //*Get command
        printf("[>]");
        fgets(buffer, sizeof(buffer), stdin);
        buffer[strlen(buffer) - 1] = '\0';

        //*Register user
        if (regex_test("^reg [[:digit:]]{5} [[:alnum:]]{8}$", buffer))
        {
            strtok(buffer, " ");
            reg();
        }
        //*Unregister user
        else if (regex_test("^(unr|unregister) [[:digit:]]{5} [[:alnum:]]{8}$", buffer))
        {
            strtok(buffer, " ");
            unr();
        }
        //*Login
        else if (regex_test("^login [[:digit:]]{5} [[:alnum:]]{8}$", buffer))
        {
            strtok(buffer, " ");
            login();
        }
        //*Logout
        else if (regex_test("^logout$", buffer))
        {
            logout();
        }
        //*Show user id
        else if (regex_test("^(su|showuid)$", buffer))
        {
            showuid();
        }
        //*Exit application
        else if (regex_test("^exit$", buffer))
        {
            exit(0);
        }
        //*Show all groups
        else if (regex_test("^(gl|groups)$", buffer))
        {
            groups();
        }
        //*Enter/Create a group
        else if (regex_test("^(s|subscribe) [[:digit:]]{2} [[:alnum:]_-]{1,24}$", buffer))
        {
            strtok(buffer, " ");
            subscribe();
        }
        //*Leave a group
        else if (regex_test("^(u|unsubscribe) [[:digit:]]{2}$", buffer))
        {
            strtok(buffer, " ");
            unsubscribe();
        }
        //*Show all the groups that the user is in
        else if (regex_test("^(mgl|my_groups)$", buffer))
        {
            my_groups();
        }
        //*Select a group
        else if (regex_test("^(sag|select) [[:digit:]]{2}$", buffer))
        {
            strtok(buffer, " ");
            sag();
        }
        //*Show current group id
        else if (regex_test("^(sg|showgid)$", buffer))
        {
            showgid();
        }
        //*Show all user of the selected group
        else if (regex_test("^(ul|ulist)$", buffer))
        {
            ulist();
        }
        //*Send a messge to group
        else if (regex_test("^post \".{1,240}\"( [[:alnum:]_.-]{1,20}\\.[[:alnum:]]{3})?$", buffer))
        {
            strtok(buffer, " ");
            post();
        }
        //*Retrieve messages from group
        else if (regex_test("^(r|retrieve) [[:digit:]]{4}$", buffer))
        {
            strtok(buffer, " ");
            retrieve();
        }
        else
        {
            fprintf(stderr, "[!]Command doesn't match, check arguments\n");
        }

        printf("\n");
    }

    exit(0);
}