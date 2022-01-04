#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <stdlib.h>
#include "file_manager.h"
#include "../../protocol_constants.h"

void init_fs()
{   
    if(mkdir("server_data", S_IRWXU) == -1 && errno != EEXIST)
    {
        fprintf(stderr, "Error: %s\n", strerror(errno));
        exit(1);
    }

    if(mkdir("server_data/USERS", S_IRWXU) == -1 && errno != EEXIST)
    {
        fprintf(stderr, "Error: %s\n", strerror(errno));
        exit(1);
    }

    if(mkdir("server_data/GROUPS", S_IRWXU) == -1 && errno != EEXIST)
    {
        fprintf(stderr, "Error: %s\n", strerror(errno));
        exit(1);
    }
}



//*Directory Management

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
