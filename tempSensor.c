#include "tempSensor.h"
#include "FreeRTOS.h"
#include "cmsis_os.h" // Required so the compiler recognizes osDelay

// Brings in TIM2 from main.c for microsecond delays
extern TIM_HandleTypeDef htim2;

#define DS18B20_PORT GPIOA
#define DS18B20_PIN  GPIO_PIN_1

// Microsecond delay using TIM2
void delay_us(uint16_t us) {
    __HAL_TIM_SET_COUNTER(&htim2, 0);
    while ((__HAL_TIM_GET_COUNTER(&htim2)) < us);
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
        delay_us(12); // Wait for sensor to pull line low if 0

        if (HAL_GPIO_ReadPin(DS18B20_PORT, DS18B20_PIN)) {
            value |= 1 << i;
        }
        delay_us(50);
    }
    return value;
}

float Read_Temperature(void) {
    uint8_t temp_lsb, temp_msb;
    uint16_t temp;

    // Lock out FreeRTOS interrupts to protect microsecond timing
    __disable_irq();

    if (!DS18B20_Start()) {
        __enable_irq();
        return -999.0f; // Sensor not found error
    }

    DS18B20_Write(0xCC); // Skip ROM
    DS18B20_Write(0x44); // Convert t

    __enable_irq(); // Let the RTOS breathe during the 750ms conversion
    osDelay(750);
    __disable_irq(); // Lock again for reading

    DS18B20_Start();
    DS18B20_Write(0xCC); // Skip ROM
    DS18B20_Write(0xBE); // Read Scratchpad

    temp_lsb = DS18B20_Read();
    temp_msb = DS18B20_Read();

    __enable_irq(); // Unlock interrupts

    temp = (temp_msb << 8) | temp_lsb;
    return (float)temp / 16.0f;
}
