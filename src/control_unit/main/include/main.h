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

// Standard libraries.
#include <stdint.h>
#include <stdbool.h>

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
 * @brief `delay( ms - (millis() - initial_timestamp_ms) )`
 */
extern void delay_remainings(int32_t ms, int64_t initial_timestamp_ms);

/**
 * @brief Log the given binary hash.
 */
extern void log_hash(const char *TAG, const char* label, uint8_t *hash, uint16_t hash_len);

#endif  /* INC_MAIN_H_ */
