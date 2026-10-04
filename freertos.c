/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
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
#include "usart.h"
#include "LoRa.h"
#include "Esp32.h"
#include "Pump.h"
#include "downlink.h"
/* USER CODE END Includes */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
extern TIM_HandleTypeDef htim2;
extern ADC_HandleTypeDef hadc1;
extern UART_HandleTypeDef huart1;
/* USER CODE END Variables */

/* Task handles */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask", .stack_size = 128 * 4, .priority = (osPriority_t) osPriorityLow,
};
osThreadId_t TempSensorHandle;
const osThreadAttr_t TempSensor_attributes = {
  .name = "TempSensor", .stack_size = 256 * 4, .priority = (osPriority_t) osPriorityNormal,
};
osThreadId_t WaterLvlSensorHandle;
const osThreadAttr_t WaterLvlSensor_attributes = {
  .name = "WaterLvlSensor", .stack_size = 256 * 4, .priority = (osPriority_t) osPriorityNormal,
};
osThreadId_t PHSensorHandle;
const osThreadAttr_t PHSensor_attributes = {
  .name = "PHSensor", .stack_size = 256 * 4, .priority = (osPriority_t) osPriorityNormal,
};
osThreadId_t ECSensorHandle;
const osThreadAttr_t ECSensor_attributes = {
  .name = "ECSensor", .stack_size = 256 * 4, .priority = (osPriority_t) osPriorityNormal,
};
osThreadId_t TurbiditySensorHandle;
const osThreadAttr_t TurbiditySensor_attributes = {
  .name = "TurbiditySensor", .stack_size = 256 * 4, .priority = (osPriority_t) osPriorityNormal,
};
osThreadId_t ThermalTaskHandle;
const osThreadAttr_t ThermalTask_attributes = {
  .name = "ThermalTask", .stack_size = 256 * 4, .priority = (osPriority_t) osPriorityNormal,
};
osThreadId_t CommsHandle;
const osThreadAttr_t Comms_attributes = {
  .name = "Comms", .stack_size = 512 * 4, .priority = (osPriority_t) osPriorityNormal,
};
osThreadId_t ServoHandle;
const osThreadAttr_t Servo_attributes = {
  .name = "Servo", .stack_size = 256 * 4, .priority = (osPriority_t) osPriorityNormal,
};
osThreadId_t PumpHandle;
const osThreadAttr_t Pump_attributes = {
  .name = "Pump", .stack_size = 256 * 4, .priority = (osPriority_t) osPriorityNormal,
};
osMessageQueueId_t SensorDataQueueHandle;
const osMessageQueueAttr_t SensorDataQueue_attributes = { .name = "SensorDataQueue" };
osMessageQueueId_t TempQueueHandle;
const osMessageQueueAttr_t TempQueue_attributes = { .name = "TempQueue" };
osMessageQueueId_t PumpCommandQueueHandle;
const osMessageQueueAttr_t PumpCommandQueue_attributes = { .name = "PumpCommandQueue" };

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
void MX_FREERTOS_Init(void);

void MX_FREERTOS_Init(void) {
  SensorDataQueueHandle = osMessageQueueNew (16, 12, &SensorDataQueue_attributes);
  TempQueueHandle = osMessageQueueNew (16, sizeof(uint32_t), &TempQueue_attributes);
  PumpCommandQueueHandle = osMessageQueueNew (8, sizeof(uint32_t), &PumpCommandQueue_attributes);

  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);
  TempSensorHandle = osThreadNew(StartTempSensorTask, NULL, &TempSensor_attributes);
  WaterLvlSensorHandle = osThreadNew(StartWaterLvlSensorTask, NULL, &WaterLvlSensor_attributes);
  PHSensorHandle = osThreadNew(StartPHSensorTask, NULL, &PHSensor_attributes);
  ECSensorHandle = osThreadNew(StartECSensorTask, NULL, &ECSensor_attributes);
  TurbiditySensorHandle = osThreadNew(StartTurbiditySensorTask, NULL, &TurbiditySensor_attributes);
  ThermalTaskHandle = osThreadNew(StartThermalTaskTask, NULL, &ThermalTask_attributes);
  CommsHandle = osThreadNew(StartCommsTask, NULL, &Comms_attributes);
  ServoHandle = osThreadNew(StartServoTask, NULL, &Servo_attributes);
  PumpHandle = osThreadNew(StartPumpTask, NULL, &Pump_attributes);
}

void StartDefaultTask(void *argument)
{
  MX_USB_DEVICE_Init();
  for(;;) {
    HAL_GPIO_TogglePin(ledPin_GPIO_Port, ledPin_Pin);
    osDelay(500);
  }
}

void StartTempSensorTask(void *argument)
{
  SensorMsg_t msg;
  msg.type = SENSOR_TYPE_TEMP;
  float current_temp = 0.0f;
  for(;;) {
    current_temp = Read_Temperature();
    if (current_temp > -900.0f) {
      osMessageQueuePut(TempQueueHandle, &current_temp, 0, 0);
      msg.value = current_temp;
      msg.timestamp = xTaskGetTickCount();
      osMessageQueuePut(SensorDataQueueHandle, &msg, 0, 0);
    }
    osDelay(2000);
  }
}

void StartWaterLvlSensorTask(void *argument)
{
  SensorMsg_t msg;
  msg.type = SENSOR_TYPE_WATER_LVL;
  for(;;) {
    msg.value = Read_Water_Level_CM();
    msg.timestamp = xTaskGetTickCount();
    osMessageQueuePut(SensorDataQueueHandle, &msg, 0, 0);
    osDelay(1000);
  }
}

void StartPHSensorTask(void *argument)
{
  SensorMsg_t msg;
  msg.type = SENSOR_TYPE_PH;
  pH_Sensor_Init();
  for(;;) {
    float val = pH_Sensor_Read(&hadc1);
    if (val >= 0.0f) {
      msg.value = val;
      msg.timestamp = xTaskGetTickCount();
      osMessageQueuePut(SensorDataQueueHandle, &msg, 0, 0);
    }
    osDelay(2000);
  }
}

void StartECSensorTask(void *argument)
{
  SensorMsg_t msg;
  msg.type = SENSOR_TYPE_EC;
  EC_Sensor_Init();
  for(;;) {
    float val = EC_Sensor_Read(&hadc1);
    if (val >= 0.0f) {
      msg.value = val;
      msg.timestamp = xTaskGetTickCount();
      osMessageQueuePut(SensorDataQueueHandle, &msg, 0, 0);
    }
    osDelay(2000);
  }
}

void StartTurbiditySensorTask(void *argument)
{
  SensorMsg_t msg;
  msg.type = SENSOR_TYPE_TURBIDITY;
  for(;;) {
    float adc_voltage = Read_Turbidity_Voltage();
    if (adc_voltage >= 0.0f) {
      float sensor_voltage = adc_voltage * 1.47f;
      float ntu = (-1120.4f * sensor_voltage * sensor_voltage) + (5742.3f * sensor_voltage) - 4352.9f;
      if (ntu < 0.0f) ntu = 0.0f;
      msg.value = ntu;
      msg.timestamp = xTaskGetTickCount();
      osMessageQueuePut(SensorDataQueueHandle, &msg, 0, 0);
    }
    osDelay(2000);
  }
}

void StartThermalTaskTask(void *argument)
{
  float received_temp = 0.0f;
  const float target_temp_Low  = 21.0f;
  const float target_temp_High = 24.0f;
  uint8_t heater_state = 0;
  for(;;) {
    if (osMessageQueueGet(TempQueueHandle, &received_temp, NULL, osWaitForever) == osOK) {
      if (received_temp < 5.0f || received_temp > 45.0f) continue;
      if (received_temp < target_temp_Low && heater_state == 0) {
        HAL_GPIO_WritePin(HeaterRelay_GPIO_Port, HeaterRelay_Pin, GPIO_PIN_SET);
        heater_state = 1;
      } else if (received_temp > target_temp_High && heater_state == 1) {
        HAL_GPIO_WritePin(HeaterRelay_GPIO_Port, HeaterRelay_Pin, GPIO_PIN_RESET);
        heater_state = 0;
      }
    }
  }
}

void StartCommsTask(void *argument)
{
  SensorMsg_t rx_msg;
  SystemTelemetry_t telemetry_cache = {
      .temp_c = 0.0f, .water_level_cm = 0.0f, .turbidity_ntu = 0.0f,
      .ph_val = 7.0f, .ec_val = 1.2f
  };

  uint8_t last_batt_state = 0xFF;
  uint8_t rx_token = 0;
  static char report_buf[512];
  static char warnings_buf[128];
  uint32_t last_tx_tick = 0;

  uint8_t fish_fed_today = 0;
  static char last_fed_time[20] = "Never";
  static char fish_activity[12] = "Normal";
  uint8_t batt_power = 0;
  uint8_t lora_online = LoRa_Init();
  uint8_t tx_status = 0;

  if (lora_online) Downlink_ArmRx();

  ESP32_Init();

  for(;;)
  {
    batt_power = (HAL_GPIO_ReadPin(PowerCheck_GPIO_Port, PowerCheck_Pin) == GPIO_PIN_RESET) ? 1 : 0;

    if (batt_power != last_batt_state)
    {
      last_batt_state = batt_power;
      if (batt_power == 1) {
        uint8_t sleep_token = 0xAA;
        HAL_UART_Transmit(&huart1, &sleep_token, 1, 50);
        if (lora_online) LoRa_Sleep();
      } else {
        uint8_t wake_token = 0x55;
        HAL_UART_Transmit(&huart1, &wake_token, 1, 50);
        if (lora_online) Downlink_ArmRx();
      }
    }

    if (ESP32_ReadToken(&rx_token, 20))
    {
      switch (rx_token) {
        case TOKEN_HUNGRY:
          fish_fed_today = 1;
          snprintf(last_fed_time, sizeof(last_fed_time), "%lu sec ago", (xTaskGetTickCount() / 1000));
          snprintf(fish_activity, sizeof(fish_activity), "Feeding");
          break;
        case TOKEN_ANOMALY:
          snprintf(fish_activity, sizeof(fish_activity), "Abnormal");
          break;
        case TOKEN_NORMAL:
        default:
          snprintf(fish_activity, sizeof(fish_activity), "Normal");
          break;
      }
      rx_token = 0;
    }

    while (osMessageQueueGet(SensorDataQueueHandle, &rx_msg, NULL, 0) == osOK)
    {
      switch (rx_msg.type) {
        case SENSOR_TYPE_TEMP:      telemetry_cache.temp_c         = rx_msg.value; break;
        case SENSOR_TYPE_WATER_LVL: telemetry_cache.water_level_cm = rx_msg.value; break;
        case SENSOR_TYPE_TURBIDITY: telemetry_cache.turbidity_ntu  = rx_msg.value; break;
        case SENSOR_TYPE_PH:        telemetry_cache.ph_val         = rx_msg.value; break;
        case SENSOR_TYPE_EC:        telemetry_cache.ec_val         = rx_msg.value; break;
        default: break;
      }
    }

    if (lora_online && batt_power == 0) {
      Downlink_Poll();
      }


    if ((xTaskGetTickCount() - last_tx_tick) >= pdMS_TO_TICKS(1000))
    {
      last_tx_tick = xTaskGetTickCount();
      tx_status = 0;

      if (lora_online && batt_power == 0)
      {
        static char lora_pkt[220];
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
            telemetry_cache.temp_c, telemetry_cache.water_level_cm,
            telemetry_cache.ph_val, telemetry_cache.ec_val,
            telemetry_cache.turbidity_ntu, fish_activity);

        if (lora_len < 0) lora_len = 0;
        else if (lora_len >= (int)sizeof(lora_pkt)) lora_len = sizeof(lora_pkt) - 1;

        tx_status = LoRa_Transmit((uint8_t*)lora_pkt, lora_len);
        Downlink_ArmRx();
      }

      uint8_t pwr_sync = (batt_power == 1) ? 0xAA : 0x55;
      HAL_UART_Transmit(&huart1, &pwr_sync, 1, 20);

      warnings_buf[0] = '\0';
      if (batt_power == 1)                                                strcat(warnings_buf, "BATTERY POWER ACTIVE; ");
      if (telemetry_cache.water_level_cm < 10.0f && telemetry_cache.water_level_cm > 0.0f)
                                                                          strcat(warnings_buf, "Low Water LVL; ");
      if (telemetry_cache.temp_c <= -900.0f || telemetry_cache.temp_c > 45.0f)
                                                                          strcat(warnings_buf, "CHECK Temp Sensor; ");
      if (telemetry_cache.ph_val < 5.5f || telemetry_cache.ph_val > 8.5f)
                                                                          strcat(warnings_buf, "CHECK pH Sensor; ");
      if (telemetry_cache.ec_val < 0.2f || telemetry_cache.ec_val > 3.0f)
                                                                          strcat(warnings_buf, "CHECK EC Sensor; ");
      if (telemetry_cache.turbidity_ntu > 100.0f)                         strcat(warnings_buf, "CHECK Turbidity Sensor; ");
      if (strlen(warnings_buf) == 0)                                      strcpy(warnings_buf, "None");

      char lora_hw_stat[32];
      if (!lora_online)                     strcpy(lora_hw_stat, "SPI2 Init FAILED (0x00)");
      else if (batt_power == 1)             strcpy(lora_hw_stat, "Asleep (Battery Save)");
      else if (tx_status == 1)              strcpy(lora_hw_stat, "Transmitted (OK)");
      else                                  strcpy(lora_hw_stat, "TX Timeout");

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
          "Pumps: %u%%\r\n\r\n",
          (batt_power == 0) ? "Mains" : "BATTERY",
          warnings_buf,
          telemetry_cache.temp_c, telemetry_cache.water_level_cm,
          telemetry_cache.ph_val, telemetry_cache.ec_val,
          telemetry_cache.turbidity_ntu,
          (batt_power == 0) ? "Online" : "Sleep",
          lora_hw_stat,
          fish_activity,
          (fish_fed_today == 1) ? "Yes" : "No",
          last_fed_time,
          (unsigned)g_pump_speed_percent);

      CDC_Transmit_FS((uint8_t*)report_buf, len);
    }

    osDelay(50);
  }
}

void StartServoTask(void *argument)
{
	Servo_Init();

	  /* Boot: 180 degrees clockwise to reference the paddle.
	   * This runs once when the system powers up. */
	  Servo_Rotate_For(BOOT_SPEED, BOOT_ROTATE_MS);
	  osDelay(200);

	  uint32_t notify_val = 0;
  for(;;) {
	  if (xTaskNotifyWait(0x00, 0xFFFFFFFF, &notify_val, portMAX_DELAY) != pdTRUE) {
	       continue;
	     }

	     if (notify_val & 0x01) {
	       /* Every feed: 90 degrees clockwise. No reversal, ever. */
	       Servo_Rotate_For(FEED_SPEED, FEED_ROTATE_MS);
	     }

	     notify_val = 0;
  }
}

void StartPumpTask(void *argument)
{
  Pump_Init();
  PumpState_t current_state = PUMP_STATE_NORMAL;

  const uint32_t FIVE_MINUTES_TICKS = pdMS_TO_TICKS(5 * 60 * 1000);
  uint32_t state_entry_tick = xTaskGetTickCount();
  uint8_t is_manual_override = 0;
  uint8_t in_feeding_cycle = 0;

  PumpEvent_t rx_event = PUMP_EVT_NONE;

  for(;;) {
    if (osMessageQueueGet(PumpCommandQueueHandle, &rx_event, NULL, 200) == osOK) {
      state_entry_tick = xTaskGetTickCount();
      switch (rx_event) {
        case PUMP_EVT_FISH_FED:
        	if (!is_manual_override) {
        	        current_state = PUMP_STATE_HIGH;
        	        Pump_Set_Speed(100);
        	        in_feeding_cycle = 1;
        	    }
        	    break;
        case PUMP_EVT_OVERRIDE_HIGH: current_state = PUMP_STATE_HIGH;
				Pump_Set_Speed(100); is_manual_override = 1;
				in_feeding_cycle = 0;
				break;
        case PUMP_EVT_OVERRIDE_MED:  current_state = PUMP_STATE_MEDIUM;
				Pump_Set_Speed(80);  is_manual_override = 1;
				in_feeding_cycle = 0;
				break;
        case PUMP_EVT_OVERRIDE_LOW:  current_state = PUMP_STATE_NORMAL;
				Pump_Set_Speed(60);  is_manual_override = 1;
				in_feeding_cycle = 0;
				break;
        default:
        		break;
      }
    }

    uint32_t elapsed = xTaskGetTickCount() - state_entry_tick;

    if (in_feeding_cycle) {
      if (current_state == PUMP_STATE_HIGH && elapsed >= FIVE_MINUTES_TICKS) {
        current_state = PUMP_STATE_MEDIUM; Pump_Set_Speed(80); state_entry_tick = xTaskGetTickCount();
      } else if (current_state == PUMP_STATE_MEDIUM && elapsed >= FIVE_MINUTES_TICKS) {
        current_state = PUMP_STATE_NORMAL; Pump_Set_Speed(60); in_feeding_cycle = 0;
      }
    }

    if (is_manual_override) {
      if (elapsed >= FIVE_MINUTES_TICKS) {
        current_state = PUMP_STATE_NORMAL; Pump_Set_Speed(60); is_manual_override = 0;
      }
    }
  }
}
