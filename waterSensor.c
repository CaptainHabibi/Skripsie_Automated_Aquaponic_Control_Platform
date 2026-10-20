#include "waterSensor.h"

extern TIM_HandleTypeDef htim4;   // dedicated timer, not shared with temp sensor

static void delay_us(uint16_t us) {
    __HAL_TIM_SET_COUNTER(&htim4, 0);
    while ((uint16_t)__HAL_TIM_GET_COUNTER(&htim4) < us);
}

float Read_Water_Level_CM(void) {
    uint32_t local_time = 0;

    HAL_GPIO_WritePin(Trig_GPIO_Port, Trig_Pin, GPIO_PIN_SET);
    delay_us(10);
    HAL_GPIO_WritePin(Trig_GPIO_Port, Trig_Pin, GPIO_PIN_RESET);

    uint32_t timeout = 0;
    while (HAL_GPIO_ReadPin(Echo_GPIO_Port, Echo_Pin) == GPIO_PIN_RESET) {
        timeout++;
        if (timeout > 10000) return -1.0f;
    }

    __HAL_TIM_SET_COUNTER(&htim4, 0);
    while (HAL_GPIO_ReadPin(Echo_GPIO_Port, Echo_Pin) == GPIO_PIN_SET) {
        local_time = __HAL_TIM_GET_COUNTER(&htim4);
        if (local_time > 30000) break;
    }

    float distance_cm = (float)local_time * 0.0034f / 2.0f;
    return distance_cm;
}
