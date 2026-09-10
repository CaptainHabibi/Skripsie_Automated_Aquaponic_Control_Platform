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
#include "Pump.h"
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
/* Definitions for Pump */
osThreadId_t PumpHandle;
const osThreadAttr_t Pump_attributes = {
  .name = "Pump",
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
/* Definitions for PumpCommandQueue */
osMessageQueueId_t PumpCommandQueueHandle;
const osMessageQueueAttr_t PumpCommandQueue_attributes = {
  .name = "PumpCommandQueue"
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
void StartPumpTask(void *argument);

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

  /* creation of PumpCommandQueue */
  PumpCommandQueueHandle = osMessageQueueNew (8, sizeof(uint32_t), &PumpCommandQueue_attributes);

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

  /* creation of Pump */
  PumpHandle = osThreadNew(StartPumpTask, NULL, &Pump_attributes);

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

	  HAL_GPIO_TogglePin(ledPin_GPIO_Port, ledPin_Pin);
	      osDelay(500);
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

		  uint8_t last_batt_state = 0xFF; // Forces initial state broadcast

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
	  	          // Hardware Power State Transition Handler
	  	          // -------------------------------------------------------------
	  	          if (batt_power != last_batt_state)
	  	          {
	  	              last_batt_state = batt_power;

	  	              if (batt_power == 1)
	  	              {
	  	                  // === ENTERING BATTERY BACKUP MODE ===

	  	                  // 1. Command ESP32-S3 Camera into Deep Sleep via UART1
	  	                  uint8_t sleep_token = 0xAA;
	  	                  HAL_UART_Transmit(&huart1, &sleep_token, 1, 50);

	  	                  // 2. Put SX1278 LoRa radio into silicon sleep (0.2 uA draw)
	  	                  if (lora_online) {
	  	                      LoRa_Sleep();
	  	                  }
	  	              }
	  	              else
	  	              {
	  	                  // === MAINS POWER RESTORED ===

	  	                  // 1. Alert ESP32-S3 to resume active vision operations
	  	                  uint8_t wake_token = 0x55;
	  	                  HAL_UART_Transmit(&huart1, &wake_token, 1, 50);

	  	                  // 2. Wake LoRa radio back to Standby mode
	  	                  if (lora_online) {
	  	                      LoRa_WriteRegister(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_STDBY);
	  	                  }
	  	              }
	  	          }

	  	      // -------------------------------------------------------------
	  	      // 2. Process Vision Tokens from ESP32-S3 over USART1
	  	      // -------------------------------------------------------------
	  	      if (ESP32_ReadToken(&rx_token, 20))
	  	      {
	  	        switch (rx_token)
	  	        {
	  	          case TOKEN_HUNGRY: // 0x02
	  	            snprintf(fish_activity, sizeof(fish_activity), "Hungry");

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

	  	      // Send state change byte to ESP32 only when power source flips
	  	          if (batt_power != last_batt_state)
	  	          {
	  	            last_batt_state = batt_power;
	  	            uint8_t pwr_token = (batt_power == 1) ? 0xAA : 0x55;
	  	            HAL_UART_Transmit(&huart1, &pwr_token, 1, 50);
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

	  	              // 1. Transmit LoRa RF Packet (skip while on battery — keep radio in silicon sleep)
	  	            // 1. Transmit LoRa RF Packet (skip while on battery — keep radio in silicon sleep)
	  	                          uint8_t tx_status = 0;
	  	                          if (lora_online && batt_power == 0)
	  	                          {
	  	                            static char lora_pkt[220];   // bumped up from 96 — SX1276 supports up to 255 bytes
	  	                            int lora_len = snprintf(lora_pkt, sizeof(lora_pkt),
	  	                                "========================\r\n"
	  	                                "   AQUAPONICS UPDATE\r\n"
	  	                                "========================\r\n"
	  	                                "Power : %s\r\n"
	  	                                "Temp  : %.1f C\r\n"
	  	                                "Water : %.1f cm\r\n"
	  	                                "pH    : %.2f\r\n"
	  	                                "EC    : %.2f mS/cm\r\n"
	  	                                "NTU   : %.0f\r\n"
	  	                                "Fish  : %s\r\n"
	  	                                "========================\r\n",
	  	                                (batt_power == 0) ? "Mains" : "BATT",
	  	                                telemetry_cache.temp_c,
	  	                                telemetry_cache.water_level_cm,
	  	                                telemetry_cache.ph_val,
	  	                                telemetry_cache.ec_val,
	  	                                telemetry_cache.turbidity_ntu,
	  	                                fish_activity
	  	                            );

	  	                            // Clamp in case snprintf would have overflowed the buffer
	  	                            if (lora_len < 0) {
	  	                                lora_len = 0;
	  	                            } else if (lora_len >= (int)sizeof(lora_pkt)) {
	  	                                lora_len = sizeof(lora_pkt) - 1;
	  	                            }

	  	                            tx_status = LoRa_Transmit((uint8_t*)lora_pkt, lora_len);
	  	                          }
	  	              // 2. Synchronize ESP32-S3 Camera Power State over UART1
	  	              uint8_t pwr_sync = (batt_power == 1) ? 0xAA : 0x55;
	  	              HAL_UART_Transmit(&huart1, &pwr_sync, 1, 20);

	  	              // 3. Determine System Warnings
	  	              warnings_buf[0] = '\0';
	  	              if (batt_power == 1) {
	  	                strcat(warnings_buf, "BATTERY POWER ACTIVE; ");
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

	  	              // 4. Build Formatted Output Dashboard String (Showing ACTUAL LoRa hardware state)
	  	              char lora_hw_stat[32];
	  	              if (!lora_online) {
	  	                strcpy(lora_hw_stat, "SPI2 Init FAILED (0x00)");
	  	              } else if (batt_power == 1) {
	  	                strcpy(lora_hw_stat, "Asleep (Battery Save)");
	  	              } else if (tx_status == 1) {
	  	                strcpy(lora_hw_stat, "Transmitted (OK)");
	  	              } else {
	  	                strcpy(lora_hw_stat, "TX Timeout (Check DIO0/PB8)");
	  	              }

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
	  	                "LoRa Radio: %s\r\n"
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
	  	                lora_hw_stat,
	  	                fish_activity,
	  	                (fish_fed_today == 1) ? "Yes" : "No",
	  	                last_fed_time
	  	              );

	  	              // Single atomic USB CDC transmit
	  	              CDC_Transmit_FS((uint8_t*)report_buf, len);
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
	    	  // 1. Dispatch event to Pump FSM
	    	      Pump_Send_Event(PUMP_EVT_FISH_FED);

	    	      // 2. Dispense
	    	      Servo_Set_Angle(180);
	    	      osDelay(1000);
	    	      Servo_Set_Angle(0);
	      }
  }
  /* USER CODE END StartServoTask */
}

/* USER CODE BEGIN Header_StartPumpTask */
/**
* @brief Function implementing the Pump thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartPumpTask */
void StartPumpTask(void *argument)
{
  /* USER CODE BEGIN StartPumpTask */
	Pump_Init();

	  PumpState_t current_state = PUMP_STATE_NORMAL;
	  Pump_Set_Speed(100);

	  const uint32_t FIVE_MINUTES_TICKS = pdMS_TO_TICKS(5 * 60 * 1000);

	  // For bench testing: use pdMS_TO_TICKS(10000) for a 10s step instead of 5 minutes
	  uint32_t state_entry_tick = xTaskGetTickCount();
	  uint8_t is_manual_override = 0;
	  uint8_t in_feeding_cycle = 0;

	  PumpEvent_t rx_event = PUMP_EVT_NONE;
  /* Infinite loop */
  for(;;)
  {
	  // Check for incoming trigger/override events (check every 200ms)
	      if (osMessageQueueGet(PumpCommandQueueHandle, &rx_event, NULL, 200) == osOK){

	        state_entry_tick = xTaskGetTickCount();

	        switch (rx_event){

	          case PUMP_EVT_FISH_FED:
	            current_state = PUMP_STATE_HIGH;
	            Pump_Set_Speed(100);
	            in_feeding_cycle = 1;
	            is_manual_override = 0;
	            break;

	          case PUMP_EVT_OVERRIDE_HIGH:
	            current_state = PUMP_STATE_HIGH;
	            Pump_Set_Speed(100);
	            is_manual_override = 1;
	            in_feeding_cycle = 0;
	            break;

	          case PUMP_EVT_OVERRIDE_MED:
	            current_state = PUMP_STATE_MEDIUM;
	            Pump_Set_Speed(80);
	            is_manual_override = 1;
	            in_feeding_cycle = 0;
	            break;

	          case PUMP_EVT_OVERRIDE_LOW:
	            current_state = PUMP_STATE_NORMAL;
	            Pump_Set_Speed(60);
	            is_manual_override = 1;
	            in_feeding_cycle = 0;
	            break;

	          default:
	            break;
	        }
	      }

	      uint32_t elapsed = xTaskGetTickCount() - state_entry_tick;

	      // 1. Handle Automatic Feeding Transitions: HIGH -> (5 min) -> MEDIUM -> (5 min) -> NORMAL
	      if (in_feeding_cycle)
	      {
	        if (current_state == PUMP_STATE_HIGH && elapsed >= FIVE_MINUTES_TICKS) {
	          current_state = PUMP_STATE_MEDIUM;
	          Pump_Set_Speed(80);
	          state_entry_tick = xTaskGetTickCount(); // Reset timer for next 5 min window
	        }
	        else if (current_state == PUMP_STATE_MEDIUM && elapsed >= FIVE_MINUTES_TICKS) {
	          current_state = PUMP_STATE_NORMAL;
	          Pump_Set_Speed(60);
	          in_feeding_cycle = 0; // Cycle complete
	        }
	      }

	      // 2. Handle Manual Override Expiration: Reverts to NORMAL after 5 minutes
	      if (is_manual_override) {
	        if (elapsed >= FIVE_MINUTES_TICKS){
	          current_state = PUMP_STATE_NORMAL;
	          Pump_Set_Speed(60);
	          is_manual_override = 0;
	        }
	      }
  }
  /* USER CODE END StartPumpTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

