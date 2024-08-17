/** @file main.h
 *  @brief  Created on: Aug 17, 2024
 *          Davide Scalisi
 *
 * 					Description:	Project common header.
 *
 * @copyright [2024] Davide Scalisi *
 * @copyright All Rights Reserved. *
 *
*/

#ifndef INC_MAIN_H_
#define INC_MAIN_H_

/************************************************************************************************************
* Included files
************************************************************************************************************/

// Platform libraries.
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>

/************************************************************************************************************
* Public Defines
************************************************************************************************************/

#define delay(ms) \
	vTaskDelay(pdMS_TO_TICKS(ms))

#define micros() \
	esp_timer_get_time()

#define millis()( \
	micros() / 1000 \
)

// !!! SPOSTARE

/**
 * @brief Converts the linear scale of the PWM to logarithmic by applying a gamma correction with a coefficient `CONFIG_PWM_GAMMA_CORRECTION`.
 */
#define pwm_led_gamma_correction(pwm_duty)( \
	(uint16_t)( \
		pow( \
			(float)(pwm_duty) / CONFIG_PWM_DUTY_MAX, \
			CONFIG_PWM_GAMMA_CORRECTION \
		) * CONFIG_PWM_DUTY_MAX \
	) \
)

/************************************************************************************************************
* Public Types Definitions
************************************************************************************************************/

/************************************************************************************************************
* Public Variables Prototypes
************************************************************************************************************/

/************************************************************************************************************
* Public Functions Prototypes
************************************************************************************************************/

#endif  /* INC_MAIN_H_ */
