#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdbool.h>
#include <ctype.h> 



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
        for(i = 0; value[i]!="\0" ; i++){
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
            }
        }

        if(i+1 != size){
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

    //TODO UDP(DSIP, Dsport, message)

    printf("SEND: %s", message);
}

int main(int argc, char *argv[]){
    char DSIP[128], buffer[128];
    char *cmd;
    int DSport;

    if(gethostname(buffer, 128) == -1){
        fprintf(stderr, "Error getting host name\n");
    }

    strcpy(DSIP,buffer);
    DSport = 58005;

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

        
        //*Login
        }else if(strcmp(cmd, "login") == 0){

        
        //*Logout
        }else if(strcmp(cmd, "logout") == 0){

        
        //*Exit application
        }else if(strcmp(cmd, "exit") == 0){

        
        //*Show all groups
        }else if(strcmp(cmd, "gl") == 0 || strcmp(cmd, "groups") == 0){

        
        //*Enter/Create a group
        }else if(strcmp(cmd, "s") == 0 || strcmp(cmd, "subscribe") == 0){
        
        
        //*Leave a group
        }else if(strcmp(cmd, "u") == 0 || strcmp(cmd, "unsubscribe") == 0){
            
        
        //*Show all the groups that the user is in
        }else if(strcmp(cmd, "mgl") == 0 || strcmp(cmd, "my_groups") == 0){
            
        
        //*Select a group
        }else if(strcmp(cmd, "sag") == 0 || strcmp(cmd, "select") == 0){
            
        
        //*Show all user of the selected group
        }else if(strcmp(cmd, "ul") == 0 || strcmp(cmd, "ulist") == 0){
            
        
        //*Send a messge to group    
        }else if(strcmp(cmd, "post") == 0){
            
        
        //*Retrieve messages from group
        }else if(strcmp(cmd, "r") == 0 || strcmp(cmd, "retrieve") == 0){
            

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