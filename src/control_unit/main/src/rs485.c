/** @file rs485.c
 *  @brief  Created on: Aug 17, 2024
 *          Davide Scalisi
 *
 * @copyright [2024] Davide Scalisi *
 * @copyright All Rights Reserved. *
 *
*/

/************************************************************************************************************
* Included files
************************************************************************************************************/

#include <rs485.h>

/************************************************************************************************************
* Private Defines
************************************************************************************************************/

#define LOG_TAG	"rs485"

/* Log */

#define __log_event_button(TAG, pwm_index, pwm_enabled, pwm_duty, device_id, button_id, button_state) \
	ESP_LOGI( \
		TAG, "PWM index %u: (enabled=%u, value=%u), triggered by: (device_id=0x%02X, button_id=%u, button_state=%u)", \
		pwm_index, pwm_enabled, pwm_duty, device_id, button_id, button_state \
	)

#define __log_event_trimmer(TAG, pwm_index, pwm_enabled, pwm_duty, device_id, trimmer_val) \
	ESP_LOGI( \
		TAG, "PWM index %u: (enabled=%u, value=%u), triggered by: (device_id=0x%02X, trimmer_val=%u)", \
		pwm_index, pwm_enabled, pwm_duty, device_id, trimmer_val \
	)

/************************************************************************************************************
* Private Types Definitions
 ************************************************************************************************************/

/************************************************************************************************************
* Private Variables
 ************************************************************************************************************/

static const char *TAG = LOG_TAG;
static TaskHandle_t __rs485_task_handle;

/************************************************************************************************************
* Private Functions Prototypes
 ************************************************************************************************************/

static esp_err_t __uart_driver_setup();
static esp_err_t __rs485_task_setup();

/**
 * @brief Poll the RS-485 bus to check if some button was pressed on some wall terminal.
 * @param TAG The `esp_log.h` tag.
 * @param device_id Device ID of the wall terminal (from `0x00` to `0x7F`); if `0xFF`, no one has pressed any button.
 * @param trimmer_val Current 10-bit trimmer value.
 * @param button_states Raw button states; load them into the `ul_button_states.h` library by using `ul_bs_set_button_states()`.
 * @note Must be called periodically to ensure a clean wall terminals polling loop.
 */
static esp_err_t __wall_terminals_poll(uint8_t *device_id, uint16_t *trimmer_val, uint16_t *button_states);
static esp_err_t __handle_trimmer_change(bool *pwm_enabled, uint16_t *pwm_duty, uint8_t device_id, uint16_t trimmer_val);
static esp_err_t __handle_button_press(bool *pwm_enabled, uint16_t *pwm_duty, uint8_t device_id, uint16_t button_states);

/**
 * @return The corresponding `pwm_index` given `device_id`, `button_id` and `button_state`; -1 if no `pwm_index` is mapped to the given indexes; -2 if there are some parameter errors.
 */
static int8_t __id_button_and_state_to_pwm_index(uint8_t device_id, ul_bs_button_id_t button_id, ul_bs_button_state_t button_state);

static void __rs485_task(void *parameters);

/************************************************************************************************************
* Private Functions Definitions
 ************************************************************************************************************/

esp_err_t __uart_driver_setup(){

	uart_config_t uart_config = {
		.baud_rate = CONFIG_RS485_UART_BAUD_RATE,
		.data_bits = UART_DATA_8_BITS,
		.parity = UART_PARITY_DISABLE,
		.stop_bits = UART_STOP_BITS_1,
		.flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
		.rx_flow_ctrl_thresh = 122,
		.source_clk = UART_SCLK_DEFAULT,
	};

	ESP_RETURN_ON_ERROR(
		uart_driver_install(
			CONFIG_RS485_UART_PORT,
			UART_HW_FIFO_LEN(2) * 2,
			0,
			0,
			NULL,
			0
		),

		TAG,
		"Error on `uart_driver_install()`"
	);

	ESP_RETURN_ON_ERROR(
		uart_param_config(
			CONFIG_RS485_UART_PORT,
			&uart_config
		),

		TAG,
		"Error on `uart_param_config()`"
	);

	ESP_RETURN_ON_ERROR(
		uart_set_pin(
			CONFIG_RS485_UART_PORT,
			CONFIG_GPIO_UART_TX,
			CONFIG_GPIO_UART_RX,
			CONFIG_GPIO_UART_DE_RE,
			UART_PIN_NO_CHANGE
		),

		TAG,
		"Error on `uart_set_pin()`"
	);

	ESP_RETURN_ON_ERROR(
		uart_set_mode(
			CONFIG_RS485_UART_PORT,
			UART_MODE_RS485_HALF_DUPLEX
		),

		TAG,
		"Error on `uart_set_mode()`"
	);

	return ESP_OK;
}

esp_err_t __rs485_task_setup(){

	BaseType_t ret_val = xTaskCreatePinnedToCore(
		__rs485_task,
		LOG_TAG"_task",
		4096,
		NULL,
		tskIDLE_PRIORITY,
		&__rs485_task_handle,
		ESP_APPLICATION_CORE
	);

	ESP_RETURN_ON_FALSE(
		ret_val == pdPASS,

		ESP_ERR_INVALID_STATE,
		TAG,
		"Error %d: unable to spawn \""LOG_TAG"_task\"",
		ret_val
	);

	return ESP_OK;
}

esp_err_t __wall_terminals_poll(uint8_t *device_id, uint16_t *trimmer_val, uint16_t *button_states){

	// Function disabled.
	if(CONFIG_RS485_WALL_TERMINALS_COUNT == 0)
		return ESP_OK;

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
			CONFIG_RS485_UART_PORT,
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
		CONFIG_RS485_UART_PORT,
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
		CONFIG_RS485_UART_PORT,
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

esp_err_t __handle_trimmer_change(bool *pwm_enabled, uint16_t *pwm_duty, uint8_t device_id, uint16_t trimmer_val){

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
	trimmer_val = pwm_led_gamma_correction(trimmer_val);

	// Update PWM.
	ESP_RETURN_ON_ERROR(
		pwm_write(
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
	__log_event_trimmer(
		TAG,
		id_to_pwm_index[device_id],
		pwm_enabled[device_id],
		pwm_duty[device_id],
		device_id,
		trimmer_val
	);

	return ESP_OK;
}

esp_err_t __handle_button_press(bool *pwm_enabled, uint16_t *pwm_duty, uint8_t device_id, uint16_t button_states){

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

		pwm_index = __id_button_and_state_to_pwm_index(device_id, button_id, button_state);

		// If the current configuration is not mapped.
		if(pwm_index == -1)
			continue;

		ESP_RETURN_ON_FALSE(
			pwm_index != -2,

			ESP_ERR_INVALID_ARG,
			TAG,
			"Error: invalid args passed on `__id_button_and_state_to_pwm_index(device_id=0x%02X, button_id=%u, button_state=%u)`",
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
				pwm_index,
				pwm_final_duty,
				CONFIG_PWM_FADE_TIME_MS
			),

			TAG,
			"Error on `pwm_write(pwm_index=%u, target_duty=%u)`",
			pwm_index, pwm_final_duty
		);

		// Log the event.
		__log_event_button(
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

int8_t __id_button_and_state_to_pwm_index(uint8_t device_id, ul_bs_button_id_t button_id, ul_bs_button_state_t button_state){

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
	int8_t __id_button_and_state_to_pwm_index
		[CONFIG_RS485_WALL_TERMINALS_COUNT]
		[CONFIG_BUTTONS_MAX_NUMBER_PER_TERMINAL]
		[UL_BS_BUTTON_STATE_MAX - 1] = CONFIG_BUTTON_MATRIX;

	return __id_button_and_state_to_pwm_index[device_id][button_id-1][button_state-1];
}

void __rs485_task(void *parameters){

	ESP_LOGI(TAG, "Started");

	/* Variables */

	// `ESP_GOTO_ON_ERROR()` return code.
	esp_err_t ret;

	// `__wall_terminals_poll()` parameters.
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
		pwm_duty[i] = pwm_led_gamma_correction(CONFIG_PWM_DEFAULT);

	ESP_ERROR_CHECK_WITHOUT_ABORT(uart_flush(CONFIG_RS485_UART_PORT));
	ESP_LOGI(TAG, "Polling slave devices");

	/* Infinite loop */
	for(;;){
		ret = ESP_OK;

		// Poll the wall terminals.
		ESP_GOTO_ON_ERROR(
			__wall_terminals_poll(&device_id, &trimmer_val, &button_states),

			task_error,
			TAG,
			"Error on `__wall_terminals_poll(device_id=0x%02X, trimmer_val=%u, button_states=%u)`",
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
				__handle_button_press(pwm_enabled, pwm_duty, device_id, button_states),

				task_error,
				TAG,
				"Error on `__handle_button_press(device_id=0x%02X, button_states=%u)`",
				device_id, button_states
			);

		// Trimmer rotated.
		else
			ESP_GOTO_ON_ERROR(
				__handle_trimmer_change(pwm_enabled, pwm_duty, device_id, trimmer_val),

				task_error,
				TAG,
				"Error on `__handle_button_press(device_id=0x%02X, trimmer_val=%u)`",
				device_id, trimmer_val
			);

		// Delay before continuing.
		goto task_continue;

		task_error:
		ESP_ERROR_CHECK_WITHOUT_ABORT(ret);
		ESP_ERROR_CHECK_WITHOUT_ABORT(uart_flush(CONFIG_RS485_UART_PORT));

		task_continue:
		delay(1);
	}
}

/************************************************************************************************************
* Public Functions Definitions
 ************************************************************************************************************/

esp_err_t rs485_setup(){

	ESP_RETURN_ON_ERROR(
		__uart_driver_setup(),

		TAG,
		"Error on `__uart_driver_setup()`"
	);

	ESP_RETURN_ON_ERROR(
		__rs485_task_setup(),

		TAG,
		"Error on `__rs485_task_setup()`"
	);

	return ESP_OK;
}
