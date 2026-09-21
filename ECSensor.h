#ifndef EC_SENSOR_H
#define EC_SENSOR_H

#include "stm32f4xx_hal.h"

void EC_Sensor_Init(void);
float EC_Sensor_Read(ADC_HandleTypeDef *hadc);

#endif /* EC_SENSOR_H */
