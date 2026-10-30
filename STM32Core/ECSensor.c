#include "ECSensor.h"
#include <stdint.h>

void EC_Sensor_Init(void) { }

float EC_Sensor_Read(ADC_HandleTypeDef *hadc, float temperature)
{
    ADC_ChannelConfTypeDef sConfig = {0};
    uint32_t samples[20];

    sConfig.Channel = ADC_CHANNEL_5;
    sConfig.Rank = 1;
    sConfig.SamplingTime = ADC_SAMPLETIME_56CYCLES;

    if (HAL_ADC_ConfigChannel(hadc, &sConfig) != HAL_OK)
        return -1.0f;

    for (int i = 0; i < 20; i++)
    {
        HAL_ADC_Start(hadc);
        if (HAL_ADC_PollForConversion(hadc, 100) == HAL_OK)
            samples[i] = HAL_ADC_GetValue(hadc);
        else
            samples[i] = 0;
        HAL_ADC_Stop(hadc);
    }

    for (int i = 0; i < 19; i++)
        for (int j = i + 1; j < 20; j++)
            if (samples[i] > samples[j]) {
                uint32_t t = samples[i];
                samples[i] = samples[j];
                samples[j] = t;
            }

    uint32_t sum = 0;
    for (int i = 4; i < 16; i++)
        sum += samples[i];
    float raw_avg = (float)sum / 12.0f;

    float ec_mS_cm = (raw_avg * 0.065f) - 13.93f;
    if (ec_mS_cm < 0.0f) ec_mS_cm = 0.0f;

    float temp = temperature;
    if (temp < -10.0f || temp > 60.0f) temp = 25.0f;
    float temp_factor = 1.0f + 0.0185f * (temp - 25.0f);
    ec_mS_cm /= temp_factor;

    return ec_mS_cm;
}
