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
	__to_bit_mask(CONFIG_GPIO_FAN) | \
	__to_bit_mask(CONFIG_GPIO_LED_1) | \
	__to_bit_mask(CONFIG_GPIO_LED_2) | \
	__to_bit_mask(CONFIG_GPIO_LED_3) | \
	__to_bit_mask(CONFIG_GPIO_LED_4) | \
	__to_bit_mask(CONFIG_GPIO_LED_5) | \
	__to_bit_mask(CONFIG_GPIO_LED_6) | \
	__to_bit_mask(CONFIG_GPIO_LED_7) | \
	__to_bit_mask(CONFIG_GPIO_LED_8) | \
	__to_bit_mask(CONFIG_GPIO_LED_9) | \
	__to_bit_mask(CONFIG_GPIO_LED_10) | \
	__to_bit_mask(CONFIG_GPIO_LED_11) | \
	__to_bit_mask(CONFIG_GPIO_LED_12) | \
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

/************************************************************************************************************
* Private Types Definitions
 ************************************************************************************************************/

/************************************************************************************************************
* Private Variables
 ************************************************************************************************************/

static const char *TAG = "setup";

/************************************************************************************************************
* Private Functions Prototypes
 ************************************************************************************************************/

/************************************************************************************************************
* Private Functions Definitions
 ************************************************************************************************************/

/************************************************************************************************************
* Public Functions Definitions
 ************************************************************************************************************/

esp_err_t GPIO_setup(){

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

esp_err_t UART_setup(){

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

	// ESP_RETURN_ON_ERROR(
	// 	uart_set_mode(
	// 		CONFIG_UART_PORT,
	// 		UART_MODE_RS485_HALF_DUPLEX
	// 	),

	// 	TAG,
	// 	"Error on `uart_set_mode()`"
	// );

	return ESP_OK;
}
