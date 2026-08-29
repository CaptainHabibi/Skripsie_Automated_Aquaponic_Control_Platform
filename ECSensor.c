#include "ECSensor.h"

void EC_Sensor_Init(void)
{
    // Initialization routines for EC board if needed
}

float EC_Sensor_Read(ADC_HandleTypeDef *hadc)
{
    ADC_ChannelConfTypeDef sConfig = {0};

    // Configure channel for PA5 (ADC1_IN5) - adjust if using a different pin
    sConfig.Channel = ADC_CHANNEL_5;
    sConfig.Rank = 1;
    sConfig.SamplingTime = ADC_SAMPLETIME_56CYCLES;

    if (HAL_ADC_ConfigChannel(hadc, &sConfig) != HAL_OK)
    {
        return -1.0f;
    }

    HAL_ADC_Start(hadc);

    if (HAL_ADC_PollForConversion(hadc, 100) == HAL_OK)
    {
        uint32_t rawValue = HAL_ADC_GetValue(hadc);
        HAL_ADC_Stop(hadc);

        // Convert raw ADC value to voltage (3.3V Vref, 12-bit ADC)
        float voltage = (float)rawValue * 3.3f / 4095.0f;

        // DFRobot EC conversion baseline (Converts voltage to mS/cm roughly)
        // Note: For absolute lab accuracy, temperature compensation code is typically factored in here.
        float ecValue = voltage * 2.0f; // Adjust multiplier based on your specific DFRobot probe specs

        return ecValue;
    }

    HAL_ADC_Stop(hadc);
    return -1.0f;
}
