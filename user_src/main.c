#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include "auxiliar/UDP.h"
#include "auxiliar/TCP.h"

struct user_info{
    char uid[5];
    char pass[8];
    int logged;
};

struct user_info user;
char DSIP[128];
char DSport[6];


void arguments_error(){
    fprintf(stderr, "Not enough arguments\n");
}


/*
*Check the argument format
*arg: argument name
*value: argument value
*size: string size desired (0 to not check)
*alphanum: true to check alphanumeric char, false to only check numeric
*/
bool check_arg(const char* arg, const char* value, int size, bool alphanum){
    int i;
    bool error = false;
  
    //*size check
    if(size != 0){
        for(i = 0; value[i]!= '\0' ; i++){
            switch (alphanum){
                case true:
                    if(isalnum(value[i]) == 0){
                        error = true;
                    }
                    break;
                case false:
                    if(isdigit(value[i]) == 0){
                        error = true;
                    }
                    break;
            }

            if(error){
                fprintf(stderr, "%s has to be %s\n", arg, alphanum? "alphanumeric":"numeric");
                break; 
            }
        }

        if(i != size){
            error = true;
            fprintf(stderr, "%s must have a size of %d\n", arg, size); 
        }
    }
 
    return error;
}


//*Execute the registration command
void reg(const char* buffer){
    char *uid, *pass;
    char message[18];

    //*Get uid and password
    if((uid = strtok(NULL, " ")) == NULL || (pass = strtok(NULL, " ")) == NULL){
        arguments_error();
        return;   
    }

    //*Check uid and pass format
    if(check_arg("uid", uid, 5, false)){
        return;
    }
    if(check_arg("password", pass, 8, true)){
        return;
    }

    sprintf(message, "REG %s %s\n", uid, pass);

    udp_send(DSIP, DSport, message);

    printf("SEND: %s", message);
}


void unr(const char* buffer){
    char *uid, *pass;
    char message[18];

    //*Get uid and password
    if((uid = strtok(NULL, " ")) == NULL || (pass = strtok(NULL, " ")) == NULL){
        arguments_error();
        return;   
    }

    //*Check uid and pass format
    if(check_arg("uid", uid, 5, false)){
        return;
    }
    if(check_arg("password", pass, 8, true)){
        return;
    }

    sprintf(message, "UNR %s %s\n", uid, pass);

    //TODO UDP(DSIP, Dsport, message)

    printf("SEND: %s", message);
}


void login(const char* buffer){
    char *uid, *pass;
    char message[18];

    //*Get uid and password
    if((uid = strtok(NULL, " ")) == NULL || (pass = strtok(NULL, " ")) == NULL){
        arguments_error();
        return;   
    }

    //*Check uid and pass format
    if(check_arg("uid", uid, 5, false)){
        return;
    }
    if(check_arg("password", pass, 8, true)){
        return;
    }

    sprintf(message, "LOG %s %s\n", uid, pass);

    //TODO UDP(DSIP, Dsport, message)

    //! DELETE, TEST ONLY
    strcpy(user.uid, uid);
    strcpy(user.pass, pass);
    user.logged = 1;

    /*
    if(){
        strcpy(user.uid, uid);
        strcpy(user.pass, pass);
        user.loogged = 1;
    }
    */

    printf("SEND: %s", message);
}


void logout(){
    char message[18];
    sprintf(message, "OUT %s %s", user.uid, user.pass);
    //TODO UDP(DSIP, Dsport, message)
    //! DELETE, TEST ONLY
    printf("Logout: %s %s", user.uid, user.pass);
    user.logged = 0;

    /*
    if(){
        user = 0;
    }
    */
    printf("SEND: %s", message);
}


void ex(){

}


void groups(const char* buffer){

}


void subscribe(const char* buffer){

}


void unsubscribe(const char* buffer){

}


void mgl(const char* buffer){

}


void sag(const char* buffer){

}


void ulist(const char* buffer){

}


void post(const char* buffer){

}


void retrieve(const char* buffer){

}


int main(int argc, char *argv[]){
    char buffer[128];
    char *cmd;


    if(gethostname(buffer, 128) == -1){
        fprintf(stderr, "Error getting host name\n");
    }

    strcpy(DSIP,"tejo.ulisboa.tecnico.pt");
    strcpy(DSport,"58000");

    while(true){
        fgets(buffer, sizeof(buffer), stdin);
        buffer[strlen(buffer)-1] = 0;

      
        if((cmd = strtok(buffer, " ")) == NULL){
            continue;
        }
        
        //*Register user
        if(strcmp(cmd, "reg") == 0){      
            reg(buffer);
        
        //*Unregister user
        }else if(strcmp(cmd, "unr") == 0 || strcmp(cmd, "unregister") == 0){
            unr(buffer);
            
        //*Login
        }else if(strcmp(cmd, "login") == 0){
            login(buffer);
        
        //*Logout
        }else if(strcmp(cmd, "logout") == 0){
            logout();
        
        //*Exit application
        }else if(strcmp(cmd, "exit") == 0){
            exit(0);
            ex();
        
        //*Show all groups
        }else if(strcmp(cmd, "gl") == 0 || strcmp(cmd, "groups") == 0){
            groups(buffer);
        
        //*Enter/Create a group
        }else if(strcmp(cmd, "s") == 0 || strcmp(cmd, "subscribe") == 0){
            subscribe(buffer);
        
        //*Leave a group
        }else if(strcmp(cmd, "u") == 0 || strcmp(cmd, "unsubscribe") == 0){
            unsubscribe(buffer);
        
        //*Show all the groups that the user is in
        }else if(strcmp(cmd, "mgl") == 0 || strcmp(cmd, "my_groups") == 0){
            mgl(buffer);
        
        //*Select a group
        }else if(strcmp(cmd, "sag") == 0 || strcmp(cmd, "select") == 0){
            sag(buffer);
        
        //*Show all user of the selected group
        }else if(strcmp(cmd, "ul") == 0 || strcmp(cmd, "ulist") == 0){
            ulist(buffer);
        
        //*Send a messge to group    
        }else if(strcmp(cmd, "post") == 0){
            post(buffer);
        
        //*Retrieve messages from group
        }else if(strcmp(cmd, "r") == 0 || strcmp(cmd, "retrieve") == 0){
            retrieve(buffer);

        }else{
            fprintf(stderr, "Command \"%s\" doesn't exist.\n", cmd);
        }


      /*   while(cmd != NULL){
            
           
        printf("%s\n", cmd);

        cmd = strtok(NULL, " ");
        } */
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