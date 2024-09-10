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
// #define LOG_STUB

// Should be greater than `UART_HW_FIFO_LEN(uart_num)`.
#define UART_RX_BUFFER_LEN_BYTES	(UART_HW_FIFO_LEN(CONFIG_RS485_UART_PORT) * 2)

// Number of wall terminals on the RS-485 bus (from 1 to 127).
#define WALL_TERMINALS_COUNT	13

// Max number of buttons per wall terminal.
#define BUTTONS_MAX_NUMBER_PER_WALL_TERMINAL	3

// Response timeout on the poll phase.
#define WALL_TERMINAL_POLL_TIMEOUT_MS	30

// Response timeout on the data exchange phase.
#define WALL_TERMINAL_CONN_TIMEOUT_MS	100

// Default PWM value at startup for every logical PWM channel.
#define PWM_DEFAULT_VALUE	__led_gamma_correction(512)

/**
 * PWM fade time passed to every `pwm_write_zone()`.
 * Note: tune this value if by rotating a trimmer you notice a "step" increase/decrease in brightness effect.
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

#define __log_event_button(TAG, zone, enabled, duty, device_id, button_id, button_state) \
	ESP_LOGI( \
		TAG, "(zone=%u, enabled=%u, duty=%u) triggered by (device_id=%02u, button_id=%u, button_state=%u)", \
		zone, enabled, duty, device_id, button_id, button_state \
	)

#define __log_event_trimmer(TAG, zone, enabled, duty, device_id, trimmer_val) \
	ESP_LOGI( \
		TAG, "(zone=%u, enabled=%u, duty=%u) triggered by (device_id=%02u, trimmer_val=%u)", \
		zone, enabled, duty, device_id, trimmer_val \
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

/**
 * @return The corresponding zone given `device_id`, `button_id` and `button_state`.
 */
static zone_t __id_button_and_state_to_zone(uint8_t device_id, ul_bs_button_id_t button_id, ul_bs_button_state_t button_state);
static int8_t __zone_to_zone_index(zone_t zone);

static esp_err_t __handle_button_press(bool *channel_enabled, uint16_t *pwm_duty, uint8_t device_id, uint16_t button_states);
static esp_err_t __handle_trimmer_change(bool *channel_enabled, uint16_t *pwm_duty, uint8_t device_id, uint16_t trimmer_val);

/**
 * @brief Poll the RS-485 bus to check if some button was pressed on some wall terminal.
 * @param TAG The `esp_log.h` tag.
 * @param device_id Device ID of the wall terminal (from `0x00` to `0x7F`); if `0xFF`, no one has pressed any button.
 * @param trimmer_val Current 10-bit trimmer value.
 * @param button_states Raw button states; load them into the `ul_button_states.h` library by using `ul_bs_set_button_states()`.
 * @note Must be called periodically to ensure a clean wall terminals polling loop.
 */
static esp_err_t __wall_terminals_poll(uint8_t *device_id, uint16_t *trimmer_val, uint16_t *button_states);

static esp_err_t __uart_driver_setup();
static esp_err_t __rs485_task_setup();

static void __rs485_task(void *parameters);

/************************************************************************************************************
* Private Functions Definitions
 ************************************************************************************************************/

zone_t __id_button_and_state_to_zone(uint8_t device_id, ul_bs_button_id_t button_id, ul_bs_button_state_t button_state){

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
		return ZONE_UNMAPPED;

	// Mapping cube matrix (-1 because the idle state is not used on the last dimension).
	zone_t id_button_and_state_to_zones
		[WALL_TERMINALS_COUNT]
		[BUTTONS_MAX_NUMBER_PER_WALL_TERMINAL]
		[UL_BS_BUTTON_STATE_MAX - 1] = ZONE_BUTTONS;

	return id_button_and_state_to_zones[device_id][button_id-1][button_state-1];
}

int8_t __zone_to_zone_index(zone_t zone){

	// !!! CORREGGERE E FARE RICERCA SIA SU ZONE DIGITAL CHE PWM

	zone_t digital_zones[] = ZONE_DIGITAL_ZONES;
	uint8_t i = 0;

	while(i < ZONE_DIGITAL_LEN){
		if(digital_zones[i] == zone)
			break;

		i++;
	}

	return (
		digital_zones[i] == zone ?
		i : -1
	);
}

esp_err_t __handle_button_press(bool *channel_enabled, uint16_t *pwm_duty, uint8_t device_id, uint16_t button_states){

	ESP_RETURN_ON_FALSE(
		channel_enabled != NULL,

		ESP_ERR_INVALID_ARG,
		TAG,
		"Error: `channel_enabled` is NULL"
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

	#ifdef LOG_STUB
	ESP_LOGW(TAG, "LOG_STUB");
	ESP_LOGI(TAG, "Device ID: %02u", device_id);
	for(
		uint8_t button = UL_BS_BUTTON_1;
		button <= BUTTONS_MAX_NUMBER_PER_WALL_TERMINAL;
		button++
	)
		ESP_LOGI(
			TAG,
			"Button %u: %u",
			button,
			ul_bs_get_button_state(button)
		);

	#else

	ul_bs_button_state_t button_state;
	zone_t zone;
	int8_t zone_index;
	uint16_t pwm_final_duty;
	esp_err_t ret;

	// For each button, check if it was pressed.
	for(
		uint8_t button_id = UL_BS_BUTTON_1;
		button_id <= BUTTONS_MAX_NUMBER_PER_WALL_TERMINAL;
		button_id++
	){
		button_state = ul_bs_get_button_state(button_id);

		// Idle state not used.
		if(button_state == UL_BS_BUTTON_STATE_IDLE)
			continue;

		// Get zone from current configurations.
		zone = __id_button_and_state_to_zone(device_id, button_id, button_state);

		// If the zone is not mapped.
		if(zone == ZONE_UNMAPPED)
			continue;

		// Get mapped zone index.
		zone_index = __zone_to_zone_index(zone);

		ESP_RETURN_ON_FALSE(
			zone_index >= 0,

			ESP_ERR_NOT_FOUND,
			TAG,
			"Error on `__zone_to_zone_index(zone=%u)`",
			zone
		);

		// Toggle channel.
		channel_enabled[zone_index] =
			!channel_enabled[zone_index];

		pwm_final_duty = (
			channel_enabled[zone_index] ?
			pwm_duty[zone_index] :
			0
		);

		// Try to write both the GPIO and the PWM channel of the zone.
		ret = pwm_write_zone(
			zone,
			pwm_final_duty,
			PWM_FADE_TIME_MS
		);

		// Ignore this error code.
		if(ret == ESP_ERR_NOT_SUPPORTED)
			ret = ESP_OK;

		ESP_RETURN_ON_ERROR(
			ret,

			TAG,
			"Error on `pwm_write_zone(zone=%u, target_duty=%u)`",
			zone, pwm_final_duty
		);

		ret = gpio_write_zone(
			zone,
			channel_enabled[zone_index]
		);

		if(ret == ESP_ERR_NOT_SUPPORTED)
			ret = ESP_OK;

		ESP_RETURN_ON_ERROR(
			ret,

			TAG,
			"Error on `gpio_write_zone(zone=%u, level=%u)`",
			zone, channel_enabled[zone_index]
		);

		// Log the event.
		__log_event_button(
			TAG,
			zone,
			channel_enabled[zone_index],
			pwm_duty[zone_index],
			device_id,
			button_id,
			button_state
		);
	}

	#endif
	return ESP_OK;
}

esp_err_t __handle_trimmer_change(bool *channel_enabled, uint16_t *pwm_duty, uint8_t device_id, uint16_t trimmer_val){

	ESP_RETURN_ON_FALSE(
		channel_enabled != NULL,

		ESP_ERR_INVALID_ARG,
		TAG,
		"Error: `channel_enabled` is NULL"
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

	#ifdef LOG_STUB
	ESP_LOGW(TAG, "LOG_STUB");
	ESP_LOGI(TAG, "Device ID: %02u", device_id);
	ESP_LOGI(TAG, "Trimmer: %u", trimmer_val);
	#else

	// !!! VEDERE SE DEFERENZIARE CON `device_id` HA SENSO IN QUANTO DOVREBBE ESSERE CONSIDERATO `zone_id`
	// !!! NEL CASO, CORREGGERE DOCUMENTAZIONE IN ZONE.H

	// Do not enable the channel by rotating the trimmer.
	if(!channel_enabled[device_id])
		return ESP_OK;

	// Get zone.
	zone_t zone = ZONE_TRIMMERS[device_id];

	// Trimmer not mapped.
	if(zone == ZONE_UNMAPPED)
		return ESP_OK;

	// !!! VEDERE SE SPOSTARE SOTTO
	// Gamma correction.
	trimmer_val = __led_gamma_correction(trimmer_val);

	// Update the corresponding PWM state.
	channel_enabled[device_id] =
		(trimmer_val > 0);

	pwm_duty[device_id] =
		(trimmer_val > 0 ? trimmer_val : PWM_DEFAULT_VALUE);

	// Update PWM.
	ESP_RETURN_ON_ERROR(
		pwm_write_zone(
			zone,
			trimmer_val,
			PWM_FADE_TIME_MS
		),

		TAG,
		"Error on `pwm_write_zone(zone=%u, target_duty=%u)`",
		zone, trimmer_val
	);

	// Log the event.
	__log_event_trimmer(
		TAG,
		zone,
		channel_enabled[device_id],
		pwm_duty[device_id],
		device_id,
		trimmer_val
	);

	#endif
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
		"Error: slave device %02u answered with different ID %02u",
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
		"Error on `uart_read_bytes()` for slave device %02u",
		poll_device_id
	);

	// Timeout.
	ESP_RETURN_ON_FALSE(
		read_bytes > 0,

		ESP_ERR_TIMEOUT,
		TAG,
		"Error: slave device %02u exceeded the prefixed %ums timeout for sending its state",
		poll_device_id, WALL_TERMINAL_CONN_TIMEOUT_MS
	);

	// Invalid response.
	ESP_RETURN_ON_FALSE(
		read_bytes == sizeof(encoded_data),

		ESP_ERR_INVALID_RESPONSE,
		TAG,
		"Error: slave device %02u sent %u bytes; 4 expected",
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
		"Error #%u on `ul_ms_decode_slave_message()` for slave device %02u",
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
		"Error: invalid CRC8 for slave device %02u; sent CRC8 is %02u but computed CRC8 is %02u",
		poll_device_id, decoded_data.crc8, crc8
	);

	// Returned values.
	*device_id = poll_device_id;
	*trimmer_val = decoded_data.trimmer_val;
	*button_states = decoded_data.button_states;

	return ESP_OK;
}

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
		CONFIG_RS485_TASK_CORE_AFFINITY
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

void __rs485_task(void *parameters){

	ESP_LOGI(TAG, "Started");

	/* Variables */

	// `ESP_GOTO_ON_ERROR()` return code.
	esp_err_t ret __attribute__((unused));

	// `__wall_terminals_poll()` parameters.
	uint8_t device_id;
	uint16_t trimmer_val, button_states;

	// PWM values for each PWM index.
	uint16_t pwm_duty[ZONE_PWM_LEN];
	bool channel_enabled[ZONE_PWM_LEN] = {0};

	/* Code */

	// `pwm_duty[]` initialization.
	for(uint8_t i=0; i < ZONE_PWM_LEN; i++)
		pwm_duty[i] = PWM_DEFAULT_VALUE;

	ESP_ERROR_CHECK_WITHOUT_ABORT(uart_flush(CONFIG_RS485_UART_PORT));
	ESP_LOGI(TAG, "Polling slave devices");

	/* Infinite loop */
	for(;;){
		ret = ESP_OK;

		// Yield task to the scheduler.
		delay(1);

		// Poll the wall terminals.
		ESP_GOTO_ON_ERROR(
			__wall_terminals_poll(&device_id, &trimmer_val, &button_states),

			task_error,
			TAG,
			"Error on `__wall_terminals_poll(device_id=%02u, trimmer_val=%u, button_states=%u)`",
			device_id, trimmer_val, button_states
		);

		// Nothing to communicate.
		if(device_id == 0xFF)
			continue;

		// Device ID check.
		ESP_GOTO_ON_FALSE(
			device_id < WALL_TERMINALS_COUNT,

			ESP_ERR_NOT_SUPPORTED,
			task_error,
			TAG,
			"Error: the returned `device_id` is %02u; max allowed is %02u",
			device_id, WALL_TERMINALS_COUNT - 1
		);

		// Button pressed.
		if(button_states != 0)
			ESP_GOTO_ON_ERROR(
				__handle_button_press(channel_enabled, pwm_duty, device_id, button_states),

				task_error,
				TAG,
				"Error on `__handle_button_press(device_id=%02u, button_states=%u)`",
				device_id, button_states
			);

		// Trimmer rotated.
		else
			ESP_GOTO_ON_ERROR(
				__handle_trimmer_change(channel_enabled, pwm_duty, device_id, trimmer_val),

				task_error,
				TAG,
				"Error on `__handle_trimmer_change(device_id=%02u, trimmer_val=%u)`",
				device_id, trimmer_val
			);

		continue;
		task_error:
		ESP_ERROR_CHECK_WITHOUT_ABORT(uart_flush(CONFIG_RS485_UART_PORT));
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
