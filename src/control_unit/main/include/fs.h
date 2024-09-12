/** @file fs.h
 *  @brief  Created on: Sep 6, 2024
 *          Davide Scalisi
 *
 * 					Description:	Filesystem code.
 * 					Preliminary settings:
 * 						-	Install the LittleFS component via the IDF Component Registry.
 * 						-	Add a LittleFS partition in your "partitions.csv".
 * 						-	Add the `idf.flashBaudRate` key in your "settings.json".
 * 						-	Add `littlefs_create_partition_image(partition_name path_to_folder_containing_files)`
 * 							to the project "CMakeLists.txt" in order to build the LittleFS image together with the fw image.
 * 						-	Add the `FLASH_IN_PROJECT` flag to flash the LittleFS image together with the fw image (annoying).
 * 							[ `littlefs_create_partition_image(partition_name path_to_folder_containing_files FLASH_IN_PROJECT)` ]
 * 						-	To manually flash the built LittleFS image (much better):
 * 							-	`pip install -r requirements.txt`
 * 							-	Right click on `flash_littlefs_image.py` and "Run Python File in Terminal".
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
#include <esp_partition.h>

// Project libraries.
#include <main.h>
#include <non_volatile_storage.h>

/************************************************************************************************************
* Public Defines
************************************************************************************************************/

// Filesystem base path.
#define FS_LITTLEFS_BASE_PATH				"/littlefs"
#define FS_LITTLEFS_BASE_PATH_LEN		(sizeof(FS_LITTLEFS_BASE_PATH) - 1)

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

/**
 * @brief Swap between the two LittleFS partitions.
 */
extern esp_err_t fs_partition_swap();

/**
 * @brief Write data to the currently unmounted LittleFS partition.
 * @param data If `NULL`, initialize the write process (this must be done on the first call).
 * @note Call `fs_partition_swap()` to mount the partition.
 */
extern esp_err_t fs_partition_unmounted_write(uint8_t* data, size_t size);

/**
 * @brief Get the SHA-256 hash for the currently unmounted LittleFS partition.
 * @param hash Pointer to a 32-bytes buffer.
 */
extern esp_err_t fs_partition_unmounted_get_sha256(uint8_t* hash);

#endif  /* INC_FS_H_ */
