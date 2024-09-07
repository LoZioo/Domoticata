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

// Local NVS namespace.
#define NVS_NAMESPACE	"fs_metadata"

// NVS keys.
#define NVS_KEY_PART_INDEX	"part_index"

#define PART_INDEX_DEFAULT	0
#define PART_NAME_PREFIX		"fs_"
#define PART_NAME_MAXLEN		10

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

static esp_err_t __nvs_get_partition_name_to_mount(char *partition_name_to_mount);

/************************************************************************************************************
* Private Functions Definitions
 ************************************************************************************************************/

esp_err_t __nvs_get_partition_name_to_mount(char *partition_name_to_mount){

	ESP_RETURN_ON_FALSE(
		nvs_available(),

		ESP_ERR_NVS_NOT_INITIALIZED,
		TAG,
		"Error: NVS not initialized"
	);

	nvs_handle_t nvs_handle;
	ESP_RETURN_ON_ERROR(
		nvs_open(
			NVS_NAMESPACE,
			NVS_READWRITE,
			&nvs_handle
		),

		TAG,
		"Error on `nvs_open()`"
	);

	uint8_t part_index;
	esp_err_t ret = nvs_get_u8(
		nvs_handle,
		NVS_KEY_PART_INDEX,
		&part_index
	);

	if(ret == ESP_ERR_NVS_NOT_FOUND){
		ESP_LOGW(TAG, "Key \"" NVS_KEY_PART_INDEX "\" uninitialized; set to default=%u", PART_INDEX_DEFAULT);
		ESP_RETURN_ON_ERROR(
			nvs_set_u8(
				nvs_handle,
				NVS_KEY_PART_INDEX,
				PART_INDEX_DEFAULT
			),

			TAG,
			"Error on `nvs_set_u8()`"
		);
	}

	else
		ESP_RETURN_ON_ERROR(
			ret,

			TAG,
			"Error on `nvs_get_u8()`"
		);

	ESP_RETURN_ON_ERROR(
		nvs_commit(nvs_handle),

		TAG,
		"Error on `nvs_commit()`"
	);

	nvs_close(nvs_handle);
	sprintf(
		partition_name_to_mount,
		PART_NAME_PREFIX "%u",
		part_index
	);

	return ESP_OK;
}

/************************************************************************************************************
* Public Functions Definitions
 ************************************************************************************************************/

esp_err_t fs_setup(){

	char part_name[PART_NAME_MAXLEN];

	ESP_RETURN_ON_ERROR(
		__nvs_get_partition_name_to_mount(part_name),

		TAG,
		"Error on `__nvs_get_partition_name_to_mount()`"
	);

	esp_vfs_littlefs_conf_t vfs_littlefs_conf = {
		.base_path = FS_LITTLEFS_BASE_PATH,
		.partition_label = part_name,
		.format_if_mount_failed = false,
		.read_only = false,
		.dont_mount = false,
		.grow_on_mount = true
	};

	ESP_LOGI(TAG, "Mounting partition \"%s\"", part_name);
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

	ESP_LOGI(TAG, "Partition size: total bytes: %d, used bytes: %d", total_bytes, used_bytes);
	__fs_mounted_on_vfs = true;

	return ESP_OK;
}

bool fs_available(){
	return __fs_mounted_on_vfs;
}
