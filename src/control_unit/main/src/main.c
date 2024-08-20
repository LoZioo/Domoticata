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
#include <pm.h>

// !!! SISTEMARE IL REBOOT IN CASO DI CRASH NEL MENUCONFIG SOTTO IL MENU Trace memory
// !!! Applique: a total of 12.19V for 4 led @ 248mA

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

	ESP_LOGI(TAG, "pm_setup()");
	ESP_ERROR_CHECK(pm_setup());

	/* USER CODE END SysInit */

	/* USER CODE BEGIN Init */

	/* USER CODE END Init */

	/* USER CODE BEGIN 1 */

	ESP_LOGI(TAG, "Completed");
	// return;

	/* Infinite loop */
	for(;;){

		static int64_t t0;
		static ul_pm_results_t pm_res;

		t0 = millis();
		ESP_ERROR_CHECK(pm_get_results(&pm_res));

		printf("Voltage:\n");
		printf("  V_pos_peak: %.2f\n", pm_res.v_pos_peak);
		printf("  V_neg_peak: %.2f\n", pm_res.v_neg_peak);
		printf("  V_pp: %.2f\n", pm_res.v_pp);
		printf("  V_rms: %.2f\n\n", pm_res.v_rms);

		printf("Current:\n");
		printf("  I_pos_peak: %.2f\n", pm_res.i_pos_peak);
		printf("  I_neg_peak: %.2f\n", pm_res.i_neg_peak);
		printf("  I_pp: %.2f\n", pm_res.i_pp);
		printf("  I_rms (mA): %.2f\n\n", pm_res.i_rms * 1000);

		printf("Power:\n");
		printf("  P_va: %.2f\n", pm_res.p_va);
		printf("  P_w: %.2f\n", pm_res.p_w);
		printf("  P_var: %.2f\n", pm_res.p_var);
		printf("  P_pf: %.2f\n\n", pm_res.p_pf);

		delay_remainings(1000, t0);
	}
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
