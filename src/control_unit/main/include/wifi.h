/** @file wifi.h
 *  @brief  Created on: Aug 17, 2024
 *          Davide Scalisi
 *
 * 					Description:	WiFi code.
 * 					Note:					You must enable these options from the menuconfig to support
 * 												multiple access points with same SSID and different channels:
 * 													- Enable 802.11k, 802.11v APIs Support
 * 													- Keep scan results in cache
 * 													- Enable Multi Band Operation Certification Support
 *
 * @copyright [2024] Davide Scalisi *
 * @copyright All Rights Reserved. *
 *
*/

#ifndef INC_WIFI_H_
#define INC_WIFI_H_

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

#include <esp_event_base.h>
#include <esp_event.h>
#include <esp_wifi.h>

#include <nvs_flash.h>
#include <nvs.h>

#include <freertos/FreeRTOS.h>

// UniLibC libraries.
#include <ul_errors.h>
#include <ul_utils.h>

// Project libraries.
#include <main.h>

// !!! DEBUG
#include <password.h>

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
extern esp_err_t wifi_setup();

/**
 * @brief Check if the network service is available.
 */
extern bool wifi_is_network_ready();

/**
 * @brief Enable/disable the WiFi power safe mode.
 */
extern esp_err_t wifi_power_save_mode(bool power_save_mode_enabled);

#endif  /* INC_WIFI_H_ */
