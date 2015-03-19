#ifndef __INIUTILS_H__
#define __INIUTILS_H__

/**
 * Author : Cola Vergil
 * Email  : vpcola@gmail.com
 * Date : Fri Mar 06 2015
 **/
#ifdef __cplusplus
extern "C" {
#endif

typedef int (*onparseini)(void * usrcfg, const char * section, const char *name, const char * value);

int parse_ini(const char * filename, 
    onparseini handler,
    void * usercfg);


#ifdef __cplusplus
}
#endif

#endif

