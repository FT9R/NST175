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
#include "cmsis_os.h"
#include "main.h"
#include "task.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "nst175_ifc.h"
#include "usart.h"
#include <stdio.h>
#include <string.h>
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

/* USER CODE END Variables */
/* Definitions for i2cTask */
osThreadId_t i2cTaskHandle;
const osThreadAttr_t i2cTask_attributes = {
    .name = "i2cTask",
    .stack_size = 128 * 4,
    .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for ledTask */
osThreadId_t ledTaskHandle;
const osThreadAttr_t ledTask_attributes = {
    .name = "ledTask",
    .stack_size = 128 * 4,
    .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for tempQueue */
osMessageQueueId_t tempQueueHandle;
const osMessageQueueAttr_t tempQueue_attributes = {.name = "tempQueue"};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void I2C_Task(void *argument);
void LED_Task(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
 * @brief  FreeRTOS initialization
 * @param  None
 * @retval None
 */
void MX_FREERTOS_Init(void)
{
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
    /* creation of tempQueue */
    tempQueueHandle = osMessageQueueNew(16, sizeof(float), &tempQueue_attributes);

    /* USER CODE BEGIN RTOS_QUEUES */
    /* add queues, ... */
    /* USER CODE END RTOS_QUEUES */

    /* Create the thread(s) */
    /* creation of i2cTask */
    i2cTaskHandle = osThreadNew(I2C_Task, NULL, &i2cTask_attributes);

    /* creation of ledTask */
    ledTaskHandle = osThreadNew(LED_Task, NULL, &ledTask_attributes);

    /* USER CODE BEGIN RTOS_THREADS */
    /* add threads, ... */
    /* USER CODE END RTOS_THREADS */

    /* USER CODE BEGIN RTOS_EVENTS */
    /* add events, ... */
    /* USER CODE END RTOS_EVENTS */
}

/* USER CODE BEGIN Header_I2C_Task */
/**
 * @brief  Function implementing the i2cTask thread.
 * @param  argument: Not used
 * @retval None
 */
/* USER CODE END Header_I2C_Task */
void I2C_Task(void *argument)
{
    /* USER CODE BEGIN I2C_Task */
    static nst175_t tempSensor;
    static float tempFiltered;
    float temp = 0;
    const float alphaTemp = 0.05;
    char plotString[50] = {0};

    NST175_SetUp(&tempSensor);
    /* Infinite loop */
    for (;;) {
        NST175_TemperatureGet(&tempSensor, &temp, 0);
        tempFiltered = alphaTemp * temp + (1 - alphaTemp) * tempFiltered;
        osMessageQueuePut(tempQueueHandle, &tempFiltered, 0, 0);

        /* Plot measured values */
        snprintf(plotString, sizeof(plotString), "%.2f\n", (double) tempFiltered);
        HAL_UART_Transmit(&huart4, (const uint8_t *) plotString, strnlen(plotString, sizeof(plotString)), 1000);

        /* - In continuous-conversion (default) mode wait between samples.
         * - In shutdown (low power) mode, one-shot measurement is automatically triggered and sample ready flag is
         * checked within `NST175_TemperatureGet()` applying timeout provided in args
         */
        osDelay(NST175_CONVERSION_TIME);
    }
    /* USER CODE END I2C_Task */
}

/* USER CODE BEGIN Header_LED_Task */
/**
 * @brief Function implementing the ledTask thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_LED_Task */
void LED_Task(void *argument)
{
    /* USER CODE BEGIN LED_Task */
    float temperatureReadings;
    /* Startup traffic light */
    for (uint8_t i = 0; i < 5; i++) {
        LEDR_ON, LEDY_ON, LEDG_ON, osDelay(100);
        LEDR_OFF, LEDY_OFF, LEDG_OFF, osDelay(100);
    }
    /* Infinite loop */
    for (;;) {
        osMessageQueueGet(tempQueueHandle, &temperatureReadings, NULL, osWaitForever);
        if (temperatureReadings > 80)
            LEDR_ON, LEDY_OFF, LEDG_OFF;
        else if (temperatureReadings > 40)
            LEDR_OFF, LEDY_ON, LEDG_OFF;
        else
            LEDR_OFF, LEDY_OFF, LEDG_ON;
    }
    /* USER CODE END LED_Task */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */
