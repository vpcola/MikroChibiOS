#ifndef __TIMEUTILS_H__
#define __TIMEUTILS_H__

/**
 * Author : Cola Vergil
 * Email  : vpcola@gmail.com
 * Date : Tue Feb 24 2015
 **/
#ifdef __cplusplus
 extern "C" {
#endif

#include <time.h>
#include "hal.h"

time_t secondsinceepoch();

void cmd_date(BaseSequentialStream * chp, int argc, char *argv[]);

#ifdef __cplusplus
 }
#endif

#endif

