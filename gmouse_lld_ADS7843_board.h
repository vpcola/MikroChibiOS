/*
 * This file is subject to the terms of the GFX License. If a copy of
 * the license was not distributed with this file, you can obtain one at:
 *
 *              http://ugfx.org/license.html
 */

#ifndef _GINPUT_LLD_MOUSE_BOARD_H
#define _GINPUT_LLD_MOUSE_BOARD_H

// Resolution and Accuracy Settings
#define GMOUSE_ADS7843_PEN_CALIBRATE_ERROR		8
#define GMOUSE_ADS7843_PEN_CLICK_ERROR			6
#define GMOUSE_ADS7843_PEN_MOVE_ERROR			4
#define GMOUSE_ADS7843_FINGER_CALIBRATE_ERROR	14
#define GMOUSE_ADS7843_FINGER_CLICK_ERROR		18
#define GMOUSE_ADS7843_FINGER_MOVE_ERROR		14

// How much extra data to allocate at the end of the GMouse structure for the board's use
#define GMOUSE_ADS7843_BOARD_DATA_SIZE			0
#define BOARD_DATA_SIZE                         GMOUSE_ADS7843_BOARD_DATA_SIZE

#define TOUCH_SPI_DRIVER  SPID1
#define TOUCH_CS_PORT     GPIOA
#define TOUCH_CS_PIN      GPIOA_TOUCH_CS
#define TOUCH_IRQ_PORT    GPIOC
#define TOUCH_IRQ_PIN     GPIOC_PIN4

static const SPIConfig touchSpiCfg = {
    0,
    TOUCH_CS_PORT,
    TOUCH_CS_PIN,
    SPI_CR1_BR_1 | SPI_CR1_BR_0,
};

static bool_t init_board(GMouse* m, unsigned driverinstance) {
  if (driverinstance)
      return FALSE;

  spiStart(&TOUCH_SPI_DRIVER, &touchSpiCfg);
  return TRUE;

}

static inline bool_t getpin_pressed(GMouse* m) {
  // Read the IRQ pin
  return (!palReadPad(TOUCH_IRQ_PORT, TOUCH_IRQ_PIN));
}

static inline void aquire_bus(GMouse* m) {
  spiAcquireBus(&TOUCH_SPI_DRIVER);
  spiStart(&TOUCH_SPI_DRIVER, &touchSpiCfg);
  spiSelect(&TOUCH_SPI_DRIVER);
}

static inline void release_bus(GMouse* m) {
  spiUnselect(&TOUCH_SPI_DRIVER);
  spiStop(&TOUCH_SPI_DRIVER);
  spiReleaseBus(&TOUCH_SPI_DRIVER);
}

static inline uint16_t read_value(GMouse* m, uint16_t port) {
  static uint8_t txbuf[3] = {0};
  static uint8_t rxbuf[3] = {0};
  (void)      m;

  txbuf[0] = port;
  spiExchange(&TOUCH_SPI_DRIVER, 3, txbuf, rxbuf);
  return ((uint16_t)rxbuf[1] << 5) | (rxbuf[2] >> 3);
}

#endif /* _GINPUT_LLD_MOUSE_BOARD_H */
