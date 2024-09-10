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
#include <zone.h>

/************************************************************************************************************
* Public Defines
************************************************************************************************************/

// PWM bit resolution.
#define PWM_BIT_RES		10

// PWM max duty.
#define PWM_DUTY_MAX	((1 << PWM_BIT_RES) - 1)

/************************************************************************************************************
* Public Types Definitions
************************************************************************************************************/

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
 * @brief Write the PWM duty target to the mapped zone.
 * @param target_duty Target duty from 0 to ((2^`PWM_BIT_RES`) - 1).
 * @param fade_time_ms Fade time up to 16383ms.
 */
extern esp_err_t pwm_write_zone(uint8_t zone, uint16_t target_duty, uint16_t fade_time_ms);

#endif  /* INC_PWM_H_ */
