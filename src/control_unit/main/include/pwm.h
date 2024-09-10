/** @file pwm.h
 *  @brief  Created on: Aug 17, 2024
 *          Davide Scalisi
 *
 * 					Description:	LEDC PWM code.
 *
 * @copyright [2024] Davide Scalisi *
 * @copyright All Rights Reserved. *
 *
*/

#ifndef INC_PWM_H_
#define INC_PWM_H_

/************************************************************************************************************
* Included files
************************************************************************************************************/

// Standard libraries.
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

// Platform libraries.
#include <esp_err.h>
#include <esp_check.h>
#include <esp_log.h>

#include <freertos/FreeRTOS.h>
#include <driver/ledc.h>

// Project libraries.
#include <main.h>

/************************************************************************************************************
* Public Defines
************************************************************************************************************/

// PWM bit resolution.
#define PWM_BIT_RES		10

// PWM max duty.
#define PWM_DUTY_MAX	((1 << PWM_BIT_RES) - 1)

// Number of logical PWM channels on the physical board.
#define PWM_ZONES		13

/************************************************************************************************************
* Public Types Definitions
************************************************************************************************************/

typedef struct __attribute__((__packed__)) {

	// 0: Fan controller, 1-12: LEDs.
	uint8_t zone: 4;

	// Up to `PWM_DUTY_MAX`.
	uint16_t target_duty: PWM_BIT_RES;

	// Up to 1023ms.
	uint16_t fade_time_ms: 10;

} pwm_data_t;

/************************************************************************************************************
* Public Variables Prototypes
************************************************************************************************************/

/************************************************************************************************************
* Public Functions Prototypes
************************************************************************************************************/

/**
 * @brief Initialize the library.
 */
extern esp_err_t pwm_setup();

/**
 * @brief Send `pwm_data` to `pwm_task` via `pwm_queue`.
 * @param zone 0: Fan controller, 1-12: LEDs.
 * @param target_duty 10-bit target duty (from 0 to (2^`CONFIG_LEDC_PWM_BIT_RES`) - 1 ).
 */
extern esp_err_t pwm_write_zone(uint8_t zone, uint16_t target_duty, uint16_t fade_time_ms);

#endif  /* INC_PWM_H_ */
