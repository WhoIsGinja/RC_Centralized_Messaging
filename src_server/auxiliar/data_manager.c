#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include "data_manager.h"
#include "../../protocol_constants.h"

#define USERS "data_server/USERS"
#define GROUPS "data_server/GROUPS"


void init_server_data()
{
    if (mkdir("data_server", S_IRWXU) == -1 && errno != EEXIST)
    {
        fprintf(stderr, "[!]Creating data directory: %s\n", strerror(errno));
        exit(1);
    }

    if (mkdir(USERS, S_IRWXU) == -1 && errno != EEXIST)
    {
        fprintf(stderr, "[!]Creating users directory: %s\n", strerror(errno));
        exit(1);
    }

    if (mkdir(GROUPS, S_IRWXU) == -1 && errno != EEXIST)
    {
        fprintf(stderr, "[!]Creating groups directory: %s\n", strerror(errno));
        exit(1);
    }
}


//* User Management
int check_pass(const char* uid, const char* pass)
{
    char buffer[40];
    FILE* f = NULL;

    sprintf(buffer, "%s/%s/%s_pass.txt", USERS, uid, uid);
    if ((f = fopen(buffer, "r")) == NULL)
    {
        fprintf(stderr, "[!]Opening user(%s) password file: %s\n", uid, strerror(errno));
        return NOK;
    }

    if (fgets(buffer, sizeof(buffer), f) == NULL)
    {
        fprintf(stderr, "[!]Reading user(%s) password: %s\n", uid, strerror(errno));
        fclose(f);
        return NOK;
    }
    if (strncmp(pass, buffer, 8) != 0)
    {
        return NOK;
    }

    fclose(f);

    return OK;
}


int user_create(const char* uid, const char* pass)
{
    char buffer[40];
    FILE* f = NULL;

    // *Create user directory
    sprintf(buffer, "%s/%s", USERS, uid);
    if (mkdir(buffer, S_IRWXU) == -1)
    {
        if (errno == EEXIST)
        {
            return DUP;
        }

        fprintf(stderr, "[!]Creating user(%s) directory: %s\n", uid, strerror(errno));
        return NOK;
    }

    //* Create password file
    sprintf(buffer, "%s/%s/%s_pass.txt", USERS, uid, uid);
    if ((f = fopen(buffer, "w+")) == NULL)
    {
        fprintf(stderr, "[!]Creating user(%s) password: %s\n", uid, strerror(errno));

        //* Delete user diretory
        sprintf(buffer, "%s/%s", USERS, uid);
        if (remove(buffer) == -1)
        {
            fprintf(stderr, "[!]Deleting user(%s) directory: %s\n", uid, strerror(errno));
            return NOK;
        }

        return NOK;
    }

    //* Write password
    if (fputs(pass, f) == EOF)
    {   
        fclose(f);

        fprintf(stderr, "[!]Saving user(%s) password\n", uid);

        //* Delete password file
        sprintf(buffer, "%s/%s/%s_pass.txt", USERS, uid, uid);
        if (remove(buffer) == -1)
        {
            fprintf(stderr, "[!]Deleting user(%s) password: %s\n", uid, strerror(errno));
            return NOK;
        }

        //* Delete user directory
        sprintf(buffer, "%s/%s", USERS, uid);
        if (remove(buffer) == -1)
        {
            fprintf(stderr, "[!]Deleting user(%s) diretory: %s\n", uid, strerror(errno));
            return NOK;
        }

        return NOK;
    }

    fclose(f);

    return OK;
}


int user_delete(const char* uid, const char* pass)
{
    char buffer[41];

    if (check_pass(uid, pass) == NOK)
    {
        return NOK;
    }

    //* Delete user login
    sprintf(buffer, "%s/%s/%s_login.txt", USERS, uid, uid);
    if (remove(buffer) == -1 && errno != ENOENT)
    {
        fprintf(stderr, "[!]Deleting user(%s) login: %s\n", uid, strerror(errno));
        return NOK;
    }

    //* Delete user password
    sprintf(buffer, "%s/%s/%s_pass.txt", USERS, uid, uid);
    if (remove(buffer) == -1)
    {
        fprintf(stderr, "[!]Deleting user(%s) password: %s\n", uid, strerror(errno));
        return NOK;
    }

    //* Delete user directory
    sprintf(buffer, "%s/%s", USERS, uid);
    if (remove(buffer) == -1)
    {
        fprintf(stderr, "[!]Deleting user(%s) diretory: %s\n", uid, strerror(errno));
        return NOK;
    }

    return OK;
}


int user_entry(const char* uid, const char* pass, bool login)
{
    char buffer[41];
    FILE *f = NULL;

    if (check_pass(uid, pass) == NOK)
    {
        return NOK;
    }

    //* Create login file
    sprintf(buffer, "%s/%s/%s_login.txt", USERS, uid, uid);
    if (login && (f = fopen(buffer, "w+")) == NULL)
    {
        fprintf(stderr, "[!]Creating user(%s) login: %s", uid, strerror(errno));
        return NOK;
    }
    //* Delete login file
    else if (!login && remove(buffer) == -1 && errno != ENOENT)
    {
        fprintf(stderr, "[!]Creating user(%s) logout: %s", uid, strerror(errno));
        return NOK;
    }

    if (f != NULL)
    {
        fclose(f);
    }

    return OK;
}


int user_logged(const char* uid)
{
    char buffer[41];
    FILE* f = NULL;

    sprintf(buffer, "%s/%s/%s_login.txt", USERS, uid, uid);
    if ((f = fopen(buffer, "r")) == NULL )
    {   
        if(errno != ENOENT)
        {
            fprintf(stderr, "[!]Opening user(%s) password file: %s\n", uid, strerror(errno));
        }
        return NOK;
    }

    return OK;
}


//* Groups Management
int group_create(const char* uid, const char* gname)
{
    DIR*  d;
    FILE* f;
    char buffer[34];
    int gnum, status;

    if ((d = opendir(GROUPS)) == NULL)
    {
        fprintf(stderr, "[!]Opening group directory: %s\n", strerror(errno));
        return NOK;
    }

    //* Number of groups
    for (gnum = -1; readdir(d) != NULL; gnum++);
    /* if(errno != 0)
    {
        fprintf(stderr, "[!]Getting number of groups: %s\n", strerror(errno));
        return NOK;
    } */
    closedir(d);

    if(gnum == 99)
    {
        fprintf(stderr, "[!]Group directory full");
        return E_FULL;
    }

    //* Create group directory
    sprintf(buffer, "%s/%02d", GROUPS, gnum);
    if (mkdir(buffer, S_IRWXU) == -1)
    {
        fprintf(stderr, "[!]Creating group directory: %s\n", strerror(errno));
        return NOK;
    }
    
    //* Create message directory
    sprintf(buffer, "%s/%02d/MSG", GROUPS, gnum);
    if (mkdir(buffer, S_IRWXU) == -1)
    {   
        fprintf(stderr, "[!]Creating messages directory: %s\n", strerror(errno));

        //* Delete group directory
        if (remove(buffer) == -1)
        {
            fprintf(stderr, "[!]Deleting group directory: %s\n", strerror(errno));
            return NOK;
        }

        return NOK;
    }

    //* Create group name file
    sprintf(buffer, "%s/%02d/%02d_name.txt", GROUPS, gnum, gnum);
    if ((f = fopen(buffer, "w+")) == NULL)
    {
        fprintf(stderr, "[!]Creating group name: %s\n", strerror(errno));

        //* Delete message directory
        sprintf(buffer, "%s/%02d/MSG", GROUPS, gnum);
        if (remove(buffer) == -1)
        {
            fprintf(stderr, "[!]Deleting message diretory: %s\n", strerror(errno));
            return NOK;
        }

        //* Delete group directory
        sprintf(buffer, "%s/%02d", GROUPS, gnum);
        if (remove(buffer) == -1)
        {
            fprintf(stderr, "[!]Deleting group directory: %s\n", strerror(errno));
            return NOK;
        }

        return NOK;
    }

    //* Write group name
    if (fputs(gname, f) == EOF)
    {
        fclose(f);

        fprintf(stderr, "[!]Saving group name\n");

        //* Delete group name file
        sprintf(buffer, "%s/%02d/%02d_name.txt", GROUPS, gnum, gnum);
        if (remove(buffer) == -1)
        {
            fprintf(stderr, "[!]Deleting group name: %s\n", strerror(errno));
            return NOK;
        }

        //* Delete message directory
        sprintf(buffer, "%s/%02d/MSG", GROUPS, gnum);
        if (remove(buffer) == -1)
        {
            fprintf(stderr, "[!]Deleting message diretory: %s\n", strerror(errno));
            return NOK;
        }

        //* Delete group directory
        sprintf(buffer, "%s/%02d", GROUPS, gnum);
        if (remove(buffer) == -1)
        {
            fprintf(stderr, "[!]Deleting group diretory: %s\n", strerror(errno));
            return NOK;
        }

        return NOK;
    }
    fclose(f);

    //* Add user to created group
    sprintf(buffer, "%02d", gnum);
    if((status = group_add(uid, buffer, gname)) != OK)
    {
        return status;
    }

    return NEW + gnum;
}


int group_add(const char* uid, const char* gid, const char* gname)
{
    DIR* d;
    FILE* f;
    char dir[22];
    char file[34];
    char name[24];

    //* Check valid gid
    sprintf(dir, "%s/%s", GROUPS, gid);
    if ((d = opendir(dir)) == NULL)
    {
        fprintf(stderr, "[!]Invalid gid(%s): %s\n", gid, strerror(errno));
        return E_GRP;
    }
    closedir(d);

    //* Check valid group name
    sprintf(file, "%s/%s/%s_name.txt", GROUPS, gid, gid);
    if ((f = fopen(file, "r")) == NULL)
    {
        fprintf(stderr, "[!]Opening group(%s) name file: %s\n", gid, strerror(errno));
        return NOK;
    }

    if (fgets(name, sizeof(name), f) == NULL)
    {
        fprintf(stderr, "[!]Reading group(%s) name: %s\n", gid, strerror(errno));
        fclose(f);
        return NOK;
    }
    if (strncmp(gname, name, strlen(name)) != 0)
    {
        return E_GNAME;
    }
    fclose(f);

    //* Create user file
    sprintf(file, "%s/%s/%s.txt", GROUPS, gid, uid);
    if ((f = fopen(file, "w+")) == NULL)
    {
        fprintf(stderr, "[!]Adding user(%s) to group(%s): %s\n", uid, gid, strerror(errno));
        return NOK;
    }

    return OK;
}

/* int create_dir(const char* dir)
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


//Files Management
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
} */
