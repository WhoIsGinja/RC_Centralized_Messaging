#include <string.h>
#include "protocol_constants.h"

char *strstatus(int status)
{
    switch (status)
    {
    case OK:
        return "OK";
        break;

    case NOK:
        return "NOK";
        break;

    case DUP:
        return "DUP";
        break;

    case E_USR:
        return "E_USR";
        break;

    case E_GRP:
        return "E_GRP";
        break;

    case E_GNAME:
        return "E_GNAME";
        break;

    case E_FULL:
        return "E_FULL";
        break;

    case ERR:
        return "ERR";
        break;

    case NEW:
        return "NEW";
        break;
    }

    return NULL;
}

int istatus(const char *status)
{
    if (strcmp(status, "OK") == 0)
    {
        return OK;
    }
    else if (strcmp(status, "NOK") == 0)
    {
        return NOK;
    }
    else if (strcmp(status, "DUP") == 0)
    {
        return DUP;
    }
    else if (strcmp(status, "E_USR") == 0)
    {
        return E_USR;
    }
    else if (strcmp(status, "E_GRP") == 0)
    {
        return E_GRP;
    }
    else if (strcmp(status, "E_GNAME") == 0)
    {
        return E_GNAME;
    }
    else if (strcmp(status, "E_FULL") == 0)
    {
        return E_FULL;
    }
    else if (strcmp(status, "ERR") == 0)
    {
        return ERR;
    }
    else if (strcmp(status, "NEW") == 0)
    {
        return NEW;
    }

    return -1;
}