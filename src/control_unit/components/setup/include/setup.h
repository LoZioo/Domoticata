/** @file setup.h
 *  @brief  Created on: Aug 3, 2024
 *          Davide Scalisi
 *
 * 					Description:	Project module to initialize onboard peripherals.
 *
 * @copyright [2024] Davide Scalisi *
 * @copyright All Rights Reserved. *
 *
*/

#ifndef INC_SETUP_H_
#define INC_SETUP_H_

/************************************************************************************************************
* Included files
************************************************************************************************************/

// Standard libraries.
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

// Platform libraries.
#include <esp_err.h>
#include <esp_check.h>

#include <freertos/FreeRTOS.h>

#include <soc/soc_caps.h>
#include <esp_adc/adc_continuous.h>

#include <driver/gpio.h>
#include <driver/ledc.h>
#include <driver/uart.h>

/************************************************************************************************************
* Public Defines
************************************************************************************************************/

/* Miscellaneous */

#define ESP_PROTOCOL_CORE			0
#define ESP_APPLICATION_CORE	1

/* LEDC PWM */

/**
 * @return `ledc_mode_t`
 * @note 0: Fan controller, 1-12: LEDs.
 */
#define pwm_get_port(i)( \
	i < LEDC_CHANNEL_MAX ? \
	LEDC_HIGH_SPEED_MODE : \
	LEDC_LOW_SPEED_MODE \
)

/**
 * @return `ledc_channel_t`
 * @note 0: Fan controller, 1-12: LEDs.
 */
#define pwm_get_channel(i)( \
	(ledc_channel_t) (i % LEDC_CHANNEL_MAX) \
)

/* PowerMonitor */

/**
 * @brief Convert the needed samples length for the "PowerMonitor" feature to the corresponding total byte length for the allocation of the `esp_adc/adc_continuous.h` driver's buffer.
 * @return `samples_len` * 2 channels * 2 bytes/sample.
 * @note The resulting number must be a multiple of `SOC_ADC_DIGI_DATA_BYTES_PER_CONV` on `soc/soc_caps.h`.
 */
#define pm_samples_len_to_buf_size(samples_len)( \
	samples_len * 4 \
)

/************************************************************************************************************
* Public Types Definitions
************************************************************************************************************/

/* LEDC PWM */

typedef struct __attribute__((__packed__)) {

	// 0: Fan controller, 1-12: LEDs.
	uint8_t pwm_index: 4;

	// Up to `CONFIG_PWM_DUTY_MAX`.
	uint16_t target_duty: CONFIG_LEDC_PWM_BIT_RES;

	// Up to 1023ms.
	uint16_t fade_time_ms: 10;

} pwm_data_t;

/************************************************************************************************************
* Public Variables Prototypes
************************************************************************************************************/

/************************************************************************************************************
* Public Functions Prototypes
************************************************************************************************************/

extern esp_err_t GPIO_setup(const char *TAG);
extern esp_err_t LEDC_setup(const char *TAG);
extern esp_err_t ADC_setup(const char *TAG);

// extern esp_err_t QUEUES_setup(const char *TAG);
// extern esp_err_t TASKS_setup(const char *TAG);

#endif  /* INC_SETUP_H_ */
