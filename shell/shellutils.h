#ifndef __SHELLUTILS_H__
#define __SHELLUTILS_H__

/**
 * Author : Cola Vergil
 * Email  : tkcov@svsqdcs01.telekurs.com
 * Date : Thu Feb 12 2015
 **/
#include "hal.h"

#ifdef __cplusplus
extern "C" {
#endif


void hexdump(BaseSequentialStream * bss, void *mem, unsigned int len);
char * strip(char * str, char stripchar);
int _getpid(void);
void _exit(int i);
int _kill(int pid, int sig);

#ifdef __cplusplus
}
#endif


#endif

