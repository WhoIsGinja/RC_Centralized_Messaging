#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include "auxiliar/connection_controller.h"
#include "../protocol_constants.h"

struct user_info
{
    char uid[6];
    char pass[9];
    int logged;
};

struct user_info user;
char DSIP[128];
char DSport[6];


void arguments_error()
{
    fprintf(stderr, "Not enough arguments\n");
}


/*
*Check the argument format
*arg: argument name
*value: argument value
*size: string size desired (0 to not check)
*alphanum: true to check alphanumeric char, false to only check numeric
*/
bool check_arg(const char* arg, const char* value, int size, bool alphanum)
{
    int i;
    bool error = false;
  
    //*size check
    if(size != 0)
    {
        for(i = 0; value[i]!= '\0' ; i++)
        {
            switch (alphanum)
            {
                case true:
                    if(isalnum(value[i]) == 0)
                    {
                        error = true;
                    }
                    break;
                case false:
                    if(isdigit(value[i]) == 0)
                    {
                        error = true;
                    }
                    break;
            }

            if(error)
            {
                fprintf(stderr, "%s has to be %s\n", arg, alphanum? "alphanumeric":"numeric");
                break; 
            }
        }

        if(i != size)
        {
            error = true;
            fprintf(stderr, "%s must have a size of %d\n", arg, size); 
        }
    }
 
    return error;
}


//*Execute the registration command
void reg(const char* buffer)
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
    if(check_arg("uid", uid, 5, false))
    {
        return;
    }
    if(check_arg("password", pass, 8, true))
    {
        return;
    }

    snprintf(message, 20, "REG %s %s\n", uid, pass);

    udp_send(DSIP, DSport, message, sizeof(message)-1);
}


void unr(const char* buffer){
    char *uid, *pass;
    char message[20];

    //*Get uid and password
    if((uid = strtok(NULL, " ")) == NULL || (pass = strtok(NULL, " ")) == NULL)
    {
        arguments_error();
        return;   
    }

    //*Check uid and pass format
    if(check_arg("uid", uid, 5, false))
    {
        return;
    }
    if(check_arg("password", pass, 8, true))
    {
        return;
    }

    snprintf(message, 20, "UNR %s %s\n", uid, pass);

    udp_send(DSIP, DSport, message, sizeof(message)-1);
}

 
void login(const char* buffer)
{   
    if(user.logged == 1)
    {
        fprintf(stderr, "An user is already logged in!\n");
        return;
    }
    char *uid, *pass;
    char message[20];

    //*Get uid and password
    if((uid = strtok(NULL, " ")) == NULL || (pass = strtok(NULL, " ")) == NULL)
    {
        arguments_error();
        return;   
    }

    //*Check if there is already a log in
    if(user.logged == 1)
    {
        return;
    }

    //*Check uid and pass format
    if(check_arg("uid", uid, 5, false))
    {
        return;
    }
    if(check_arg("password", pass, 8, true))
    {
        return;
    }

    snprintf(message, 20, "LOG %s %s\n", uid, pass);

    

    if(udp_send(DSIP, DSport, message, sizeof(message)-1) == OK)
    {
        sprintf(user.uid,"%s",uid);
        sprintf(user.pass,"%s",pass);
        user.logged = 1;
    }
}


void logout()
{
    if(user.logged == 0)
    {
        fprintf(stderr, "No user logged in!\n");
        return;
    }

    char message[20];

    snprintf(message, 20, "OUT %s %s\n", user.uid, user.pass);
    if(udp_send(DSIP, DSport, message, sizeof(message)-1) == OK)
    {
        user.logged = 0;
    }
}


void showuid()
{
    printf("Current user ID: %s", user.uid);
}


void ex()
{
    //TODO Close all TCP connections, maybe uselless
}


void groups(const char* buffer)
{
    char message[5];

    snprintf(message, 5, "GLS\n");

    udp_send(DSIP, DSport, message, sizeof(message)-1);
}


void subscribe(const char* buffer)
{

}


void unsubscribe(const char* buffer)
{

}


void mgl(const char* buffer)
{

}


void sag(const char* buffer)
{

}


void showgid()
{

}


void ulist(const char* buffer)
{

}


void post(const char* buffer)
{

}


void retrieve(const char* buffer)
{

}


int main(int argc, char *argv[])
{
    char buffer[128];
    char *cmd;

    //TODO read flags

    if(gethostname(buffer, 128) == -1)
    {
        fprintf(stderr, "Error getting host name\n");
    }

    //FIXME hardcoded for testing
    strcpy(DSIP,"tejo.tecnico.ulisboa.pt");
    strcpy(DSport,"58011");

    while(true)
    {
        fgets(buffer, sizeof(buffer), stdin);
        buffer[strlen(buffer)-1] = 0;

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
            mgl(buffer);
        }
        //*Select a group
        else if(strcmp(cmd, "sag") == 0 || strcmp(cmd, "select") == 0)
        {
            sag(buffer);
        }
        //*Show current group id
        else if(strcmp(cmd, "sg") == 0 || strcmp(cmd, "showgid") == 0)
        {
            showgid(buffer);
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

    /* switch(argc){
        case 1:
            strcpy(DSIP,buffer);
            DSport = 58005;
            break;
        case 3:
            if(strcmp(argv[1],"-n") == 0){

            }else if(strcmp(argv[1],"-p") == 0){

            }else{

            }

            break;
        case 5:
            break;
        default:
            fprintf(stderr, "Invalid application launch format \n");
    }

    if(argc == 1){
        strcpy(DSIP,buffer);
        DSport = 58005;
    }else if(argc == 3){
        

    }else if(argc == 5){

    } */

    exit(0);
}