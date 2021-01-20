/**
 * Bosch BMP180 Digital Pressure Sensor
 *
 */

#ifndef BPM180_BMP180WRAPPER_H_
#define BPM180_BMP180WRAPPER_H_

#include <BMP180.h>
#include "mbed.h"
#include "DevI2C.h"

class BMP180Wrapper
{
public:
    BMP180Wrapper(DevI2C *i2c);
    virtual int init(void *init);
    virtual int read_id(uint8_t *id);
    virtual int get_humidity(float *pfData);
    virtual int get_temperature(float *pfData);
    int enable(void);
private:
    BMP180 bmp180;
};

#endif /* BPM180_BMP180WRAPPER_H_ */
