#include "turbiditySensor.h"
#include "stm32f4xx_hal.h"

extern ADC_HandleTypeDef hadc1;

float Read_Turbidity_Voltage(void)
{
    uint32_t raw_adc = 0;

    HAL_ADC_Start(&hadc1);
    if (HAL_ADC_PollForConversion(&hadc1, 100) == HAL_OK) {
        raw_adc = HAL_ADC_GetValue(&hadc1);
    }
    HAL_ADC_Stop(&hadc1);

    // ADC voltage at the STM32 pin (0-3.3V)
    float adc_voltage = (float)raw_adc * 3.3f / 4095.0f;

    // Undo the 12k / (12k + 33.3k) divider
    // actual sensor output = adc_voltage / (33.3 / 45.3) = adc_voltage / 0.735
    float sensor_voltage = adc_voltage / 0.735f;

    return sensor_voltage;
}
