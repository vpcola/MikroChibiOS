#include "iniutils.h"
#include "ff.h"
#include "shellutils.h"

#include <string.h>
#include <stdlib.h>

#define MAX_LINE_LEN 250
#define MAX_SESSION_LEN 50

int parse_ini(const char * filename, 
    onparseini handler,
    void * usercfg)
{
    FIL fp;
    char * linebuff = NULL, * p, * pend;
    char section[MAX_SESSION_LEN];

    if(f_open(&fp, (const TCHAR *) filename, FA_READ) != FR_OK)
        return -1;

    linebuff = (char *) malloc(MAX_LINE_LEN);
    if (!linebuff) return -1;

    while(f_gets(linebuff, MAX_LINE_LEN, &fp) != NULL)
    {
        p = linebuff;
        p = lskip(rstrip(p));
        if (*p == ';') 
            continue; // skip comments
        // TODO: also trim end of line comments
        p = rtrim(p, ';');
        if (strlen(p) == 0) continue;

        if (*p == '[') // beginning of a section
        {
            // Search the end of the section
            pend = strchr(p+1, ']');
            if (pend)
            {
                // end of section is found, copy section
                *pend = '\0';
                strncpy(section, p+1, MAX_SESSION_LEN);
            }else
            {
                free(linebuff);
                // end of line comments not found!
                f_close(&fp);
                return -1;
            }
        }
        // look for name=value pair
        if ((pend = strstr(p, "=")) != NULL)
        {
            // Split the string
            // and call the handler for the name/value 
            // pair
            *pend = '\0';
            if (handler) handler(usercfg, 
                (const char *) section,
                (const char *) lskip(rstrip(p)),
                (const char *) lskip(rstrip(pend+1)));
        }

    }

    free(linebuff);
    f_close(&fp);

    return 0;
}


