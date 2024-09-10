/** @file gpio.c
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

#include <gpio.h>

/************************************************************************************************************
* Private Defines
************************************************************************************************************/

#define LOG_TAG	"gpio"

/**
 * @brief Helper.
 */
#define __gpio_to_bit_mask(gpio_num)	(1ULL << gpio_num)

// `gpio_config_t::pin_bit_mask` for output GPIOs.
#define GPIO_OUT_BIT_MASK	( \
	__gpio_to_bit_mask(CONFIG_GPIO_ALARM) | \
	__gpio_to_bit_mask(CONFIG_GPIO_RELAY_1) | \
	__gpio_to_bit_mask(CONFIG_GPIO_RELAY_2) | \
	__gpio_to_bit_mask(CONFIG_GPIO_RELAY_3) | \
	__gpio_to_bit_mask(CONFIG_GPIO_RELAY_4) \
)

/************************************************************************************************************
* Private Types Definitions
 ************************************************************************************************************/

/************************************************************************************************************
* Private Variables
 ************************************************************************************************************/

static const char *TAG = LOG_TAG;

/************************************************************************************************************
* Private Functions Prototypes
 ************************************************************************************************************/

/************************************************************************************************************
* Private Functions Definitions
 ************************************************************************************************************/

/************************************************************************************************************
* Public Functions Definitions
 ************************************************************************************************************/

esp_err_t gpio_setup(){

	gpio_config_t io_config = {
		.pin_bit_mask = GPIO_OUT_BIT_MASK,
		.mode = GPIO_MODE_OUTPUT,
		.intr_type = GPIO_INTR_DISABLE,
		.pull_up_en = 0,
		.pull_down_en = 0
	};

	ESP_RETURN_ON_ERROR(
		gpio_config(&io_config),

		TAG,
		"Error on `gpio_config()`"
	);

	return ESP_OK;
}

esp_err_t gpio_write(gpio_num_t gpio_num, uint8_t level){

	level = (level > 0);
	ESP_RETURN_ON_ERROR(
		gpio_set_level(
			gpio_num,
			level
		),

		TAG,
		"Error on `gpio_set_level(gpio_num=%u, level=%u)`",
		gpio_num, level
	);

	return ESP_OK;
}
