#include "servoMotor.h"

extern TIM_HandleTypeDef htim3;

void Servo_Init(void) {
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
}

void Servo_Set_Angle(uint8_t angle) {
    if (angle > 180) {
        angle = 180;
    }

    // 16MHz clock with PSC=16-1 and ARR=20000-1 gives 1us per tick.
    // Maps 0-180 degrees to 1000us (1ms) - 2000us (2ms) pulses.
    uint32_t pulse = 1000 + ((uint32_t)angle * 1000) / 180;

    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, pulse);
}
