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
static uint8_t __fs_current_partition_index = PART_INDEX_DEFAULT;

// Used by `__littlefs_partition_mount/umount()`.
static bool __fs_mounted_on_vfs = false;

/************************************************************************************************************
* Private Functions Prototypes
 ************************************************************************************************************/

static esp_err_t __nvs_get_handle(nvs_handle_t *nvs_handle);

static esp_err_t __nvs_load_current_fs_partition_index();
static esp_err_t __nvs_store_current_fs_partition_index();

static esp_err_t __littlefs_partition_mount();
static esp_err_t __littlefs_partition_umount();

/************************************************************************************************************
* Private Functions Definitions
 ************************************************************************************************************/

esp_err_t __nvs_get_handle(nvs_handle_t *nvs_handle){

	ESP_RETURN_ON_FALSE(
		nvs_handle != NULL,

		ESP_ERR_INVALID_ARG,
		TAG,
		"Error: `nvs_handle` is NULL"
	);

	ESP_RETURN_ON_FALSE(
		nvs_available(),

		ESP_ERR_NVS_NOT_INITIALIZED,
		TAG,
		"Error: NVS not initialized"
	);

	ESP_RETURN_ON_ERROR(
		nvs_open(
			NVS_NAMESPACE,
			NVS_READWRITE,
			nvs_handle
		),

		TAG,
		"Error on `nvs_open()`"
	);

	return ESP_OK;
}

esp_err_t __nvs_load_current_fs_partition_index(){

	nvs_handle_t nvs_handle;
	ESP_RETURN_ON_ERROR(
		__nvs_get_handle(&nvs_handle),

		TAG,
		"Error on `__nvs_get_handle()`"
	);

	esp_err_t ret = nvs_get_u8(
		nvs_handle,
		NVS_KEY_PART_INDEX,
		&__fs_current_partition_index
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

		ESP_RETURN_ON_ERROR(
			nvs_commit(nvs_handle),

			TAG,
			"Error on `nvs_commit()`"
		);
	}

	else
		ESP_RETURN_ON_ERROR(
			ret,

			TAG,
			"Error on `nvs_get_u8()`"
		);

	nvs_close(nvs_handle);
	return ESP_OK;
}

esp_err_t __nvs_store_current_fs_partition_index(){

	nvs_handle_t nvs_handle;
	ESP_RETURN_ON_ERROR(
		__nvs_get_handle(&nvs_handle),

		TAG,
		"Error on `__nvs_get_handle()`"
	);

	ESP_RETURN_ON_ERROR(
		nvs_set_u8(
			nvs_handle,
			NVS_KEY_PART_INDEX,
			__fs_current_partition_index
		),

		TAG,
		"Error on `nvs_set_u8()`"
	);

	ESP_RETURN_ON_ERROR(
		nvs_commit(nvs_handle),

		TAG,
		"Error on `nvs_commit()`"
	);

	nvs_close(nvs_handle);
	return ESP_OK;
}

esp_err_t __littlefs_partition_mount(){

	char part_name[PART_NAME_MAXLEN];
	sprintf(
		part_name,
		PART_NAME_PREFIX "%u",
		__fs_current_partition_index
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

	ESP_LOGI(
		TAG, "Partition size: %d bytes (%.1f%% used)",
		total_bytes,
		((float) used_bytes / (float) total_bytes) * 100
	);

	__fs_mounted_on_vfs = true;
	return ESP_OK;
}

esp_err_t __littlefs_partition_umount(){

	char part_name[PART_NAME_MAXLEN];
	sprintf(
		part_name,
		PART_NAME_PREFIX "%u",
		__fs_current_partition_index
	);

	ESP_LOGI(TAG, "Umounting partition \"%s\"", part_name);
	ESP_RETURN_ON_ERROR(
		esp_vfs_littlefs_unregister(part_name),

		TAG,
		"Error on `esp_vfs_littlefs_unregister()`"
	);

	__fs_mounted_on_vfs = false;
	return ESP_OK;
}

/************************************************************************************************************
* Public Functions Definitions
 ************************************************************************************************************/

esp_err_t fs_setup(){

	ESP_RETURN_ON_ERROR(
		__nvs_load_current_fs_partition_index(),

		TAG,
		"Error on `__nvs_load_current_fs_partition_index()`"
	);

	ESP_RETURN_ON_ERROR(
		__littlefs_partition_mount(),

		TAG,
		"Error on `__littlefs_partition_mount()`"
	);

	return ESP_OK;
}

bool fs_available(){
	return __fs_mounted_on_vfs;
}

esp_err_t fs_part_swap(){

	ESP_RETURN_ON_FALSE(
		fs_available(),

		ESP_ERR_INVALID_STATE,
		TAG,
		"Error: filesystem partition not mounted"
	);

	ESP_RETURN_ON_ERROR(
		__littlefs_partition_umount(),

		TAG,
		"Error on `__littlefs_partition_umount()`"
	);

	// Swap active partition.
	__fs_current_partition_index = !__fs_current_partition_index;

	ESP_RETURN_ON_ERROR(
		__nvs_store_current_fs_partition_index(),

		TAG,
		"Error on `__nvs_store_current_fs_partition_index()`"
	);

	ESP_RETURN_ON_ERROR(
		__littlefs_partition_mount(),

		TAG,
		"Error on `__littlefs_partition_mount()`"
	);

	return ESP_OK;
}

esp_err_t fs_get_current_part_index(uint8_t *fs_current_partition_index){

	ESP_RETURN_ON_FALSE(
		fs_current_partition_index != NULL,

		ESP_ERR_INVALID_ARG,
		TAG,
		"Error: `fs_current_partition_index` is NULL"
	);

	ESP_RETURN_ON_FALSE(
		fs_available(),

		ESP_ERR_INVALID_STATE,
		TAG,
		"Error: filesystem partition not mounted"
	);

	*fs_current_partition_index = __fs_current_partition_index;
	return ESP_OK;
}
