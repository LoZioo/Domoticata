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

// Local NVS namespace.
#define NVS_NAMESPACE	"fs_metadata"

// NVS keys.
#define NVS_KEY_PART_INDEX	"part_index"

#define PART_COUNT					2
#define PART_INDEX_DEFAULT	0

#define PART_LABEL_PREFIX		"fs_"
#define PART_LABEL_MAXLEN		10

/************************************************************************************************************
* Private Types Definitions
 ************************************************************************************************************/

/************************************************************************************************************
* Private Variables
 ************************************************************************************************************/

static const char *TAG = LOG_TAG;
static uint8_t __current_partition_index = PART_INDEX_DEFAULT;

// Filesystem partitions.
static const esp_partition_t *__fs_partitions[PART_COUNT];

// Used by `__littlefs_partition_mount/umount()`.
static bool __fs_mounted_on_vfs = false;

/************************************************************************************************************
* Private Functions Prototypes
 ************************************************************************************************************/

/**
 * @brief Initialize `__fs_partitions` array.
 */
static esp_err_t __get_littlefs_partitions();

static esp_err_t __nvs_load_current_fs_partition_index();
static esp_err_t __nvs_store_current_fs_partition_index();

static esp_err_t __littlefs_partition_mount();
static esp_err_t __littlefs_partition_umount();

/************************************************************************************************************
* Private Functions Definitions
 ************************************************************************************************************/

esp_err_t __get_littlefs_partitions(){
	esp_err_t ret = ESP_OK;

	char part_label[PART_LABEL_MAXLEN];
	for(uint8_t i=0; i<PART_COUNT; i++){
		snprintf(part_label, PART_LABEL_MAXLEN, PART_LABEL_PREFIX "%u", i);

		esp_partition_iterator_t part_iterator =
			esp_partition_find(
				ESP_PARTITION_TYPE_DATA,
				ESP_PARTITION_SUBTYPE_DATA_LITTLEFS,
				part_label
			);

		ESP_GOTO_ON_FALSE(
			part_iterator != NULL,

			ESP_ERR_NOT_FOUND,
			label_iterator_free,
			TAG,
			"Error on `esp_partition_find()`"
		);

		__fs_partitions[i] = esp_partition_get(part_iterator);

		label_iterator_free:
		esp_partition_iterator_release(part_iterator);

		if(ret != ESP_OK)
			return ret;
	}

	return ret;
}

esp_err_t __nvs_load_current_fs_partition_index(){
	esp_err_t ret = ESP_OK;

	nvs_handle_t nvs_handle;
	ESP_RETURN_ON_ERROR(
		nvs_new_handle(&nvs_handle, NVS_NAMESPACE),

		TAG,
		"Error on `nvs_new_handle()`"
	);

	ESP_GOTO_ON_ERROR(
		nvs_get_u8(
			nvs_handle,
			NVS_KEY_PART_INDEX,
			&__current_partition_index
		),

		label_check_err_nvs_not_found,
		TAG,
		"Error on `nvs_get_u8()`"
	);

	label_check_err_nvs_not_found:
	if(ret == ESP_ERR_NVS_NOT_FOUND){

		// Reset the error.
		ret = ESP_OK;

		ESP_LOGW(TAG, "Key \"" NVS_KEY_PART_INDEX "\" uninitialized; set to default=%u", PART_INDEX_DEFAULT);
		ESP_GOTO_ON_ERROR(
			nvs_set_u8(
				nvs_handle,
				NVS_KEY_PART_INDEX,
				PART_INDEX_DEFAULT
			),

			label_free,
			TAG,
			"Error on `nvs_set_u8()`"
		);

		ESP_GOTO_ON_ERROR(
			nvs_commit(nvs_handle),

			label_free,
			TAG,
			"Error on `nvs_commit()`"
		);
	}

	label_free:
	nvs_close(nvs_handle);
	return ret;
}

esp_err_t __nvs_store_current_fs_partition_index(){
	esp_err_t ret = ESP_OK;

	nvs_handle_t nvs_handle;
	ESP_RETURN_ON_ERROR(
		nvs_new_handle(&nvs_handle, NVS_NAMESPACE),

		TAG,
		"Error on `nvs_new_handle()`"
	);

	ESP_GOTO_ON_ERROR(
		nvs_set_u8(
			nvs_handle,
			NVS_KEY_PART_INDEX,
			__current_partition_index
		),

		label_free,
		TAG,
		"Error on `nvs_set_u8()`"
	);

	ESP_GOTO_ON_ERROR(
		nvs_commit(nvs_handle),

		label_free,
		TAG,
		"Error on `nvs_commit()`"
	);

	label_free:
	nvs_close(nvs_handle);
	return ret;
}

esp_err_t __littlefs_partition_mount(){

	esp_vfs_littlefs_conf_t vfs_littlefs_conf = {
		.base_path = FS_LITTLEFS_BASE_PATH,
		.partition_label = NULL,
		.partition = __fs_partitions[__current_partition_index],
		.format_if_mount_failed = true,
		.read_only = false,
		.dont_mount = false,
		.grow_on_mount = true
	};

	ESP_LOGI(TAG, "Mounting partition index=%u", __current_partition_index);
	ESP_RETURN_ON_ERROR(
		esp_vfs_littlefs_register(&vfs_littlefs_conf),

		TAG,
		"Error on `esp_vfs_littlefs_register()`"
	);

	size_t total_bytes = 0, used_bytes = 0;
	ESP_RETURN_ON_ERROR(
		esp_littlefs_partition_info(
			vfs_littlefs_conf.partition,
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

esp_err_t __littlefs_partition_umount(){

	ESP_LOGI(TAG, "Umounting partition index=%u", __current_partition_index);
	ESP_RETURN_ON_ERROR(
		esp_vfs_littlefs_unregister_partition(
			__fs_partitions[__current_partition_index]
		),

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
		__get_littlefs_partitions(),

		TAG,
		"Error on `__get_littlefs_partitions()`"
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

esp_err_t fs_partition_swap(){

	ESP_RETURN_ON_FALSE(
		fs_available(),

		ESP_ERR_INVALID_STATE,
		TAG,
		"Error: filesystem service not initialized"
	);

	ESP_RETURN_ON_ERROR(
		__littlefs_partition_umount(),

		TAG,
		"Error on `__littlefs_partition_umount()`"
	);

	// Swap active partition.
	__current_partition_index = !__current_partition_index;

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

esp_err_t fs_partition_unmounted_write(uint8_t* data, size_t size){

	static size_t offset;
	const esp_partition_t *part =
		__fs_partitions[!__current_partition_index];

	// First call.
	if(data == NULL){
		offset = 0;

		uint32_t part_size_to_clear_rounded_down =
			part->size - (part->size % part->erase_size);

		ESP_RETURN_ON_ERROR(
			esp_partition_erase_range(
				part,
				offset,
				part_size_to_clear_rounded_down
			),

			TAG,
			"Error on `esp_partition_erase_range()`"
		);

		return ESP_OK;
	}

	assert_param_size_ok(size);
	ESP_RETURN_ON_ERROR(
		esp_partition_write(
			part,
			offset,
			data,
			size
		),

		TAG,
		"Error on `esp_partition_write()`"
	);

	offset += size;
	return ESP_OK;
}

esp_err_t fs_partition_unmounted_get_sha256(uint8_t* hash){
	assert_param_notnull(hash);

	ESP_RETURN_ON_ERROR(
		esp_partition_get_sha256(
			__fs_partitions[
				!__current_partition_index
			],
			hash
		),

		TAG,
		"Error on `esp_partition_get_sha256()`"
	);

	return ESP_OK;
}
