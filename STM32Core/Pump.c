#include "Pump.h"

extern TIM_HandleTypeDef htim4;
extern osMessageQueueId_t PumpCommandQueueHandle;
#define PUMP_TIMER_ARR  9332
void Pump_Init(void)
{
    //HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_4);
    Pump_Set_Speed(66); // Default to normal state (66%)
}

volatile uint8_t g_pump_speed_percent = 66;

void Pump_Set_Speed(uint8_t speed_percent)
{
    if (speed_percent == 0) {
        __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_4, PUMP_TIMER_ARR);
        g_pump_speed_percent = 0;
        return;
    }
    if (speed_percent < 66)  speed_percent = 66;
    if (speed_percent > 100) speed_percent = 100;
    uint32_t pulse = PUMP_TIMER_ARR - (((uint32_t)speed_percent * PUMP_TIMER_ARR) / 100);
    __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_4, pulse);
    g_pump_speed_percent = speed_percent;
}

void Pump_Send_Event(PumpEvent_t evt)
{
    if (PumpCommandQueueHandle != NULL)
    {
        osMessageQueuePut(PumpCommandQueueHandle, &evt, 0, 0);
    }
}


