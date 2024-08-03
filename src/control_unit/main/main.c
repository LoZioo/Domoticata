/* USER CODE BEGIN Header */
/**
	******************************************************************************
	* @file           : main.c
	* @brief          : main program body.
	******************************************************************************
	* @attention
	*
	* Copyright (c) [2024] Davide Scalisi *
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
/* USER CODE BEGIN Includes */

// Standard libraries.
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

// Platform libraries.
#include <esp_err.h>
#include <esp_check.h>
#include <esp_log.h>

#include <freertos/FreeRTOS.h>

#include <driver/gpio.h>
#include <driver/uart.h>

// UniLibC libraries.
#include <ul_errors.h>
#include <ul_utils.h>

// Project libraries.
#include <setup.h>

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

#define delay(ms) \
	vTaskDelay(pdMS_TO_TICKS(ms))

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

static const char *TAG = "main";

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

esp_err_t UART_poll();

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
 * @brief App entry point.
*/
void app_main(){

	/* MCU Configuration--------------------------------------------------------*/
	/* USER CODE BEGIN SysInit */

	ESP_LOGI(TAG, "GPIO_setup()");
	ESP_ERROR_CHECK(GPIO_setup());

	ESP_LOGI(TAG, "UART_setup()");
	ESP_ERROR_CHECK(UART_setup());

	/* USER CODE END SysInit */

	/* USER CODE BEGIN Init */

	/* USER CODE END Init */

	/* USER CODE BEGIN 1 */

	ESP_LOGI(TAG, "polling slaves");

	/* Infinite loop */
	for(;;){
		UART_poll();
		delay(1);
	}
	/* USER CODE END 1 */
}

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 2 */

esp_err_t UART_poll(){

	static uint8_t poll_device_id = 0x00;

	return ESP_OK;
}

/* USER CODE END 2 */

/* Private user code for ISR (Interrupt Service Routines) --------------------*/
/* USER CODE BEGIN ISR */

/* USER CODE END ISR */
