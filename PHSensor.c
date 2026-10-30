#include "PHSensor.h"

void pH_Sensor_Init(void)
{
    // Placeholder for any calibration or startup routines
}

float pH_Sensor_Read(ADC_HandleTypeDef *hadc)
{
    ADC_ChannelConfTypeDef sConfig = {0};

    // Configure channel for PA4 (ADC1_IN4)
    sConfig.Channel = ADC_CHANNEL_4;
    sConfig.Rank = 1;
    sConfig.SamplingTime = ADC_SAMPLETIME_56CYCLES;

    if (HAL_ADC_ConfigChannel(hadc, &sConfig) != HAL_OK)
    {
        return -1.0f; // Error indicator
    }

    HAL_ADC_Start(hadc);

    if (HAL_ADC_PollForConversion(hadc, 100) == HAL_OK)
    {
        // Pass the ADC handle pointer into HAL_ADC_GetValue
        uint32_t rawValue = HAL_ADC_GetValue(hadc);
        HAL_ADC_Stop(hadc);

        // Convert raw ADC value (12-bit, 3.3V Vref) to voltage
        float voltage = (float)rawValue * 3.3f / 4095.0f;

        // DFRobot V2 linear conversion formula: pH = 3.5 * Voltage
        float phValue = 3.5f * voltage;

        return phValue;
    }

    HAL_ADC_Stop(hadc);
    return -1.0f; // Timeout or error
}
