#ifndef TEMPSENSOR_H
#define TEMPSENSOR_H

#include "stm32f4xx_hal.h"

// Returns the temperature in Celsius, or a negative error code (-999.0f) if it fails
float Read_Temperature(void);

#endif /* TEMPSENSOR_H */
