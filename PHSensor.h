#ifndef PH_SENSOR_H
#define PH_SENSOR_H

#include "stm32f4xx_hal.h"

// Initialize or configure any pH specific variables if needed
void pH_Sensor_Init(void);

// Read current voltage and return calculated pH value
float pH_Sensor_Read(ADC_HandleTypeDef *hadc);

#endif /* PH_SENSOR_H */
