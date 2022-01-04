#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include "file_manager.h"
#include "../../protocol_constants.h"

#define USERS "server_data/USERS"
#define GROUPS "server_data/GROUPS"

void init_server_data()
{   
    if(mkdir("server_data", S_IRWXU) == -1 && errno != EEXIST)
    {
        fprintf(stderr, "Error: %s\n", strerror(errno));
        exit(1);
    }

    if(mkdir(USERS, S_IRWXU) == -1 && errno != EEXIST)
    {
        fprintf(stderr, "Error: %s\n", strerror(errno));
        exit(1);
    }

    if(mkdir(GROUPS, S_IRWXU) == -1 && errno != EEXIST)
    {
        fprintf(stderr, "Error: %s\n", strerror(errno));
        exit(1);
    }
}

//* User Management
int user_create(const char* uid, const char* pass)
{   
    char dir[25];
    char file[40];
    FILE* f = NULL;

    sprintf(dir, "%s/%s", USERS, uid);

    //*Create user directory
    if(mkdir(dir, S_IRWXU) == -1)
    {   
        if(errno == EEXIST)
        {
            return DUP;
        }

        fprintf(stderr, "Error(create user(%s) directory): %s\n", uid, strerror(errno));
        return NOK;
    }

    //* Create password file
    sprintf(file, "%s/%s_pass.txt", dir, uid);
    if((f = fopen(file, "w+")) == NULL)
    {
        //* Error creating password file-> delete directory
        fprintf(stderr, "Error(create user(%s) password): %s\n", uid, strerror(errno));
        
        if(remove(dir) == -1)
        {   
            fprintf(stderr, "Error(delete user(%s) diretory): %s\n", uid, strerror(errno));
            return NOK;
        }

        return NOK;
    }

    //* Write password
    if(fputs(pass, f) == EOF)
    {   
        //* Error writing password-> delete directory and password file
        fprintf(stderr, "Error associating user password)");

        if(remove(file) == -1)
        {   
            fprintf(stderr, "Error(delete user(%s) pass file): %s\n", uid, strerror(errno));
            return NOK;
        }

        if(remove(dir) == -1)
        {   
            fprintf(stderr, "Error(delete user(%s) diretory): %s\n", uid, strerror(errno));
            return NOK;
        }

        return NOK;
    }

    fclose(f);

    return OK;
}

int user_delete(const char* uid)
{
    char dir[25];
    char file[41];

    sprintf(dir, "%s/%s", USERS, uid);

    sprintf(file, "%s/%s_login.txt", dir, uid);
    if(remove(file) == -1 && errno != ENOENT)
    {   
        fprintf(stderr, "Error(delete user(%s) login file): %s\n", uid, strerror(errno));
        return NOK;
    }

    sprintf(file, "%s/%s_pass.txt", dir, uid);
    if(remove(file) == -1)
    {   
        fprintf(stderr, "Error(delete user(%s) pass file): %s\n", uid, strerror(errno));
        return NOK;
    }

    if(remove(dir) == -1)
    {   
        fprintf(stderr, "Error(delete user(%s) diretory): %s\n", uid, strerror(errno));
        return NOK;
    }

    return OK;
}

int check_pass(const char* uid, const char* pass)
{
    char file[40];
    char stored_pass[8];
    FILE* f = NULL;

    sprintf(file, "%s/%s/%s_pass.txt", USERS, uid, uid);
    if((f = fopen(file, "w+")) == NULL)
    {   
        fprintf(stderr, "Error(opening user(%s) password file): %s", uid, strerror(errno));
        return NOK;
    }

    if(fgets(stored_pass, 8, f) == NULL)
    {
        fprintf(stderr, "Error(reading user(%s) password): %s\n", uid, strerror(errno));
        return NOK;
    }

    fclose(f);

    if(strncmp(pass, stored_pass, 8) != 0)
    {
        return NOK;
    }

    return OK;
}

int user_entry(const char* uid, const char* pass, bool login)
{   
    char file[41];
    FILE* f = NULL;

    if(check_pass(uid, pass) == NOK)
    {
        return NOK;
    }
    
    sprintf(file, "%s/%s/%s_login.txt", USERS, uid, uid);
    if(login && (f = fopen(file, "w+")) == NULL)
    {   
        fprintf(stderr, "Error(creating user(%s) login): %s", uid, strerror(errno));
        return NOK;
    }
    else if(!login && remove(file) == -1 && errno != ENOENT)
    {
        fprintf(stderr, "Error(creating user(%s) logout): %s", uid, strerror(errno));
        return NOK;
    }

    fclose(f);

    return OK;
}

//*Groups Management

int create_dir(const char* dir)
{
    if(mkdir(dir, S_IRWXU) == -1)
    {
        fprintf(stderr, "Error: %s\n", strerror(errno));
        return NOK;
    }

    return OK;
}

int delete_dir(const char* dir)
{
    if(remove(dir) == -1)
    {
        fprintf(stderr, "Error: %s\n", strerror(errno));
        return NOK;
    }

    return OK;
}


//*Files Management
int write_file(const char* file, const char* content)
{   
    FILE* f;

    if((f = fopen(file, "w+")) == NULL)
    {
        fprintf(stderr, "Error: %s\n", strerror(errno));
        return NOK;
    }

    if(content != NULL)
    {
        fputs(content, f);
    }

    fclose(f);

    return OK;
}

int delete_file(const char* file)
{
    if(remove(file) == -1)
    {
        fprintf(stderr, "Error: %s\n", strerror(errno));
        return NOK;
    }

    return OK;
}
