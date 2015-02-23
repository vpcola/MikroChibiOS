/*
 * htu21d.h
 *
 *  Created on: 3 Feb, 2015
 *      Author: Vergil
 */

#ifndef I2CDEVICES_HTU21D_H_
#define I2CDEVICES_HTU21D_H_

#ifdef __cplusplus
extern "C" {
#endif

#define HTU21D_I2C_DRIVER     I2CD1
#define HTU21D_I2C_ADDRESS    0x40
#define TRIGGER_TEMP_MEASURE  0xE3
#define TRIGGER_HUMD_MEASURE  0xE5

// Returns the temperature in degrees celcius
float getTemp(void);
// Returns relative humidity
float getHumidity(void);

#ifdef __cplusplus
}
#endif
#endif /* I2CDEVICES_HTU21D_H_ */
