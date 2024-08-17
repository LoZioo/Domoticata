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

#define micros() \
	esp_timer_get_time()

#define millis()( \
	micros() / 1000 \
)

#define log_event_pwm_button(TAG, pwm_index, pwm_enabled, pwm_duty, device_id, button_id, button_state) \
	ESP_LOGI( \
		TAG, "PWM index %u: (enabled=%u, value=%u), triggered by: (device_id=0x%02X, button_id=%u, button_state=%u)", \
		pwm_index, pwm_enabled, pwm_duty, device_id, button_id, button_state \
	)

#define log_event_pwm_trimmer(TAG, pwm_index, pwm_enabled, pwm_duty, device_id, trimmer_val) \
	ESP_LOGI( \
		TAG, "PWM index %u: (enabled=%u, value=%u), triggered by: (device_id=0x%02X, trimmer_val=%u)", \
		pwm_index, pwm_enabled, pwm_duty, device_id, trimmer_val \
	)

// !!! METTERE A FUNZIONE E O ADDIRITTURA RIDURRE LA FORMULA A UN SINGOLO COEFFICIENTE
// !!! RIMUOVERE ANCHE `CONFIG_PWM_GAMMA_CORRECTION` NEL CASO e rimuovere math.h
#define led_gamma_correction(pwm_duty)( \
	(uint16_t)( \
		pow( \
			(float)(pwm_duty) / CONFIG_PWM_DUTY_MAX, \
			CONFIG_PWM_GAMMA_CORRECTION \
		) * CONFIG_PWM_DUTY_MAX \
	) \
)

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
 * @param device_id Device ID of the wall terminal (from `0x00` to `0x7F`); if `0xFF`, no one has pressed any button.
 * @param trimmer_val Current 10-bit trimmer value.
 * @param button_states Raw button states; load them into the `ul_button_states.h` library by using `ul_bs_set_button_states()`.
 * @note Must be called periodically to ensure a clean wall terminals polling loop.
 */
esp_err_t wall_terminals_poll(const char *TAG, uint8_t *device_id, uint16_t *trimmer_val, uint16_t *button_states);
esp_err_t handle_trimmer_change(const char *TAG, bool *pwm_enabled, uint16_t *pwm_duty, uint8_t device_id, uint16_t trimmer_val);
esp_err_t handle_button_press(const char *TAG, bool *pwm_enabled, uint16_t *pwm_duty, uint8_t device_id, uint16_t button_states);

/**
 * @return The corresponding `pwm_index` given `device_id`, `button_id` and `button_state`; -1 if no `pwm_index` is mapped to the given indexes; -2 if there are some parameter errors.
 */
int8_t id_button_and_state_to_pwm_index(uint8_t device_id, ul_bs_button_id_t button_id, ul_bs_button_state_t button_state);

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

void rs485_task(void *parameters){

	const char *TAG = "rs485_task";
	ESP_LOGI(TAG, "Started");

	/* Variables */

	// `ESP_GOTO_ON_ERROR()` return code.
	esp_err_t ret;

	// `wall_terminals_poll()` parameters.
	uint8_t device_id;
	uint16_t trimmer_val, button_states;

	// PWM values for each PWM index.
	uint16_t pwm_duty[CONFIG_PWM_INDEXES];
	bool pwm_enabled[CONFIG_PWM_INDEXES] = {0};

	/* Code */

	// `pwm_duty[]` initialization.
	for(
		uint8_t i = 0;
		i < sizeof(pwm_duty) / sizeof(uint16_t);
		i++
	)
		pwm_duty[i] = led_gamma_correction(CONFIG_PWM_DEFAULT);

	ESP_ERROR_CHECK_WITHOUT_ABORT(uart_flush(CONFIG_UART_PORT));
	ESP_LOGI(TAG, "Polling slave devices");

	/* Infinite loop */
	for(;;){
		ret = ESP_OK;

		// Poll the wall terminals.
		ESP_GOTO_ON_ERROR(
			wall_terminals_poll(TAG, &device_id, &trimmer_val, &button_states),

			task_error,
			TAG,
			"Error on `wall_terminals_poll(device_id=0x%02X, trimmer_val=%u, button_states=%u)`",
			device_id, trimmer_val, button_states
		);

		// Nothing to communicate.
		if(device_id == 0xFF)
			goto task_continue;

		// Device ID check.
		ESP_GOTO_ON_FALSE(
			device_id < CONFIG_RS485_WALL_TERMINALS_COUNT,

			ESP_ERR_NOT_SUPPORTED,
			task_error,
			TAG,
			"Error: the returned `device_id` is 0x%02X; max allowed is 0x%02X",
			device_id, CONFIG_RS485_WALL_TERMINALS_COUNT - 1
		);

		// Button pressed.
		if(button_states != 0)
			ESP_GOTO_ON_ERROR(
				handle_button_press(TAG, pwm_enabled, pwm_duty, device_id, button_states),

				task_error,
				TAG,
				"Error on `handle_button_press(device_id=0x%02X, button_states=%u)`",
				device_id, button_states
			);

		// Trimmer rotated.
		else
			ESP_GOTO_ON_ERROR(
				handle_trimmer_change(TAG, pwm_enabled, pwm_duty, device_id, trimmer_val),

				task_error,
				TAG,
				"Error on `handle_button_press(device_id=0x%02X, trimmer_val=%u)`",
				device_id, trimmer_val
			);

		// Delay before continuing.
		goto task_continue;

		task_error:
		ESP_ERROR_CHECK_WITHOUT_ABORT(ret);
		ESP_ERROR_CHECK_WITHOUT_ABORT(uart_flush(CONFIG_UART_PORT));

		task_continue:
		delay(1);
	}
}

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

esp_err_t wall_terminals_poll(const char *TAG, uint8_t *device_id, uint16_t *trimmer_val, uint16_t *button_states){

	// Function disabled.
	if(CONFIG_RS485_WALL_TERMINALS_COUNT == 0)
		return ESP_OK;

	ESP_RETURN_ON_FALSE(
		TAG != NULL,

		ESP_ERR_INVALID_ARG,
		"???",
		"Error: `TAG` is NULL"
	);

	ESP_RETURN_ON_FALSE(
		device_id != NULL,

		ESP_ERR_INVALID_ARG,
		TAG,
		"Error: `device_id` is NULL"
	);

	ESP_RETURN_ON_FALSE(
		trimmer_val != NULL,

		ESP_ERR_INVALID_ARG,
		TAG,
		"Error: `adc_val` is NULL"
	);

	ESP_RETURN_ON_FALSE(
		button_states != NULL,

		ESP_ERR_INVALID_ARG,
		TAG,
		"Error: `button_states` is NULL"
	);

	// Default returned values.
	*device_id = 0xFF;
	*trimmer_val = *button_states = 0x0000;

	// Slave ID increment.
	static uint8_t poll_device_id = CONFIG_RS485_WALL_TERMINALS_COUNT - 1;
	poll_device_id = (poll_device_id + 1) % CONFIG_RS485_WALL_TERMINALS_COUNT;

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

	// Encoded data buffer.
	uint8_t encoded_data[4];

	// Decoded data buffer.
	struct __attribute__((__packed__)) {
		uint16_t trimmer_val: 10;
		uint16_t button_states: 6;
		uint8_t crc8;
	} decoded_data;

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
		"Error: slave device 0x%02X exceeded the prefixed %ums timeout for sending its state",
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
		ul_utils_cast_to_mem(decoded_data),
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
	uint8_t crc8 =
		ul_crc_crc8(ul_utils_cast_to_mem(decoded_data), 2);

	// CRC8 check.
	ESP_RETURN_ON_FALSE(
		crc8 == decoded_data.crc8,

		ESP_ERR_INVALID_CRC,
		TAG,
		"Error: invalid CRC8 for slave device 0x%02X; sent CRC8 is 0x%02X but computed CRC8 is 0x%02X",
		poll_device_id, decoded_data.crc8, crc8
	);

	// Returned values.
	*device_id = poll_device_id;
	*trimmer_val = decoded_data.trimmer_val;
	*button_states = decoded_data.button_states;

	return ESP_OK;
}

esp_err_t handle_trimmer_change(const char *TAG, bool *pwm_enabled, uint16_t *pwm_duty, uint8_t device_id, uint16_t trimmer_val){

	ESP_RETURN_ON_FALSE(
		TAG != NULL,

		ESP_ERR_INVALID_ARG,
		"???",
		"Error: `TAG` is NULL"
	);

	ESP_RETURN_ON_FALSE(
		pwm_enabled != NULL,

		ESP_ERR_INVALID_ARG,
		TAG,
		"Error: `pwm_enabled` is NULL"
	);

	ESP_RETURN_ON_FALSE(
		pwm_duty != NULL,

		ESP_ERR_INVALID_ARG,
		TAG,
		"Error: `pwm_duty` is NULL"
	);

	ESP_RETURN_ON_FALSE(
		device_id < CONFIG_RS485_WALL_TERMINALS_COUNT,

		ESP_ERR_INVALID_ARG,
		TAG,
		"Error: `device_id` must be less than %u",
		CONFIG_RS485_WALL_TERMINALS_COUNT
	);

	ESP_RETURN_ON_FALSE(
		trimmer_val <= CONFIG_PWM_DUTY_MAX,

		ESP_ERR_INVALID_ARG,
		TAG,
		"Error: `trimmer_val` must be between 0 and %u",
		CONFIG_PWM_DUTY_MAX
	);

	// Mapping array.
	int8_t id_to_pwm_index[] = CONFIG_TRIMMER_MATRIX;

	// Trimmer not mapped.
	if(id_to_pwm_index[device_id] == -1)
		return ESP_OK;

	// Gamma correction.
	trimmer_val = led_gamma_correction(trimmer_val);

	// Update PWM.
	ESP_RETURN_ON_ERROR(
		pwm_write(
			TAG,
			id_to_pwm_index[device_id],
			trimmer_val,
			CONFIG_PWM_FADE_TIME_MS
		),

		TAG,
		"Error on `pwm_write(pwm_index=%u, target_duty=%u)`",
		id_to_pwm_index[device_id], trimmer_val
	);

	// Update the corresponding PWM state.
	pwm_enabled[id_to_pwm_index[device_id]] =
		(trimmer_val > 0);

	pwm_duty[id_to_pwm_index[device_id]] =
		(trimmer_val > 0 ? trimmer_val : CONFIG_PWM_DEFAULT);

	// Log the event.
	log_event_pwm_trimmer(
		TAG,
		id_to_pwm_index[device_id],
		pwm_enabled[device_id],
		pwm_duty[device_id],
		device_id,
		trimmer_val
	);

	return ESP_OK;
}

esp_err_t handle_button_press(const char *TAG, bool *pwm_enabled, uint16_t *pwm_duty, uint8_t device_id, uint16_t button_states){

	ESP_RETURN_ON_FALSE(
		TAG != NULL,

		ESP_ERR_INVALID_ARG,
		"???",
		"Error: `TAG` is NULL"
	);

	ESP_RETURN_ON_FALSE(
		pwm_enabled != NULL,

		ESP_ERR_INVALID_ARG,
		TAG,
		"Error: `pwm_enabled` is NULL"
	);

	ESP_RETURN_ON_FALSE(
		pwm_duty != NULL,

		ESP_ERR_INVALID_ARG,
		TAG,
		"Error: `pwm_duty` is NULL"
	);

	ESP_RETURN_ON_FALSE(
		device_id < CONFIG_RS485_WALL_TERMINALS_COUNT,

		ESP_ERR_INVALID_ARG,
		TAG,
		"Error: `device_id` must be less than %u",
		CONFIG_RS485_WALL_TERMINALS_COUNT
	);

	ESP_RETURN_ON_FALSE(
		button_states != 0,

		ESP_ERR_INVALID_ARG,
		TAG,
		"Error: `button_states` is 0"
	);

	ul_bs_set_button_states(button_states);

	// Auxiliary variables.
	int8_t pwm_index = -1;
	ul_bs_button_state_t button_state;

	// For each button, check if it was pressed.
	for(
		uint8_t button_id = UL_BS_BUTTON_1;
		button_id <= CONFIG_BUTTONS_MAX_NUMBER_PER_TERMINAL;
		button_id++
	){
		button_state = ul_bs_get_button_state(button_id);

		// Idle state is not used.
		if(button_state == UL_BS_BUTTON_STATE_IDLE)
			continue;

		pwm_index = id_button_and_state_to_pwm_index(device_id, button_id, button_state);

		// If the current configuration is not mapped.
		if(pwm_index == -1)
			continue;

		ESP_RETURN_ON_FALSE(
			pwm_index != -2,

			ESP_ERR_INVALID_ARG,
			TAG,
			"Error: invalid args passed on `id_button_and_state_to_pwm_index(device_id=0x%02X, button_id=%u, button_state=%u)`",
			device_id, button_id, button_state
		);

		// Turn ON/OFF PWM.
		pwm_enabled[pwm_index] = !pwm_enabled[pwm_index];

		uint16_t pwm_final_duty = (
			pwm_enabled[pwm_index] ?
			pwm_duty[pwm_index] :
			0
		);

		// Update PWM.
		ESP_RETURN_ON_ERROR(
			pwm_write(
				TAG,
				pwm_index,
				pwm_final_duty,
				CONFIG_PWM_FADE_TIME_MS
			),

			TAG,
			"Error on `pwm_write(pwm_index=%u, target_duty=%u)`",
			pwm_index, pwm_final_duty
		);

		// Log the event.
		log_event_pwm_button(
			TAG,
			pwm_index,
			pwm_enabled[pwm_index],
			pwm_duty[pwm_index],
			device_id,
			button_id,
			button_state
		);
	}

	return ESP_OK;
}

int8_t id_button_and_state_to_pwm_index(uint8_t device_id, ul_bs_button_id_t button_id, ul_bs_button_state_t button_state){

	// Index error.
	if(
		device_id >= CONFIG_RS485_WALL_TERMINALS_COUNT ||

		!ul_utils_between(
			button_id,
			UL_BS_BUTTON_1,
			CONFIG_BUTTONS_MAX_NUMBER_PER_TERMINAL
		) ||

		!ul_utils_between(
			button_state,
			UL_BS_BUTTON_STATE_PRESSED,
			UL_BS_BUTTON_STATE_HELD
		)
	)
		return -2;

	// Mapping cube matrix (idle state is not used on the last index).
	int8_t id_button_and_state_to_pwm_index
		[CONFIG_RS485_WALL_TERMINALS_COUNT]
		[CONFIG_BUTTONS_MAX_NUMBER_PER_TERMINAL]
		[UL_BS_BUTTON_STATE_MAX - 1] = CONFIG_BUTTON_MATRIX;

	return id_button_and_state_to_pwm_index[device_id][button_id-1][button_state-1];
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
