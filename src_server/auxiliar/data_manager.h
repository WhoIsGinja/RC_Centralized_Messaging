#ifndef DATA_MANAGER_H
#define DATA_MANAGER_H

#include <stdbool.h>

//TODO more api calls
void init_server_data();

//* User Management
int user_create(const char* uid, const char* pass);
int user_delete(const char* uid, const char* pass);
int user_entry(const char* uid, const char* pass, bool login);
int user_logged(const char* uid);

//*Groups Management
int group_create(const char* uid, const char* gname);
int group_add(const char* uid, const char* gid, const char* gname);
int group_remove(const char* uid, const char* gid);
int groups_get(char** glist, const char* uid);
int group_msg_add(const char* uid, const char* gid, const char *text);
int group_msg_remove(const char* gid, const char* mid);
int group_msg_add_file(const char* gid, const char* mid, const char* filename, char* pathname);
#endif