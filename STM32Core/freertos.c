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
#include <string.h>
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
#include "rtc.h"            /* [FIX] for HAL_RTC_SetAlarm_IT */
/* USER CODE END Includes */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
extern TIM_HandleTypeDef htim2;
extern ADC_HandleTypeDef hadc1;
extern UART_HandleTypeDef huart1;
extern RTC_HandleTypeDef hrtc;          /* [FIX] RTC handle */

volatile float g_current_temperature = 25.0f;

/* [FIX] RTC alarm re-arm state — every 24 h from the moment of arming */
static RTC_TimeTypeDef s_alarm_time;
static RTC_DateTypeDef s_alarm_date;
/* USER CODE END Variables */

/* Task handles --------------------------------------------------------------*/
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask", .stack_size = 128 * 4, .priority = (osPriority_t) osPriorityLow,
};
osThreadId_t TempSensorHandle;
const osThreadAttr_t TempSensor_attributes = {
  .name = "TempSensor", .stack_size = 256 * 4, .priority = (osPriority_t) osPriorityAboveNormal, /* [FIX] */
};
osThreadId_t WaterLvlSensorHandle;
const osThreadAttr_t WaterLvlSensor_attributes = {
  .name = "WaterLvlSensor", .stack_size = 256 * 4, .priority = (osPriority_t) osPriorityNormal,
};
osThreadId_t PHSensorHandle;
const osThreadAttr_t PHSensor_attributes = {
  .name = "PHSensor", .stack_size = 256 * 4, .priority = (osPriority_t) osPriorityBelowNormal, /* [FIX] */
};
osThreadId_t ECSensorHandle;
const osThreadAttr_t ECSensor_attributes = {
  .name = "ECSensor", .stack_size = 256 * 4, .priority = (osPriority_t) osPriorityBelowNormal, /* [FIX] */
};
osThreadId_t TurbiditySensorHandle;
const osThreadAttr_t TurbiditySensor_attributes = {
  .name = "TurbiditySensor", .stack_size = 256 * 4, .priority = (osPriority_t) osPriorityBelowNormal, /* [FIX] */
};
osThreadId_t ThermalTaskHandle;
const osThreadAttr_t ThermalTask_attributes = {
  .name = "ThermalTask", .stack_size = 256 * 4, .priority = (osPriority_t) osPriorityHigh, /* [FIX] */
};
osThreadId_t CommsHandle;
const osThreadAttr_t Comms_attributes = {
  .name = "Comms", .stack_size = 512 * 4, .priority = (osPriority_t) osPriorityAboveNormal, /* [FIX] */
};
osThreadId_t ServoHandle;
const osThreadAttr_t Servo_attributes = {
  .name = "Servo", .stack_size = 256 * 4, .priority = (osPriority_t) osPriorityNormal,
};
osThreadId_t PumpHandle;
const osThreadAttr_t Pump_attributes = {
  .name = "Pump", .stack_size = 256 * 4, .priority = (osPriority_t) osPriorityAboveNormal, /* [FIX] */
};
osThreadId_t RtcFeedHandle;                  /* [FIX] new task */
const osThreadAttr_t RtcFeed_attributes = {
  .name = "RtcFeed", .stack_size = 256 * 4, .priority = (osPriority_t) osPriorityNormal,
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

static void Rtc_ArmNextAlarm(void);            /* [FIX] */
/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);
void StartTempSensorTask(void *argument);
void StartWaterLvlSensorTask(void *argument);
void StartPHSensorTask(void *argument);
void StartECSensorTask(void *argument);
void StartTurbiditySensorTask(void *argument);
void StartThermalTask(void *argument);         /* [FIX] was StartThermalTaskTask */
void StartCommsTask(void *argument);
void StartServoTask(void *argument);
void StartPumpTask(void *argument);
void StartRtcFeedTask(void *argument);         /* [FIX] new */

extern void MX_USB_DEVICE_Init(void);
void MX_FREERTOS_Init(void);

void MX_FREERTOS_Init(void) {
  /* [FIX] queue item sizes now match the types actually sent/read */
  SensorDataQueueHandle   = osMessageQueueNew(16, sizeof(SensorMsg_t),  &SensorDataQueue_attributes);
  TempQueueHandle         = osMessageQueueNew(16, sizeof(float),        &TempQueue_attributes);
  PumpCommandQueueHandle  = osMessageQueueNew(8,  sizeof(PumpEvent_t),  &PumpCommandQueue_attributes);

  defaultTaskHandle        = osThreadNew(StartDefaultTask,        NULL, &defaultTask_attributes);
  TempSensorHandle         = osThreadNew(StartTempSensorTask,     NULL, &TempSensor_attributes);
  WaterLvlSensorHandle     = osThreadNew(StartWaterLvlSensorTask, NULL, &WaterLvlSensor_attributes);
  PHSensorHandle           = osThreadNew(StartPHSensorTask,       NULL, &PHSensor_attributes);
  ECSensorHandle           = osThreadNew(StartECSensorTask,       NULL, &ECSensor_attributes);
  TurbiditySensorHandle    = osThreadNew(StartTurbiditySensorTask,NULL, &TurbiditySensor_attributes);
  ThermalTaskHandle        = osThreadNew(StartThermalTask,        NULL, &ThermalTask_attributes);
  CommsHandle              = osThreadNew(StartCommsTask,          NULL, &Comms_attributes);
  ServoHandle              = osThreadNew(StartServoTask,          NULL, &Servo_attributes);
  PumpHandle               = osThreadNew(StartPumpTask,           NULL, &Pump_attributes);
  RtcFeedHandle            = osThreadNew(StartRtcFeedTask,        NULL, &RtcFeed_attributes);

  /* [FIX] arm the first daily RTC alarm */
  Rtc_ArmNextAlarm();
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
  extern volatile float g_current_temperature;
  for(;;) {
    float current_temp = Read_Temperature();
    if (current_temp > -900.0f) {
      g_current_temperature = current_temp;
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
  SensorMsg_t msg; msg.type = SENSOR_TYPE_WATER_LVL;
  for(;;) {
    msg.value = Read_Water_Level_CM();
    msg.timestamp = xTaskGetTickCount();
    osMessageQueuePut(SensorDataQueueHandle, &msg, 0, 0);
    osDelay(1000);
  }
}

void StartPHSensorTask(void *argument)
{
  SensorMsg_t msg; msg.type = SENSOR_TYPE_PH;
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
  SensorMsg_t msg; msg.type = SENSOR_TYPE_EC;
  EC_Sensor_Init();
  extern volatile float g_current_temperature;
  for(;;) {
    float val = EC_Sensor_Read(&hadc1, g_current_temperature);
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
  SensorMsg_t msg; msg.type = SENSOR_TYPE_TURBIDITY;
  for(;;) {
    ADC_ChannelConfTypeDef sConfig = {0};
    sConfig.Channel = ADC_CHANNEL_0;
    sConfig.Rank = 1;
    sConfig.SamplingTime = ADC_SAMPLETIME_56CYCLES;
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);
    HAL_ADC_Start(&hadc1);
    HAL_ADC_PollForConversion(&hadc1, 100);
    uint32_t raw = HAL_ADC_GetValue(&hadc1);
    HAL_ADC_Stop(&hadc1);

    float ntu = ((float)raw * -0.9126f) + 3148.5f;
    if (ntu < 0.0f)    ntu = 0.0f;
    if (ntu > 3000.0f) ntu = 3000.0f;

    msg.value = ntu;
    msg.timestamp = xTaskGetTickCount();
    osMessageQueuePut(SensorDataQueueHandle, &msg, 0, 0);
    osDelay(2000);
  }
}

/* [FIX] renamed from StartThermalTaskTask */
void StartThermalTask(void *argument)
{
  float received_temp = 0.0f;
  const float T_LOW  = 21.0f;
  const float T_HIGH = 24.0f;
  uint8_t heater_state = 0;

  for(;;) {
    if (osMessageQueueGet(TempQueueHandle, &received_temp, NULL, osWaitForever) == osOK) {
      if (received_temp < 5.0f || received_temp > 45.0f) continue;

      if (received_temp < T_LOW && heater_state == 0) {
        HAL_GPIO_WritePin(HeaterRelay_GPIO_Port, HeaterRelay_Pin, GPIO_PIN_SET);
        heater_state = 1;
      } else if (received_temp > T_HIGH && heater_state == 1) {
        HAL_GPIO_WritePin(HeaterRelay_GPIO_Port, HeaterRelay_Pin, GPIO_PIN_RESET);
        heater_state = 0;
      }
      /* [NOTE] On battery the report says the heater relay opens.
         If you want to enforce that here, add:
         if (on_battery && heater_state) { relay off; heater_state = 0; }
         But since the heater is not a battery load (Section 3.3.2),
         leave the relay driver unpowered in hardware — no firmware
         change needed. */
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

  #define HUNGRY_PERSIST_COUNT   3
  #define MIN_FEED_INTERVAL_MS   (4UL * 60UL * 60UL * 1000UL)
  #define FEED_COOLDOWN_MS       (30UL * 60UL * 1000UL)
  #define MAX_FEEDS_PER_DAY      3
  #define DAY_MS                 (24UL * 60UL * 60UL * 1000UL)

  uint8_t  hungry_streak    = 0;
  uint32_t last_feed_tick   = 0;
  uint32_t last_day_tick    = xTaskGetTickCount();
  uint8_t  feeds_today      = 0;

  if (lora_online) Downlink_ArmRx();
  ESP32_Init();

  for(;;) {
    uint32_t now = xTaskGetTickCount();

    /* [FIX] daily reset — note this only resets on Comms-tick boundary; the
       RTC path also resets feeds_today when its own day flips. */
    if ((now - last_day_tick) >= DAY_MS) {
      last_day_tick = now;
      feeds_today   = 0;
      fish_fed_today = 0;
    }

    batt_power = (HAL_GPIO_ReadPin(PowerCheck_GPIO_Port, PowerCheck_Pin) == GPIO_PIN_RESET) ? 1 : 0;

    if (batt_power != last_batt_state) {
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

    /* Vision tokens: mains only (Section 3.3.2 — behaviour-triggered feeding
       is disabled while on battery) */
    if (batt_power == 0 && ESP32_ReadToken(&rx_token, 20)) {
        switch (rx_token) {
          case TOKEN_HUNGRY: {
            hungry_streak++;
            if (hungry_streak >= HUNGRY_PERSIST_COUNT) {
              hungry_streak = 0;
              if (RtcFeedHandle != NULL) {
                xTaskNotify((TaskHandle_t)RtcFeedHandle, 0x02, eSetBits);
              }
              snprintf(fish_activity, sizeof(fish_activity), "Requesting feed");
            }
            break;
          }
          case TOKEN_NIGHT:
            /* Darkness — never feed, and do not accumulate a hungry streak */
            hungry_streak = 0;
            snprintf(fish_activity, sizeof(fish_activity), "Night");
            break;
          case TOKEN_NORMAL:
          default:
            hungry_streak = 0;
            snprintf(fish_activity, sizeof(fish_activity), "Normal");
            break;
        }
      rx_token = 0;
    }

    while (osMessageQueueGet(SensorDataQueueHandle, &rx_msg, NULL, 0) == osOK) {
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

    if ((now - last_tx_tick) >= pdMS_TO_TICKS(1000)) {
      last_tx_tick = now;
      tx_status = 0;

      if (lora_online && batt_power == 0) {
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
      if (!lora_online)          strcpy(lora_hw_stat, "SPI2 Init FAILED (0x00)");
      else if (batt_power == 1)  strcpy(lora_hw_stat, "Asleep (Battery Save)");
      else if (tx_status == 1)   strcpy(lora_hw_stat, "Transmitted (OK)");
      else                       strcpy(lora_hw_stat, "TX Timeout");

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
          "Feeds Today: %u\r\n"
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
          (unsigned)feeds_today,
          (unsigned)g_pump_speed_percent);

      CDC_Transmit_FS((uint8_t*)report_buf, len);
    }

    osDelay(50);
  }
}

void StartServoTask(void *argument)
{
  Servo_Init();
  Servo_BootAlign();
  osDelay(200);

  uint32_t notify_val = 0;
  for(;;) {
    if (xTaskNotifyWait(0x00, 0xFFFFFFFF, &notify_val, portMAX_DELAY) != pdTRUE) {
      continue;
    }
    if (notify_val & 0x01) {
      Servo_Feed();
      ulTaskNotifyTake(pdTRUE, 0);     /* drain duplicates */
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
  uint8_t in_feeding_cycle   = 0;

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
        case PUMP_EVT_OVERRIDE_HIGH:
          current_state = PUMP_STATE_HIGH;  Pump_Set_Speed(100);
          is_manual_override = 1; in_feeding_cycle = 0; break;
        case PUMP_EVT_OVERRIDE_MED:
          current_state = PUMP_STATE_MEDIUM; Pump_Set_Speed(80);
          is_manual_override = 1; in_feeding_cycle = 0; break;
        case PUMP_EVT_OVERRIDE_LOW:
          current_state = PUMP_STATE_NORMAL; Pump_Set_Speed(66);
          is_manual_override = 1; in_feeding_cycle = 0; break;
        default: break;
      }
    }

    uint32_t elapsed = xTaskGetTickCount() - state_entry_tick;

    if (in_feeding_cycle) {
      if (current_state == PUMP_STATE_HIGH && elapsed >= FIVE_MINUTES_TICKS) {
        current_state = PUMP_STATE_MEDIUM; Pump_Set_Speed(80);
        state_entry_tick = xTaskGetTickCount();
      } else if (current_state == PUMP_STATE_MEDIUM && elapsed >= FIVE_MINUTES_TICKS) {
        current_state = PUMP_STATE_NORMAL; Pump_Set_Speed(66);
        in_feeding_cycle = 0;
      }
    }

    if (is_manual_override && elapsed >= FIVE_MINUTES_TICKS) {
      current_state = PUMP_STATE_NORMAL; Pump_Set_Speed(66);
      is_manual_override = 0;
    }
  }
}

/* ============================================================================
 *  [FIX] RTC daily feed task + alarm callback
 *
 *  Replaces "Now" with "arm at next feed time", then the ISR fires
 *  HAL_RTC_AlarmAEventCallback which notifies this task. The task runs
 *  the feeding policy (4 h min interval, 30 min cooldown, 3/day cap,
 *  mains-or-battery permitted) and then re-arms the alarm for +24 h.
 *
 *  This matches Figure 3.7 in the report: RTC alarm -> feeding policy
 *  -> Servo_Feed -> PUMP_EVT_FISH_FED.
 * ========================================================================= */

/* Feed time — choose whatever you like, e.g. 08:00:00 */
#define FEED_HOUR   8
#define FEED_MIN    0
#define FEED_SEC    0

static void Rtc_ArmNextAlarm(void)
{
  RTC_TimeTypeDef t = {0};
  RTC_DateTypeDef d = {0};
  HAL_RTC_GetTime(&hrtc, &t, RTC_FORMAT_BIN);
  HAL_RTC_GetDate(&hrtc, &d, RTC_FORMAT_BIN);

  RTC_AlarmTypeDef a = {0};
  a.AlarmTime.Hours   = FEED_HOUR;
  a.AlarmTime.Minutes = FEED_MIN;
  a.AlarmTime.Seconds = FEED_SEC;
  a.AlarmTime.SubSeconds = 0;
  a.AlarmTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
  a.AlarmTime.StoreOperation = RTC_STOREOPERATION_RESET;
  /* Mask date so the alarm fires every day at the set time */
  a.AlarmMask = RTC_ALARMMASK_DATEWEEKDAY;
  a.AlarmSubSecondMask = RTC_ALARMSUBSECONDMASK_ALL;
  a.AlarmDateWeekDaySel = RTC_ALARMDATEWEEKDAYSEL_DATE;
  a.AlarmDateWeekDay = 1;
  a.Alarm = RTC_ALARM_A;

  HAL_RTC_SetAlarm_IT(&hrtc, &a, RTC_FORMAT_BIN);

  s_alarm_time = t;   /* remember for diagnostics if you want */
  s_alarm_date = d;
}

void StartRtcFeedTask(void *argument)
{
  /* Shared counters are local to CommsTask in your original design,
     but the report's Figure 3.7 puts the feeding policy in one place.
     Keep the policy state here so both the RTC and (optionally) the
     Comms vision path can consult it. For now, mirror Comms's policy. */

  #define RTC_MIN_FEED_INTERVAL_MS   (4UL * 60UL * 60UL * 1000UL)
  #define RTC_FEED_COOLDOWN_MS       (30UL * 60UL * 1000UL)
  #define RTC_MAX_FEEDS_PER_DAY      3

  uint32_t rtc_last_feed_tick  = 0;
  uint32_t rtc_last_day_tick   = xTaskGetTickCount();
  uint8_t  rtc_feeds_today     = 0;

  for(;;) {
    /* Wait forever — the ISR will send a notification */
    uint32_t notif = 0;
    xTaskNotifyWait(0x00, 0xFFFFFFFF, &notif, portMAX_DELAY);
    if ((notif & 0x01) == 0) continue;

    uint32_t now = xTaskGetTickCount();

    /* Daily reset */
    if ((now - rtc_last_day_tick) >= pdMS_TO_TICKS(24UL * 60UL * 60UL * 1000UL)) {
      rtc_last_day_tick = now;
      rtc_feeds_today   = 0;
    }

    uint32_t since_last = (rtc_last_feed_tick == 0)
                          ? RTC_MIN_FEED_INTERVAL_MS
                          : (now - rtc_last_feed_tick);
    uint8_t  cooldown_ok = (rtc_last_feed_tick == 0) ||
                           ((now - rtc_last_feed_tick) >= RTC_FEED_COOLDOWN_MS);

    if (since_last >= RTC_MIN_FEED_INTERVAL_MS &&
        rtc_feeds_today < RTC_MAX_FEEDS_PER_DAY &&
        cooldown_ok)
    {
      rtc_last_feed_tick = now;
      rtc_feeds_today++;

      xTaskNotify((TaskHandle_t)ServoHandle, 0x01, eSetBits);
      Pump_Send_Event(PUMP_EVT_FISH_FED);
    }

    /* Re-arm the alarm for +24 h (next day, same time) */
    Rtc_ArmNextAlarm();
  }
}

/* [FIX] RTC Alarm ISR — must not call Servo_Feed directly */
void HAL_RTC_AlarmAEventCallback(RTC_HandleTypeDef *hrtc_arg)
{
  BaseType_t hp = pdFALSE;
  if (RtcFeedHandle != NULL) {
    xTaskNotifyFromISR((TaskHandle_t)RtcFeedHandle, 0x01, eSetBits, &hp);
  }
  portYIELD_FROM_ISR(hp);
}
