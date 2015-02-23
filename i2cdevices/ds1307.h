#ifndef __DS1307_H__
#define __DS1307_H__

/**
 * Author : Cola Vergil
 * Email  : tkcov@svsqdcs01.telekurs.com
 * Date : Tue Feb 03 2015
 **/

#ifdef __cplusplus
extern "C" {
#endif

#define DS1307_ADDRESS 0x68
#define DS1307_I2C_DRIVER     I2CD1     // Driver must be fully initialized

typedef struct {
	int sec;        /*!< seconds [0..59] */
	int min;        /*!< minutes {0..59] */
	int hour;       /*!< hours [0..23] */
	int wday;       /*!< weekday [1..7, where 1 = sunday, 2 = monday, ... */
	int date;       /*!< day of month [0..31] */
	int month;        /*!< month of year [1..12] */
	int year;       /*!< year [2000..2255] */
} DS1307_TIME;

typedef enum {
    RS1Hz = 0,
    RS4kHz = 1,
    RS8kHz = 2,
    RS32kHz = 3
} DS1307_FREQ;

bool rtcGetTime(DS1307_TIME * time);
bool rtcSetTime(DS1307_TIME * time);
const char * toAsciiTime(DS1307_TIME * time);
const char * now();

bool rtcStartClock(void);
bool rtcStopClock(void);

bool rtcSetSQOutput(bool enable,  DS1307_FREQ freq);


#ifdef __cplusplus
}
#endif

#endif

