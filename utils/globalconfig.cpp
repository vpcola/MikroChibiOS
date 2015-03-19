#include "globalconfig.h"
#include "dictionary.h"
#include "iniutils.h"

#include <stdlib.h>
#include <string.h>

#define MAX_DICT_NAME 200
static char tmpstr[MAX_DICT_NAME];

static int onconfigparse(void * usercfg, const char * section, const char * name, const char * value)
{
    strncpy(tmpstr, section, MAX_DICT_NAME);
    strcat(tmpstr, "_");
    strcat(tmpstr, name);

    // add the item into the dictionary
    if ( dict_install(tmpstr, value) != NULL)
        return 0;
    else 
        return -1;
}

int parseconfig(const char * filename)
{
   return parse_ini(filename, onconfigparse, NULL);
}

const char * getconfig(const char * section, const char * name)
{
    strncpy(tmpstr, section, MAX_DICT_NAME);
    strcat(tmpstr, "_");
    strcat(tmpstr, name);

    nlist * dta = dict_lookup(tmpstr);
    if (dta)
        return dta->defn;

    return NULL;
}


