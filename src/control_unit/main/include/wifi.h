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
#include <esp_mac.h>
#include <esp_system.h>
#include <esp_smartconfig.h>
#include <smartconfig_ack.h>

#include <freertos/FreeRTOS.h>

// Project libraries.
#include <main.h>
#include <non_volatile_storage.h>

/************************************************************************************************************
* Public Defines
************************************************************************************************************/

#define WIFI_IPV4_ADDR_STR_LEN	16

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
extern bool wifi_network_available();

/**
 * @brief Enable/disable WiFi power save mode.
 */
extern esp_err_t wifi_power_save_mode(bool power_save_mode_enabled);

#endif  /* INC_WIFI_H_ */
