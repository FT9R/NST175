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

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */
#define ARRAY_LENGTH(ARRAY) (sizeof((ARRAY)) / sizeof(*(ARRAY)))

#define LEDG_ON  HAL_GPIO_WritePin(LEDG_GPIO_Port, LEDG_Pin, GPIO_PIN_RESET)
#define LEDG_OFF HAL_GPIO_WritePin(LEDG_GPIO_Port, LEDG_Pin, GPIO_PIN_SET)
#define LEDG_TOG HAL_GPIO_TogglePin(LEDG_GPIO_Port, LEDG_Pin)
#define LEDY_ON  HAL_GPIO_WritePin(LEDY_GPIO_Port, LEDY_Pin, GPIO_PIN_RESET)
#define LEDY_OFF HAL_GPIO_WritePin(LEDY_GPIO_Port, LEDY_Pin, GPIO_PIN_SET)
#define LEDY_TOG HAL_GPIO_TogglePin(LEDY_GPIO_Port, LEDY_Pin)
#define LEDR_ON  HAL_GPIO_WritePin(LEDR_GPIO_Port, LEDR_Pin, GPIO_PIN_RESET)
#define LEDR_OFF HAL_GPIO_WritePin(LEDR_GPIO_Port, LEDR_Pin, GPIO_PIN_SET)
#define LEDR_TOG HAL_GPIO_TogglePin(LEDR_GPIO_Port, LEDR_Pin)

#define NST175_GET_INT HAL_GPIO_ReadPin(NST175_ALERT_GPIO_Port, NST175_ALERT_Pin)
/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define LEDG_Pin GPIO_PIN_13
#define LEDG_GPIO_Port GPIOC
#define LEDY_Pin GPIO_PIN_14
#define LEDY_GPIO_Port GPIOC
#define LEDR_Pin GPIO_PIN_15
#define LEDR_GPIO_Port GPIOC
#define NST175_ALERT_Pin GPIO_PIN_7
#define NST175_ALERT_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
