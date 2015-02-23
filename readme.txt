*****************************************************************************
** ChibiOS/RT port for ARM-Cortex-M4 STM32F415RG (Mikroe Mini-M4)          **
*****************************************************************************

** TARGET **

The demo runs on an Mikroe Mini-M4 board.

** The Demo **

This is the source code for my base board for future IOT projects.

The base board consists of the following modules:
- ILI9341 TFT LCD display (SPI)
- ADS7843 Touch panel driver (SPI)
- External SD card on the SPI bus
- HTU21D Temperature and Humidity sensor on the I2C bus
- ESP8266 wifi chip on USART

The SPI bus is multiplexed between different devices (to save IOs). This 
is accomplished through ChibiOS/RT's acquireBus() and related APIs. Modifications
to the origianal mmc_spi driver has been made to accommodate sharing of SPI bus
among various devices.

Other software/firmware used on this project includes:
- PolarSSL
- jsmn (Jasmine) JSON parser

** Build Procedure **

The demo has been tested by using the free Codesourcery GCC-based toolchain
and YAGARTO. just modify the TRGT line in the makefile in order to use
different GCC toolchains.

** Notes **

Some files used by the demo are not part of ChibiOS/RT but are copyright of
ST Microelectronics and are licensed under a different license.
Also note that not all the files present in the ST library are distributed
with ChibiOS/RT, you can find the whole library on the ST web site:

                             http://www.st.com
