/** @file webserver.h
 *  @brief  Created on: Sep 9, 2024
 *          Davide Scalisi
 *
 * 					Description:	Webserver code.
 *
 * @copyright [2024] Davide Scalisi *
 * @copyright All Rights Reserved. *
 *
*/

#ifndef INC_WEBSERVER_H_
#define INC_WEBSERVER_H_

/************************************************************************************************************
* Included files
************************************************************************************************************/

// Standard libraries.
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

// Platform libraries.
#include <esp_err.h>
#include <esp_check.h>
#include <esp_log.h>

#include <esp_http_server.h>

// UniLibC libraries.
#include <ul_errors.h>
#include <ul_utils.h>
#include <ul_linked_list.h>

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
 * @brief Initialize the library.
 */
extern esp_err_t webserver_setup();

/**
 * @brief Reload the webserver daemon, reloading also the web routes.
 */
extern esp_err_t webserver_reload();

#endif  /* INC_WEBSERVER_H_ */
