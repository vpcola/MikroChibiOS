/*
 * updater.h
 *
 *  Created on: 22 Feb, 2015
 *      Author: Vergil
 */

#ifndef NETAPPS_UPDATER_H_
#define NETAPPS_UPDATER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "hal.h"

void startHumidTempUpdater(BaseSequentialStream * dbgstrm);

#ifdef __cplusplus
}
#endif


#endif /* NETAPPS_UPDATER_H_ */
