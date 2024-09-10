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
#define __gpio_to_bit_mask(digital_gpio)	(1ULL << digital_gpio)

/************************************************************************************************************
* Private Types Definitions
 ************************************************************************************************************/

/************************************************************************************************************
* Private Variables
 ************************************************************************************************************/

static const char *TAG = LOG_TAG;
static bool __is_initialized = false;

/************************************************************************************************************
* Private Functions Prototypes
 ************************************************************************************************************/

static int8_t __zone_to_digital_gpio(zone_t zone);

/************************************************************************************************************
* Private Functions Definitions
 ************************************************************************************************************/

int8_t __zone_to_digital_gpio(zone_t zone){

	uint8_t digital_gpios[] = ZONE_DIGITAL_GPIO;
	zone_t digital_zones[] = ZONE_DIGITAL_ZONES;
	uint8_t i = 0;

	while(i < ZONE_DIGITAL_LEN){
		if(digital_zones[i] == zone)
			break;

		i++;
	}

	return (
		digital_zones[i] == zone ?
		digital_gpios[i] :
		-1
	);
}

/************************************************************************************************************
* Public Functions Definitions
 ************************************************************************************************************/

esp_err_t gpio_setup(){

	gpio_config_t io_config = {
		.pin_bit_mask = 0,
		.mode = GPIO_MODE_OUTPUT,
		.intr_type = GPIO_INTR_DISABLE,
		.pull_up_en = 0,
		.pull_down_en = 0
	};

	uint8_t digital_gpios[] = ZONE_DIGITAL_GPIO;
	for(uint8_t i=0; i<ZONE_DIGITAL_LEN; i++)
		io_config.pin_bit_mask |=
			__gpio_to_bit_mask(digital_gpios[i]);

	ESP_RETURN_ON_ERROR(
		gpio_config(&io_config),

		TAG,
		"Error on `gpio_config()`"
	);

	__is_initialized = true;
	return ESP_OK;
}

esp_err_t gpio_write_zone(zone_t zone, uint8_t level){

	ESP_RETURN_ON_FALSE(
		__is_initialized,

		ESP_ERR_INVALID_STATE,
		TAG,
		"Error: library not initialized"
	);

	int8_t digital_gpio = __zone_to_digital_gpio(zone);
	level = (level > 0);

	// The zone is is not a digital zone.
	if(digital_gpio == -1)
		return ESP_ERR_NOT_SUPPORTED;

	ESP_RETURN_ON_ERROR(
		gpio_set_level(
			digital_gpio,
			level
		),

		TAG,
		"Error on `gpio_set_level(digital_gpio=%u, level=%u)`",
		digital_gpio, level
	);

	return ESP_OK;
}
