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
#include "servoMotor.h"
#include "PHSensor.h"
#include "ECSensor.h"
#include "turbiditySensor.h"

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
extern ADC_HandleTypeDef hadc1;
/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
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
/* Definitions for PHSensor */
osThreadId_t PHSensorHandle;
const osThreadAttr_t PHSensor_attributes = {
  .name = "PHSensor",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for ECSensor */
osThreadId_t ECSensorHandle;
const osThreadAttr_t ECSensor_attributes = {
  .name = "ECSensor",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for TurbiditySensor */
osThreadId_t TurbiditySensorHandle;
const osThreadAttr_t TurbiditySensor_attributes = {
  .name = "TurbiditySensor",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for ThermalTask */
osThreadId_t ThermalTaskHandle;
const osThreadAttr_t ThermalTask_attributes = {
  .name = "ThermalTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityHigh,
};
/* Definitions for SensorDataQueue */
osMessageQueueId_t SensorDataQueueHandle;
const osMessageQueueAttr_t SensorDataQueue_attributes = {
  .name = "SensorDataQueue"
};
/* Definitions for TempQueue */
osMessageQueueId_t TempQueueHandle;
const osMessageQueueAttr_t TempQueue_attributes = {
  .name = "TempQueue"
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
void StartTempSensorTask(void *argument);
void StartWaterLvlSensorTask(void *argument);
void StartPHSensorTask(void *argument);
void StartECSensorTask(void *argument);
void StartTurbiditySensorTask(void *argument);
void StartThermalTaskTask(void *argument);

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

  /* creation of TempQueue */
  TempQueueHandle = osMessageQueueNew (16, sizeof(uint32_t), &TempQueue_attributes);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* creation of TempSensor */
  TempSensorHandle = osThreadNew(StartTempSensorTask, NULL, &TempSensor_attributes);

  /* creation of WaterLvlSensor */
  WaterLvlSensorHandle = osThreadNew(StartWaterLvlSensorTask, NULL, &WaterLvlSensor_attributes);

  /* creation of PHSensor */
  PHSensorHandle = osThreadNew(StartPHSensorTask, NULL, &PHSensor_attributes);

  /* creation of ECSensor */
  ECSensorHandle = osThreadNew(StartECSensorTask, NULL, &ECSensor_attributes);

  /* creation of TurbiditySensor */
  TurbiditySensorHandle = osThreadNew(StartTurbiditySensorTask, NULL, &TurbiditySensor_attributes);

  /* creation of ThermalTask */
  ThermalTaskHandle = osThreadNew(StartThermalTaskTask, NULL, &ThermalTask_attributes);

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
  int BattPower = 0 ;
  char buffer[48];
  /* Infinite loop */
  for(;;)
  {
	  BattPower = HAL_GPIO_ReadPin(PowerCheck_GPIO_Port, PowerCheck_Pin); //Read Pin to check if mains is active
	  if(BattPower == 1){
		  HAL_GPIO_TogglePin(GPIOC, ledPin_Pin);
		      osDelay(200);
		      HAL_GPIO_TogglePin(GPIOC, ledPin_Pin);
		          osDelay(200);
		          HAL_GPIO_TogglePin(GPIOC, ledPin_Pin);
		                  osDelay(500);
		                  sprintf(buffer, "Power Status: Mains\r\n");
	  }
	  else if(BattPower == 0){
		  HAL_GPIO_TogglePin(GPIOC, ledPin_Pin);
		  		      osDelay(200);
		  		      HAL_GPIO_TogglePin(GPIOC, ledPin_Pin);
		  		          osDelay(200);
		  		        sprintf(buffer, "Power Status: BATTERY\r\n");
	  }
         //      HAL_GPIO_WritePin(Pump_PWM_GPIO_Port, Pump_PWM_Pin, 1);
            //   osDelay(500);
               //HAL_GPIO_WritePin(Pump_PWM_GPIO_Port, Pump_PWM_Pin, 0);
               //osDelay(4000);
	  CDC_Transmit_FS((uint8_t*)buffer, strlen(buffer));
	  		      osDelay(1000);

  }

  /* USER CODE END StartDefaultTask */
}

/* USER CODE BEGIN Header_StartTempSensorTask */
/**
* @brief Function implementing the TempSensor thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartTempSensorTask */
void StartTempSensorTask(void *argument)
{
  /* USER CODE BEGIN StartTempSensorTask */
	char buffer[48];
		  float temp;
	  /* Infinite loop */
	  for(;;)
	  {
		  temp = Read_Temperature(); //[cite: 1]

		      // Drop the temperature ticket onto the queue rail (Timeout = 0)
		      osMessageQueuePut(TempQueueHandle, &temp, 0, 0);

		      if (temp <= -900.0f) { //[cite: 1]
		        sprintf(buffer, "Temp: ERROR\r\n");
		      } else {
		        sprintf(buffer, "Temp: %.2f C\r\n", temp);
		      }

		      CDC_Transmit_FS((uint8_t*)buffer, strlen(buffer)); //[cite: 1]
		      osDelay(2000); //[cite: 1]
	  }
  /* USER CODE END StartTempSensorTask */
}

/* USER CODE BEGIN Header_StartWaterLvlSensorTask */
/**
* @brief Function implementing the WaterLvlSensor thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartWaterLvlSensorTask */
void StartWaterLvlSensorTask(void *argument)
{
  /* USER CODE BEGIN StartWaterLvlSensorTask */
  /* Infinite loop */
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

  /* USER CODE END StartWaterLvlSensorTask */
}

/* USER CODE BEGIN Header_StartPHSensorTask */
/**
* @brief Function implementing the PHSensor thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartPHSensorTask */
void StartPHSensorTask(void *argument)
{
  /* USER CODE BEGIN StartPHSensorTask */
	// Initialization
	pH_Sensor_Init();

	  for(;;)
	  {
	    float current_ph = pH_Sensor_Read(&hadc1);

	    // Create a string buffer to send over USB CDC / UART
	    char buffer[64];
	    int len = 0;

	    if (current_ph >= 0.0f)
	    {
	        len = snprintf(buffer, sizeof(buffer), "pH: %.2f\r\n", current_ph);
	    }
	    else
	    {
	        len = snprintf(buffer, sizeof(buffer), "pH: ERROR\r\n");
	    }

	    // Send data (e.g., via USB CDC virtual COM port)
	    CDC_Transmit_FS((uint8_t*)buffer, len);

	    osDelay(1000);
	  }
  /* USER CODE END StartPHSensorTask */
}

/* USER CODE BEGIN Header_StartECSensorTask */
/**
* @brief Function implementing the ECSensor thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartECSensorTask */
void StartECSensorTask(void *argument)
{
  /* USER CODE BEGIN StartECSensorTask */
	EC_Sensor_Init();

	  for(;;)
	  {
	    float current_ec = EC_Sensor_Read(&hadc1);
	    char buffer[64];
	    int len = 0;

	    if (current_ec >= 0.0f)
	    {
	        len = snprintf(buffer, sizeof(buffer), "EC: %.2f mS/cm\r\n", current_ec);
	    }
	    else
	    {
	        len = snprintf(buffer, sizeof(buffer), "EC: ERROR\r\n");
	    }

	    CDC_Transmit_FS((uint8_t*)buffer, len);
	    osDelay(1000);
	  }
  /* USER CODE END StartECSensorTask */
}

/* USER CODE BEGIN Header_StartTurbiditySensorTask */
/**
* @brief Function implementing the TurbiditySensor thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartTurbiditySensorTask */
void StartTurbiditySensorTask(void *argument)
{
  /* USER CODE BEGIN StartTurbiditySensorTask */
  /* Infinite loop */
	for(;;)
	  {
	    // 1. Get the stepped-down 0-3.3V reading from your sensor driver
	    float adc_voltage = Read_Turbidity_Voltage(); //[cite: 1, 2]

	    // 2. Reverse your E12 resistor voltage divider (10k & 4.7k) to get true sensor voltage
	    // V_sensor = V_adc * ((10000 + 4700) / 10000)
	    float sensor_voltage = adc_voltage * 1.47f;

	    // 3. Apply the DFRobot official voltage-to-NTU mapping curve[cite: 5]
	    float ntu_value = (-1120.4f * sensor_voltage * sensor_voltage) + (5742.3f * sensor_voltage) - 4352.9f; //[cite: 5]

	    // 4. Clamp negative values (clean water can sometimes dip slightly below 0 NTU on the curve)
	    if (ntu_value < 0.0f) {
	        ntu_value = 0.0f;
	    }

	    char buffer[64];
	    int len = 0;

	    if (adc_voltage >= 0.0f)
	    {
	        len = snprintf(buffer, sizeof(buffer), "Turbidity: %.2f NTU\r\n", ntu_value);
	    }
	    else
	    {
	        len = snprintf(buffer, sizeof(buffer), "Turbidity: ERROR\r\n");
	    }

	    // Send data over serial monitor
	    CDC_Transmit_FS((uint8_t*)buffer, len);

	    // Run every 2 seconds
	    osDelay(2000);
	  }
  /* USER CODE END StartTurbiditySensorTask */
}

/* USER CODE BEGIN Header_StartThermalTaskTask */
/**
* @brief Function implementing the ThermalTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartThermalTaskTask */
void StartThermalTaskTask(void *argument)
{
  /* USER CODE BEGIN StartThermalTaskTask */
	float received_temp;
	  float target_temp_Low = 21.0f;
	  float target_temp_High = 24.0f;
  /* Infinite loop */
  for(;;)
  {
	  // Task naturally sleeps here until a temperature arrives in the queue
	      if (osMessageQueueGet(TempQueueHandle, &received_temp, NULL, osWaitForever) == osOK)
	      {
	        // Ignore sensor errors to prevent the heater from locking ON during a fault
	        if (received_temp > -900.0f)
	        {
	           if (received_temp < target_temp_Low) {
	              // Water is cold, actuate relay (Heater ON)
	              HAL_GPIO_WritePin(HeaterRelay_GPIO_Port, HeaterRelay_Pin, 1);
	           }
	           else  if(received_temp>target_temp_High){
	              // Target reached, release relay (Heater OFF)
	              HAL_GPIO_WritePin(HeaterRelay_GPIO_Port, HeaterRelay_Pin, 0);
	           }
	        }
	      }
  }
  /* USER CODE END StartThermalTaskTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

