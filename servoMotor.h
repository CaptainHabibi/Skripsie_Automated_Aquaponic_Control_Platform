#ifndef SERVO_MOTOR_H
#define SERVO_MOTOR_H

#include "main.h"

void Servo_Init(void);
void Servo_Set_Pulse(uint32_t pulse_us);
void Servo_Rotate_For_Pulse(uint32_t pulse_us, uint32_t ms);
void Servo_Feed(void);
void Servo_BootAlign(void);

#endif
