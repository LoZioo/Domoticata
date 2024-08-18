/** @file pm.h
 *  @brief  Created on: Aug 17, 2024
 *          Davide Scalisi
 *
 * 					Description:	PowerMonitor code.
 *
 * @copyright [2024] Davide Scalisi *
 * @copyright All Rights Reserved. *
 *
*/

#ifndef INC_PM_H_
#define INC_PM_H_

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
#include <esp_attr.h>

#include <freertos/FreeRTOS.h>
#include <esp_adc/adc_continuous.h>

// UniLibC libraries.
#include <ul_errors.h>
#include <ul_utils.h>
#include <ul_pm.h>

// Project libraries.
#include <main.h>

/************************************************************************************************************
* Public Defines
************************************************************************************************************/

// !!! vedere se rendere private

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

/************************************************************************************************************
* Public Variables Prototypes
************************************************************************************************************/

/************************************************************************************************************
* Public Functions Prototypes
************************************************************************************************************/

/**
 * @brief Initialize the library.
 */
extern esp_err_t pm_setup();

#endif  /* INC_PM_H_ */
