#include "servoMotor.h"
#include "cmsis_os.h"

extern TIM_HandleTypeDef htim3;

/* Calibrated values from bench testing (Arduino write() values -> STM32 pulse widths) */
#define SERVO_STOP_US      1522   /* write(94) -> 1522us : completely still */
#define SERVO_FEED_US      1767   /* write(138) -> 1767us : ~90 deg CW in 150ms */
#define SERVO_BOOT_US      1989   /* write(178) -> 1989us : full speed CW for boot alignment */

#define FEED_ROTATE_MS     150    /* calibrated: 90 deg CW at SERVO_FEED_US */
#define BOOT_ROTATE_MS     300    /* boot alignment: ~180 deg CW */

void Servo_Init(void)
{
    Servo_Set_Pulse(SERVO_STOP_US);
}

/* Set raw pulse width in microseconds (1000..2000) */
void Servo_Set_Pulse(uint32_t pulse_us)
{
    if (pulse_us < 1000) pulse_us = 1000;
    if (pulse_us > 2000) pulse_us = 2000;
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, pulse_us);
}

/* Run the servo at the given pulse for the given duration, then stop */
void Servo_Rotate_For_Pulse(uint32_t pulse_us, uint32_t ms)
{
    Servo_Set_Pulse(pulse_us);
    osDelay(ms);
    Servo_Set_Pulse(SERVO_STOP_US);
}

/* Feed: one 90 degree clockwise rotation */
void Servo_Feed(void)
{
    Servo_Rotate_For_Pulse(SERVO_FEED_US, FEED_ROTATE_MS);
}

/* Boot alignment: rotate the paddle to a known pocket position */
void Servo_BootAlign(void)
{
    Servo_Rotate_For_Pulse(SERVO_BOOT_US, BOOT_ROTATE_MS);
}
