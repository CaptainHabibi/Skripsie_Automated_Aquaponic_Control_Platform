#include "downlink.h"
#include "main.h"
#include "cmsis_os.h"
#include "FreeRTOS.h"
#include "task.h"
#include "LoRa.h"
#include "Pump.h"

/* ServoHandle is created in freertos.c and wakes the servo task on notify bit 0x01. */
extern osThreadId_t ServoHandle;

void Downlink_ArmRx(void)
{
    LoRa_StartReceive();
}

uint8_t Downlink_Poll(void)
{
    uint8_t cmd_buf[8];
    uint8_t cmd_len = LoRa_ReceivePacket(cmd_buf, sizeof(cmd_buf));
    if (cmd_len == 0) return 0;

    switch (cmd_buf[0])
    {
        case 0xC1:  /* manual feed */
            xTaskNotify((TaskHandle_t)ServoHandle, 0x01, eSetBits);
            break;
        case 0xC2:  /* pump HIGH */
            Pump_Send_Event(PUMP_EVT_OVERRIDE_HIGH);
            break;
        case 0xC3:  /* pump MED */
            Pump_Send_Event(PUMP_EVT_OVERRIDE_MED);
            break;
        case 0xC4:  /* pump LOW */
            Pump_Send_Event(PUMP_EVT_OVERRIDE_LOW);
            break;
        default:
            break;
    }

    return 1;
}
