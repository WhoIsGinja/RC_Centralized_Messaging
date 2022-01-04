#ifndef FILE_MANAGER_H
#define FILE_MANAGER_H

//TODO more api calls
int init_fs();

int create_dir(const char* dir);
int delete_dir(const char* dir);

int write_file(const char* file, const char* content);
int delete_file(const char* file);


#endif