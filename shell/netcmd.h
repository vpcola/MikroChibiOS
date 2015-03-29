#ifndef __NETCMD_H__
#define __NETCMD_H__

/**
 * Author : Cola Vergil
 * Email  : vpcola@gmail.com
 * Date : Fri Feb 13 2015
 **/

#ifdef __cplusplus
extern "C" {
#endif

#include "ch.h"
#include "hal.h"

void cmd_wget(BaseSequentialStream *chp, int argc, char *argv[]);
void cmd_weather(BaseSequentialStream *chp, int argc, char *argv[]);
void cmd_ipstat(BaseSequentialStream *chp, int argc, char *argv[]);
void cmd_ntpdate(BaseSequentialStream * chp, int argc, char * argv[]);
void cmd_nettest(BaseSequentialStream * chp, int argc, char * argv[]);

#ifdef __cplusplus
}
#endif

#endif

