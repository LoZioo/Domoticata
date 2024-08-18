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

// Should be greater than `UART_HW_FIFO_LEN(uart_num)`.
#define UART_RX_BUFFER_LEN_BYTES	(UART_HW_FIFO_LEN(CONFIG_RS485_UART_PORT) * 2)

// Number of wall terminals on the RS-485 bus (from 1 to 127).
#define WALL_TERMINALS_COUNT	13

// Response timeout on the poll phase.
#define WALL_TERMINAL_POLL_TIMEOUT_MS	30

// Response timeout on the data exchange phase.
#define WALL_TERMINAL_CONN_TIMEOUT_MS	100

// Max number of buttons per wall terminal.
#define BUTTONS_MAX_NUMBER_PER_WALL_TERMINAL	UL_BS_BUTTON_3

// Default PWM value at startup for every logical PWM channel.
#define PWM_DEFAULT_VALUE	__led_gamma_correction(512)

/**
 * PWM fade time passed to every `pwm_write()`.
 * Note: tune this value if by rotating the trimmer you notice a "step" increase/decrease in brightness effect.
 */
#define PWM_FADE_TIME_MS	500

// PWM LED gamma correction factor.
#define LED_GAMMA_CORRECTION_FACTOR		2.2

/**
 * @brief Converts the linear scale of the PWM to logarithmic by applying a gamma correction with coefficient `LED_GAMMA_CORRECTION_FACTOR`.
 */
#define __led_gamma_correction(pwm_duty)( \
	(uint16_t)( \
		pow( \
			(float)(pwm_duty) / PWM_DUTY_MAX, \
			LED_GAMMA_CORRECTION_FACTOR \
		) * PWM_DUTY_MAX \
	) \
)

#define __log_event_button(TAG, zone, pwm_enabled, pwm_duty, device_id, button_id, button_state) \
	ESP_LOGI( \
		TAG, "PWM index %u: (enabled=%u, value=%u), triggered by: (device_id=0x%02X, button_id=%u, button_state=%u)", \
		zone, pwm_enabled, pwm_duty, device_id, button_id, button_state \
	)

#define __log_event_trimmer(TAG, zone, pwm_enabled, pwm_duty, device_id, trimmer_val) \
	ESP_LOGI( \
		TAG, "PWM index %u: (enabled=%u, value=%u), triggered by: (device_id=0x%02X, trimmer_val=%u)", \
		zone, pwm_enabled, pwm_duty, device_id, trimmer_val \
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
 * @return The corresponding `rs485_keymap_t` containing the mapped zones, given `device_id`, `button_id` and `button_state`; if one zone is equal to 0, it means that it is unmapped.
 */
static rs485_keymap_t __id_button_and_state_to_zones(uint8_t device_id, ul_bs_button_id_t button_id, ul_bs_button_state_t button_state);

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
			UART_RX_BUFFER_LEN_BYTES,
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
		CONFIG_RS485_TASK_STACK_SIZE_BYTES,
		NULL,
		CONFIG_RS485_TASK_PRIORITY,
		&__rs485_task_handle,

		#ifdef CONFIG_RS485_TASK_CORE_AFFINITY_APPLICATION
			ESP_APPLICATION_CORE
		#else
			ESP_PROTOCOL_CORE
		#endif
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
	static uint8_t poll_device_id = WALL_TERMINALS_COUNT - 1;
	poll_device_id = (poll_device_id + 1) % WALL_TERMINALS_COUNT;

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
		pdMS_TO_TICKS(WALL_TERMINAL_POLL_TIMEOUT_MS)
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
		sizeof(encoded_data),
		pdMS_TO_TICKS(WALL_TERMINAL_CONN_TIMEOUT_MS)
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
		poll_device_id, WALL_TERMINAL_CONN_TIMEOUT_MS
	);

	// Invalid response.
	ESP_RETURN_ON_FALSE(
		read_bytes == sizeof(encoded_data),

		ESP_ERR_INVALID_RESPONSE,
		TAG,
		"Error: slave device 0x%02X sent %u bytes; 4 expected",
		poll_device_id, read_bytes
	);

	// Decode the received bytes.
	ul_err_t ret_val = ul_ms_decode_slave_message(
		ul_utils_cast_to_mem(decoded_data),
		encoded_data,
		sizeof(encoded_data)
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
		device_id < WALL_TERMINALS_COUNT,

		ESP_ERR_INVALID_ARG,
		TAG,
		"Error: `device_id` must be less than %u",
		WALL_TERMINALS_COUNT
	);

	ESP_RETURN_ON_FALSE(
		trimmer_val <= PWM_DUTY_MAX,

		ESP_ERR_INVALID_ARG,
		TAG,
		"Error: `trimmer_val` must be between 0 and %u",
		PWM_DUTY_MAX
	);

	// Mapping array.
	rs485_keymap_t id_to_zone[] = RS485_KEYMAP_TRIMMER;

	// Trimmer not mapped.
	if(id_to_zone[device_id].raw == 0)
		return ESP_OK;

	// Gamma correction.
	trimmer_val = __led_gamma_correction(trimmer_val);

	uint8_t zones_arr[] = {
		id_to_zone[device_id].zones.zone_1,
		id_to_zone[device_id].zones.zone_2
	};

	for(uint8_t i=0; i<sizeof(zones_arr); i++){

		// Do not enable the channel by rotating the trimmer.
		if(!pwm_enabled[zones_arr[i]])
			continue;

		// Update the corresponding PWM state.
		pwm_enabled[zones_arr[i]] =
			(trimmer_val > 0);

		pwm_duty[zones_arr[i]] =
			(trimmer_val > 0 ? trimmer_val : PWM_DEFAULT_VALUE);

		// Update PWM.
		ESP_RETURN_ON_ERROR(
			pwm_write(
				zones_arr[i],
				trimmer_val,
				PWM_FADE_TIME_MS
			),

			TAG,
			"Error on `pwm_write(zone=%u, target_duty=%u)`",
			zones_arr[i], trimmer_val
		);

		// Log the event.
		__log_event_trimmer(
			TAG,
			zones_arr[i],
			pwm_enabled[zones_arr[i]],
			pwm_duty[zones_arr[i]],
			device_id,
			trimmer_val
		);
	}

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
		device_id < WALL_TERMINALS_COUNT,

		ESP_ERR_INVALID_ARG,
		TAG,
		"Error: `device_id` must be less than %u",
		WALL_TERMINALS_COUNT
	);

	ESP_RETURN_ON_FALSE(
		button_states != 0,

		ESP_ERR_INVALID_ARG,
		TAG,
		"Error: `button_states` is 0"
	);

	ul_bs_set_button_states(button_states);

	// Auxiliary variables.
	rs485_keymap_t zones;
	ul_bs_button_state_t button_state;

	// For each button, check if it was pressed.
	for(
		uint8_t button_id = UL_BS_BUTTON_1;
		button_id <= BUTTONS_MAX_NUMBER_PER_WALL_TERMINAL;
		button_id++
	){
		button_state = ul_bs_get_button_state(button_id);

		// Idle state is not used.
		if(button_state == UL_BS_BUTTON_STATE_IDLE)
			continue;

		zones = __id_button_and_state_to_zones(device_id, button_id, button_state);

		// If the current configuration is not mapped.
		if(zones.raw == 0)
			continue;

		uint8_t zones_arr[] = {
			zones.zones.zone_1,
			zones.zones.zone_2
		};

		bool one_zone_enabled = false;
		for(uint8_t i=0; i<sizeof(zones_arr); i++)
			if(
				zones_arr[i] > 0 &&
				!pwm_enabled[zones_arr[i]]
			){

				// If at least one zone is enabled, enable every selected zone.
				one_zone_enabled = true;
				break;
			}

		uint16_t pwm_final_duty;
		for(uint8_t i=0; i<sizeof(zones_arr); i++)
			if(zones_arr[i] > 0){

				// Turn ON/OFF PWM.
				pwm_enabled[zones_arr[i]] = (
					one_zone_enabled ?
					true :
					!pwm_enabled[zones_arr[i]]
				);

				pwm_final_duty = (
					pwm_enabled[zones_arr[i]] ?
					pwm_duty[zones_arr[i]] :
					0
				);

				// Update PWM.
				ESP_RETURN_ON_ERROR(
					pwm_write(
						zones_arr[i],
						pwm_final_duty,
						PWM_FADE_TIME_MS
					),

					TAG,
					"Error on `pwm_write(zone=%u, target_duty=%u)`",
					zones_arr[i], pwm_final_duty
				);

				// Log the event.
				__log_event_button(
					TAG,
					zones_arr[i],
					pwm_enabled[zones_arr[i]],
					pwm_duty[zones_arr[i]],
					device_id,
					button_id,
					button_state
				);
			}
	}

	return ESP_OK;
}

rs485_keymap_t __id_button_and_state_to_zones(uint8_t device_id, ul_bs_button_id_t button_id, ul_bs_button_state_t button_state){

	// Index error.
	if(
		device_id >= WALL_TERMINALS_COUNT ||

		!ul_utils_between(
			button_id,
			UL_BS_BUTTON_1,
			BUTTONS_MAX_NUMBER_PER_WALL_TERMINAL
		) ||

		!ul_utils_between(
			button_state,
			UL_BS_BUTTON_STATE_PRESSED,
			UL_BS_BUTTON_STATE_HELD
		)
	)
		return (rs485_keymap_t){
			.raw = 0
		};

	// Mapping cube matrix (idle state is not used on the last index).
	rs485_keymap_t id_button_and_state_to_zones
		[WALL_TERMINALS_COUNT]
		[BUTTONS_MAX_NUMBER_PER_WALL_TERMINAL]
		[UL_BS_BUTTON_STATE_MAX - 1] = RS485_KEYMAP_BUTTON;

	return id_button_and_state_to_zones[device_id][button_id-1][button_state-1];
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
	uint16_t pwm_duty[PWM_ZONES];
	bool pwm_enabled[PWM_ZONES] = {0};

	/* Code */

	// `pwm_duty[]` initialization.
	for(
		uint8_t i = 0;
		i < sizeof(pwm_duty) / sizeof(uint16_t);
		i++
	)
		pwm_duty[i] = PWM_DEFAULT_VALUE;

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
			device_id < WALL_TERMINALS_COUNT,

			ESP_ERR_NOT_SUPPORTED,
			task_error,
			TAG,
			"Error: the returned `device_id` is 0x%02X; max allowed is 0x%02X",
			device_id, WALL_TERMINALS_COUNT - 1
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
				"Error on `__handle_trimmer_change(device_id=0x%02X, trimmer_val=%u)`",
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
