/** @file fs.h
 *  @brief  Created on: Sep 6, 2024
 *          Davide Scalisi
 *
 * 					Description:	Filesystem code.
 * 					Notes:
 * 						-	You must install the LittleFS component via the IDF Component Registry.
 * 						-	You must specify a LittleFS partition in a custom .csv partition table.
 * 						-	You must add `littlefs_create_partition_image(partition_name path_to_folder_containing_files FLASH_IN_PROJECT)`
 * 							to the project CMakeLists.txt
 * 						-	The `FLASH_IN_PROJECT` flag means that the fs image must be flashed together with the fw image.
 *
 * @copyright [2024] Davide Scalisi *
 * @copyright All Rights Reserved. *
 *
*/

#ifndef INC_FS_H_
#define INC_FS_H_

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

#include <esp_littlefs.h>

// Project libraries.
#include <main.h>

/************************************************************************************************************
* Public Defines
************************************************************************************************************/

// Filesystem base path.
#define FS_LITTLEFS_BASE_PATH		"/littlefs"

/**
 * @brief `FS_LITTLEFS_BASE_PATH` + `path`
 */
#define fs_full_path(path) \
	FS_LITTLEFS_BASE_PATH path

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
extern esp_err_t fs_setup();

/**
 * @brief Check if the filesystem service is available.
 */
extern bool fs_available();

#endif  /* INC_FS_H_ */
