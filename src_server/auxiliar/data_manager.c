#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include "data_manager.h"
#include "../../protocol_constants.h"

#define BUFFER_SIZE 64
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
int check_pass(const char *uid, const char *pass)
{
    char buffer[BUFFER_SIZE];
    FILE *f = NULL;

    //*Get password
    sprintf(buffer, "%s/%s/%s_pass.txt", USERS, uid, uid);
    if ((f = fopen(buffer, "r")) == NULL)
    {
        fprintf(stderr, "[!]Opening user(%s) password file: %s\n", uid, strerror(errno));
        return NOK;
    }
    if (fgets(buffer, sizeof(buffer), f) == NULL)
    {
        fprintf(stderr, "[!]Reading user(%s) password\n", uid);
        fclose(f);
        return NOK;
    }
    fclose(f);

    //* Compare password
    if (strcmp(pass, buffer) != 0)
    {
        return NOK;
    }

    return OK;
}

int user_create(const char *uid, const char *pass)
{
    char buffer[BUFFER_SIZE];
    FILE *f = NULL;

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

int user_delete(const char *uid, const char *pass)
{
    char buffer[BUFFER_SIZE];

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

int user_entry(const char *uid, const char *pass, bool login)
{
    char buffer[BUFFER_SIZE];
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

int user_logged(const char *uid)
{
    char buffer[BUFFER_SIZE];

    sprintf(buffer, "%s/%s/%s_login.txt", USERS, uid, uid);
    if (access(buffer, F_OK) != 0)
    {
        if (errno != ENOENT)
        {
            fprintf(stderr, "[!]Opening user(%s) password file: %s\n", uid, strerror(errno));
        }
        return NOK;
    }

    return OK;
}

//* Groups Management
int group_create(const char *uid, const char *gname)
{
    DIR *d;
    FILE *f;
    char buffer[BUFFER_SIZE];
    int gnum, status;

    if ((d = opendir(GROUPS)) == NULL)
    {
        fprintf(stderr, "[!]Opening group directory: %s\n", strerror(errno));
        return NOK;
    }

    //* Number of groups
    for (gnum = -1; readdir(d) != NULL; gnum++)
        ;

    closedir(d);

    if (gnum >= 99)
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
    if ((status = group_add(uid, buffer, gname)) != OK)
    {
        return status;
    }

    return NEW + gnum;
}

int group_add(const char *uid, const char *gid, const char *gname)
{
    FILE *f;
    char buffer[BUFFER_SIZE];
    char name[32];

    //* Check valid gid
    sprintf(buffer, "%s/%s", GROUPS, gid);
    if (access(buffer, F_OK) != 0)
    {
        return E_GRP;
    }

    //* Check valid group name
    sprintf(buffer, "%s/%s/%s_name.txt", GROUPS, gid, gid);
    if ((f = fopen(buffer, "r")) == NULL)
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
    fclose(f);

    if (strcmp(gname, name) != 0)
    {
        return E_GNAME;
    }

    //* Create user file
    sprintf(buffer, "%s/%s/%s.txt", GROUPS, gid, uid);
    if ((f = fopen(buffer, "w+")) == NULL)
    {
        fprintf(stderr, "[!]Adding user(%s) to group(%s): %s\n", uid, gid, strerror(errno));
        return NOK;
    }
    fclose(f);

    return OK;
}

int group_remove(const char* uid, const char* gid)
{
    char buffer[BUFFER_SIZE];

    //* Check valid gid
    sprintf(buffer, "%s/%s", GROUPS, gid);
    if (access(buffer, F_OK) != 0)
    {
        return E_GRP;
    }

    //* Delete user
    sprintf(buffer, "%s/%s/%s.txt", GROUPS, gid, uid);
    if (remove(buffer) == -1 && errno != ENOENT)
    {   
        fprintf(stderr, "[!]Deleting user(%s) from group(%s): %s\n", uid, gid, strerror(errno));
        return NOK;
    }

    return OK;
}

int groups_get(char **glist, const char *uid)
{
    struct dirent **groups;
    struct dirent **msgs;
    FILE* f;
    int i, gnum, mnum, gcount;
    char buffer[BUFFER_SIZE];
    char gname[25];
    
    if ((gnum = scandir(GROUPS, &groups, NULL, alphasort)) == -1)
    {
        fprintf(stderr, "[!]Getting groups: %s\n", strerror(errno));
        return NOK;
    }

    //* Take into account . and ..
    if (gnum - 2 == 0)
    {
        *glist = NULL;
        free(groups);
        return OK;
    }

    //* Allocate memory for list
    if ((*glist = (char*) calloc(4096, sizeof(char))) == NULL)
    {
        fprintf(stderr, "[!]Allocating memory for response\n");
        free(groups);
        return NOK;
    }

    gcount = 0;
    //* Number of groups
    for (i = 2; i < gnum; i++)
    {
        if (uid != NULL)
        {
            sprintf(buffer, "%s/%s/%s.txt", GROUPS, groups[i]->d_name, uid);
            if (access(buffer, F_OK) != 0)
            {
                if (errno != ENOENT)
                {
                    fprintf(stderr, "[!]Opening user(%s) password file: %s\n", uid, strerror(errno));
                    free(groups);
                    free(*glist);
                    return NOK;
                }
                
                continue;
            }
        }
        gcount++;
    }

    if(uid == NULL)
    {
        sprintf(buffer, "RGL %d", gcount);
        strcat(*glist, buffer);
    }
    else
    {
        sprintf(buffer, "RGM %d", gcount);
        strcat(*glist, buffer);
    }

    //* Build list
    for (i = 2; i < gnum; i++)
    {   
        //* If it's a user group
        if (uid != NULL)
        {
            sprintf(buffer, "%s/%s/%s.txt", GROUPS, groups[i]->d_name, uid);
            if (access(buffer, F_OK) != 0)
            {
                if (errno != ENOENT)
                {
                    fprintf(stderr, "[!]Opening user(%s) password file: %s\n", uid, strerror(errno));
                    free(groups);
                    free(*glist);
                    return NOK;
                }
                
                continue;
            }
        }

        //*Get group name
        sprintf(buffer, "%s/%s/%s_name.txt", GROUPS, groups[i]->d_name, groups[i]->d_name);
        if ((f = fopen(buffer, "r")) == NULL)
        {
            fprintf(stderr, "[!]Opening group name file: %s\n", strerror(errno));
            free(groups);
            free(*glist);
            return NOK;
        }

        if (fgets(gname, sizeof(gname), f) == NULL)
        {
            fprintf(stderr, "[!]Reading group name\n");
            free(groups);
            free(*glist);
            fclose(f);
            return NOK;
        }
        fclose(f);

        //*Get last MID
        sprintf(buffer, "%s/%s/MSG", GROUPS, groups[i]->d_name);
        if ((mnum = scandir(buffer, &msgs, NULL, alphasort)) == -1)
        {
            fprintf(stderr, "[!]Getting groups: %s\n", strerror(errno));
            free(groups);
            free(*glist);
            return NOK;
        }

        if (mnum - 2 == 0)
        {
            sprintf(buffer, " %s %s 0000", groups[i]->d_name, gname);
        }
        else
        {
            sprintf(buffer, " %s %s %s", groups[i]->d_name, gname, msgs[mnum - 1]->d_name);
        }
        free(msgs);

        strcat(*glist, buffer);
    } 
    free(groups);

    strcat(*glist, "\n");

    return OK;
}


