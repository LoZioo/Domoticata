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
// !!! RIMETTERE IL PINOUT APPOSTO DAL MENUCONFIG

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
#include <esp_attr.h>

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

// Project libraries.
#include <setup.h>

// !!! RIMUOVERE
#include <temp_configs.h>

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

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* FreeRTOS tasks */

TaskHandle_t rs485_task_handle;
void rs485_task(void *parameters);

QueueHandle_t pwm_queue;
TaskHandle_t pwm_task_handle;
void pwm_task(void *parameters);

adc_continuous_handle_t adc_handle;
TaskHandle_t pm_task_handle;
void pm_task(void *parameters);

/* Generic functions */

/**
 * @brief Poll the RS-485 bus to check if some button was pressed on some wall terminal.
 * @param TAG The `esp_log.h` tag.
 * @param device_id The device ID of the wall terminal (from `0x00` to `0x7F`); if `0xFF`, no one has pressed any button.
 * @param button_states The raw button states; load them into the `ul_button_states.h` library by using `ul_bs_set_button_states()`.
 * @note Must be called periodically to ensure a clean wall terminals polling loop.
 */
esp_err_t wall_terminals_poll(const char *TAG, uint8_t *device_id, uint16_t *button_states);

/**
 * @brief Send `pwm_data` to `pwm_task` via `pwm_queue`.
 * @param TAG The `esp_log.h` tag.
 * @param index 0: Fan controller, 1-12: LEDs.
 */
esp_err_t pwm_write(const char *TAG, uint8_t index, uint8_t duty_target_perc, uint16_t fade_time_ms);

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

	ESP_LOGI(TAG, "UART_setup()");
	ESP_ERROR_CHECK(UART_setup(TAG));

	ESP_LOGI(TAG, "ADC_setup()");
	ESP_ERROR_CHECK(ADC_setup(TAG));

	/* USER CODE END SysInit */

	/* USER CODE BEGIN Init */

	ESP_LOGI(TAG, "QUEUES_setup()");
	ESP_ERROR_CHECK(QUEUES_setup(TAG));

	ESP_LOGI(TAG, "TASKS_setup()");
	ESP_ERROR_CHECK(TASKS_setup(TAG));

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

// !!! SISTEMARE TUTTI I TASK CON for(;;) CON GOTO_ON_ERROR/FALSE

void rs485_task(void *parameters){

	const char *TAG = "rs485_task";
	ESP_LOGI(TAG, "Started");

	/* Variables */

	uint8_t device_id;
	uint16_t button_states;
	esp_err_t ret;

	/* Code */

	ESP_ERROR_CHECK_WITHOUT_ABORT(uart_flush(CONFIG_UART_PORT));
	ESP_LOGI(TAG, "Polling slave devices");

	/* Infinite loop */
	for(;;){

		delay(1);

		ret = wall_terminals_poll(TAG, &device_id, &button_states);
		ESP_ERROR_CHECK_WITHOUT_ABORT(ret);

		if(ret != ESP_OK){
			ESP_ERROR_CHECK_WITHOUT_ABORT(uart_flush(CONFIG_UART_PORT));
			continue;
		}

		if(device_id == 0xFF)
			continue;

		// Update button states.
		ul_bs_set_button_states(button_states);

		// !!! Print button states.
		printf("\nDevice ID: 0x%02X\n", device_id);
		for(uint8_t button=UL_BS_BUTTON_1; button<=UL_BS_BUTTON_8; button++)
			printf(
				"Button %u: %u\n",
				button,
				ul_bs_get_button_state(
					(ul_bs_button_id_t) button
				)
			);
	}
}

void pwm_task(void *parameters){

	const char *TAG = "pwm_task";
	ESP_LOGI(TAG, "Started");

	/* Variables */

	pwm_data_t pwm;

	/* Code */

	/* Infinite loop */
	for(;;){

		// Waiting for `pwm_write()` requests.
		if(xQueueReceive(pwm_queue, &pwm, portMAX_DELAY) == pdFALSE)
			continue;

		// Set PWM parameters.
		ESP_ERROR_CHECK_WITHOUT_ABORT(
			ledc_set_fade_with_time(
				pwm_get_port(pwm.index),
				pwm_get_channel(pwm.index),
				pwm.duty_target,
				pwm.fade_time_ms
			)
		);

		// Fade start.
		ESP_ERROR_CHECK_WITHOUT_ABORT(
			ledc_fade_start(
				pwm_get_port(pwm.index),
				pwm_get_channel(pwm.index),
				LEDC_FADE_NO_WAIT
			)
		);
	}
}

void pm_task(void *parameters){

	const char *TAG = "pm_task";
	ESP_LOGI(TAG, "Started");

	/* Variables */

	// `ESP_GOTO_ON_ERROR()` return code.
	esp_err_t ret;

	// Must be multiple of 4.
	uint16_t samples[12];
	uint32_t samples_size = sizeof(samples) / sizeof(uint16_t);
	uint32_t read_samples_size;

	/* Code */

	ESP_LOGI(TAG, "Sampling from ADC");

	/* Infinite loop */
	for(;;){

		// Reset the return code.
		ret = ESP_OK;

		// Flush old data.
		ESP_GOTO_ON_ERROR(
			adc_continuous_flush_pool(adc_handle),
			pm_task_continue,
			TAG,
			"Error on `adc_continuous_flush_pool()`"
		);

		// Start the sample acquisition.
		ESP_GOTO_ON_ERROR(
			adc_continuous_start(adc_handle),
			pm_task_continue,
			TAG,
			"Error on `adc_continuous_start()`"
		);

		// Wait for the ISR and then clear the notification (`pdTRUE`).
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

		// Read the acquired samples from the driver.
		ESP_GOTO_ON_ERROR(
			adc_continuous_read(adc_handle, (uint8_t*) samples, samples_size, &read_samples_size, 40),
			pm_task_continue,
			TAG,
			"Error on `adc_continuous_read()`"
		);

		printf("samples: { ");
		for(uint32_t i=0; i<read_samples_size; i++){

			printf("(%u, %u)",
				((adc_digi_output_data_t*) &samples[i])->type1.channel,
				((adc_digi_output_data_t*) &samples[i])->type1.data
			);

			if(i < samples_size - 1)
				printf(", ");
		}
		printf(" }\n");
		printf("read_samples_size: %lu\n", read_samples_size);
		printf("samples_size: %lu\n", samples_size);
		printf("\n");

		pm_task_continue:

		// Check the return code.
		ESP_ERROR_CHECK_WITHOUT_ABORT(ret);

		// Before continuing, stop the sample acquisition.
		ESP_ERROR_CHECK_WITHOUT_ABORT(
			adc_continuous_stop(adc_handle)
		);

		// !!! SISTEMARE RELATIVAMENTE AL TEMPO CHE MANCA PER ESSERE TRASCORSO UN SECONDO (METTERE ANCHE A COSTANTE)
		delay(1000);
	}
}

esp_err_t wall_terminals_poll(const char *TAG, uint8_t *device_id, uint16_t *button_states){

	if(TAG == NULL)
		return ESP_ERR_INVALID_ARG;

	// Function disabled.
	if(CONFIG_RS485_WALL_TERMINAL_COUNT == 0)
		return ESP_OK;

	ESP_RETURN_ON_FALSE(
		device_id != NULL,

		ESP_ERR_INVALID_ARG,
		TAG,
		"Error: `device_id` is NULL"
	);

	ESP_RETURN_ON_FALSE(
		button_states != NULL,

		ESP_ERR_INVALID_ARG,
		TAG,
		"Error: `button_states` is NULL"
	);

	// Default returned values.
	*device_id = 0xFF;
	*button_states = 0x0000;

	// Slave ID increment.
	static uint8_t poll_device_id = CONFIG_RS485_WALL_TERMINAL_COUNT - 1;
	poll_device_id = (poll_device_id + 1) % CONFIG_RS485_WALL_TERMINAL_COUNT;

	uint8_t tmp = ul_ms_encode_master_byte(poll_device_id);

	// Poll the slave device.
	ESP_RETURN_ON_FALSE(
		uart_write_bytes(
			CONFIG_UART_PORT,
			ul_utils_cast_to_mem(tmp),
			1
		) >= 0,

		ESP_ERR_INVALID_ARG,
		TAG,
		"Error on `uart_write_bytes()`"
	);

	int read_bytes;

	// Wait for the response.
	read_bytes = uart_read_bytes(
		CONFIG_UART_PORT,
		ul_utils_cast_to_mem(tmp),
		1,
		pdMS_TO_TICKS(CONFIG_RS485_WALL_TERMINAL_POLL_TIMEOUT_MS)
	);

	// Error.
	ESP_RETURN_ON_FALSE(
		read_bytes >= 0,

		ESP_ERR_TIMEOUT,
		TAG,
		"Error on `uart_read_bytes()`"
	);

	// Timeout.
	if(read_bytes == 0)
		return ESP_OK;

	// Decode the device ID.
	uint8_t read_device_id = ul_ms_decode_slave_byte(tmp);

	// Invalid response.
	ESP_RETURN_ON_FALSE(
		(
			ul_ms_is_slave_byte(tmp) &&
			read_device_id == poll_device_id
		),

		ESP_ERR_INVALID_RESPONSE,
		TAG,
		"Error: slave device 0x%02X answered with different ID 0x%02X",
		poll_device_id, read_device_id
	);

	// Encoded and decoded received data buffers.
	uint8_t encoded_data[4], decoded_data[3];

	// Wait for the remaining bytes.
	read_bytes = uart_read_bytes(
		CONFIG_UART_PORT,
		encoded_data,
		4,
		pdMS_TO_TICKS(CONFIG_RS485_WALL_TERMINAL_CONN_TIMEOUT_MS)
	);

	// Error.
	ESP_RETURN_ON_FALSE(
		read_bytes >= 0,

		ESP_ERR_TIMEOUT,
		TAG,
		"Error on `uart_read_bytes()` for slave device 0x%02X",
		poll_device_id
	);

	// Timeout.
	ESP_RETURN_ON_FALSE(
		read_bytes > 0,

		ESP_ERR_TIMEOUT,
		TAG,
		"Error: slave device 0x%02X exceeded the prefixed %ums timeout for sending the button states",
		poll_device_id, CONFIG_RS485_WALL_TERMINAL_CONN_TIMEOUT_MS
	);

	// Invalid response.
	ESP_RETURN_ON_FALSE(
		read_bytes == 4,

		ESP_ERR_INVALID_RESPONSE,
		TAG,
		"Error: slave device 0x%02X sent %u bytes; 4 expected",
		poll_device_id, read_bytes
	);

	// Decode the received bytes.
	ul_err_t ret_val = ul_ms_decode_slave_message(
		decoded_data,
		encoded_data,
		4
	);

	// `ul_ms_decode_slave_message()` failed.
	ESP_RETURN_ON_FALSE(
		ret_val == UL_OK,

		ESP_FAIL,
		TAG,
		"Error #%u on `ul_ms_decode_slave_message()` for slave device 0x%02X",
		ret_val, poll_device_id
	);

	// CRC8 computation.
	uint8_t crc8 = ul_crc_crc8(decoded_data, 2);

	// CRC8 check.
	ESP_RETURN_ON_FALSE(
		crc8 == decoded_data[2],

		ESP_ERR_INVALID_CRC,
		TAG,
		"Error: invalid data CRC for slave device 0x%02X; sent CRC is 0x%02X but computed CRC is 0x%02X",
		poll_device_id, decoded_data[2], crc8
	);

	// Returned values.
	*device_id = poll_device_id;
	*button_states = ul_utils_cast_to_type(decoded_data, uint16_t);

	return ESP_OK;
}

esp_err_t pwm_write(const char *TAG, uint8_t index, uint8_t duty_target_perc, uint16_t fade_time_ms){

	if(TAG == NULL)
		return ESP_ERR_INVALID_ARG;

	if(duty_target_perc > 100)
		duty_target_perc = 100;

	pwm_data_t pwm_data = {
		.index = index,
		.duty_target =
			ul_utils_map_int(
				duty_target_perc,
				0, 100, 0, 128
			),

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

/* USER CODE END 2 */

/* Private user code for ISR (Interrupt Service Routines) --------------------*/
/* USER CODE BEGIN ISR */

bool adc_conversion_done(adc_continuous_handle_t adc_handle, const adc_continuous_evt_data_t *edata, void *user_data){

	BaseType_t must_yield = pdFALSE;
	vTaskNotifyGiveFromISR(pm_task_handle, &must_yield);

	return (must_yield == pdTRUE);
}

/* USER CODE END ISR */

// ESP_RETURN_ON_ERROR(
// 	ledc_set_duty(pwm_get_port(i), pwm_get_channel(i), 127),
// 	TAG,
// 	"Error on `ledc_set_duty()`"
// );

// ESP_RETURN_ON_ERROR(
// 	ledc_update_duty(pwm_get_port(i), pwm_get_channel(i)),
// 	TAG,
// 	"Error on `ledc_update_duty()`"
// );
