/** @file fs.c
 *  @brief  Created on: Sep 6, 2024
 *          Davide Scalisi
 *
 * @copyright [2024] Davide Scalisi *
 * @copyright All Rights Reserved. *
 *
*/

/************************************************************************************************************
* Included files
************************************************************************************************************/

#include <fs.h>

/************************************************************************************************************
* Private Defines
************************************************************************************************************/

#define LOG_TAG	"fs"

/************************************************************************************************************
* Private Types Definitions
 ************************************************************************************************************/

/************************************************************************************************************
* Private Variables
 ************************************************************************************************************/

static const char *TAG = LOG_TAG;
static bool __fs_mounted_on_vfs = false;

/************************************************************************************************************
* Private Functions Prototypes
 ************************************************************************************************************/

/************************************************************************************************************
* Private Functions Definitions
 ************************************************************************************************************/

/************************************************************************************************************
* Public Functions Definitions
 ************************************************************************************************************/

esp_err_t fs_setup(){

	esp_vfs_littlefs_conf_t vfs_littlefs_conf = {
		.base_path = FS_LITTLEFS_BASE_PATH,
		.partition_label = "fs",
		.format_if_mount_failed = false,
		.read_only = false,
		.dont_mount = false,
		.grow_on_mount = true
	};

	ESP_RETURN_ON_ERROR(
		esp_vfs_littlefs_register(&vfs_littlefs_conf),

		TAG,
		"Error on `esp_vfs_littlefs_register()`"
	);

	size_t total_bytes = 0, used_bytes = 0;
	ESP_RETURN_ON_ERROR(
		esp_littlefs_info(
			vfs_littlefs_conf.partition_label,
			&total_bytes,
			&used_bytes
		),

		TAG,
		"Error on `esp_vfs_littlefs_register()`"
	);

	ESP_LOGI(TAG, "Partition size: total bytes: %d, used bytes: %d", total_bytes, used_bytes);
	__fs_mounted_on_vfs = true;

	return ESP_OK;
}

bool fs_available(){
	return __fs_mounted_on_vfs;
}
