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
regex_t reg_fname;

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
    fprintf(stderr, "Not enough arguments\n");
}

//*Execute the registration command
void reg(char* buffer)
{
    char *uid, *pass;
    char message[20];

    //*Get uid and password
    if((uid = strtok(NULL, " ")) == NULL || (pass = strtok(NULL, " ")) == NULL)
    {
        arguments_error();
        return;   
    }

    //*Check uid and pass format
    if(regexec(&reg_uid, uid, 0, NULL, 0) != 0)
    {   
        fprintf(stderr, "UID has to be 5 numeric characters!\n");
        return;
    }
    if(regexec(&reg_pass, pass, 0, NULL, 0) != 0)
    {
        fprintf(stderr, "Password has to be 8 alphanumeric characters!\n");
        return;
    }

    snprintf(message, 20, "REG %s %s\n", uid, pass);

    udp_send(DSIP, DSport, message, sizeof(message)-1);
}

void unr(char* buffer){
    char *uid, *pass;
    char message[20];

    //*Get uid and password
    if((uid = strtok(NULL, " ")) == NULL || (pass = strtok(NULL, " ")) == NULL)
    {
        arguments_error();
        return;   
    }

    //*Check uid and pass format
    if(regexec(&reg_uid, uid, 0, NULL, 0) != 0)
    {   
        fprintf(stderr, "UID has to be 5 numeric characters!\n");
        return;
    }
    if(regexec(&reg_pass, pass, 0, NULL, 0) != 0)
    {
        fprintf(stderr, "Password has to be 8 alphanumeric characters!\n");
        return;
    }

    snprintf(message, 20, "UNR %s %s\n", uid, pass);

    udp_send(DSIP, DSport, message, sizeof(message)-1);
}

 
void login(char* buffer)
{   
    char *uid, *pass;
    char message[20];

    //*Check if there is already a log in
    if(user.logged == true)
    {
        fprintf(stderr, "An user is already logged in!\n");
        return;
    }

    //*Get uid and password
    if((uid = strtok(NULL, " ")) == NULL || (pass = strtok(NULL, " ")) == NULL)
    {
        arguments_error();
        return;   
    }

    //*Check uid and pass format
    if(regexec(&reg_uid, uid, 0, NULL, 0) != 0)
    {   
        fprintf(stderr, "UID has to be 5 numeric characters!\n");
        return;
    }
    if(regexec(&reg_pass, pass, 0, NULL, 0) != 0)
    {
        fprintf(stderr, "Password has to be 8 alphanumeric characters!\n");
        return;
    }

    snprintf(message, 20, "LOG %s %s\n", uid, pass);

    

    if(udp_send(DSIP, DSport, message, sizeof(message)-1) == OK)
    {
        sprintf(user.uid,"%s",uid);
        sprintf(user.pass,"%s",pass);
        user.logged = true;
    }
}


void logout()
{
    char message[20];

    if(user.logged == false)
    {
        fprintf(stderr, "No user logged in!\n");
        return;
    }

    snprintf(message, 20, "OUT %s %s\n", user.uid, user.pass);
    if(udp_send(DSIP, DSport, message, sizeof(message)-1) == OK)
    {   
        //*Localy clean user info
        user.logged = false;
        memset(user.uid, 0, 6);
        memset(user.pass, 0, 9);
        memset(user.gid, 0, 3);
    }


}


void showuid()
{
    if(user.logged == false)
    {
        fprintf(stderr, "No user logged in!\n");
        return;
    }

    printf("Current user ID: %s", user.uid);
}


void ex()
{
    //TODO Close all TCP connections, maybe useless
}


void groups(char* buffer)
{
    char message[5];

    sprintf(message, "GLS\n");

    udp_send(DSIP, DSport, message, sizeof(message)-1);
}


void subscribe(char* buffer)
{
    char message[38];
    char *GID, *GName;

    if(user.logged == false)
    {
        fprintf(stderr, "No user logged in!\n");
        return;
    }

    if((GID = strtok(NULL, " ")) == NULL || (GName = strtok(NULL, " ")) == NULL)
    {
        arguments_error();
        return;   
    }
    
    if(strlen(GID) != 2 || strlen(GName) > 24)
    {   
        //FIXME informational message
        arguments_error();
        return;
    }

    snprintf(message, 38, "GSR %s %s %s\n", user.uid, GID, GName);

    printf("%s", message);

    udp_send(DSIP, DSport, message, strlen(message));
}


void unsubscribe(char* buffer)
{
    char message[16];
    char *GID;

    if(user.logged == false)
    {
        fprintf(stderr, "No user logged in!\n");
        return;
    }

    if((GID = strtok(NULL, " ")) == NULL)
    {
        arguments_error();
        return;   
    }

    if(strlen(GID) != 2)
    {
        //FIXME informational message
        arguments_error();
        return;
    }

    snprintf(message, 16, "GUR %s %s\n", user.uid, GID);

    printf("%s", message);

    udp_send(DSIP, DSport, message, strlen(message));
}


void my_groups(char* buffer)
{
    char message[11];

    if(user.logged == false)
    {
        fprintf(stderr, "No user logged in!\n");
        return;
    }

    snprintf(message, 11, "GLM %s\n", user.uid);

    udp_send(DSIP,DSport, message, strlen(message));
}


void sag(char* buffer)
{
    char *GID;

    if(user.logged == false)
    {
        fprintf(stderr, "No user logged in!\n");
        return;
    }

    if((GID = strtok(NULL, " ")) == NULL)
    {
        arguments_error();
        return;   
    }

    if(regexec(&reg_gid, GID, 0, NULL, 0) != 0)
    {
        fprintf(stderr, "Invalid group ID!\n");
        return;
    }

    snprintf(user.gid, 3, "%s", GID);

    printf("%s\n", user.gid);
    
}


void showgid()
{
    if(user.logged == false)
    {
        fprintf(stderr, "No user logged in!\n");
        return;
    }

    if(regexec(&reg_gid, user.gid, 0, NULL, 0) != 0)
    {
        fprintf(stderr, "No group ID!\n");
        return;
    }

    printf("Selected group ID: %s", user.gid);
}


void ulist()
{
    char message[8];

    if(user.logged == false)
    {
        fprintf(stderr, "No user logged in!\n");
        return;
    }
    if(regexec(&reg_gid, user.gid, 0, NULL, 0) != 0)
    {
        fprintf(stderr, "No group selected!\n");
        return;
    }

    snprintf(message, 8, "ULS %s\n", user.gid);

    tcp_send(DSIP,DSport,message, strlen(message));
    
}


void post(char* buffer)
{
    //char message[TSIZE], *fName;
    char message[270], *text, *fName;
    FILE *fp;
    char array[1024] = {0};
    char data[10000000];

    if(user.logged == false)
    {
        fprintf(stderr, "No user logged in!\n");
        return;
    }
    if(regexec(&reg_gid, user.gid, 0, NULL, 0) != 0)
    {
        fprintf(stderr, "No group selected!\n");
        return;
    }
    if(strncmp(buffer+5,"\"",1) != 0)
    {
        fprintf(stderr, "Must have text delimited by \"\"\n");
        return;
    }

    text = strtok(NULL, "\"");
    printf("texto ->> %s\n", text);

    if(strlen(text) > 240)
    {
        fprintf(stderr, "Text over characters limit!\n");
        return;
    }
    if(regexec(&reg_text, text, 0, NULL, 0) != 0)
    {
        fprintf(stderr, "Invalid character in text\n");
        return;
    }

    if((fName = strtok(NULL, " ")) != NULL)
    {

        if(regexec(&reg_fname, fName, 0, NULL, 0) != 0)
        {
            fprintf(stderr, "Invalid file name\n");
            return;
        }

        printf("%s\n", fName);

        fp = fopen(fName, "r");
        if(fp == NULL)
        {
            fprintf(stderr, "Error opening file!\n");
        }

        while(fgets(array, 1024, fp) != NULL) {
            strncat(data,array, strlen(array));
            bzero(array, 1024);
        }
                                //    18 + len  |  len fname + 10 + len data
        snprintf(message, 28 + strlen(text) + strlen(fName) + strlen(data), "PST %s %s %ld %s %s %ld %s\n", user.uid, user.gid, strlen(text), text, fName);
        printf("%ld\n",strlen(data));
        //printf("uno %s\n", message);

        /*meter cenas a enviar*/
    }
    else
    {    
        
                              // 4   6  3  4  240 +1
        snprintf(message, 18 + strlen(text), "PST %s %s %ld %s\n", user.uid, user.gid, strlen(text), text);
        printf("dos %s\n", message);
    }

    //tcp_send(DSIP,DSport,message, strlen(message));
    printf("YAYAAYAY\n");
}


void retrieve(char* buffer)
{
    char message[19];
    char *mid;

    if((mid = strtok(NULL, " ")) == NULL)
    {
        arguments_error();
        return;
    }
    if(user.logged == false)
    {
        fprintf(stderr, "No user logged in!\n");
        return;
    }
    if(regexec(&reg_gid, user.gid, 0, NULL, 0) != 0)
    {   
        fprintf(stderr, "No group selected\n");
        return;
    }
    if(regexec(&reg_mid, mid, 0, NULL, 0) != 0)
    {   
        fprintf(stderr, "MID has to be 4 numeric characters!\n");
        return;
    }
    
    snprintf(message, 19, "RTV %s %s %s\n", user.uid, user.gid, mid);
    printf("%s", message);

    tcp_send(DSIP, DSport, message, strlen(message));

    

}


void init()
{
    //*Initialize local user
    user.logged = false;
    memset(user.uid, 0, 6);
    memset(user.pass, 0, 9);
    memset(user.gid, 0, 3);

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
    if(regcomp(&reg_text, "^.{1,240}$", REG_EXTENDED) != 0)
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
    char buffer[BUFFER];
    char *cmd;

    init();
    
    //TODO read flags

    if(gethostname(buffer, sizeof(buffer)) == -1)
    {
        fprintf(stderr, "Error getting host name\n");
    }

    //FIXME hardcoded for testing
    strcpy(DSIP,"tejo.tecnico.ulisboa.pt");
    strcpy(DSport,"58011");

    while(true)
    {
        fgets(buffer, sizeof(buffer), stdin);
        buffer[strlen(buffer)-1] = '\0';

        if((cmd = strtok(buffer, " ")) == NULL)
        {
            continue;
        }
        
        //*Register user
        if(strcmp(cmd, "reg") == 0)
        {      
            reg(buffer);
        }
        //*Unregister user
        else if(strcmp(cmd, "unr") == 0 || strcmp(cmd, "unregister") == 0)
        {
            unr(buffer);
        }
        //*Login
        else if(strcmp(cmd, "login") == 0)
        {
            login(buffer);
        }
        //*Logout
        else if(strcmp(cmd, "logout") == 0)
        {
            logout();
        }
        //*Show user id
        else if(strcmp(cmd, "su") == 0 ||strcmp(cmd, "showuid") == 0)
        {
            showuid();
        }
        //*Exit application
        else if(strcmp(cmd, "exit") == 0)
        {
            exit(0);
            ex();
        }
        //*Show all groups
        else if(strcmp(cmd, "gl") == 0 || strcmp(cmd, "groups") == 0)
        {
            groups(buffer);
        }
        //*Enter/Create a group
        else if(strcmp(cmd, "s") == 0 || strcmp(cmd, "subscribe") == 0)
        {
            subscribe(buffer);
        }
        //*Leave a group
        else if(strcmp(cmd, "u") == 0 || strcmp(cmd, "unsubscribe") == 0)
        {
            unsubscribe(buffer);
        }
        //*Show all the groups that the user is in
        else if(strcmp(cmd, "mgl") == 0 || strcmp(cmd, "my_groups") == 0)
        {
            my_groups(buffer);
        }
        //*Select a group
        else if(strcmp(cmd, "sag") == 0 || strcmp(cmd, "select") == 0)
        {
            sag(buffer);
        }
        //*Show current group id
        else if(strcmp(cmd, "sg") == 0 || strcmp(cmd, "showgid") == 0)
        {
            showgid();
        }
        //*Show all user of the selected group
        else if(strcmp(cmd, "ul") == 0 || strcmp(cmd, "ulist") == 0)
        {
            ulist(buffer);
        
        }
        //*Send a messge to group    
        else if(strcmp(cmd, "post") == 0)
        {
            post(buffer);
        }
        //*Retrieve messages from group
        else if(strcmp(cmd, "r") == 0 || strcmp(cmd, "retrieve") == 0)
        {
            retrieve(buffer);
        }
        else
        {
            fprintf(stderr, "Command \"%s\" doesn't exist.\n", cmd);
        }
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