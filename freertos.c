/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include "usbd_cdc_if.h"
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
extern TIM_HandleTypeDef htim2; // Brings in TIM2 from main.c
/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for ActuatorTask */
osThreadId_t ActuatorTaskHandle;
const osThreadAttr_t ActuatorTask_attributes = {
  .name = "ActuatorTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityHigh,
};
/* Definitions for CommTask */
osThreadId_t CommTaskHandle;
const osThreadAttr_t CommTask_attributes = {
  .name = "CommTask",
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityAboveNormal,
};
/* Definitions for SensorTask */
osThreadId_t SensorTaskHandle;
const osThreadAttr_t SensorTask_attributes = {
  .name = "SensorTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for SensorDataQueue */
osMessageQueueId_t SensorDataQueueHandle;
const osMessageQueueAttr_t SensorDataQueue_attributes = {
  .name = "SensorDataQueue"
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

void delay_us(uint16_t us);

uint8_t DS18B20_Start(void);
void DS18B20_Write(uint8_t data);
uint8_t DS18B20_Read(void);
float Read_Temperature(void);
/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);
void StartActuatorTask(void *argument);
void StartCommTask(void *argument);
void StartSensorTask(void *argument);

extern void MX_USB_DEVICE_Init(void);
void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* Create the queue(s) */
  /* creation of SensorDataQueue */
  SensorDataQueueHandle = osMessageQueueNew (16, sizeof(uint16_t), &SensorDataQueue_attributes);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* creation of ActuatorTask */
  ActuatorTaskHandle = osThreadNew(StartActuatorTask, NULL, &ActuatorTask_attributes);

  /* creation of CommTask */
  CommTaskHandle = osThreadNew(StartCommTask, NULL, &CommTask_attributes);

  /* creation of SensorTask */
  SensorTaskHandle = osThreadNew(StartSensorTask, NULL, &SensorTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* init code for USB_DEVICE */
  MX_USB_DEVICE_Init();
  /* USER CODE BEGIN StartDefaultTask */
  /* Infinite loop */
  for(;;)
  {
	  HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
    osDelay(500);
  }
  /* USER CODE END StartDefaultTask */
}

/* USER CODE BEGIN Header_StartActuatorTask */
/**
* @brief Function implementing the ActuatorTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartActuatorTask */
void StartActuatorTask(void *argument)
{
  /* USER CODE BEGIN StartActuatorTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartActuatorTask */
}

/* USER CODE BEGIN Header_StartCommTask */
/**
* @brief Function implementing the CommTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartCommTask */
void StartCommTask(void *argument)
{
  /* USER CODE BEGIN StartCommTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartCommTask */
}

/* USER CODE BEGIN Header_StartSensorTask */
/**
* @brief Function implementing the SensorTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartSensorTask */
void StartSensorTask(void *argument)
{
  /* USER CODE BEGIN StartSensorTask */
	char buffer[48];
	  float temp;
  /* Infinite loop */
  for(;;)
  {
	  temp = Read_Temperature();

	      if (temp <= -900.0f) {
	        sprintf(buffer, "Error: DS18B20 Not Responding\r\n");
	      } else {
	        sprintf(buffer, "Temperature: %.2f C\r\n", temp);
	      }

	      CDC_Transmit_FS((uint8_t*)buffer, strlen(buffer));
	      osDelay(1000);
  }
  /* USER CODE END StartSensorTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
void delay_us(uint16_t us) {
    __HAL_TIM_SET_COUNTER(&htim2, 0);
    while ((uint16_t)__HAL_TIM_GET_COUNTER(&htim2) < us);
}

uint8_t DS18B20_Init_Raw(void) {
    uint8_t i;
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET);
    delay_us(480);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET);
    delay_us(70);
    i = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_1);
    delay_us(410);
    return i; // Returns 0 if sensor is present
}

void DS18B20_Write_Byte(uint8_t dat) {
    for (uint8_t i = 0; i < 8; i++) {
        if ((dat & (1 << i)) != 0) {
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET);
            delay_us(1);
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET);
            delay_us(60);
        } else {
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET);
            delay_us(60);
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET);
            delay_us(1);
        }
    }
}

uint8_t DS18B20_Read_Byte(void) {
    uint8_t dat = 0;
    for (uint8_t i = 0; i < 8; i++) {
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET);
        delay_us(2);
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET);
        delay_us(10);
        if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_1)) {
            dat |= (1 << i);
        }
        delay_us(50);
    }
    return dat;
}

float Read_Temperature(void) {
    uint8_t Presence, MSB, LSB;
    int16_t raw_temperature;

    // Freeze ALL interrupts so FreeRTOS cannot disrupt microsecond timing
    __disable_irq();

    if (DS18B20_Init_Raw() != 0) {
        __enable_irq();
        return -999.0f; // Sensor error flag
    }

    DS18B20_Write_Byte(0xCC); // Skip ROM
    DS18B20_Write_Byte(0x44); // Convert T

    __enable_irq(); // Unfreeze interrupts during the 750ms sensor wait

    osDelay(750); // Sensor internal conversion time

    __disable_irq(); // Re-freeze for reading data
    if (DS18B20_Init_Raw() != 0) {
        __enable_irq();
        return -999.0f;
    }

    DS18B20_Write_Byte(0xCC); // Skip ROM
    DS18B20_Write_Byte(0xBE); // Read Scratchpad
    LSB = DS18B20_Read_Byte();
    MSB = DS18B20_Read_Byte();

    __enable_irq(); // Done reading, restore normal OS operation

    raw_temperature = ((MSB << 8) | LSB);
    return (float)raw_temperature * 0.0625f;
}
/* USER CODE END Application */

