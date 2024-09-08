/** @file ota.h
 *  @brief  Created on: Aug 29, 2024
 *          Davide Scalisi
 *
 * 					Description:	OTA update code.
 *
 * @copyright [2024] Davide Scalisi *
 * @copyright All Rights Reserved. *
 *
*/

#ifndef INC_OTA_H_
#define INC_OTA_H_

/************************************************************************************************************
* Included files
************************************************************************************************************/

// Standard libraries.
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>

// Platform libraries.
#include <esp_err.h>
#include <esp_check.h>
#include <esp_log.h>

#include <esp_system.h>
#include <esp_ota_ops.h>
#include <esp_app_format.h>
#include <esp_flash_partitions.h>
#include <esp_partition.h>
#include <esp_http_client.h>

#include <freertos/FreeRTOS.h>

// Project libraries.
#include <main.h>
#include <wifi.h>
#include <fs.h>

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
 * @brief Trigger a filesystem update.
 */
extern esp_err_t ota_update_fs();

/**
 * @brief Trigger a firmware update.
 */
extern esp_err_t ota_update_fw();

#endif  /* INC_OTA_H_ */
