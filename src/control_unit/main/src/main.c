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

// Project libraries.
#include <main.h>
#include <gpio.h>
#include <rs485.h>
#include <pwm.h>
#include <wifi.h>
#include <ota.h>
#include <fs.h>
// #include <pm.h>

// !!! SISTEMARE IL REBOOT IN CASO DI CRASH NEL MENUCONFIG SOTTO IL MENU Trace memory

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
/* USER CODE BEGIN PV */

const char *TAG = "main";

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

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

	ESP_LOGI(TAG, "Started");

	// ESP_LOGI(TAG, "gpio_setup()");
	// ESP_ERROR_CHECK(gpio_setup());

	// ESP_LOGI(TAG, "pwm_setup()");
	// ESP_ERROR_CHECK(pwm_setup());

	// ESP_LOGI(TAG, "rs485_setup()");
	// ESP_ERROR_CHECK(rs485_setup());

	ESP_LOGI(TAG, "nvs_setup()");
	ESP_ERROR_CHECK(nvs_setup());

	// ESP_LOGI(TAG, "wifi_setup()");
	// ESP_ERROR_CHECK(wifi_setup());

	ESP_LOGI(TAG, "fs_setup()");
	ESP_ERROR_CHECK(fs_setup());

	// ESP_LOGI(TAG, "pm_setup()");
	// ESP_ERROR_CHECK(pm_setup());

	/* USER CODE END SysInit */

	/* USER CODE BEGIN Init */

	/* USER CODE END Init */

	/* USER CODE BEGIN 1 */

	FILE *file;

	// file = fopen(fs_full_path("/hello_world.txt"), "w");
	// ESP_ERROR_CHECK(file == NULL ? ESP_FAIL : ESP_OK);

	// fprintf(file, "ciaooo");
	// fclose(file);

	file = fopen(fs_full_path("/hello_world.txt"), "r");
	ESP_ERROR_CHECK(file == NULL ? ESP_ERR_NOT_FOUND : ESP_OK);

	char file_content[40];
	fscanf(file, "%s\n", file_content);
	fclose(file);

	ESP_LOGI(TAG, "Read from file: %s", file_content);

	ESP_LOGI(TAG, "Completed");
	return;

	/* Infinite loop */
	// for(;;){
	// 	delay(1);
	// }
	/* USER CODE END 1 */
}

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 2 */

void delay_remainings(int32_t ms, int64_t initial_timestamp_ms){
	ms -= millis() - initial_timestamp_ms;
	if(ms > 0)
		delay(ms);
}

/* USER CODE END 2 */

/* Private user code for ISR (Interrupt Service Routines) --------------------*/
/* USER CODE BEGIN ISR */

/* USER CODE END ISR */
