/*
    ChibiOS/RT - Copyright (C) 2006-2013 Giovanni Di Sirio

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#include "ch.hpp"
#include "hal.h"
#include "chprintf.h"

#include "ff.h"
#include "gfx.h"
#include "guiapp.h"
#include "shellapps.h"
#include "wifichannel.h"
#include "updater.h"

#include <string.h>

using namespace chibios_rt;

/*===========================================================================*/
/* Hardware defines                                                          */
/*===========================================================================*/
/* Define the I2C driver used */
#define I2C_DRIVER  I2CD1
/* I2C LED Blinkers */
#define I2C_CNT_ADDR 0x21

/*===========================================================================*/
/* Hardware configurations                                                   */
/*===========================================================================*/
static const SerialConfig serialcfg = {
    115200, // baud rate
    0,
    0,
    0,
};

static const SerialConfig esp8266cfg = {
    115200, //9600, // baud rate
    0,
    0,
    0,
};

// Configuration for SPI ILI9341
static const SPIConfig spi1cfg = {
  NULL,
  GPIOA,
  GPIOA_SPI1_CS,
  SPI_CR1_BR_1 | SPI_CR1_BR_0
};


/* I2C interface #2 */
static const I2CConfig i2cfg1 = {
    OPMODE_I2C,
    100000,
    FAST_DUTY_CYCLE_2,
};

/**
 * @brief FS object.
 */
FATFS MMC_FS;

/**
 * MMC driver instance.
 */
MMCDriver MMCD1;

/* BR bits and clock speeds (For this setup
 * Core clock is 84 MHz
 *  000  /2   =   42 MHz
 *  001  /4   =   21 MHz
 *  010  /8   =   10.5 MHz
 *  011  /16  =   5.25 MHz
 *  100  /32  =   2.625 MHz
 *  101  /64  =   1.3125 MHz
 *  110  /128 =   656250 kHz
 *  111  /256 =   328125 kHz
 **/

/* Maximum speed SPI configuration (18MHz, CPHA=0, CPOL=0, MSb first).*/
static SPIConfig hs_spicfg = {NULL, GPIOB, GPIOB_SPI2_NSS, SPI_CR1_BR_0 }; // 010 (15 MHz)

/* Low speed SPI configuration (281.250kHz, CPHA=0, CPOL=0, MSb first).*/
static SPIConfig ls_spicfg = {NULL, GPIOB, GPIOB_SPI2_NSS,
                              SPI_CR1_BR_2 | SPI_CR1_BR_1 }; // | SPI_CR1_BR_0 }; // 111 (468 kHz)

/* MMC/SD over SPI driver configuration.*/
static MMCConfig mmccfg = {&SPID2, &ls_spicfg, &hs_spicfg};

/*===========================================================================*/
/* Initialization Routines                                                   */
/*===========================================================================*/

static void initI2CHw(void)
{
    i2cStart(&I2C_DRIVER, &i2cfg1);
}

static void initMMCSpiHw(void)
{
    mmcObjectInit(&MMCD1);
    mmcStart(&MMCD1, &mmccfg);
}

/*===========================================================================*/
/* FatFs related.                                                            */
/*===========================================================================*/

/* FS mounted and ready.*/
bool_t fs_ready = FALSE;

/* Root Path */
char rootpath[50] = "sd";

/**
 * Mount a default filesystem on the root FS
 **/
static int mountFS(void)
{
    if (mmcConnect(&MMCD1))
    {
        chprintf((BaseSequentialStream *)&SD1, "Can not connect to MMC!\r\n");
        return 0;
    }
    // err = f_mount(0, &MMC_FS);
    if (!gfileMount('F', rootpath))
    {
        mmcDisconnect(&MMCD1);
        return 0;
    }
    chprintf((BaseSequentialStream *)&SD1, "Card mounted!\r\n");
    fs_ready = TRUE;
    return 1;
}

/*===========================================================================*/
/* Threads                                                                   */
/*===========================================================================*/

/*
 * Counter thread, times are in milliseconds.
 */
static WORKING_AREA(waThread2, 128);
static msg_t Thread2(void *arg) {

    (void)arg;
    uint8_t data[2], count = 0;
    msg_t status = RDY_OK;

    chRegSetThreadName("counter");

    // First set the direction registers
    data[0] = 0x00; // direction reg
    data[1] = 0x00; // all outputs
    i2cAcquireBus(&I2C_DRIVER);
    status = i2cMasterTransmitTimeout(&I2C_DRIVER, I2C_CNT_ADDR, data, 2, NULL, 0, 1000);
    i2cReleaseBus(&I2C_DRIVER);

    data[0] = 0x14; // output reg
    while (TRUE) {
        data[1] = count;
        i2cAcquireBus(&I2C_DRIVER);
        status = i2cMasterTransmitTimeout(&I2C_DRIVER, I2C_CNT_ADDR, data, 2, NULL, 0, 1000);
        i2cReleaseBus(&I2C_DRIVER);
        chThdSleepMilliseconds(100);
        count++;
    }

    // keep compiler happy
    return status;
}

/*===========================================================================*/
/* User functions                                                            */
/*===========================================================================*/


/*===========================================================================*/
/* Main Function                                                             */
/*===========================================================================*/

/*
 * Application entry point.
 */
int main(void) {

  Thread *shelltp = NULL;

  /*
   * System initializations.
   * - HAL initialization, this also initializes the configured device drivers
   *   and performs the board-specific initializations.
   * - Kernel initialization, the main() function becomes a thread and the
   *   RTOS is active.
   */
  halInit();
  System::init();

  /**
   * Activates the serial driver using the driver default configuration.
   * PB6(TX) and PB7(RX) are routed to USART1.
   **/
  sdStart(&SD1, &serialcfg);
  sdStart(&SD3, &esp8266cfg);


  /**
   * Initialize I2C1
   **/
  chprintf((BaseSequentialStream *)&SD1, "Initializing I2C...\r\n");
  initI2CHw();

  /**
   * Initialize wifi
   **/
#if 0
  chprintf((BaseSequentialStream *)&SD1, "Initializing ESP8266\r\n");
   if (wifiInit(WIFI_MODE_APSTA, &SD3,  &SD1) == WIFI_ERR_NONE)
     wifiConnectAP("MyRepublic-9D18", "026FD991CD");


   /**
    * Start the updater process
    */
   startHumidTempUpdater((BaseSequentialStream *) &SD1);
#endif


  /*
   * Initialize the MMC over SPI hardware
   */
   chprintf((BaseSequentialStream *)&SD1, "Initializing MMC Hardware...\r\n");
   initMMCSpiHw();
   chprintf((BaseSequentialStream *)&SD1, "Mounting filesystem ...\r\n");
   /* Do an initial ls */
   if (mountFS())
    chprintf((BaseSequentialStream *)&SD1, "Filesystem mounted ...\r\n");
   else
    chprintf((BaseSequentialStream *)&SD1, "WARNING: Failed mounting filesystem ...\r\n");



  /*
   * Creates the blinker thread.
   */
  chprintf((BaseSequentialStream *)&SD1, "Starting counter thread...\r\n");
  chThdCreateStatic(waThread2, sizeof(waThread2), NORMALPRIO, Thread2, NULL);

  /**
   * Initialize the display
   **/
  gfxInit();
  startGUI();

  /**
   * Shell manager initialization.
   **/
  chprintf((BaseSequentialStream *)&SD1, "Initializing shell ...\r\n");
  shellInit();

  chprintf((BaseSequentialStream *)&SD1, "Starting shell ...\r\n");

  /*
   * Normal main() thread activity, in this demo it does nothing except
   * sleeping in a loop and listen for events.
   */
  while (TRUE) {
    if (shelltp == NULL)
      shelltp = shellCreate(&shell_cfg, SHELL_WA_SIZE, NORMALPRIO);
    else if (chThdTerminated(shelltp))
    {
      chThdRelease(shelltp);    // Recovers memory of the previous shell.
      shelltp = NULL;           // Triggers spawning of a new shell.
    }
    chThdSleepMilliseconds(100);
  }

  return 0;
}
