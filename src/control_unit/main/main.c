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
#include <ul_button_states.h>
#include <ul_master_slave.h>
#include <ul_crc.h>

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

esp_err_t uart_poll();

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

	uart_flush(CONFIG_UART_PORT);
	ESP_LOGI(TAG, "Polling slave devices");

	/* Infinite loop */
	for(;;){

		esp_err_t ret = uart_poll();
		ESP_ERROR_CHECK_WITHOUT_ABORT(ret);

		if(ret != ESP_OK)
			uart_flush(CONFIG_UART_PORT);

		delay(1);
	}
	/* USER CODE END 1 */
}

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 2 */

esp_err_t uart_poll(){

	// Function disabled.
	if(CONFIG_APP_SLAVE_COUNT == 0)
		return ESP_OK;

	// Slave ID increment.
	static uint8_t poll_device_id = CONFIG_APP_SLAVE_COUNT - 1;
	poll_device_id = (poll_device_id + 1) % CONFIG_APP_SLAVE_COUNT;

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
		pdMS_TO_TICKS(CONFIG_APP_SLAVE_POLL_TIMEOUT_MS)
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
		pdMS_TO_TICKS(CONFIG_APP_SLAVE_CONN_TIMEOUT_MS)
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
		poll_device_id, CONFIG_APP_SLAVE_CONN_TIMEOUT_MS
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

	// Update the button states.
	ul_bs_set_button_states(
		ul_utils_cast_to_type(decoded_data, uint16_t)
	);

	// !!! Print button states.
	printf("\n");
	for(uint8_t button=UL_BS_BUTTON_1; button<=UL_BS_BUTTON_8; button++)
		printf(
			"Button %u: %u\n",
			button,
			ul_bs_get_button_state(
				(ul_bs_button_id_t) button
			)
		);

	return ESP_OK;
}

/* USER CODE END 2 */

/* Private user code for ISR (Interrupt Service Routines) --------------------*/
/* USER CODE BEGIN ISR */

/* USER CODE END ISR */
