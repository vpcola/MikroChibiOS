#ifndef __NTPUTILS_H__
#define __NTPUTILS_H__

/**
 * Author : Cola Vergil
 * Email  : vpcola@gmail.com
 * Date : Mon Mar 02 2015
 **/

#ifdef __cplusplus
extern "C" {
#endif

// simillar to unix timeval
typedef struct _timeval_x {
        long tv_sec;
        long tv_usec;
} timeval_x;


int sntp_get(const char * hostname, int port, timeval_x *tv);

#ifdef __cplusplus
}
#endif


#endif

