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

// !!! SISTEMARE IL REBOOT IN CASO DI CRASH NEL MENUCONFIG SOTTO IL MENU Trace memory
// !!! SISTEMARE PRIORITA' TASKS, CORE DI ESECUZIONE E STACK ALLOCATO

/* Includes ------------------------------------------------------------------*/
/* USER CODE BEGIN Includes */

// !!! SFOLTIRE

// Standard libraries.
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

// Platform libraries.
#include <esp_err.h>
#include <esp_check.h>
#include <esp_log.h>
#include <esp_attr.h>
#include <esp_timer.h>

#include <freertos/FreeRTOS.h>

#include <esp_adc/adc_continuous.h>
#include <driver/gpio.h>
#include <driver/ledc.h>
#include <driver/uart.h>

// UniLibC libraries.
#include <ul_errors.h>
#include <ul_utils.h>
#include <ul_button_states.h>
#include <ul_master_slave.h>
#include <ul_crc.h>
#include <ul_pm.h>

// Project libraries.
#include <main.h>
#include <gpio.h>
#include <rs485.h>
#include <pwm.h>
#include <pm.h>

// !!! su ogni libreria, sistemare include "Standard libraries"
// !!! su ogni libreria, implementare assert di inizializzazione
// !!! su ogni libreria, vedere se e' il caso di alleggerire assert su funzioni private
// !!! su ogni libreria, mettere a costante privata ogni numero

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

	const char *TAG = "setup_task";
	ESP_LOGI(TAG, "Started");

	ESP_LOGI(TAG, "gpio_setup()");
	ESP_ERROR_CHECK(gpio_setup());

	ESP_LOGI(TAG, "pwm_setup()");
	ESP_ERROR_CHECK(pwm_setup());

	ESP_LOGI(TAG, "rs485_setup()");
	ESP_ERROR_CHECK(rs485_setup());

	ESP_LOGI(TAG, "pm_setup()");
	ESP_ERROR_CHECK(pm_setup());

	/* USER CODE END SysInit */

	/* USER CODE BEGIN Init */

	/* USER CODE END Init */

	/* USER CODE BEGIN 1 */

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
