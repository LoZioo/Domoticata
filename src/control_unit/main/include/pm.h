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

/**
 * @brief Start the computation.
 */
extern esp_err_t pm_start_compute();

/**
 * @brief Stop the computation.
 */
extern esp_err_t pm_stop_compute();

/**
 * @brief Get the latest PowerMonitor measurements.
 */
extern esp_err_t pm_get_results(ul_pm_results_t *ul_pm_results);

#endif  /* INC_PM_H_ */
