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
#include <private.h>

/************************************************************************************************************
* Private Defines
************************************************************************************************************/

#define LOG_TAG	"fs"

// Partition label.
#define PART_LABEL	"fs"

/************************************************************************************************************
* Private Types Definitions
 ************************************************************************************************************/

/************************************************************************************************************
* Private Variables
 ************************************************************************************************************/

static const char *TAG = LOG_TAG;

// Used by `fs_available()`.
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
		.partition_label = PART_LABEL,
		.partition = NULL,
		.format_if_mount_failed = true,
		.read_only = false,
		.dont_mount = false,
		.grow_on_mount = true
	};

	ESP_LOGI(TAG, "Mounting filesystem partition on VFS");
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
		"Error on `esp_littlefs_info()`"
	);

	ESP_LOGI(
		TAG, "Partition size is %d bytes (%.1f%% used)",
		total_bytes,
		((float) used_bytes / (float) total_bytes) * 100
	);

	__fs_mounted_on_vfs = true;
	return ESP_OK;
}

bool fs_available(){
	return __fs_mounted_on_vfs;
}
