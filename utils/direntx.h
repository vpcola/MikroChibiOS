#ifndef __DIRENTX_H__
#define __DIRENTX_H__

/**
 * Author : Cola Vergil
 * Email  : vpcola@gmail.com
 * Date : Fri Mar 06 2015
 **/

#ifdef __cplusplus
extern "C" {
#endif

#include "ff.h"

/** 
 * The functions contained here is to emulate/translate
 * normal dirent.h calls - opendir, readdir and closedir
 * to fatfs calls.
 **/
typedef struct _dirent {
    int	d_ino;
    int d_off;
    unsigned short d_reclen;
    unsigned char  d_type;
    char d_name[_MAX_LFN];
} dirent;

DIR * opendir(const char * filename);
dirent * readdir(DIR * dirp);
int closedir(DIR * dirp);


#ifdef __cplusplus
}
#endif

#endif

