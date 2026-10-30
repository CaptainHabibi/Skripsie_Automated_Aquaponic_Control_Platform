#ifndef ESP32_H
#define ESP32_H

#include "main.h"
#include <stdint.h>

/* Behavioral Tokens matching ESP32-S3 definitions */
#define TOKEN_NORMAL      0x01
#define TOKEN_HUNGRY      0x02
#define TOKEN_NIGHT     0x03

/* STM32 Downstream Request Commands */
#define CMD_TRIGGER_AI    'C'
#define CMD_PING_ESP32    'P'

/* Function Prototypes */
void    ESP32_Init(void);
uint8_t ESP32_ReadToken(uint8_t *token, uint32_t timeout_ms);
void    ESP32_SendCommand(uint8_t cmd);

#endif /* ESP32_H */
