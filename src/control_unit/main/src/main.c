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
#include <esp_system.h>

// Project libraries.
#include <main.h>
#include <gpio.h>
#include <rs485.h>
#include <pwm.h>
#include <wifi.h>
#include <fs.h>
#include <webserver.h>
#include <pm.h>

// !!! SISTEMARE IL REBOOT IN CASO DI CRASH NEL MENUCONFIG SOTTO IL MENU Trace memory
// !!! OTTIMIZZARE CODICE ZONE.H

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

	ESP_LOGI(TAG, "gpio_setup()");
	ESP_ERROR_CHECK(gpio_setup());

	ESP_LOGI(TAG, "pwm_setup()");
	ESP_ERROR_CHECK(pwm_setup());

	ESP_LOGI(TAG, "rs485_setup()");
	ESP_ERROR_CHECK(rs485_setup());

	ESP_LOGI(TAG, "nvs_setup()");
	ESP_ERROR_CHECK(nvs_setup());

	ESP_LOGI(TAG, "wifi_setup()");
	ESP_ERROR_CHECK(wifi_setup());

	ESP_LOGI(TAG, "fs_setup()");
	ESP_ERROR_CHECK(fs_setup());

	ESP_LOGI(TAG, "webserver_setup()");
	ESP_ERROR_CHECK(webserver_setup());

	ESP_LOGI(TAG, "pm_setup()");
	ESP_ERROR_CHECK(pm_setup());

	/* USER CODE END SysInit */

	/* USER CODE BEGIN Init */

	ESP_LOGI(TAG, "Starting fans");
	ESP_ERROR_CHECK(pwm_set_fan(512));

	ESP_LOGI(TAG, "Completed");
	vTaskDelete(NULL);
	return;

	/* USER CODE END Init */

	/* USER CODE BEGIN 1 */

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

void log_hash(const char *TAG, const char* label, uint8_t *hash, uint16_t hash_len){

	char str[hash_len * 2 + 1];
	str[sizeof(str) - 1] = 0;

	for(uint16_t i=0; i<hash_len; i++)
		snprintf(&str[i * 2], hash_len, "%02x", hash[i]);

	ESP_LOGI(TAG, "%s: %s", label, str);
}

/* USER CODE END 2 */

/* Private user code for ISR (Interrupt Service Routines) --------------------*/
/* USER CODE BEGIN ISR */

/* USER CODE END ISR */
