#include "tempSensor.h"
#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"

extern TIM_HandleTypeDef htim2;

#define DS18B20_PORT GPIOA
#define DS18B20_PIN  GPIO_PIN_1

void delay_us(uint16_t us) {
    __HAL_TIM_SET_COUNTER(&htim2, 0);
    uint32_t safety_timeout = 50000; // Hard CPU loop escape
    while ((__HAL_TIM_GET_COUNTER(&htim2) < us) && (--safety_timeout > 0));
}

uint8_t DS18B20_Start(void) {
    uint8_t response = 0;

    HAL_GPIO_WritePin(DS18B20_PORT, DS18B20_PIN, GPIO_PIN_RESET);
    delay_us(480);

    HAL_GPIO_WritePin(DS18B20_PORT, DS18B20_PIN, GPIO_PIN_SET);
    delay_us(80);

    if (!(HAL_GPIO_ReadPin(DS18B20_PORT, DS18B20_PIN))) response = 1;
    else response = 0;

    delay_us(400);
    return response;
}

void DS18B20_Write(uint8_t data) {
    for (int i = 0; i < 8; i++) {
        if ((data & (1 << i)) != 0) {
            HAL_GPIO_WritePin(DS18B20_PORT, DS18B20_PIN, GPIO_PIN_RESET);
            delay_us(1);
            HAL_GPIO_WritePin(DS18B20_PORT, DS18B20_PIN, GPIO_PIN_SET);
            delay_us(60);
        } else {
            HAL_GPIO_WritePin(DS18B20_PORT, DS18B20_PIN, GPIO_PIN_RESET);
            delay_us(60);
            HAL_GPIO_WritePin(DS18B20_PORT, DS18B20_PIN, GPIO_PIN_SET);
        }
    }
}

uint8_t DS18B20_Read(void) {
    uint8_t value = 0;
    for (int i = 0; i < 8; i++) {
        HAL_GPIO_WritePin(DS18B20_PORT, DS18B20_PIN, GPIO_PIN_RESET);
        delay_us(2);
        HAL_GPIO_WritePin(DS18B20_PORT, DS18B20_PIN, GPIO_PIN_SET);
        delay_us(12);

        if (HAL_GPIO_ReadPin(DS18B20_PORT, DS18B20_PIN)) {
            value |= (1 << i);
        }
        delay_us(50);
    }
    return value;
}

float Read_Temperature(void) {
    uint8_t temp_lsb, temp_msb;
    uint16_t temp;

    taskENTER_CRITICAL(); // Protect 1-Wire start from scheduler
    if (!DS18B20_Start()) {
        taskEXIT_CRITICAL();
        return -999.0f; // Sensor not responding
    }

    DS18B20_Write(0xCC); // Skip ROM
    DS18B20_Write(0x44); // Start conversion
    taskEXIT_CRITICAL();

    osDelay(750); // Let FreeRTOS run other tasks during conversion

    taskENTER_CRITICAL();
    if (!DS18B20_Start()) {
        taskEXIT_CRITICAL();
        return -999.0f;
    }

    DS18B20_Write(0xCC); // Skip ROM
    DS18B20_Write(0xBE); // Read scratchpad

    temp_lsb = DS18B20_Read();
    temp_msb = DS18B20_Read();
    taskEXIT_CRITICAL();

    temp = (temp_msb << 8) | temp_lsb;
    return (float)temp / 16.0f;
}
