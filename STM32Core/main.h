/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */
typedef enum {
    SENSOR_TYPE_TEMP = 0,
    SENSOR_TYPE_WATER_LVL,
    SENSOR_TYPE_TURBIDITY,
    SENSOR_TYPE_PH,
    SENSOR_TYPE_EC
} SensorType_t;

typedef struct {
    SensorType_t type;
    float        value;
    uint32_t     timestamp;
} SensorMsg_t;

typedef struct {
    float temp_c;
    float water_level_cm;
    float turbidity_ntu;
    float ph_val;
    float ec_val;
} SystemTelemetry_t;
/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define ledPin_Pin GPIO_PIN_13
#define ledPin_GPIO_Port GPIOC
#define Turbidity_Pin GPIO_PIN_0
#define Turbidity_GPIO_Port GPIOA
#define TempSensorPin_Pin GPIO_PIN_1
#define TempSensorPin_GPIO_Port GPIOA
#define Trig_Pin GPIO_PIN_2
#define Trig_GPIO_Port GPIOA
#define Echo_Pin GPIO_PIN_3
#define Echo_GPIO_Port GPIOA
#define PHSensor_Pin GPIO_PIN_4
#define PHSensor_GPIO_Port GPIOA
#define ECSensor_Pin GPIO_PIN_5
#define ECSensor_GPIO_Port GPIOA
#define Servo_PWM_Pin GPIO_PIN_6
#define Servo_PWM_GPIO_Port GPIOA
#define PowerCheck_Pin GPIO_PIN_7
#define PowerCheck_GPIO_Port GPIOA
#define HeaterRelay_Pin GPIO_PIN_0
#define HeaterRelay_GPIO_Port GPIOB
#define LoRa_CS_Pin GPIO_PIN_12
#define LoRa_CS_GPIO_Port GPIOB
#define LoRa_Rst_Pin GPIO_PIN_5
#define LoRa_Rst_GPIO_Port GPIOB
#define LoRa_D100_Pin GPIO_PIN_8
#define LoRa_D100_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
