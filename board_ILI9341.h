/*
 * This file is subject to the terms of the GFX License. If a copy of
 * the license was not distributed with this file, you can obtain one at:
 *
 *              http://ugfx.org/license.html
 */

#ifndef _GDISP_LLD_BOARD_H
#define _GDISP_LLD_BOARD_H

#include "hal.h"

#define LCD_SPI_DRIVER  SPID1
#define LCD_PORT    GPIOA
#define LCD_CS      GPIOA_SPI1_CS

#define LCD_LED     GPIOA_PIN0
#define LCD_RES     GPIOA_PIN1
#define LCD_DC      GPIOA_PIN2

#define LCD_DC_CMD  palClearPad(LCD_PORT, LCD_DC)
#define LCD_DC_DATA palSetPad(LCD_PORT, LCD_DC)
#define LCD_SCK_SET palSetPad(LCD_PORT, LCD_SCK)
#define LCD_SCK_RES palClearPad(LCD_PORT, LCD_SCK)
#define LCD_CS_RES  palSetPad(LCD_PORT, LCD_CS)
#define LCD_CS_SET  palClearPad(LCD_PORT, LCD_CS)

/**
 * SPI configuration structure.
 * Speed 12 MHz, CPHA=0, CPOL=0, 8bits frames, MSb transmitted first.
 * Soft slave select.
 */
static const SPIConfig spi2cfg = {
  NULL,
  LCD_PORT,
  LCD_CS,
  (SPI_CR1_MSTR | SPI_CR1_SPE | SPI_CR1_SSM | SPI_CR1_SSI)
};


static inline void init_board(GDisplay *g) {
	(void) g;
	  g->board = 0;

	  palSetPadMode(LCD_PORT, LCD_CS, PAL_MODE_OUTPUT_PUSHPULL);
	  palSetPadMode(LCD_PORT, LCD_DC, PAL_MODE_OUTPUT_PUSHPULL);
	  palSetPadMode(LCD_PORT, LCD_RES, PAL_MODE_OUTPUT_PUSHPULL);

      palSetPad(LCD_PORT, LCD_RES);

	  spiStart(&LCD_SPI_DRIVER, &spi2cfg);
}

static inline void post_init_board(GDisplay *g) {
	(void) g;
}

static inline void setpin_reset(GDisplay *g, bool_t state) {
	(void) g;
	(void) state;

    if (state == TRUE) {
        palClearPad(LCD_PORT, LCD_RES);
    } else {
        palSetPad(LCD_PORT, LCD_RES);
    }
}

static inline void set_backlight(GDisplay *g, uint8_t percent) {
	(void) g;
	(void) percent;

//	if (percent > 0)
//	  palSetPad(LCD_PORT, LCD_LED);
//	else
//	  palClearPad(LCD_PORT, LCD_LED);
}

static inline void acquire_bus(GDisplay *g) {
	(void) g;

	spiAcquireBus(&LCD_SPI_DRIVER);
	spiStart(&LCD_SPI_DRIVER, &spi2cfg);
}

static inline void release_bus(GDisplay *g) {
	(void) g;

    spiStop(&LCD_SPI_DRIVER);
    spiReleaseBus(&LCD_SPI_DRIVER);
}

static inline void write_index(GDisplay *g, uint16_t index) {
	(void) g;
	(void) index;

	palClearPad(LCD_PORT, LCD_DC); // Set command mode
	spiSelect(&LCD_SPI_DRIVER);
	spiSend(&LCD_SPI_DRIVER, 1, (const void *) &index);
	palSetPad(LCD_PORT, LCD_DC);
}

static inline void write_data(GDisplay *g, uint16_t data) {
	(void) g;
	(void) data;

    palSetPad(LCD_PORT, LCD_DC);  // Set data mode
    spiSelect(&LCD_SPI_DRIVER);
    spiSend(&LCD_SPI_DRIVER, 1, (const void *) &data);

}

static inline void setreadmode(GDisplay *g) {
	(void) g;
}

static inline void setwritemode(GDisplay *g) {
	(void) g;
}

static inline uint16_t read_data(GDisplay *g) {
	(void) g;
	return 0;
}

#endif /* _GDISP_LLD_BOARD_H */
