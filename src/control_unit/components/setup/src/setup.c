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
	__to_bit_mask(CONFIG_GPIO_UART_DE_RE) | \
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
	return ESP_OK;
}
