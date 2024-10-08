/** @file gpio.h
 *  @brief  Created on: Aug 17, 2024
 *          Davide Scalisi
 *
 * 					Description:	RS485 code.
 *
 * @copyright [2024] Davide Scalisi *
 * @copyright All Rights Reserved. *
 *
*/

#ifndef INC_GPIO_H_
#define INC_GPIO_H_

/************************************************************************************************************
* Included files
************************************************************************************************************/

// Standard libraries.
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

// Platform libraries.
#include <esp_err.h>
#include <esp_check.h>
#include <esp_log.h>

#include <driver/gpio.h>

// Project libraries.
#include <main.h>
#include <zone.h>

/************************************************************************************************************
* Public Defines
************************************************************************************************************/

/**
 * @brief Turn ON/OFF alarm.
 */
#define gpio_set_alarm(level) \
	gpio_write_zone(ZONE_ALARM, level)

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
extern esp_err_t gpio_setup();

/**
 * @brief Write 0 or 1 to the mapped zone.
 */
extern esp_err_t gpio_write_zone(zone_t zone, uint8_t level);

#endif  /* INC_GPIO_H_ */
