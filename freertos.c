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
#include "tempSensor.h"
#include "waterSensor.h"

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
/* Definitions for Comms */
osThreadId_t CommsHandle;
const osThreadAttr_t Comms_attributes = {
  .name = "Comms",
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityAboveNormal,
};
/* Definitions for TempSensor */
osThreadId_t TempSensorHandle;
const osThreadAttr_t TempSensor_attributes = {
  .name = "TempSensor",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for WaterLvlSensor */
osThreadId_t WaterLvlSensorHandle;
const osThreadAttr_t WaterLvlSensor_attributes = {
  .name = "WaterLvlSensor",
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
void StartComms(void *argument);
void StartTempSensor(void *argument);
void StartWaterLvlSensor(void *argument);

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

  /* creation of Comms */
  CommsHandle = osThreadNew(StartComms, NULL, &Comms_attributes);

  /* creation of TempSensor */
  TempSensorHandle = osThreadNew(StartTempSensor, NULL, &TempSensor_attributes);

  /* creation of WaterLvlSensor */
  WaterLvlSensorHandle = osThreadNew(StartWaterLvlSensor, NULL, &WaterLvlSensor_attributes);

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
	  HAL_GPIO_TogglePin(GPIOC, ledPin_Pin);
    osDelay(200);
    HAL_GPIO_TogglePin(GPIOC, ledPin_Pin);
        osDelay(200);
        HAL_GPIO_TogglePin(GPIOC, ledPin_Pin);
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

/* USER CODE BEGIN Header_StartComms */
/**
* @brief Function implementing the Comms thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartComms */
void StartComms(void *argument)
{
  /* USER CODE BEGIN StartComms */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartComms */
}

/* USER CODE BEGIN Header_StartTempSensor */
/**
* @brief Function implementing the TempSensor thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartTempSensor */
void StartTempSensor(void *argument)
{
  /* USER CODE BEGIN StartTempSensor */
	char buffer[48];
	  float temp;
  /* Infinite loop */
  for(;;)
  {
	  temp = Read_Temperature();

	      if (temp <= -900.0f) {
	        sprintf(buffer, "Temp: ERROR\r\n");
	      } else {
	        sprintf(buffer, "Temp: %.2f C\r\n", temp);
	      }

	      CDC_Transmit_FS((uint8_t*)buffer, strlen(buffer));
	      osDelay(2000); // Give the 750ms conversion plenty of breathing room
  }
  /* USER CODE END StartTempSensor */
}

/* USER CODE BEGIN Header_StartWaterLvlSensor */
/**
* @brief Function implementing the WaterLvlSensor thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartWaterLvlSensor */
void StartWaterLvlSensor(void *argument)
{
  /* USER CODE BEGIN StartWaterLvlSensor */
	char buffer[48];
	  float water_level;
  /* Infinite loop */
  for(;;)
  {
	  water_level = Read_Water_Level_CM();

	      if (water_level < 0.0f) {
	        sprintf(buffer, "Water Level: TIMEOUT\r\n");
	      } else {
	        sprintf(buffer, "Water Level: %.1f cm\r\n", water_level);
	      }

	      CDC_Transmit_FS((uint8_t*)buffer, strlen(buffer));
	      osDelay(500);
  }
  /* USER CODE END StartWaterLvlSensor */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

