#include "turbiditySensor.h"
#include "stm32f4xx_hal.h"
// Bring in ADC1 handle from main.c
extern ADC_HandleTypeDef hadc1;

float Read_Turbidity_Voltage(void) {
    uint32_t raw_adc = 0;

    // 1. Start ADC conversion
    HAL_ADC_Start(&hadc1);

    // 2. Poll conversion with 100ms timeout
    if (HAL_ADC_PollForConversion(&hadc1, 100) == HAL_OK) {
        raw_adc = HAL_ADC_GetValue(&hadc1);
    }
    HAL_ADC_Stop(&hadc1);

    // 3. Convert 12-bit raw count (0-4095) to Voltage (0.0V - 3.3V)
    float voltage = (float)raw_adc * 3.3f / 4095.0f;
    return voltage;
}
