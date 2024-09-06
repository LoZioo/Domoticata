/** @file non_volatile_storage.h
 *  @brief  Created on: Sep 6, 2024
 *          Davide Scalisi
 *
 * 					Description:	Non-Volatile Storage code.
 *
 * @copyright [2024] Davide Scalisi *
 * @copyright All Rights Reserved. *
 *
*/

#ifndef INC_NON_VOLATILE_STORAGE_H_
#define INC_NON_VOLATILE_STORAGE_H_

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

#include <nvs_flash.h>
#include <nvs.h>

// UniLibC libraries.
#include <ul_errors.h>
#include <ul_utils.h>

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
extern esp_err_t nvs_setup();

/**
 * @brief Check if the Non-Volatile Storage service is available.
 */
extern bool nvs_available();

#endif  /* INC_NON_VOLATILE_STORAGE_H_ */
