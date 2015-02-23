/*
 * htu21d.cpp
 *
 *  Created on: 3 Feb, 2015
 *      Author: Vergil
 */

#include "htu21d.h"
#include "ch.hpp"
#include "hal.h"

float getTemp()
{
  uint8_t rxbuf[2];
  uint8_t txbuf[1];
  msg_t result;

  // Set the register to read temps
  txbuf[0] = TRIGGER_TEMP_MEASURE;

  // Read 2 bytes of data
  i2cAcquireBus(&HTU21D_I2C_DRIVER);
  result = i2cMasterTransmitTimeout(&HTU21D_I2C_DRIVER, HTU21D_I2C_ADDRESS, txbuf, 1, rxbuf, 2, 1000);
  i2cReleaseBus(&HTU21D_I2C_DRIVER);

  if (result != RDY_OK) return 0.0;

  // Algorithm from datasheet to compute temperature.
  unsigned int rawTemperature = ((unsigned int) rxbuf[0] << 8) | (unsigned int) rxbuf[1];
  rawTemperature &= 0xFFFC;

  float tempTemperature = rawTemperature / (float)65536; //2^16 = 65536
  float realTemperature = -46.85 + (175.72 * tempTemperature); //From page 14

  return realTemperature;
}

float getHumidity(void)
{
  uint8_t rxbuf[2];
  uint8_t txbuf[1];
  msg_t result;

  // Set the register to read temps
  txbuf[0] = TRIGGER_HUMD_MEASURE;

  // Read 2 bytes of data
  i2cAcquireBus(&HTU21D_I2C_DRIVER);
  result = i2cMasterTransmitTimeout(&HTU21D_I2C_DRIVER, HTU21D_I2C_ADDRESS, txbuf, 1, rxbuf, 2, 1000);
  i2cReleaseBus(&HTU21D_I2C_DRIVER);

  if (result != RDY_OK) return 0.0;

  //Algorithm from datasheet.
  unsigned int rawHumidity = ((unsigned int) rxbuf[0] << 8) | (unsigned int) rxbuf[1];

  rawHumidity &= 0xFFFC; //Zero out the status bits but keep them in place

  //Given the raw humidity data, calculate the actual relative humidity
  float tempRH = rawHumidity / (float)65536; //2^16 = 65536
  float rh = -6 + (125 * tempRH); //From page 14

  return rh;
}

