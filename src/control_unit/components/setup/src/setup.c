/** @file setup.c
 *  @brief  Created on: Aug 3, 2024
 *          Davide Scalisi
 *
 * @copyright [2024] Davide Scalisi *
 * @copyright All Rights Reserved. *
 *
*/

/************************************************************************************************************
* Included files
************************************************************************************************************/

#include <setup.h>

/************************************************************************************************************
* Private Defines
************************************************************************************************************/

/**
 * @brief Helper.
 */
#define __to_bit_mask(x)	(1ULL << x)

// `gpio_config_t::pin_bit_mask` for output GPIOs.
#define GPIO_OUT_BIT_MASK	( \
	__to_bit_mask(CONFIG_GPIO_ALARM) | \
	__to_bit_mask(CONFIG_GPIO_RELAY_1) | \
	__to_bit_mask(CONFIG_GPIO_RELAY_2) | \
	__to_bit_mask(CONFIG_GPIO_RELAY_3) | \
	__to_bit_mask(CONFIG_GPIO_RELAY_4) \
)

// `gpio_config_t::pin_bit_mask` for input GPIOs.
#define GPIO_IN_BIT_MASK	( \
	__to_bit_mask(CONFIG_GPIO_TEMP) | \
	__to_bit_mask(CONFIG_GPIO_AC_V) | \
	__to_bit_mask(CONFIG_GPIO_AC_I) \
)

#define PWM_CH_TO_GPIO_INIT	{ \
	CONFIG_GPIO_FAN, \
	CONFIG_GPIO_LED_1, \
	CONFIG_GPIO_LED_2, \
	CONFIG_GPIO_LED_3, \
	CONFIG_GPIO_LED_4, \
	CONFIG_GPIO_LED_5, \
	CONFIG_GPIO_LED_6, \
	CONFIG_GPIO_LED_7, \
	CONFIG_GPIO_LED_8, \
	CONFIG_GPIO_LED_9, \
	CONFIG_GPIO_LED_10, \
	CONFIG_GPIO_LED_11, \
	CONFIG_GPIO_LED_12 \
}

/************************************************************************************************************
* Private Types Definitions
 ************************************************************************************************************/

/************************************************************************************************************
* Private Variables
 ************************************************************************************************************/

/************************************************************************************************************
* Private Functions Prototypes
 ************************************************************************************************************/

/************************************************************************************************************
* Private Functions Definitions
 ************************************************************************************************************/

/************************************************************************************************************
* Public Functions Definitions
 ************************************************************************************************************/

esp_err_t GPIO_setup(const char *TAG){

	// Shared configs.
	gpio_config_t io_config = {
		.intr_type = GPIO_INTR_DISABLE,
		.pull_up_en = 0,
		.pull_down_en = 0
	};

	// Output.
	io_config.mode = GPIO_MODE_OUTPUT;
	io_config.pin_bit_mask = GPIO_OUT_BIT_MASK;

	ESP_RETURN_ON_ERROR(
		gpio_config(&io_config),
		TAG,
		"Error on `gpio_config()`"
	);

	// Input.
	io_config.mode = GPIO_MODE_INPUT;
	io_config.pin_bit_mask = GPIO_IN_BIT_MASK;

	ESP_RETURN_ON_ERROR(
		gpio_config(&io_config),
		TAG,
		"Error on `gpio_config()`"
	);

	return ESP_OK;
}

esp_err_t LEDC_setup(const char *TAG){

	/* LEDC base config */

	ledc_timer_config_t ledc_tim_config = {
		.freq_hz = 30000,
		.duty_resolution = LEDC_TIMER_8_BIT,
		.timer_num = LEDC_TIMER_1,
		.clk_cfg = LEDC_AUTO_CLK
	};

	/**
	 * LEDC hardware:
	 * 	-	One peripheral.
	 * 	-	Two ports (high/low speed).
	 * 	-	Eight PWM channel per port.
	 */
	ledc_channel_config_t ledc_ch_config = {
		.intr_type = LEDC_INTR_DISABLE,
		.timer_sel = LEDC_TIMER_1,
		.duty = 0,
		.hpoint = 0,
		.flags.output_invert = false
	};

	// Timer setup for both ports.
	for(uint8_t i=0; i<LEDC_SPEED_MODE_MAX; i++){

		ledc_tim_config.speed_mode = i;

		ESP_RETURN_ON_ERROR(
			ledc_timer_config(&ledc_tim_config),
			TAG,
			"Error on `ledc_timer_config({.speed_mode = %u})`",
			i
		);
	}

	uint8_t pwm_ch_to_gpio[] = PWM_CH_TO_GPIO_INIT;

	// Channel setup for both ports.
	for(uint8_t i=0; i<sizeof(pwm_ch_to_gpio); i++){

		ledc_ch_config.gpio_num = pwm_ch_to_gpio[i];
		ledc_ch_config.speed_mode = pwm_get_port(i);
		ledc_ch_config.channel = pwm_get_channel(i);

		ESP_RETURN_ON_ERROR(
			ledc_channel_config(&ledc_ch_config),
			TAG,
			"Error on `ledc_channel_config({.gpio_num = %u})`",
			pwm_ch_to_gpio[i]
		);
	}

	/* LEDC fade config */

	ledc_fade_func_install(0);

	return ESP_OK;
}

esp_err_t UART_setup(const char *TAG){

	uart_config_t uart_config = {
		.baud_rate = CONFIG_UART_BAUD_RATE,
		.data_bits = UART_DATA_8_BITS,
		.parity = UART_PARITY_DISABLE,
		.stop_bits = UART_STOP_BITS_1,
		.flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
		.rx_flow_ctrl_thresh = 122,
		.source_clk = UART_SCLK_DEFAULT,
	};

	ESP_RETURN_ON_ERROR(
		uart_driver_install(
			CONFIG_UART_PORT,
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
			CONFIG_UART_PORT,
			&uart_config
		),

		TAG,
		"Error on `uart_param_config()`"
	);

	ESP_RETURN_ON_ERROR(
		uart_set_pin(
			CONFIG_UART_PORT,
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
			CONFIG_UART_PORT,
			UART_MODE_RS485_HALF_DUPLEX
		),

		TAG,
		"Error on `uart_set_mode()`"
	);

	return ESP_OK;
}

esp_err_t TASKS_setup(const char *TAG){

	BaseType_t task_creation_ret;

	/* rs485_task */

	extern TaskHandle_t rs485_task_handle;
	extern void rs485_task(void *parameters);

	task_creation_ret = xTaskCreatePinnedToCore(
		rs485_task,
		"rs485_task",
		4096,
		NULL,
		tskIDLE_PRIORITY,
		&rs485_task_handle,
		ESP_APPLICATION_CORE
	);

	ESP_RETURN_ON_FALSE(
		task_creation_ret == pdPASS,

		ESP_ERR_INVALID_STATE,
		TAG,
		"Error %d: unable to spawn the \"rs485_task\"",
		task_creation_ret
	);

	return ESP_OK;
}
