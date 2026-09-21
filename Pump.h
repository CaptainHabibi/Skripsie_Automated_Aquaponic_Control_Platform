#ifndef PUMP_H
#define PUMP_H

#include "main.h"
#include "cmsis_os.h"

typedef enum {
    PUMP_STATE_NORMAL = 0, // 60%
    PUMP_STATE_MEDIUM,     // 80%
    PUMP_STATE_HIGH        // 100%
} PumpState_t;

typedef enum {
    PUMP_EVT_NONE = 0,
    PUMP_EVT_FISH_FED,      // Transitions to HIGH -> MEDIUM (5m) -> NORMAL (5m)
    PUMP_EVT_OVERRIDE_LOW,  // Forces 60% (reverts to NORMAL after 5m)
    PUMP_EVT_OVERRIDE_MED,  // Forces 80% (reverts to NORMAL after 5m)
    PUMP_EVT_OVERRIDE_HIGH  // Forces 100% (reverts to NORMAL after 5m)
} PumpEvent_t;

void Pump_Init(void);
void Pump_Set_Speed(uint8_t speed_percent);
void Pump_Send_Event(PumpEvent_t evt);

extern volatile uint8_t g_pump_speed_percent;


#endif /* PUMP_H */
