#ifndef PROTOCOL_CONSTANTS_H
#define PROTOCOL_CONSTANTS_H

//. NEW is always last value!
enum status
{
    OK,
    NOK,
    DUP,
    E_USR,
    E_GRP,
    E_GNAME,
    E_FULL,
    ERR,
    NEW
};

char *strstatus(int status);
int istatus(const char *status);

#endif