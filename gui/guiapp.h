/*
 * guiapp.h
 *
 *  Created on: 1 Feb, 2015
 *      Author: Vergil
 */

#ifndef GUI_GUIAPP_H_
#define GUI_GUIAPP_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "ff.h"
#include "gfx.h"

#define CALIBRATION_FILE "calib.gfx"

void startGUI(void);
bool_t SaveMouseCalibration(unsigned instance, const void *calbuf, size_t sz);
bool_t LoadMouseCalibration(unsigned instance, void *calbuf, size_t size);

#ifdef __cplusplus
}
#endif

#endif /* GUI_GUIAPP_H_ */
