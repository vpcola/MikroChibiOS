#include "shellutils.h"
#include "chprintf.h"
#include "chrtclib.h"
#include <errno.h>
#include <string.h>
#include <ctype.h>

#undef errno
extern int errno;

void hexdump(BaseSequentialStream * bss, void *mem, unsigned int len)
{
    unsigned int i, j;
    if (bss)
    {
        for(i = 0; i < len + ((len % 16) ? (16 - len % 16) : 0); i++)
        {
            /* print offset */
            if(i % 16 == 0)
                chprintf(bss, "0x%06x: ", i);

            /* print hex data */
            if(i < len)
                chprintf(bss,"%02x ", 0xFF & ((char*)mem)[i]);
            else /* end of block, just aligning for ASCII dump */
                chprintf(bss, "   ");

            /* print ASCII dump */
            if(i % 16 == (16 - 1))
            {
                for(j = i - (16 - 1); j <= i; j++)
                {
                    if(j >= len) /* end of block, not really printing */
                        chprintf(bss," ");
                    else if( ((((char*)mem)[j]) > 0x20) && ((((char*)mem)[j]) < 0x7F)) /* printable char */
                        chprintf(bss, "%c", 0xFF & ((char*)mem)[j]);        
                    else /* other char */
                        chprintf(bss, ".");
                }
                chprintf(bss, "\r\n");
            }
        }
        chprintf(bss,"\r\n");
    }
}

// strip trailing stripchar from string
char * rtrim(char * str, char trimchar)
{
  char *pos;
  if ((pos=strrchr(str, trimchar)) != NULL)
      *pos = '\0';

  return str;
}

char * ltrim(char * str, char trimchar)
{
  char *pos;
  if ((pos=strchr(str, trimchar)) != NULL)
      *pos = '\0';

  return str;
}

char* rstrip(char* s)
{
    char* pstr = s + strlen(s);
    while (pstr > s && isspace((unsigned char)(*--pstr)))
        *pstr = '\0';
    return s;
}

char* lskip(const char* s)
{
    while (*s && isspace((unsigned char)(*s)))
        s++;
    return (char*)s;
}



/* libc stub */
int _getpid(void)
{
  return 1;
}
/* libc stub */
void _exit(int i)
{
  (void)i;
}

/* libc stub */
int _kill(int pid, int sig)
{
  (void)pid;
  (void)sig;
  errno = EINVAL;
  return -1;
}

