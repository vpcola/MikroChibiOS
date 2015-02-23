#include "ds1307.h"

#include "ch.hpp"
#include "hal.h"
#include "chprintf.h"

#include <string.h>
#include <stdio.h>

static const char * weekDays[] = { "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };
static const char * monthYear[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

static uint8_t bcd2bin (uint8_t val) { return val - 6 * (val >> 4); }
static uint8_t bin2bcd (uint8_t val) { return val + 6 * (val / 10); }


bool rtcGetTime(DS1307_TIME * time)
{
    // Get data and time from RTC
    uint8_t rxbuf[8];
    uint8_t txbuf[1]; 
    msg_t result;

    if (time == NULL) return false;

    txbuf[0] = 0x0; // Read register 0

    i2cAcquireBus(&DS1307_I2C_DRIVER);
    result = i2cMasterTransmitTimeout(&DS1307_I2C_DRIVER, DS1307_ADDRESS, txbuf, 1, rxbuf, 7, 1000);
    i2cReleaseBus(&DS1307_I2C_DRIVER);

    if (result == RDY_OK)
    {
        time->sec = bcd2bin(rxbuf[0] & 0x7F); // get only the second component.
        time->min = bcd2bin(rxbuf[1]);
        time->hour = bcd2bin(rxbuf[2]);
        time->wday = bcd2bin(rxbuf[3]);
        time->date  = bcd2bin(rxbuf[4]);
        time->month  = bcd2bin(rxbuf[5]);
        time->year  = bcd2bin(rxbuf[6]) + 2000;
        return true;
    } else
        return false;
}

bool rtcSetTime(DS1307_TIME * time)
{
    //uint8_t rxbuf;
    uint8_t txbuf[10];
    msg_t result;

    if (time == NULL) return false;

    txbuf[0]=0x0;   // Write to register 0
    txbuf[1]=bin2bcd(time->sec); 
    txbuf[2]=bin2bcd(time->min);
    txbuf[3]=bin2bcd(time->hour);
    txbuf[4]= bin2bcd(time->wday);
    txbuf[5]=bin2bcd(time->date);
    txbuf[6]=bin2bcd(time->month);
    txbuf[7]=bin2bcd(time->year-2000);
    txbuf[8]=0;

    i2cAcquireBus(&DS1307_I2C_DRIVER);
    result = i2cMasterTransmitTimeout(&DS1307_I2C_DRIVER, DS1307_ADDRESS, txbuf, 9, NULL, 0, 1000);
    i2cReleaseBus(&DS1307_I2C_DRIVER);

    return (result == RDY_OK);
}

const char * toAsciiTime(DS1307_TIME * time)
{
    static char tmpbuffer[100];
    if (time == NULL) return NULL;

    chsnprintf(tmpbuffer, 100, "%s %s %02d, %04d %02d:%02d:%02d",
               weekDays[time->wday - 1],
               monthYear[time->month - 1],
               time->date,
               time->year,
               time->hour,
               time->min,
               time->sec);

    return (const char *) tmpbuffer;
}

const char * now()
{
    DS1307_TIME tm;

    if (rtcGetTime(&tm))
        return toAsciiTime(&tm);

    return NULL;
}

bool rtcStartClock(void)
{
    // Read first byte of reg 0
    uint8_t startStop;
    uint8_t txbuf[2]; 
    msg_t result;


    // Read register 0x0 (first byte, the seconds part contains the flags)
    txbuf[0] = 0x0; 
    i2cAcquireBus(&DS1307_I2C_DRIVER);
    result = i2cMasterTransmitTimeout(&DS1307_I2C_DRIVER, DS1307_ADDRESS, txbuf, 1, &startStop, 1, 1000);
    i2cReleaseBus(&DS1307_I2C_DRIVER);

    if (result != RDY_OK) return false;

    startStop &= 0x7F; // start the clock

    // Write back to register 0x0 (first byte)
    txbuf[0] = 0x0; 
    txbuf[1] = startStop;
    i2cAcquireBus(&DS1307_I2C_DRIVER);
    result = i2cMasterTransmitTimeout(&DS1307_I2C_DRIVER, DS1307_ADDRESS, txbuf, 2, NULL, 0, 1000);
    i2cReleaseBus(&DS1307_I2C_DRIVER);

    return (result == RDY_OK);
}

bool rtcStopClock(void)
{
    // Read first byte of reg 0
    uint8_t startStop;
    uint8_t txbuf[2]; 
    msg_t result;


    // Read register 0x0 (first byte)
    txbuf[0] = 0x0; 
    i2cAcquireBus(&DS1307_I2C_DRIVER);
    result = i2cMasterTransmitTimeout(&DS1307_I2C_DRIVER, DS1307_ADDRESS, txbuf, 1, &startStop, 1, 1000);
    i2cReleaseBus(&DS1307_I2C_DRIVER);

    if (result != RDY_OK) return false;

    startStop |= 0x80;

    // Write back to register 0x0 (first byte)
    txbuf[0] = 0x0; 
    txbuf[1] = startStop;
    i2cAcquireBus(&DS1307_I2C_DRIVER);
    result = i2cMasterTransmitTimeout(&DS1307_I2C_DRIVER, DS1307_ADDRESS, txbuf, 2, NULL, 0, 1000);
    i2cReleaseBus(&DS1307_I2C_DRIVER);

    return (result == RDY_OK);

}

bool rtcSetSQOutput(bool enable,  DS1307_FREQ freq)
{
    uint8_t sqInfo;
    uint8_t txbuf[2]; 
    msg_t result;


    // Read register 0x7 
    txbuf[0] = 0x7; 
    i2cAcquireBus(&DS1307_I2C_DRIVER);
    result = i2cMasterTransmitTimeout(&DS1307_I2C_DRIVER, DS1307_ADDRESS, txbuf, 1, &sqInfo, 1, 1000);
    i2cReleaseBus(&DS1307_I2C_DRIVER);

    if (result != RDY_OK) return false;

    sqInfo = (sqInfo & 0x80) | (enable ? 0x10 : 0) | ((char)freq & 0x03);

    // Write back to register 0x7
    txbuf[0] = 0x7; 
    txbuf[1] = sqInfo;
    i2cAcquireBus(&DS1307_I2C_DRIVER);
    result = i2cMasterTransmitTimeout(&DS1307_I2C_DRIVER, DS1307_ADDRESS, txbuf, 2, NULL, 0, 1000);
    i2cReleaseBus(&DS1307_I2C_DRIVER);

    return (result == RDY_OK);
}

