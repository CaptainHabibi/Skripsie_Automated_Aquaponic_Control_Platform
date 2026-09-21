#ifndef SERVO_MOTOR_H
#define SERVO_MOTOR_H

#include "main.h"

// Public function prototypes to control the fish feeder servo angle (0 to 180 degrees)
void Servo_Init(void);
void Servo_Set_Angle(uint8_t angle);

#endif /* SERVO_MOTOR_H */
