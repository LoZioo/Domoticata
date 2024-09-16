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
#include <private.h>

/************************************************************************************************************
* Private Defines
************************************************************************************************************/

#define LOG_TAG	"ota"

#define __stringify(x)	#x
#define __to_string(x)	__stringify(x)

#define UPDATE_URL_BEFORE_IP	\
	"http://"

#define UPDATE_URL_AFTER_IP	\
	":" \
	__to_string(CONFIG_OTA_HTTP_SERVER_PORT) \
	CONFIG_OTA_HTTP_BINARY_FILE_PATH \
	"/"

#define UPDATE_URL_AFTER_IP_FS	\
	UPDATE_URL_AFTER_IP \
	CONFIG_OTA_FS_BINARY_FILE_NAME \
	".bin"

#define UPDATE_URL_AFTER_IP_FW	\
	UPDATE_URL_AFTER_IP \
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

static void __log_partition_info(const char *part_name, const esp_partition_t *partition);
static esp_err_t __log_fw_versions(uint8_t *buffer);

static esp_err_t __esp_http_client_ret_to_esp_err_t(int64_t esp_http_client_ret);

/************************************************************************************************************
* Private Functions Definitions
 ************************************************************************************************************/

void __log_partition_info(const char *part_name, const esp_partition_t *part){
	ESP_LOGI(
		TAG,
		"%s partition info ("
			"label=\"%s\", "
			"type=%u "
			"subtype=%u, "
			"offset=0x%08"PRIx32", "
			"size=%lu"
		")",

		part_name,
		part->label,
		part->type,
		part->subtype,
		part->address,
		part->size
	);
}

esp_err_t __log_fw_versions(uint8_t *buffer){

	esp_app_desc_t app_info;
	ESP_RETURN_ON_ERROR(
		esp_ota_get_partition_description(
			esp_ota_get_running_partition(),
			&app_info
		),

		TAG,
		"Error on `esp_ota_get_partition_description()`"
	);

	ESP_LOGI(TAG, "Running firmware version: %s", app_info.version);

	memcpy(
		&app_info,
		&buffer[
			sizeof(esp_image_header_t) +
			sizeof(esp_image_segment_header_t)
		],
		sizeof(esp_app_desc_t)
	);

	ESP_LOGI(TAG, "New firmware version: %s", app_info.version);
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

esp_err_t ota_update_fs(esp_ip4_addr_t ota_server_ip){
	esp_err_t ret = ESP_OK;

	char url[
		sizeof(UPDATE_URL_BEFORE_IP) - 1 +
		WIFI_IPV4_ADDR_STR_LEN +
		sizeof(UPDATE_URL_AFTER_IP_FS)
	];

	snprintf(
		url, sizeof(url),
		UPDATE_URL_BEFORE_IP IPSTR UPDATE_URL_AFTER_IP_FS,
		IP2STR(&ota_server_ip)
	);

	ESP_LOGI(TAG, "Beginning filesystem update");
	ESP_LOGI(TAG, "Target URL is \"%s\"", url);

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

	uint8_t *buffer = malloc(CONFIG_OTA_BUFFER_LEN_BYTES);
	ESP_GOTO_ON_FALSE(
		buffer != NULL,

		ESP_ERR_NO_MEM,
		label_restore_wifi_ps_mode,
		TAG,
		"Error on `malloc(size=%u)`",
		CONFIG_OTA_BUFFER_LEN_BYTES
	);

	esp_http_client_config_t http_client_config = {
		.url = url,
		.timeout_ms = UPDATE_TIMEOUT_MS,
		.keep_alive_enable = true,
	};

	esp_http_client_handle_t http_client_handle =
		esp_http_client_init(&http_client_config);

	ESP_GOTO_ON_FALSE(
		http_client_handle != NULL,

		ESP_FAIL,
		label_free_buffer,
		TAG,
		"Error on `esp_http_client_init()`"
	);

	ESP_GOTO_ON_ERROR(
		esp_http_client_open(http_client_handle, 0),

		label_free_http_client,
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

	ESP_LOGI(TAG, "Erasing filesystem partition...");
	ESP_GOTO_ON_ERROR(
		fs_partition_unmounted_write(NULL, 0),

		label_free_http_client,
		TAG,
		"Error on `fs_partition_unmounted_write(data=NULL)`"
	);

	ESP_LOGI(TAG, "Downloading and writing OTA filesystem to flash...");

	uint32_t data_total_len = 0;
	int read_len;

	// Begin sequential data reading loop.
	for(;;){

		// Needed to detect TCP/IP errors.
		errno = 0;

		read_len = esp_http_client_read(
			http_client_handle,
			(char*) buffer,
			sizeof(buffer)
		);

		// TCP/IP error or simply connection close.
		if(read_len == 0)
			break;

		// Check `esp_http_client_read()`.
		ESP_GOTO_ON_ERROR(
			__esp_http_client_ret_to_esp_err_t(read_len),

			label_free_http_client,
			TAG,
			"Error on `esp_http_client_read()`"
		);

		ESP_GOTO_ON_ERROR(
			fs_partition_unmounted_write(
				buffer,
				read_len
			),

			label_free_http_client,
			TAG,
			"Error on `fs_partition_unmounted_write()`"
		);

		data_total_len += read_len;
	}

	ESP_LOGI(TAG, "Total written bytes: %lu", data_total_len);
	ESP_GOTO_ON_FALSE(
		errno == 0,

		ESP_ERR_INVALID_STATE,
		label_free_http_client,
		TAG,
		"Error: TCP/IP error; errno=%d",
		errno
	);

	ESP_GOTO_ON_FALSE(
		esp_http_client_is_complete_data_received(
			http_client_handle
		),

		ESP_ERR_OTA_VALIDATE_FAILED,
		label_free_http_client,
		TAG,
		"Error on `esp_http_client_is_complete_data_received()`"
	);

	uint8_t sha256[32];
	ESP_GOTO_ON_ERROR(
		fs_partition_unmounted_get_sha256(sha256),

		label_free_http_client,
		TAG,
		"Error on `fs_partition_unmounted_get_sha256()`"
	);

	log_hash(
		TAG,
		"SHA-256 for filesystem partition",
		sha256, sizeof(sha256)
	);

	ESP_LOGI(TAG, "Filesystem update done");

	label_free_http_client:
	ESP_ERROR_CHECK_WITHOUT_ABORT(esp_http_client_cleanup(http_client_handle));

	label_free_buffer:
	free(buffer);

	label_restore_wifi_ps_mode:
	ESP_ERROR_CHECK_WITHOUT_ABORT(wifi_power_save_mode(true));

	return ret;
}

esp_err_t ota_update_fw(esp_ip4_addr_t ota_server_ip){
	esp_err_t ret = ESP_OK;

	char url[
		sizeof(UPDATE_URL_BEFORE_IP) - 1 +
		WIFI_IPV4_ADDR_STR_LEN +
		sizeof(UPDATE_URL_AFTER_IP_FW)
	];

	snprintf(
		url, sizeof(url),
		UPDATE_URL_BEFORE_IP IPSTR UPDATE_URL_AFTER_IP_FW,
		IP2STR(&ota_server_ip)
	);

	ESP_LOGI(TAG, "Beginning firmware update");
	ESP_LOGI(TAG, "Target URL is \"%s\"", url);

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

	uint8_t *buffer = malloc(CONFIG_OTA_BUFFER_LEN_BYTES);
	ESP_GOTO_ON_FALSE(
		buffer != NULL,

		ESP_ERR_NO_MEM,
		label_restore_wifi_ps_mode,
		TAG,
		"Error on `malloc(size=%u)`",
		CONFIG_OTA_BUFFER_LEN_BYTES
	);

	const esp_partition_t *part_boot = esp_ota_get_boot_partition();
	const esp_partition_t *part_running = esp_ota_get_running_partition();
	const esp_partition_t *part_target = esp_ota_get_next_update_partition(NULL);

	ESP_GOTO_ON_FALSE(
		part_boot != NULL,

		ESP_ERR_NOT_FOUND,
		label_free_buffer,
		TAG,
		"Error on `esp_ota_get_boot_partition()`"
	);

	ESP_GOTO_ON_FALSE(
		part_running != NULL,

		ESP_ERR_NOT_FOUND,
		label_free_buffer,
		TAG,
		"Error on `esp_ota_get_running_partition()`"
	);

	ESP_GOTO_ON_FALSE(
		part_target != NULL,

		ESP_ERR_NOT_FOUND,
		label_free_buffer,
		TAG,
		"Error on `esp_ota_get_next_update_partition()`"
	);

	if(part_boot != part_running){
		ESP_LOGW(
			TAG, "Configured OTA boot partition at offset 0x%08"PRIx32", but running from offset 0x%08"PRIx32,
			part_boot->address, part_running->address
		);

		ESP_LOGW(TAG, "(This can happen if either the OTA boot data or preferred boot image become corrupted somehow.)");
	}

	__log_partition_info("Running", part_running);
	__log_partition_info("Target", part_target);

	esp_http_client_config_t http_client_config = {
		.url = url,
		.timeout_ms = UPDATE_TIMEOUT_MS,
		.keep_alive_enable = true,
	};

	esp_http_client_handle_t http_client_handle =
		esp_http_client_init(&http_client_config);

	ESP_GOTO_ON_FALSE(
		http_client_handle != NULL,

		ESP_FAIL,
		label_free_buffer,
		TAG,
		"Error on `esp_http_client_init()`"
	);

	ESP_GOTO_ON_ERROR(
		esp_http_client_open(http_client_handle, 0),

		label_free_http_client,
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
	uint32_t data_total_len = 0;
	int read_len;
	bool app_header_was_checked = false;

	// Begin sequential data reading loop.
	for(;;){

		// Needed to detect TCP/IP errors.
		errno = 0;

		read_len = esp_http_client_read(
			http_client_handle,
			(char*) buffer,
			sizeof(buffer)
		);

		// TCP/IP error or simply connection close.
		if(read_len == 0)
			break;

		// Check `esp_http_client_read()`.
		ESP_GOTO_ON_ERROR(
			__esp_http_client_ret_to_esp_err_t(read_len),

			label_free_ota,
			TAG,
			"Error on `esp_http_client_read()`"
		);

		// Only on the first loop.
		if(!app_header_was_checked){

			ESP_GOTO_ON_FALSE(
				read_len > (	// That's the total app header size.
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
				__log_fw_versions(buffer),

				label_free_http_client,
				TAG,
				"Error on `__log_fw_versions()`"
			);

			// App header ok.
			app_header_was_checked = true;
			ESP_LOGI(TAG, "Downloading and writing OTA app to flash...");

			ESP_GOTO_ON_ERROR(
				esp_ota_begin(
					part_target,
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
				buffer,
				read_len
			),

			label_free_ota,
			TAG,
			"Error on `esp_ota_write()`"
		);

		data_total_len += read_len;
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
		esp_ota_set_boot_partition(part_target),

		label_free_ota,
		TAG,
		"Error on `esp_ota_set_boot_partition()`"
	);

	ESP_LOGI(TAG, "Firmware update done");

	label_free_ota:
	ESP_ERROR_CHECK_WITHOUT_ABORT(esp_ota_end(ota_handle));

	label_free_http_client:
	ESP_ERROR_CHECK_WITHOUT_ABORT(esp_http_client_cleanup(http_client_handle));

	label_free_buffer:
	free(buffer);

	label_restore_wifi_ps_mode:
	ESP_ERROR_CHECK_WITHOUT_ABORT(wifi_power_save_mode(true));

	return ret;
}
