#include "direntx.h"
#include <string.h>
#include <stdlib.h>

/**
 * Functions here are not re-entrant!
 * carefull when using multiple threads.
 **/
static FILINFO fnop;

DIR * opendir(const char * path)
{
    DIR * tmpdir = (DIR *) malloc(sizeof(DIR));
    if (tmpdir)
    {
        if (f_opendir(tmpdir, path) == FR_OK)
            return tmpdir;
        else
            free(tmpdir);
    }
    return NULL;
}

dirent * readdir(DIR * dirp)
{
    static dirent dire;

    if ((f_readdir(dirp, &fnop) == FR_OK) &&
        fnop.fname[0] != 0) 
    {
        dire.d_type = fnop.fattrib;

        strncpy(dire.d_name, fnop.fname, _MAX_LFN);

        return &dire;
    }
    return NULL;
}

int closedir(DIR * dirp)
{
    if (dirp)
    {
        f_closedir(dirp);
        return 0;
    }
    return -1;
}

