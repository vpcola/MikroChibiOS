#ifndef __SHELL_H__
#define __SHELL_H__

/**
 * Author : Cola Vergil
 * Email  : tkcov@svsqdcs01.telekurs.com
 * Date : Tue Feb 03 2015
 **/

#ifdef __cplusplus
extern "C" {
#endif

#include "shell.h"



#define SHELL_WA_SIZE   THD_WA_SIZE(1024)
#define TEST_WA_SIZE    THD_WA_SIZE(256)


extern const ShellConfig shell_cfg; 

#ifdef __cplusplus
}
#endif

#endif

