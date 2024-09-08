/** @file ota.c
 *  @brief  Created on: Aug 29, 2024
 *          Davide Scalisi
 *
 * @copyright [2024] Davide Scalisi *
 * @copyright All Rights Reserved. *
 *
*/

/************************************************************************************************************
* Included files
************************************************************************************************************/

#include <ota.h>

/************************************************************************************************************
* Private Defines
************************************************************************************************************/

#define LOG_TAG	"ota"

#define STRINGIFY(x)	#x
#define TOSTRING(x)		STRINGIFY(x)

#define UPDATE_URL	\
	"http://" \
	CONFIG_OTA_HTTP_SERVER_IP_ADDR \
	":" \
	TOSTRING(CONFIG_OTA_HTTP_SERVER_PORT) \
	CONFIG_OTA_HTTP_BINARY_FILE_PATH \
	"/"

#define UPDATE_FS_URL	\
	UPDATE_URL \
	CONFIG_OTA_FS_BINARY_FILE_NAME \
	".bin"

#define UPDATE_FW_URL	\
	UPDATE_URL \
	CONFIG_OTA_FW_BINARY_FILE_NAME \
	".bin"

#define UPDATE_TIMEOUT_MS		5000

/************************************************************************************************************
* Private Types Definitions
 ************************************************************************************************************/

/************************************************************************************************************
* Private Variables
 ************************************************************************************************************/

static const char *TAG = LOG_TAG;

/************************************************************************************************************
* Private Functions Prototypes
 ************************************************************************************************************/

static void __log_sha256(const char* label, uint8_t *hash, uint16_t hash_len);

static esp_err_t __fw_log_partitions_sha256();
static void __fw_log_running_partition_info();
static esp_err_t __fw_log_firmware_versions(uint8_t *ota_buffer);

static esp_err_t __esp_http_client_ret_to_esp_err_t(int64_t esp_http_client_ret);

/************************************************************************************************************
* Private Functions Definitions
 ************************************************************************************************************/

void __log_sha256(const char* label, uint8_t *hash, uint16_t hash_len){

	char str[hash_len * 2 + 1];
	str[sizeof(str) - 1] = 0;

	for(uint16_t i=0; i<hash_len; i++)
		sprintf(&str[i * 2], "%02x", hash[i]);

	ESP_LOGI(TAG, "%s: %s", label, str);
}

esp_err_t __fw_log_partitions_sha256(){

	struct {
		char label[32];
		esp_partition_t partition;
	} partitions[] = {

		// Running partition.
		{
			.label = "SHA-256 for current firmware",
			.partition = *esp_ota_get_running_partition()
		},

		// Partition table.
		{
			.label = "SHA-256 for the partition table",
			.partition = {
				.address = ESP_PARTITION_TABLE_OFFSET,
				.size = ESP_PARTITION_TABLE_MAX_LEN,
				.type = ESP_PARTITION_TYPE_DATA
			}
		},

		// Bootloader.
		{
			.label = "SHA-256 for bootloader",
			.partition = {
				.address = ESP_BOOTLOADER_OFFSET,
				.size = ESP_PARTITION_TABLE_OFFSET,
				.type = ESP_PARTITION_TYPE_APP
			}
		},
	};

	uint8_t sha256[32] = {0};
	for(uint8_t i=0; i<3; i++){
		ESP_RETURN_ON_ERROR(
			esp_partition_get_sha256(&partitions[i].partition, sha256),

			TAG,
			"Error on `esp_partition_get_sha256()`"
		);

		__log_sha256(partitions[i].label, sha256, sizeof(sha256));
	}

	return ESP_OK;
}

void __fw_log_running_partition_info(){

	const esp_partition_t *configured_boot_partition = esp_ota_get_boot_partition();
	const esp_partition_t *running_partition = esp_ota_get_running_partition();

	if(configured_boot_partition != running_partition){
		ESP_LOGW(
			TAG, "Configured OTA boot partition at offset 0x%08"PRIx32", but running from offset 0x%08"PRIx32,
			configured_boot_partition->address, running_partition->address
		);

		ESP_LOGW(TAG, "(This can happen if either the OTA boot data or preferred boot image become corrupted somehow.)");
	}

	ESP_LOGI(
		TAG, "Running partition type %d subtype %d (offset 0x%08"PRIx32")",
		running_partition->type, running_partition->subtype, running_partition->address
	);
}

esp_err_t __fw_log_firmware_versions(uint8_t *ota_buffer){

	esp_app_desc_t app_info;
	memcpy(
		&app_info,
		&ota_buffer[
			sizeof(esp_image_header_t) +
			sizeof(esp_image_segment_header_t)
		],
		sizeof(esp_app_desc_t)
	);

	ESP_LOGI(TAG, "New firmware version: %s", app_info.version);
	ESP_RETURN_ON_ERROR(
		esp_ota_get_partition_description(
			esp_ota_get_running_partition(),
			&app_info
		),

		TAG,
		"Error on `esp_ota_get_partition_description()`"
	);

	ESP_LOGI(TAG, "Running firmware version: %s", app_info.version);
	return ESP_OK;
}

esp_err_t __esp_http_client_ret_to_esp_err_t(int64_t esp_http_client_ret){
	switch(esp_http_client_ret){
		case -1:
			return ESP_FAIL;

		case 0:
			return ESP_ERR_INVALID_RESPONSE;

		case -ESP_ERR_HTTP_EAGAIN:
			return ESP_ERR_TIMEOUT;

		default:
			return ESP_OK;
	}
}

/************************************************************************************************************
* Public Functions Definitions
 ************************************************************************************************************/

esp_err_t ota_update_fs(){
	esp_err_t ret = ESP_OK;

	ESP_LOGI(TAG, "Beginning filesystem update");
	ESP_LOGI(TAG, "Target URL is \"" UPDATE_FS_URL "\"");

	ESP_RETURN_ON_FALSE(
		wifi_network_available(),

		ESP_ERR_INVALID_STATE,
		TAG,
		"Error: network service not available"
	);

	ESP_RETURN_ON_FALSE(
		fs_available(),

		ESP_ERR_INVALID_STATE,
		TAG,
		"Error: filesystem service not initialized"
	);

	ESP_RETURN_ON_ERROR(
		wifi_power_save_mode(false),

		TAG,
		"Error on `wifi_power_save_mode(power_save_mode_enabled=false)`"
	);

	// ESP_GOTO_ON_ERROR(
	// 	__fw_log_partitions_sha256(),

	// 	label_restore_wifi_ps_mode,
	// 	TAG,
	// 	"Error on `__fw_log_partitions_sha256()`"
	// );

	// __fw_log_running_partition_info();

	esp_partition_t const *target_partition;

	ESP_GOTO_ON_ERROR(
		fs_get_partition(false, &target_partition),

		label_restore_wifi_ps_mode,
		TAG,
		"Error on `fs_get_partition()`"
	);

	ESP_LOGI(
		TAG, "Writing to partition subtype %d at offset 0x%"PRIx32,
		target_partition->subtype, target_partition->address
	);

	esp_http_client_config_t http_client_config = {
		.url = UPDATE_FS_URL,
		.timeout_ms = UPDATE_TIMEOUT_MS,
		.keep_alive_enable = true,
	};

	esp_http_client_handle_t http_client_handle =
		esp_http_client_init(&http_client_config);

	ESP_GOTO_ON_FALSE(
		http_client_handle != NULL,

		ESP_FAIL,
		label_restore_wifi_ps_mode,
		TAG,
		"Error on `esp_http_client_init()`"
	);

	ESP_GOTO_ON_ERROR(
		esp_http_client_open(http_client_handle, 0),

		label_restore_wifi_ps_mode,
		TAG,
		"Error on `esp_http_client_open()`"
	);

	ESP_GOTO_ON_ERROR(
		__esp_http_client_ret_to_esp_err_t(
			esp_http_client_fetch_headers(http_client_handle)
		),

		label_free_http_client,
		TAG,
		"Error on `esp_http_client_fetch_headers()`"
	);

	esp_ota_handle_t ota_handle;
	uint8_t ota_buffer[CONFIG_OTA_BUFFER_LEN_BYTES];
	uint32_t data_total_len = 0;
	int data_len;

	// !!! esp_ota_begin NON FUNZIONA PER LE PARTIZIONI NON-OTA, QUINDI PREDISPORRE API IN fs.h
	ESP_LOGI(TAG, "Downloading and writing OTA filesystem to flash...");
	ESP_GOTO_ON_ERROR(
		esp_ota_begin(
			target_partition,
			OTA_WITH_SEQUENTIAL_WRITES,
			&ota_handle
		),

		label_free_http_client,
		TAG,
		"Error on `esp_ota_begin()`"
	);

	// Begin sequential data reading loop.
	for(;;){

		// Needed to detect TCP/IP errors.
		errno = 0;

		data_len = esp_http_client_read(
			http_client_handle,
			(char*) ota_buffer,
			sizeof(ota_buffer)
		);

		// TCP/IP error or simply connection close.
		if(data_len == 0)
			break;

		// Check `esp_http_client_read()`.
		ESP_GOTO_ON_ERROR(
			__esp_http_client_ret_to_esp_err_t(data_len),

			label_free_ota,
			TAG,
			"Error on `esp_http_client_read()`"
		);

		ESP_GOTO_ON_ERROR(
			esp_ota_write(
				ota_handle,
				ota_buffer,
				sizeof(ota_buffer)
			),

			label_free_ota,
			TAG,
			"Error on `esp_ota_write()`"
		);

		data_total_len += data_len;
	}

	ESP_LOGI(TAG, "Total written bytes: %lu", data_total_len);
	ESP_GOTO_ON_FALSE(
		errno == 0,

		ESP_ERR_INVALID_STATE,
		label_free_ota,
		TAG,
		"Error: TCP/IP error; errno=%d",
		errno
	);

	ESP_GOTO_ON_FALSE(
		esp_http_client_is_complete_data_received(
			http_client_handle
		),

		ESP_ERR_OTA_VALIDATE_FAILED,
		label_free_ota,
		TAG,
		"Error on `esp_http_client_is_complete_data_received()`"
	);

	ESP_GOTO_ON_ERROR(
		fs_partition_swap(),

		label_free_ota,
		TAG,
		"Error on `fs_partition_swap()`"
	);

	ESP_LOGI(TAG, "Filesystem update done");

	label_free_ota:
	ESP_ERROR_CHECK_WITHOUT_ABORT(esp_ota_end(ota_handle));

	label_free_http_client:
	ESP_ERROR_CHECK_WITHOUT_ABORT(esp_http_client_cleanup(http_client_handle));

	label_restore_wifi_ps_mode:
	ESP_ERROR_CHECK_WITHOUT_ABORT(wifi_power_save_mode(true));

	return ret;
}

esp_err_t ota_update_fw(){
	esp_err_t ret = ESP_OK;

	ESP_LOGI(TAG, "Beginning firmware update");
	ESP_LOGI(TAG, "Target URL is \"" UPDATE_FW_URL "\"");

	ESP_RETURN_ON_FALSE(
		wifi_network_available(),

		ESP_ERR_INVALID_STATE,
		TAG,
		"Error: network service not available"
	);

	ESP_RETURN_ON_ERROR(
		wifi_power_save_mode(false),

		TAG,
		"Error on `wifi_power_save_mode(power_save_mode_enabled=false)`"
	);

	ESP_GOTO_ON_ERROR(
		__fw_log_partitions_sha256(),

		label_restore_wifi_ps_mode,
		TAG,
		"Error on `__fw_log_partitions_sha256()`"
	);

	__fw_log_running_partition_info();

	const esp_partition_t *target_partition =
		esp_ota_get_next_update_partition(NULL);

	ESP_GOTO_ON_FALSE(
		target_partition != NULL,

		ESP_ERR_NOT_FOUND,
		label_restore_wifi_ps_mode,
		TAG,
		"Error on `esp_ota_get_next_update_partition()`"
	);

	ESP_LOGI(
		TAG, "Writing to partition subtype %d at offset 0x%"PRIx32,
		target_partition->subtype, target_partition->address
	);

	esp_http_client_config_t http_client_config = {
		.url = UPDATE_FW_URL,
		.timeout_ms = UPDATE_TIMEOUT_MS,
		.keep_alive_enable = true,
	};

	esp_http_client_handle_t http_client_handle =
		esp_http_client_init(&http_client_config);

	ESP_GOTO_ON_FALSE(
		http_client_handle != NULL,

		ESP_FAIL,
		label_restore_wifi_ps_mode,
		TAG,
		"Error on `esp_http_client_init()`"
	);

	ESP_GOTO_ON_ERROR(
		esp_http_client_open(http_client_handle, 0),

		label_restore_wifi_ps_mode,
		TAG,
		"Error on `esp_http_client_open()`"
	);

	ESP_GOTO_ON_ERROR(
		__esp_http_client_ret_to_esp_err_t(
			esp_http_client_fetch_headers(http_client_handle)
		),

		label_free_http_client,
		TAG,
		"Error on `esp_http_client_fetch_headers()`"
	);

	esp_ota_handle_t ota_handle;
	uint8_t ota_buffer[CONFIG_OTA_BUFFER_LEN_BYTES];
	uint32_t data_total_len = 0;
	int data_len;
	bool app_header_was_checked = false;

	// Begin sequential data reading loop.
	for(;;){

		// Needed to detect TCP/IP errors.
		errno = 0;

		data_len = esp_http_client_read(
			http_client_handle,
			(char*) ota_buffer,
			sizeof(ota_buffer)
		);

		// TCP/IP error or simply connection close.
		if(data_len == 0)
			break;

		// Check `esp_http_client_read()`.
		ESP_GOTO_ON_ERROR(
			__esp_http_client_ret_to_esp_err_t(data_len),

			label_free_ota,
			TAG,
			"Error on `esp_http_client_read()`"
		);

		// Only on the first loop.
		if(!app_header_was_checked){

			ESP_GOTO_ON_FALSE(
				data_len > (	// That's the total app header size.
					sizeof(esp_image_header_t) +
					sizeof(esp_image_segment_header_t) +
					sizeof(esp_app_desc_t)
				),

				ESP_ERR_OTA_VALIDATE_FAILED,
				label_free_http_client,
				TAG,
				"Error: HTTP server answered with an invalid app header"
			);

			ESP_GOTO_ON_ERROR(
				__fw_log_firmware_versions(ota_buffer),

				label_free_http_client,
				TAG,
				"Error on `__fw_log_firmware_versions()`"
			);

			// App header ok.
			app_header_was_checked = true;
			ESP_LOGI(TAG, "Downloading and writing OTA app to flash...");

			ESP_GOTO_ON_ERROR(
				esp_ota_begin(
					target_partition,
					OTA_WITH_SEQUENTIAL_WRITES,
					&ota_handle
				),

				label_free_http_client,
				TAG,
				"Error on `esp_ota_begin()`"
			);
		}

		ESP_GOTO_ON_ERROR(
			esp_ota_write(
				ota_handle,
				ota_buffer,
				sizeof(ota_buffer)
			),

			label_free_ota,
			TAG,
			"Error on `esp_ota_write()`"
		);

		data_total_len += data_len;
	}

	ESP_LOGI(TAG, "Total written bytes: %lu", data_total_len);
	ESP_GOTO_ON_FALSE(
		errno == 0,

		ESP_ERR_INVALID_STATE,
		label_free_ota,
		TAG,
		"Error: TCP/IP error; errno=%d",
		errno
	);

	ESP_GOTO_ON_FALSE(
		esp_http_client_is_complete_data_received(
			http_client_handle
		),

		ESP_ERR_OTA_VALIDATE_FAILED,
		label_free_ota,
		TAG,
		"Error on `esp_http_client_is_complete_data_received()`"
	);

	ESP_GOTO_ON_ERROR(
		esp_ota_end(ota_handle),

		label_free_ota,
		TAG,
		"Error on `esp_ota_end()`"
	);

	ESP_GOTO_ON_ERROR(
		esp_ota_set_boot_partition(target_partition),

		label_free_ota,
		TAG,
		"Error on `esp_ota_set_boot_partition()`"
	);

	ESP_LOGI(TAG, "Firmware update done");
	ESP_LOGW(TAG, "Triggering system reset...");
	esp_restart();
	return ret;

	label_free_ota:
	ESP_ERROR_CHECK_WITHOUT_ABORT(esp_ota_end(ota_handle));

	label_free_http_client:
	ESP_ERROR_CHECK_WITHOUT_ABORT(esp_http_client_cleanup(http_client_handle));

	label_restore_wifi_ps_mode:
	ESP_ERROR_CHECK_WITHOUT_ABORT(wifi_power_save_mode(true));

	return ret;
}
