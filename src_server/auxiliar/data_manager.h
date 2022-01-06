#ifndef DATA_MANAGER_H
#define DATA_MANAGER_H

#include <stdbool.h>

//TODO more api calls
void init_server_data();

//* User Management
int user_create(const char* uid, const char* pass);
int user_delete(const char* uid, const char* pass);
int user_entry(const char* uid, const char* pass, bool login);

//*Groups Management

#endif