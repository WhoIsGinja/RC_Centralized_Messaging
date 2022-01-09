#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <regex.h>
#include "auxiliar/connection_manager.h"
#include "../protocol_constants.h"

regex_t reg_uid;
regex_t reg_pass;
regex_t reg_gid;
regex_t reg_gname;
regex_t reg_mid;
regex_t reg_text;
regex_t reg_file;

struct user_info
{
    bool logged;
    char uid[6];
    char pass[9];
    char gid[3];
};

struct user_info user;
char DSIP[128];
char DSport[6];

void arguments_error()
{
    fprintf(stderr, "[!]Wrong number of arguments\n");
}

//* Register user
void reg()
{
    char *uid, *pass, *end;
    char message[20];

    //*Get uid and password
    if ((uid = strtok(NULL, " ")) == NULL || (pass = strtok(NULL, " ")) == NULL || (end = strtok(NULL, " ")) != NULL)
    {
        arguments_error();
        return;
    }

    //*Check uid and pass format
    if (regexec(&reg_uid, uid, 0, NULL, 0) != 0)
    {
        fprintf(stderr, "[!]UID has to be 5 numeric characters\n");
        return;
    }
    if (regexec(&reg_pass, pass, 0, NULL, 0) != 0)
    {
        fprintf(stderr, "[!]Password has to be 8 alphanumeric characters\n");
        return;
    }

    snprintf(message, 20, "REG %s %s\n", uid, pass);

    udp_send(DSIP, DSport, message, strlen(message));
}

//* Unregister user
void unr()
{
    char *uid, *pass, *end;
    char message[20];

    //*Get uid and password
    if ((uid = strtok(NULL, " ")) == NULL || (pass = strtok(NULL, " ")) == NULL || (end = strtok(NULL, " ")) != NULL)
    {
        arguments_error();
        return;
    }

    //*Check uid and pass format
    if (regexec(&reg_uid, uid, 0, NULL, 0) != 0)
    {
        fprintf(stderr, "[!]UID has to be 5 numeric characters\n");
        return;
    }
    if (regexec(&reg_pass, pass, 0, NULL, 0) != 0)
    {
        fprintf(stderr, "[!]Password has to be 8 alphanumeric characters\n");
        return;
    }

    snprintf(message, 20, "UNR %s %s\n", uid, pass);

    udp_send(DSIP, DSport, message, strlen(message));
}

//* Login user
void login()
{
    char *uid, *pass, *end;
    char message[20];

    //*Check if there is already a log in
    if (user.logged == true)
    {
        fprintf(stderr, "[!]An user is already logged in\n");
        return;
    }

    //*Get uid and password
    if ((uid = strtok(NULL, " ")) == NULL || (pass = strtok(NULL, " ")) == NULL || (end = strtok(NULL, " ")) != NULL)
    {
        arguments_error();
        return;
    }

    //*Check uid and pass format
    if (regexec(&reg_uid, uid, 0, NULL, 0) != 0)
    {
        fprintf(stderr, "[!]UID has to be 5 numeric characters\n");
        return;
    }
    if (regexec(&reg_pass, pass, 0, NULL, 0) != 0)
    {
        fprintf(stderr, "[!]Password has to be 8 alphanumeric characters\n");
        return;
    }

    snprintf(message, 20, "LOG %s %s\n", uid, pass);

    if (udp_send(DSIP, DSport, message, strlen(message)) == OK)
    {
        sprintf(user.uid, "%s", uid);
        sprintf(user.pass, "%s", pass);
        user.logged = true;
    }
}

//* Logout user
void logout()
{
    char message[20];

    //* Check if there is no login
    if (user.logged == false)
    {
        fprintf(stderr, "[!]No user logged in\n");
        return;
    }

    snprintf(message, 20, "OUT %s %s\n", user.uid, user.pass);
    if (udp_send(DSIP, DSport, message, strlen(message)) == OK)
    {
        //*Localy clean user info
        user.logged = false;
        memset(user.uid, 0, 6);
        memset(user.pass, 0, 9);
        memset(user.gid, 0, 3);
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

    printf("[-]Current user ID: %s\n", user.uid);
}

//* Show all groups
void groups()
{
    char message[5];

    sprintf(message, "GLS\n");

    udp_send(DSIP, DSport, message, strlen(message));
}

//* Enter/Create a group
void subscribe()
{
    char *gid, *gname, *end;
    char message[38];

    //* Check if there is no login
    if (user.logged == false)
    {
        fprintf(stderr, "[!]No user logged in\n");
        return;
    }

    //* Get gid and gname
    if ((gid = strtok(NULL, " ")) == NULL || (gname = strtok(NULL, " ")) == NULL || (end = strtok(NULL, " ")) != NULL)
    {
        arguments_error();
        return;
    }

    //*Check gid and gname format
    if (regexec(&reg_gid, gid, 0, NULL, 0) != 0)
    {
        fprintf(stderr, "[!]Gid has to be 2 numeric characters\n");
        return;
    }
    if (regexec(&reg_gname, gname, 0, NULL, 0) != 0)
    {
        fprintf(stderr, "[!]Gname has to be up to 24 alphanumeric characters (plus '-' or '_'\n");
        return;
    }

    snprintf(message, 38, "GSR %s %s %s\n", user.uid, gid, gname);

    udp_send(DSIP, DSport, message, strlen(message));
}

//* Leave a group
void unsubscribe()
{
    char *gid, *end;
    char message[16];

    //* Check if no user if logged in
    if (user.logged == false)
    {
        fprintf(stderr, "[!]No user logged in\n");
        return;
    }

    //* Get gid
    if ((gid = strtok(NULL, " ")) == NULL || (end = strtok(NULL, " ")) != NULL)
    {
        arguments_error();
        return;
    }

    //*Check gid format
    if (regexec(&reg_gid, gid, 0, NULL, 0) != 0)
    {
        fprintf(stderr, "[!]Gid has to be 2 numeric characters\n");
        return;
    }

    snprintf(message, 16, "GUR %s %s\n", user.uid, gid);

    udp_send(DSIP, DSport, message, strlen(message));
}

//* Show groups the current user is subscribed
void my_groups()
{
    char message[11];

    //* Check if user no is logged in
    if (user.logged == false)
    {
        fprintf(stderr, "[!]No user logged in\n");
        return;
    }

    snprintf(message, 11, "GLM %s\n", user.uid);

    udp_send(DSIP, DSport, message, strlen(message));
}

//* Select group
void sag()
{
    char *gid, *end;

    //* Check if no user is logged in
    if (user.logged == false)
    {
        fprintf(stderr, "[!]No user logged in\n");
        return;
    }

    //* Get gid
    if ((gid = strtok(NULL, " ")) == NULL || (end = strtok(NULL, " ")) != NULL)
    {
        arguments_error();
        return;
    }

    if (regexec(&reg_gid, gid, 0, NULL, 0) != 0)
    {
        fprintf(stderr, "[!]Gid has to be 2 numeric characters\n");
        return;
    }

    snprintf(user.gid, 3, "%s", gid);
}

//* Show current selected group
void showgid()
{
    //* Check if no user is logged in
    if (user.logged == false)
    {
        fprintf(stderr, "No user logged in!\n");
        return;
    }

    //* Check if there is a selected group
    if (user.gid == 0)
    {
        fprintf(stderr, "[!]No group selected\n");
        return;
    }

    printf("[-]Selected group ID: %s\n", user.gid);
}

//* Show all users of current selected group
void ulist()
{
    char message[8];

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

    snprintf(message, 8, "ULS %s\n", user.gid);

    tcp_send(DSIP, DSport, message, strlen(message), NULL);
}

//* Send a message to the current group
void post()
{
    char *str, *text, *filename, *end;
    char message[512];

    //* Check if there is no login
    if (user.logged == false)
    {
        fprintf(stderr, "[!]No user logged in\n");
        return;
    }

    //* Get rest of the command
    if ((str = strtok(NULL, "\0")) == NULL)
    {
        arguments_error();
        return;
    }

    //* If is only text
    if (regexec(&reg_text, str, 0, NULL, 0) == 0)
    {
        if ((text = strtok(str, "\"")) == NULL || (filename = strtok(NULL, "\"")) != NULL)
        {
            arguments_error();
            return;
        }
    }
    //* If its text and file
    else if (regexec(&reg_file, str, 0, NULL, 0) == 0)
    {
        if ((text = strtok(str, "\"")) == NULL || (filename = strtok(NULL, "\0")) == NULL || (end = strtok(NULL, "\0")) != NULL)
        {
            arguments_error();
            return;
        }

        filename++;
    }
    else
    {
        fprintf(stderr, "[!]Wrong format for post, check arguments\n");
        ;
        return;
    }

    sprintf(message, "PST %s %s %ld %s", user.uid, user.gid, strlen(text), text);
    if (filename == NULL)
    {
        tcp_send(DSIP, DSport, message, strlen(message), NULL);
    }
    else
    {
        tcp_send(DSIP, DSport, message, strlen(message), filename);
    }
}

//TODO
void retrieve()
{
    char message[19];
    char *mid;

    if ((mid = strtok(NULL, " ")) == NULL)
    {
        arguments_error();
        return;
    }
    if (user.logged == false)
    {
        fprintf(stderr, "No user logged in!\n");
        return;
    }
    if (regexec(&reg_gid, user.gid, 0, NULL, 0) != 0)
    {
        fprintf(stderr, "No group selected\n");
        return;
    }
    if (regexec(&reg_mid, mid, 0, NULL, 0) != 0)
    {
        fprintf(stderr, "MID has to be 4 numeric characters!\n");
        return;
    }

    snprintf(message, 19, "RTV %s %s %s\n", user.uid, user.gid, mid);
    printf("%s", message);

    tcp_send(DSIP, DSport, message, strlen(message), NULL);
}

//* Initialize client
void init()
{
    //*Initialize local user
    user.logged = false;
    memset(user.uid, 0, 6);
    memset(user.pass, 0, 9);
    memset(user.gid, 0, 3);

    //*Initialize regular expressions
    if (regcomp(&reg_uid, "^[0-9]{5}$", REG_EXTENDED) != 0)
    {
        fprintf(stderr, "Regular expression for uid compilation failed!");
        exit(1);
    }
    if (regcomp(&reg_pass, "^[0-9a-zA-Z]{8}$", REG_EXTENDED) != 0)
    {
        fprintf(stderr, "Regular expression for pass compilation failed!");
        exit(1);
    }
    if (regcomp(&reg_gid, "^[0-9]{2}$", REG_EXTENDED) != 0)
    {
        fprintf(stderr, "Regular expression for gid compilation failed!");
        exit(1);
    }
    if (regcomp(&reg_gname, "^[0-9a-zA-Z_-]{1,24}$", REG_EXTENDED) != 0)
    {
        fprintf(stderr, "Regular expression for gname compilation failed!");
        exit(1);
    }
    if (regcomp(&reg_mid, "^[0-9]{4}$", REG_EXTENDED) != 0)
    {
        fprintf(stderr, "Regular expression for mid compilation failed!");
        exit(1);
    }
    if (regcomp(&reg_text, "^\"[^\"]{1,240}\"$", REG_EXTENDED) != 0)
    {
        fprintf(stderr, "Regular expression for text compilation failed!");
        exit(1);
    }
    if (regcomp(&reg_file, "^\"[^\"]{1,240}\" [0-9a-zA-Z_.-]{1,20}\\.[0-9a-z]{3}$", REG_EXTENDED) != 0)
    {
        fprintf(stderr, "Regular expression for fname compilation failed!");
        exit(1);
    }
}


int main(int argc, char *argv[])
{
    char buffer[512];
    char *cmd;

    init();

    //TODO read flags

    if (gethostname(buffer, sizeof(buffer)) == -1)
    {
        fprintf(stderr, "Error getting host name\n");
    }

    //FIXME hardcoded for testing
    strcpy(DSIP, "tejo.tecnico.ulisboa.pt");
    strcpy(DSport, "58011");

    while (true)
    {
        fgets(buffer, sizeof(buffer), stdin);
        buffer[strlen(buffer) - 1] = '\0';

        if ((cmd = strtok(buffer, " ")) == NULL)
        {
            continue;
        }

        //*Register user
        if (strcmp(cmd, "reg") == 0)
        {
            reg();
        }
        //*Unregister user
        else if (strcmp(cmd, "unr") == 0 || strcmp(cmd, "unregister") == 0)
        {
            unr();
        }
        //*Login
        else if (strcmp(cmd, "login") == 0)
        {
            login();
        }
        //*Logout
        else if (strcmp(cmd, "logout") == 0)
        {
            logout();
        }
        //*Show user id
        else if (strcmp(cmd, "su") == 0 || strcmp(cmd, "showuid") == 0)
        {
            showuid();
        }
        //*Exit application
        else if (strcmp(cmd, "exit") == 0)
        {
            exit(0);
        }
        //*Show all groups
        else if (strcmp(cmd, "gl") == 0 || strcmp(cmd, "groups") == 0)
        {
            groups();
        }
        //*Enter/Create a group
        else if (strcmp(cmd, "s") == 0 || strcmp(cmd, "subscribe") == 0)
        {
            subscribe();
        }
        //*Leave a group
        else if (strcmp(cmd, "u") == 0 || strcmp(cmd, "unsubscribe") == 0)
        {
            unsubscribe();
        }
        //*Show all the groups that the user is in
        else if (strcmp(cmd, "mgl") == 0 || strcmp(cmd, "my_groups") == 0)
        {
            my_groups();
        }
        //*Select a group
        else if (strcmp(cmd, "sag") == 0 || strcmp(cmd, "select") == 0)
        {
            sag();
        }
        //*Show current group id
        else if (strcmp(cmd, "sg") == 0 || strcmp(cmd, "showgid") == 0)
        {
            showgid();
        }
        //*Show all user of the selected group
        else if (strcmp(cmd, "ul") == 0 || strcmp(cmd, "ulist") == 0)
        {
            ulist();
        }
        //*Send a messge to group
        else if (strcmp(cmd, "post") == 0)
        {
            post();
        }
        //*Retrieve messages from group
        else if (strcmp(cmd, "r") == 0 || strcmp(cmd, "retrieve") == 0)
        {
            retrieve();
        }
        else
        {
            fprintf(stderr, "[!]Command \"%s\" doesn't exist.\n", cmd);
        }
    }

    regfree(&reg_uid);
    regfree(&reg_pass);
    regfree(&reg_gid);
    regfree(&reg_gname);
    regfree(&reg_mid);
    regfree(&reg_text);
    regfree(&reg_file);

    exit(0);
}