#include "Pump.h"

extern TIM_HandleTypeDef htim4;
extern osMessageQueueId_t PumpCommandQueueHandle;
#define PUMP_TIMER_ARR  9332
void Pump_Init(void)
{
    HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_4);
    Pump_Set_Speed(60); // Default to normal state (60%)
}

void Pump_Set_Speed(uint8_t speed_percent)
{
	if (speed_percent == 0)
	    {
	        // 100% HIGH on PB9 -> BJT ON -> Gates 0V -> Pump OFF
	        __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_4, PUMP_TIMER_ARR);
	        return;
	    }

    if (speed_percent < 60)  speed_percent = 60;
    if (speed_percent > 100) speed_percent = 100;

    // Inverted logic:
        // 100% speed -> CCR = 0 (0V on PB9 -> Gate = 5V -> Pump = 5V)
        //  60% speed -> CCR = 40% of ARR
        uint32_t pulse = PUMP_TIMER_ARR - (((uint32_t)speed_percent * PUMP_TIMER_ARR) / 100);
    __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_4, pulse);
}

void Pump_Send_Event(PumpEvent_t evt)
{
    if (PumpCommandQueueHandle != NULL)
    {
        osMessageQueuePut(PumpCommandQueueHandle, &evt, 0, 0);
    }
}
