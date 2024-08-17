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
// !!! SISTEMARE PRIORITA' TASKS E STACK ALLOCATO

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
#include <rs485.h>

// !!! RIMUOVERE
#include <temp_configs.h>

// !!! SMEMBRARE E RIMUOVERE
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

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* FreeRTOS tasks */

QueueHandle_t pwm_queue;
TaskHandle_t pwm_task_handle;
void pwm_task(void *parameters);

adc_continuous_handle_t adc_handle;
TaskHandle_t pm_task_handle;
void pm_task(void *parameters);

/* Generic functions */

/**
 * @brief Send `pwm_data` to `pwm_task` via `pwm_queue`.
 * @param TAG The `esp_log.h` tag.
 * @param index 0: Fan controller, 1-12: LEDs.
 * @param target_duty 10-bit target duty (from 0 to (2^`CONFIG_LEDC_PWM_BIT_RES`) - 1 ).
 */
esp_err_t pwm_write(const char *TAG, uint8_t pwm_index, uint16_t target_duty, uint16_t fade_time_ms);

/**
 * @brief `delay( ms - (millis() - initial_timestamp_ms) )`
 */
void delay_remainings(int32_t ms, int64_t initial_timestamp_ms);

/* ISR */

bool IRAM_ATTR adc_conversion_done(adc_continuous_handle_t adc_handle, const adc_continuous_evt_data_t *edata, void *user_data);

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

	ESP_LOGI(TAG, "GPIO_setup()");
	ESP_ERROR_CHECK(GPIO_setup(TAG));

	ESP_LOGI(TAG, "LEDC_setup()");
	ESP_ERROR_CHECK(LEDC_setup(TAG));

	ESP_LOGI(TAG, "rs485_setup()");
	ESP_ERROR_CHECK(rs485_setup());

	ESP_LOGI(TAG, "ADC_setup()");
	ESP_ERROR_CHECK(ADC_setup(TAG));

	/* USER CODE END SysInit */

	/* USER CODE BEGIN Init */

	// !!! SMEMBRARE IN OGNI SINGOLO FILE DI FUNZIONI
	// ESP_LOGI(TAG, "QUEUES_setup()");
	// ESP_ERROR_CHECK(QUEUES_setup(TAG));

	// ESP_LOGI(TAG, "TASKS_setup()");
	// ESP_ERROR_CHECK(TASKS_setup(TAG));

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

void pwm_task(void *parameters){

	const char *TAG = "pwm_task";
	ESP_LOGI(TAG, "Started");

	/* Variables */

	// `ESP_GOTO_ON_ERROR()` return code.
	esp_err_t ret;

	// PWM incoming data.
	pwm_data_t pwm;

	/* Code */

	/* Infinite loop */
	for(;;){
		ret = ESP_OK;

		// Waiting for `pwm_write()` requests.
		if(xQueueReceive(pwm_queue, &pwm, portMAX_DELAY) == pdFALSE)
			goto task_continue;

		// Set PWM parameters.
		ESP_GOTO_ON_ERROR(
			ledc_set_fade_with_time(
				pwm_get_port(pwm.pwm_index),
				pwm_get_channel(pwm.pwm_index),
				pwm.target_duty,
				pwm.fade_time_ms
			),

			task_error,
			TAG,
			"Error on `ledc_set_fade_with_time(speed_mode=%u, channel=%u, target_duty=%u, max_fade_time_ms=%u)`",
			pwm_get_port(pwm.pwm_index), pwm_get_channel(pwm.pwm_index), pwm.target_duty, pwm.fade_time_ms
		);

		// Fade start.
		ESP_GOTO_ON_ERROR(
			ledc_fade_start(
				pwm_get_port(pwm.pwm_index),
				pwm_get_channel(pwm.pwm_index),
				LEDC_FADE_NO_WAIT
			),

			task_error,
			TAG,
			"Error on `ledc_fade_start(speed_mode=%u, channel=%u)`",
			pwm_get_port(pwm.pwm_index), pwm_get_channel(pwm.pwm_index)
		);

		// Delay before continuing.
		goto task_continue;

		task_error:
		ESP_ERROR_CHECK_WITHOUT_ABORT(ret);

		task_continue:
		delay(1);
	}
}

// !!! CONTINUARE AGGIUNGENDO ELABORAZIONE DATI
void pm_task(void *parameters){

	const char *TAG = "pm_task";
	ESP_LOGI(TAG, "Started");

	/* Variables */

	// `ESP_GOTO_ON_ERROR()` return code.
	esp_err_t ret;

	// Timings.
	int64_t t0;

	// Must be multiple of 4.
	static uint16_t samples[pm_samples_len_to_buf_size(CONFIG_ADC_SAMPLES)];
	uint32_t read_size;

	/* Code */

	ESP_LOGI(TAG, "Sampling from ADC");

	/* Infinite loop */
	for(;;){
		t0 = millis();
		ret = ESP_OK;

		// Flush old samples.
		ESP_GOTO_ON_ERROR(
			adc_continuous_flush_pool(adc_handle),

			task_error,
			TAG,
			"Error on `adc_continuous_flush_pool()`"
		);

		// Start the sample acquisition.
		ESP_GOTO_ON_ERROR(
			adc_continuous_start(adc_handle),

			task_error,
			TAG,
			"Error on `adc_continuous_start()`"
		);

		// Wait for the ISR and then clear the notification (`pdTRUE`).
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

		// Read the acquired samples from the driver.
		ESP_GOTO_ON_ERROR(
			adc_continuous_read(
				adc_handle,
				(uint8_t*) samples,
				pm_samples_len_to_buf_size(CONFIG_ADC_SAMPLES),
				&read_size,
				40	// Two 50Hz cycles.
			),

			task_error,
			TAG,
			"Error on `adc_continuous_read()`"
		);

		// !!! DEBUG
		// printf("samples: { ");
		// for(uint32_t i=0; i<16; i++){

		// 	printf("(%d, %d)",
		// 		((adc_digi_output_data_t*) &samples[i])->type1.channel,
		// 		((adc_digi_output_data_t*) &samples[i])->type1.data
		// 	);

		// 	if(i < 15)
		// 		printf(", ");
		// }
		// printf(" }\n");
		// printf("read_size: %lu\n", read_size);
		// printf("samples_size: %u\n", pm_samples_len_to_buf_size(CONFIG_ADC_SAMPLES));
		// printf("\n");
		// !!! DEBUG

		// Delay before continuing.
		goto task_continue;

		task_error:
		ESP_ERROR_CHECK_WITHOUT_ABORT(ret);

		task_continue:
		ESP_ERROR_CHECK_WITHOUT_ABORT(
			adc_continuous_stop(adc_handle)
		);

		delay_remainings(1000, t0);
	}
}

esp_err_t pwm_write(const char *TAG, uint8_t pwm_index, uint16_t target_duty, uint16_t fade_time_ms){

	ESP_RETURN_ON_FALSE(
		TAG != NULL,

		ESP_ERR_INVALID_ARG,
		"???",
		"Error: `TAG` is NULL"
	);

	ESP_RETURN_ON_FALSE(
		pwm_index < CONFIG_PWM_INDEXES,

		ESP_ERR_INVALID_ARG,
		TAG,
		"Error: `pwm_index` must be between 0 and %u",
		CONFIG_PWM_INDEXES - 1
	);

	if(target_duty > CONFIG_PWM_DUTY_MAX)
		target_duty = CONFIG_PWM_DUTY_MAX;

	pwm_data_t pwm_data = {
		.pwm_index = pwm_index,
		.target_duty = target_duty,
		.fade_time_ms = fade_time_ms
	};

	ESP_RETURN_ON_FALSE(
		xQueueSend(pwm_queue, &pwm_data, 0) == pdTRUE,

		ESP_ERR_INVALID_STATE,
		TAG,
		"Error: `pwm_queue` is full"
	);

	return ESP_OK;
}

void delay_remainings(int32_t ms, int64_t initial_timestamp_ms){
	ms -= millis() - initial_timestamp_ms;
	if(ms > 0)
		delay(ms);
}

/* USER CODE END 2 */

/* Private user code for ISR (Interrupt Service Routines) --------------------*/
/* USER CODE BEGIN ISR */

bool adc_conversion_done(adc_continuous_handle_t adc_handle, const adc_continuous_evt_data_t *edata, void *user_data){

	BaseType_t must_yield = pdFALSE;
	vTaskNotifyGiveFromISR(pm_task_handle, &must_yield);

	return (must_yield == pdTRUE);
}

/* USER CODE END ISR */
