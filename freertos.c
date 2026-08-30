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
#include "usart.h"     // Provides huart1 definition and UART HAL types
#include "LoRa.h"
#include "Esp32.h"
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
extern UART_HandleTypeDef huart1;
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
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for Comms */
osThreadId_t CommsHandle;
const osThreadAttr_t Comms_attributes = {
  .name = "Comms",
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for Servo */
osThreadId_t ServoHandle;
const osThreadAttr_t Servo_attributes = {
  .name = "Servo",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityNormal,
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
void StartCommsTask(void *argument);
void StartServoTask(void *argument);

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
  SensorDataQueueHandle = osMessageQueueNew (16, 12, &SensorDataQueue_attributes);

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

  /* creation of Comms */
  CommsHandle = osThreadNew(StartCommsTask, NULL, &Comms_attributes);

  /* creation of Servo */
  ServoHandle = osThreadNew(StartServoTask, NULL, &Servo_attributes);

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
		                  osDelay(500);

         //      HAL_GPIO_WritePin(Pump_PWM_GPIO_Port, Pump_PWM_Pin, 1);
            //   osDelay(500);
               //HAL_GPIO_WritePin(Pump_PWM_GPIO_Port, Pump_PWM_Pin, 0);
               //osDelay(4000);


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
	SensorMsg_t msg;
	  msg.type = SENSOR_TYPE_TEMP;
	  float current_temp = 0.0f;
	  /* Infinite loop */
	  for(;;)
	  {
		  current_temp = Read_Temperature(); // From tempSensor.h

		      if (current_temp > -900.0f)
		      {
		        // 1. Post to Thermal Control Task mailbox (for Relay control)
		        osMessageQueuePut(TempQueueHandle, &current_temp, 0, 0);

		        // 2. Post to Central Telemetry Queue (for USB CDC & LoRa)
		        msg.value = current_temp;
		        msg.timestamp = xTaskGetTickCount();
		        osMessageQueuePut(SensorDataQueueHandle, &msg, 0, 0);
		      }

		      osDelay(2000); // 2-second cycle
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
	SensorMsg_t msg;
	  msg.type = SENSOR_TYPE_WATER_LVL;
	  /* Infinite loop */
	  for(;;)
	  {
		  msg.value = Read_Water_Level_CM(); // From waterSensor.h
		      msg.timestamp = xTaskGetTickCount();

		      osMessageQueuePut(SensorDataQueueHandle, &msg, 0, 0);

		      osDelay(1000); // Sample water level every 1s
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
	SensorMsg_t msg;
	  msg.type = SENSOR_TYPE_PH;
	  pH_Sensor_Init();


	  for(;;)
	  {
		  float val = pH_Sensor_Read(&hadc1);
		      if (val >= 0.0f)
		      {
		        msg.value = val;
		        msg.timestamp = xTaskGetTickCount();
		        osMessageQueuePut(SensorDataQueueHandle, &msg, 0, 0);
		      }
		      osDelay(2000);
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
	SensorMsg_t msg;
	  msg.type = SENSOR_TYPE_EC;
	  EC_Sensor_Init();
	  for(;;)
	  {
		  float val = EC_Sensor_Read(&hadc1);
		      if (val >= 0.0f)
		      {
		        msg.value = val;
		        msg.timestamp = xTaskGetTickCount();
		        osMessageQueuePut(SensorDataQueueHandle, &msg, 0, 0);
		      }
		      osDelay(2000);
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
	SensorMsg_t msg;
	  msg.type = SENSOR_TYPE_TURBIDITY;
  /* Infinite loop */
	for(;;)
	  {
		float adc_voltage = Read_Turbidity_Voltage();
		    if (adc_voltage >= 0.0f)
		    {
		      float sensor_voltage = adc_voltage * 1.47f;
		      float ntu = (-1120.4f * sensor_voltage * sensor_voltage) + (5742.3f * sensor_voltage) - 4352.9f;
		      if (ntu < 0.0f) ntu = 0.0f;

		      msg.value = ntu;
		      msg.timestamp = xTaskGetTickCount();
		      osMessageQueuePut(SensorDataQueueHandle, &msg, 0, 0);
		    }
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
	float received_temp = 0.0f;
	  const float target_temp_Low  = 21.0f;
	  const float target_temp_High = 24.0f;
	  uint8_t heater_state = 0;
  /* Infinite loop */
  for(;;)
  {
	  if (osMessageQueueGet(TempQueueHandle, &received_temp, NULL, osWaitForever) == osOK)
	      {
	        // 1. REJECT SENSOR ERRORS AND IMPOSSIBLE VALUES IMMEDIATELY
	        if (received_temp < 5.0f || received_temp > 45.0f)
	        {
	          // Do not touch the relay pin on sensor errors!
	          continue;
	        }

	        // 2. Strict Hysteresis Switching
	        if (received_temp < target_temp_Low && heater_state == 0)
	        {
	          HAL_GPIO_WritePin(HeaterRelay_GPIO_Port, HeaterRelay_Pin, GPIO_PIN_SET);
	          heater_state = 1;
	        }
	        else if (received_temp > target_temp_High && heater_state == 1)
	        {
	          HAL_GPIO_WritePin(HeaterRelay_GPIO_Port, HeaterRelay_Pin, GPIO_PIN_RESET);
	          heater_state = 0;
	        }
	      }
  }
  /* USER CODE END StartThermalTaskTask */
}

/* USER CODE BEGIN Header_StartCommsTask */
/**
* @brief Function implementing the Comms thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartCommsTask */
void StartCommsTask(void *argument)
{
  /* USER CODE BEGIN StartCommsTask */
	SensorMsg_t rx_msg;
	  SystemTelemetry_t telemetry_cache = {
	      .temp_c = 0.0f,
	      .water_level_cm = 0.0f,
	      .turbidity_ntu = 0.0f,
	      .ph_val = 7.0f,
	      .ec_val = 1.2f
	  };

	  uint8_t rx_token = 0;
	    static char report_buf[512];    // 'static' keeps these off the task stack
	    static char warnings_buf[128];
	    uint32_t last_tx_tick = 0;

	    uint8_t fish_fed_today = 0;
	    static char last_fed_time[20] = "Never";
	    static char fish_activity[12] = "Normal";
	    uint8_t batt_power = 0;
	    uint8_t lora_online = LoRa_Init();
	    ESP32_Init();
  /* Infinite loop */
  for(;;)
  {
	  // -------------------------------------------------------------
	      // 1. Check Power Grid State (PA7 PowerCheck pin)
	      // -------------------------------------------------------------
	      batt_power = (HAL_GPIO_ReadPin(PowerCheck_GPIO_Port, PowerCheck_Pin) == GPIO_PIN_RESET) ? 1 : 0;

	      // -------------------------------------------------------------
	      // 2. Process Vision Tokens from ESP32-S3 over USART1
	      // -------------------------------------------------------------
	      if (ESP32_ReadToken(&rx_token, 20))
	      {
	        switch (rx_token)
	        {
	          case TOKEN_HUNGRY: // 0x02
	            snprintf(fish_activity, sizeof(fish_activity), "Hungry");

	            // Wake up Servo task via FreeRTOS notification
	                        xTaskNotify((TaskHandle_t)ServoHandle, 0x01, eSetBits);

	            fish_fed_today = 1;
	            snprintf(last_fed_time, sizeof(last_fed_time), "%lu sec ago", (xTaskGetTickCount() / 1000));
	            snprintf(fish_activity, sizeof(fish_activity), "Feeding");
	            break;

	          case TOKEN_ANOMALY: // 0x03
	            snprintf(fish_activity, sizeof(fish_activity), "Abnormal");
	            break;

	          case TOKEN_NORMAL: // 0x01
	          default:
	            snprintf(fish_activity, sizeof(fish_activity), "Normal");
	            break;
	        }
	        rx_token = 0;
	      }

	      // -------------------------------------------------------------
	      // 3. Drain and Cache Sensor Data from Queue
	      // -------------------------------------------------------------
	      while (osMessageQueueGet(SensorDataQueueHandle, &rx_msg, NULL, 0) == osOK)
	      {
	        switch (rx_msg.type)
	        {
	          case SENSOR_TYPE_TEMP:
	            telemetry_cache.temp_c = rx_msg.value;
	            break;
	          case SENSOR_TYPE_WATER_LVL:
	            telemetry_cache.water_level_cm = rx_msg.value;
	            break;
	          case SENSOR_TYPE_TURBIDITY:
	            telemetry_cache.turbidity_ntu = rx_msg.value;
	            break;
	          case SENSOR_TYPE_PH:
	            telemetry_cache.ph_val = rx_msg.value;
	            break;
	          case SENSOR_TYPE_EC:
	            telemetry_cache.ec_val = rx_msg.value;
	            break;
	          default:
	            break;
	        }
	      }

	      // -------------------------------------------------------------
	      // 4. Periodic Formatted Report Output (Every 5 seconds)
	      // -------------------------------------------------------------
	      if ((xTaskGetTickCount() - last_tx_tick) >= pdMS_TO_TICKS(5000))
	      {
	        last_tx_tick = xTaskGetTickCount();

	        // Determine System Warnings
	        warnings_buf[0] = '\0';
	        if (batt_power == 1) {
	          strcat(warnings_buf, "BATTERY POWER ACTIVE (Comms Offline); ");
	        }
	        if (telemetry_cache.water_level_cm < 10.0f && telemetry_cache.water_level_cm > 0.0f) {
	          strcat(warnings_buf, "Low Water LVL; ");
	        }
	        if (telemetry_cache.temp_c <= -900.0f || telemetry_cache.temp_c > 45.0f) {
	          strcat(warnings_buf, "CHECK Temp Sensor; ");
	        }
	        if (telemetry_cache.ph_val < 5.5f || telemetry_cache.ph_val > 8.5f) {
	          strcat(warnings_buf, "CHECK pH Sensor; ");
	        }
	        if (telemetry_cache.ec_val < 0.2f || telemetry_cache.ec_val > 3.0f) {
	          strcat(warnings_buf, "CHECK EC Sensor; ");
	        }
	        if (telemetry_cache.turbidity_ntu > 100.0f) {
	          strcat(warnings_buf, "CHECK Turbidity Sensor; ");
	        }
	        if (strlen(warnings_buf) == 0) {
	          strcpy(warnings_buf, "None");
	        }

	        // Build Formatted Output Dashboard String
	        int len = snprintf(report_buf, sizeof(report_buf),
	          "\r\n_Aquaponic_System_Info_\r\n\r\n"
	          "PowerStatus: %s\r\n"
	          "Warnings: %s\r\n"
	          "___Sensors___\r\n"
	          "Temperature: %.2f C\r\n"
	          "WaterLvl: %.2f cm\r\n"
	          "PH: %.2f pH\r\n"
	          "EC: %.2f mS/cm\r\n"
	          "Turbidity: %.2f NTU\r\n\r\n"
	          "___Comms_Activity___\r\n"
	          "Camera: %s\r\n"
	          "LoRa: %s\r\n"
	          "Fish_Activity: %s\r\n"
	          "Fish Fed Today: %s\r\n"
	          "Last Fed: %s\r\n"
	          "Pumps: 60%%\r\n\r\n",
	          (batt_power == 0) ? "Mains" : "BATTERY",
	          warnings_buf,
	          telemetry_cache.temp_c,
	          telemetry_cache.water_level_cm,
	          telemetry_cache.ph_val,
	          telemetry_cache.ec_val,
	          telemetry_cache.turbidity_ntu,
	          (batt_power == 0) ? "Online" : "Sleep",
	          (batt_power == 0) ? "Online" : "Sleep",
	          fish_activity,
	          (fish_fed_today == 1) ? "Yes" : "No",
	          last_fed_time
	        );

	        // Stream to Serial USB Terminal (Fedora)
	        CDC_Transmit_FS((uint8_t*)report_buf, len);

	        // Transmit to LoRa Transceiver via SPI2
	        // Only attempt RF broadcast if the SX1278 responded with 0x12 during init
	        if (lora_online) {
	            LoRa_Transmit((uint8_t*)report_buf, len);
	        }
	      }

	      osDelay(50);
  }
  /* USER CODE END StartCommsTask */
}

/* USER CODE BEGIN Header_StartServoTask */
/**
* @brief Function implementing the Servo thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartServoTask */
void StartServoTask(void *argument)
{
  /* USER CODE BEGIN StartServoTask */

	Servo_Init();

	  // Set feeder to rest position (0 deg)
	  Servo_Set_Angle(0);
	  uint32_t notify_val = 0;
  /* Infinite loop */
  for(;;)
  {
	  // Block indefinitely until the RTC Alarm or Vision Queue triggers it (0% CPU load)
	      xTaskNotifyWait(0x00, 0xFFFFFFFF, &notify_val, portMAX_DELAY);

	      if (notify_val & 0x01){
	  // Feeding sequence: sweep open to dispense, wait, then close
	      Servo_Set_Angle(180);
	      osDelay(1000); // Dispense window

	      Servo_Set_Angle(0);

	      }
  }
  /* USER CODE END StartServoTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

