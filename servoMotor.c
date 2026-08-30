#include "servoMotor.h"

extern TIM_HandleTypeDef htim3;

void Servo_Init(void) {
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
}

void Servo_Set_Angle(uint8_t angle) {
    if (angle > 180) {
        angle = 180;
    }
    // Maps 0-180 deg to 1000us (0 deg) - 2000us (180 deg)
    uint32_t pulse = 1000 + ((uint32_t)angle * 1000) / 180;
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, pulse);
}
