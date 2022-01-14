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

#define BUFFER_64B 64
#define BUFFER_1KB 1024
#define USERS "data_server/USERS"
#define GROUPS "data_server/GROUPS"

//* Uid to filter group
char f_uid[6];
//* Gid to filter users
char f_gid[3];
//* Mid to filter msg
int f_mid;

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
    char buffer[BUFFER_64B];
    FILE *f = NULL;

    //*Get password
    sprintf(buffer, "%s/%s/%s_pass.txt", USERS, uid, uid);
    if ((f = fopen(buffer, "r")) == NULL)
    {
        if (errno != ENOENT)
        {
            fprintf(stderr, "[!]Opening user(%s) password file: %s\n", uid, strerror(errno));
        }
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
    char buffer[BUFFER_64B];
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
    char buffer[BUFFER_64B];

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
    char buffer[BUFFER_64B];
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
    char buffer[BUFFER_64B];

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
    char buffer[BUFFER_64B];
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
    char buffer[BUFFER_64B];
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
    if ((f = fopen(buffer, "w+")) == NULL && errno != EEXIST)
    {
        fprintf(stderr, "[!]Adding user(%s) to group(%s): %s\n", uid, gid, strerror(errno));
        return NOK;
    }
    fclose(f);

    return OK;
}

int group_remove(const char *uid, const char *gid)
{
    char buffer[BUFFER_64B];

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

//* Filters groups
int filter_groups(const struct dirent *entry)
{
    char buffer[BUFFER_1KB];
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
    {
        return 0;
    }

    if (f_uid[0] != 0)
    {
        sprintf(buffer, "%s/%s/%s.txt", GROUPS, entry->d_name, f_uid);
        return (access(buffer, F_OK) == 0);
    }

    return 1;
}

int groups_get(char **glist, const char *uid)
{
    struct dirent **groups;
    struct dirent **msgs;
    FILE *f;
    int i, gnum, mnum;
    char buffer[BUFFER_1KB];
    char gname[25];

    //* Filter for groups
    if (uid != NULL)
    {
        stpcpy(f_uid, uid);
    }
    else
    {
        memset(f_uid, 0, sizeof(f_uid));
    }

    //* Get groups
    if ((gnum = scandir(GROUPS, &groups, filter_groups, alphasort)) == -1)
    {
        fprintf(stderr, "[!]Getting groups: %s\n", strerror(errno));
        *glist = NULL;
        return NOK;
    }

    //* Take into account . and ..
    if (gnum == 0)
    {
        *glist = NULL;
        free(groups);
        return OK;
    }

    //* Allocate memory for list
    if ((*glist = (char *)calloc(BUFFER_1KB * 4, sizeof(char))) == NULL)
    {
        free(groups);
        fprintf(stderr, "[!]Allocating memory for response\n");
        return NOK;
    }

    sprintf(buffer, "%d", gnum);
    strcat(*glist, buffer);

    //* Build list
    for (i = 0; i < gnum; i++)
    {
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

    return OK;
}

//* Filter users
int filter_users(const struct dirent *entry)
{
    char buffer[BUFFER_64B];
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0 || strcmp(entry->d_name, "MSG") == 0)
    {
        return 0;
    }

    sprintf(buffer, "%s_name.txt", f_gid);

    return (strcmp(entry->d_name, buffer) != 0);
}

int group_users(char **ulist, const char *gid)
{
    struct dirent **users;
    FILE *f;
    char buffer[BUFFER_1KB];
    int i, unum;

    strcpy(f_gid, gid);

    //* Check valid gid
    sprintf(buffer, "%s/%s", GROUPS, gid);
    if (access(buffer, F_OK) != 0)
    {
        return NOK;
    }

    //* Get users
    if ((unum = scandir(buffer, &users, filter_users, alphasort)) == -1)
    {
        fprintf(stderr, "[!]Getting users: %s\n", strerror(errno));
        *ulist = NULL;
        return NOK;
    }

    //* Allocate memory for list
    if ((*ulist = (char *)calloc(32 + unum * 6, sizeof(char))) == NULL)
    {
        free(users);
        fprintf(stderr, "[!]Allocating memory for response\n");
        free(users);
        return NOK;
    }

    //* Get group name
    sprintf(buffer, "%s/%s/%s_name.txt", GROUPS, gid, gid);
    if ((f = fopen(buffer, "r")) == NULL)
    {
        fprintf(stderr, "[!]Opening group(%s) name file: %s\n", gid, strerror(errno));
        free(*ulist);
        free(users);
        return NOK;
    }
    if (fgets(*ulist, sizeof(buffer), f) == NULL)
    {
        fprintf(stderr, "[!]Reading group(%s) name\n", gid);
        free(*ulist);
        free(users);
        fclose(f);
        return NOK;
    }
    fclose(f);

    //* Build list
    for (i = 0; i < unum; i++)
    {
        strcat(*ulist, " ");
        strncat(*ulist, users[i]->d_name, 5);
    }

    return OK;
}

int group_msg_add(const char *uid, const char *gid, const char *text, char *mid)
{
    DIR *d;
    FILE *f;
    char buffer[BUFFER_64B];
    int mnum;

    //* Check valid gid
    sprintf(buffer, "%s/%s", GROUPS, gid);
    if (access(buffer, F_OK) != 0)
    {
        return NOK;
    }

    //* Check valid uid
    sprintf(buffer, "%s/%s/%s.txt", GROUPS, gid, uid);
    if (access(buffer, F_OK) != 0)
    {
        return NOK;
    }

    sprintf(buffer, "%s/%s/MSG", GROUPS, gid);
    if ((d = opendir(buffer)) == NULL)
    {
        fprintf(stderr, "[!]Opening message directory of (%s): %s\n", gid, strerror(errno));
        return NOK;
    }

    //* Number of messages
    for (mnum = -1; readdir(d) != NULL; mnum++)
        ;

    closedir(d);

    if (mnum >= 9999)
    {
        fprintf(stderr, "[!]Message directory of %s full\n", gid);
        return NOK;
    }

    //* Create message directory
    sprintf(buffer, "%s/%s/MSG/%04d", GROUPS, gid, mnum);
    if (mkdir(buffer, S_IRWXU) == -1)
    {
        fprintf(stderr, "[!]Creating message for %s: %s\n", gid, strerror(errno));
        return NOK;
    }

    //* Create message author UID file
    sprintf(buffer, "%s/%s/MSG/%04d/A U T H O R.txt", GROUPS, gid, mnum);
    if ((f = fopen(buffer, "w+")) == NULL)
    {
        fprintf(stderr, "[!]Creating author file for %s(%04d): %s\n", gid, mnum, strerror(errno));

        sprintf(buffer, "%s/%s/MSG/%04d", GROUPS, gid, mnum);
        if (remove(buffer) == -1)
        {
            fprintf(stderr, "[!]Deleting %04d directory of %s: %s\n", mnum, gid, strerror(errno));
            return NOK;
        }

        return NOK;
    }

    //* Write author UID
    if (fputs(uid, f) == EOF)
    {
        fclose(f);

        fprintf(stderr, "[!]Saving author UID(%s)\n", uid);

        sprintf(buffer, "%s/%s/MSG/%04d/A U T H O R.txt", GROUPS, gid, mnum);
        if (remove(buffer) == -1)
        {
            fprintf(stderr, "[!]Deleting author file of %s(%04d): %s\n", gid, mnum, strerror(errno));

            return NOK;
        }

        sprintf(buffer, "%s/%s/MSG/%04d", GROUPS, gid, mnum);
        if (remove(buffer) == -1)
        {
            fprintf(stderr, "[!]Deleting %04d directory of %s: %s\n", mnum, gid, strerror(errno));

            return NOK;
        }

        return NOK;
    }
    fclose(f);

    //* Create message author UID file
    sprintf(buffer, "%s/%s/MSG/%04d/T E X T.txt", GROUPS, gid, mnum);
    if ((f = fopen(buffer, "w+")) == NULL)
    {
        fprintf(stderr, "[!]Creating text file for %s(%04d): %s\n", gid, mnum, strerror(errno));

        sprintf(buffer, "%s/%s/MSG/%04d", GROUPS, gid, mnum);
        if (remove(buffer) == -1)
        {
            fprintf(stderr, "[!]Deleting %04d directory of %s: %s\n", mnum, gid, strerror(errno));
            return NOK;
        }

        return NOK;
    }

    //* Write text
    if (fputs(text, f) == EOF)
    {
        fclose(f);

        fprintf(stderr, "[!]Saving text for %s(%04d)\n", gid, mnum);

        sprintf(buffer, "%s/%s/MSG/%04d/T E X T.txt", GROUPS, gid, mnum);
        if (remove(buffer) == -1)
        {
            fprintf(stderr, "[!]Deleting text file of %s(%04d): %s\n", gid, mnum, strerror(errno));
            return NOK;
        }

        sprintf(buffer, "%s/%s/MSG/%04d", GROUPS, gid, mnum);
        if (remove(buffer) == -1)
        {
            fprintf(stderr, "[!]Deleting %04d directory of %s: %s\n", mnum, gid, strerror(errno));
            return NOK;
        }

        return NOK;
    }
    fclose(f);

    sprintf(mid, "%04d", mnum);

    return OK;
}

int group_msg_remove(const char *gid, const char *mid)
{
    char buffer[BUFFER_64B];

    sprintf(buffer, "%s/%s/MSG/%s/A U T H O R.txt", GROUPS, gid, mid);
    if (remove(buffer) == -1)
    {
        fprintf(stderr, "[!]Deleting author file of %s(%s): %s\n", gid, mid, strerror(errno));
        return NOK;
    }

    sprintf(buffer, "%s/%s/MSG/%s/T E X T.txt", GROUPS, gid, mid);
    if (remove(buffer) == -1)
    {
        fprintf(stderr, "[!]Deleting text file %s(%s): %s\n", gid, mid, strerror(errno));
        return NOK;
    }

    sprintf(buffer, "%s/%s/MSG/%s", GROUPS, gid, mid);
    if (remove(buffer) == -1)
    {
        fprintf(stderr, "[!]Deleting message(%s) of %s: %s\n", mid, gid, strerror(errno));
        return NOK;
    }

    return OK;
}

int group_msg_file(const char *gid, const char *mid, const char *filename, char *pathname)
{
    char buffer[BUFFER_64B];
    FILE *f;

    //* Create message author UID file
    sprintf(buffer, "%s/%s/MSG/%s/%s", GROUPS, gid, mid, filename);
    if ((f = fopen(buffer, "w+")) == NULL)
    {
        fprintf(stderr, "[!]Creating file for %s(%s): %s\n", gid, mid, strerror(errno));
        return NOK;
    }
    fclose(f);

    strcpy(pathname, buffer);

    return OK;
}

//* Filter msg
int filter_msgs(const struct dirent *entry)
{
    int mid;
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
    {
        return 0;
    }

    mid = atoi(entry->d_name);

    if (mid >= f_mid && mid < f_mid + 20)
    {
        return 1;
    }

    return 0;
}

int group_msgs_get(const char *uid, const char *gid, const char *mid, char *pathname, struct dirent ***mids, int *nmsg)
{
    char buffer[BUFFER_1KB];

    //* Check valid gid
    sprintf(buffer, "%s/%s", GROUPS, gid);
    if (access(buffer, F_OK) != 0)
    {
        return NOK;
    }

    f_mid = atoi(mid);

    //* Get mids
    sprintf(buffer, "%s/%s/MSG", GROUPS, gid);
    strcpy(pathname, buffer);
    if ((*nmsg = scandir(buffer, mids, filter_msgs, alphasort)) == -1)
    {
        fprintf(stderr, "[!]Getting messages: %s\n", strerror(errno));
        return NOK;
    }

    return OK;
}