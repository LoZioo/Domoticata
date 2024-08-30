/** @file wifi.c
 *  @brief  Created on: Aug 17, 2024
 *          Davide Scalisi
 *
 * @copyright [2024] Davide Scalisi *
 * @copyright All Rights Reserved. *
 *
*/

/************************************************************************************************************
* Included files
************************************************************************************************************/

#include <wifi.h>

/************************************************************************************************************
* Private Defines
************************************************************************************************************/

#define LOG_TAG	"wifi"

/************************************************************************************************************
* Private Types Definitions
 ************************************************************************************************************/

/************************************************************************************************************
* Private Variables
 ************************************************************************************************************/

static const char *TAG = LOG_TAG;

static TaskHandle_t __wifi_task_handle = NULL;
static bool __network_ready = false;

/************************************************************************************************************
* Private Functions Prototypes
 ************************************************************************************************************/

static void __net_event_callback(void* event_handler_arg, esp_event_base_t event_base, int32_t event_id, void* event_data);

/************************************************************************************************************
* Private Functions Definitions
 ************************************************************************************************************/

void __net_event_callback(void* event_handler_arg, esp_event_base_t event_base, int32_t event_id, void* event_data){

	// Start connection.
	if(
		event_base == WIFI_EVENT &&
		event_id == WIFI_EVENT_STA_START
	){
		ESP_LOGI(TAG, "`WIFI_EVENT_STA_START` triggered");
		ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_connect());
	}

	// Handle reconnection.
	else if(
		event_base == WIFI_EVENT &&
		event_id == WIFI_EVENT_STA_DISCONNECTED
	){
		__network_ready = false;

		ESP_LOGW(TAG, "`WIFI_EVENT_STA_DISCONNECTED` triggered, reconnecting...");
		ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_connect());
	}

	else if(
		event_base == IP_EVENT &&
		event_id == IP_EVENT_STA_GOT_IP
	){
		__network_ready = true;
		ESP_LOGI(TAG, "`IP_EVENT_STA_GOT_IP` triggered");

		// Set the event if `wifi_setup()` was called.
		if(__wifi_task_handle != NULL)
			xTaskNotifyGive(__wifi_task_handle);
	}
}

/************************************************************************************************************
* Public Functions Definitions
 ************************************************************************************************************/

esp_err_t wifi_setup(){

	// Set current task handle.
	__wifi_task_handle = xTaskGetCurrentTaskHandle();

	// Initialize NVS partition (Non-Volatile Storage) for storing WiFi auxiliary data.
	esp_err_t ret = nvs_flash_init();

	if(
		ul_utils_either(
			ESP_ERR_NVS_NO_FREE_PAGES,
			ESP_ERR_NVS_NEW_VERSION_FOUND,
			==, ret
		)
	){
		ESP_RETURN_ON_ERROR(
			nvs_flash_erase(),

			TAG,
			"Error on `nvs_flash_erase()`"
		);

		ret = nvs_flash_init();
	}

	ESP_RETURN_ON_ERROR(
		ret,

		TAG,
		"Error on `nvs_flash_init()`"
	);

	/**
	 * The async execution pattern is:
	 * - `esp_wifi_start()`
	 * - Wait for the `__net_ok_semaphore` to be set.
	 * - On the default ESP event loop, `__net_event_callback()` is called with `WIFI_EVENT_STA_START`.
	 * - ...so, `esp_wifi_connect()`
	 * - On the default ESP event loop, '__net_event_callback()' is called with `WIFI_EVENT_STA_DISCONNECTED` or `IP_EVENT_STA_GOT_IP`.
	 * - If `IP_EVENT_STA_GOT_IP`, set the `__net_ok_semaphore` semaphore and unlock the calling task.
	 */

	// Initialize the underlying LwIP (TCP/IP) stack.
	ESP_RETURN_ON_ERROR(
		esp_netif_init(),

		TAG,
		"Error on `esp_netif_init()`"
	);

	// Start the default event loop.
	ESP_RETURN_ON_ERROR(
		esp_event_loop_create_default(),

		TAG,
		"Error on `esp_event_loop_create_default()`"
	);

	// Handle network interface events on the default event loop.
	esp_netif_create_default_wifi_sta();

	// WiFi driver initialization.
	wifi_init_config_t wifi_init_sta_config = WIFI_INIT_CONFIG_DEFAULT();
	ESP_RETURN_ON_ERROR(
		esp_wifi_init(&wifi_init_sta_config),

		TAG,
		"Error on `esp_wifi_init()`"
	);

	// Needed if you want to unregister the events.
	esp_event_handler_instance_t wifi_event_registration_receipt;
	esp_event_handler_instance_t ip_event_registration_receipt;

	// Register the events to the default event loop.
	ESP_RETURN_ON_ERROR(
		esp_event_handler_instance_register(
			WIFI_EVENT,
			ESP_EVENT_ANY_ID,
			__net_event_callback,
			NULL,
			&wifi_event_registration_receipt
		),

		TAG,
		"Error on `esp_event_handler_instance_register(event_id=ESP_EVENT_ANY_ID)`"
	);

	ESP_RETURN_ON_ERROR(
		esp_event_handler_instance_register(
			IP_EVENT,
			IP_EVENT_STA_GOT_IP,
			__net_event_callback,
			NULL,
			&ip_event_registration_receipt
		),

		TAG,
		"Error on `esp_event_handler_instance_register(event_id=IP_EVENT_STA_GOT_IP)`"
	);

	// WiFi configurations.
	wifi_config_t wifi_sta_config = {
		.sta.ssid = "",
		.sta.password = ""
	};

	strcpy((char*) wifi_sta_config.sta.ssid, CONFIG_STA_SSID);
	strcpy((char*) wifi_sta_config.sta.password, CONFIG_STA_PSK);

	// Station mode.
	ESP_RETURN_ON_ERROR(
		esp_wifi_set_mode(WIFI_MODE_STA),

		TAG,
		"Error on `esp_wifi_set_mode()`"
	);

	// Set configurations.
	ESP_RETURN_ON_ERROR(
		esp_wifi_set_config(WIFI_IF_STA, &wifi_sta_config),

		TAG,
		"Error on `esp_wifi_set_config()`"
	);

	// Start the connection async procedure described above.
	ESP_RETURN_ON_ERROR(
		esp_wifi_start(),

		TAG,
		"Error on `esp_wifi_start()`"
	);

	// Wait for `IP_EVENT_STA_GOT_IP` and clear the notification (act as a binary semaphore; `pdTRUE`).
	ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

	// !!! METTERE WIFI AP IN CASO DI FAIL NELLA CONNESSIONE

	// Reset current task handle.
	__wifi_task_handle = NULL;

	return ESP_OK;
}

bool wifi_is_network_ready(){
	return __network_ready;
}

esp_err_t wifi_power_save_mode(bool power_save_mode_enabled){
	return esp_wifi_set_ps(
		power_save_mode_enabled ?
		WIFI_PS_MIN_MODEM :
		WIFI_PS_NONE
	);
}
