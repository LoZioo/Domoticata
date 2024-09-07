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

// Local NVS namespace.
#define NVS_NAMESPACE		"wifi_configs"

// NVS keys.
#define NVS_KEY_STA_SSID	"sta_ssid"
#define NVS_KEY_STA_PSK		"sta_psk"

/**
 * @brief Statement to check if the library was initialized.
 */
#define __is_initialized()( \
	__network_ready_semaphore != NULL \
)

/**
 * @brief Unblock `wifi_setup()` if it was called.
 */
#define __unblock_caller() \
	if(__wifi_task_handle != NULL) \
		xTaskNotifyGive(__wifi_task_handle)

/************************************************************************************************************
* Private Types Definitions
 ************************************************************************************************************/

/************************************************************************************************************
* Private Variables
 ************************************************************************************************************/

static const char *TAG = LOG_TAG;

static TaskHandle_t __wifi_task_handle = NULL;
static SemaphoreHandle_t __network_ready_semaphore = NULL;

// Wi-Fi configurations.
wifi_config_t wifi_sta_config = {
	.sta.ssid = ""
};

/************************************************************************************************************
* Private Functions Prototypes
 ************************************************************************************************************/

/**
 * @return `ESP_ERR_NVS_NOT_FOUND` if no valid Wi-Fi configurations were found in the NVS.
 */
static esp_err_t __nvs_load_wifi_configs();
static esp_err_t __nvs_store_wifi_configs();

/**
 * @return `ESP_ERR_WIFI_NOT_CONNECT` on station connection fail.
 */
static esp_err_t __wifi_mode_sta();
static esp_err_t __wifi_mode_smart_config();

static void __net_event_callback(void* event_handler_arg, esp_event_base_t event_base, int32_t event_id, void* event_data);

/************************************************************************************************************
* Private Functions Definitions
 ************************************************************************************************************/

esp_err_t __nvs_load_wifi_configs(){
	esp_err_t ret = ESP_OK;

	nvs_handle_t nvs_handle;
	ESP_RETURN_ON_ERROR(
		nvs_new_handle(&nvs_handle, NVS_NAMESPACE),

		TAG,
		"Error on `nvs_new_handle()`"
	);

	size_t required_size;

	// Retrive the `required_size`.
	ESP_GOTO_ON_ERROR(
		nvs_get_str(
			nvs_handle,
			NVS_KEY_STA_SSID,
			NULL,
			&required_size
		),

		label_check_err_nvs_not_found,
		TAG,
		"Error on `nvs_get_str(key=NVS_KEY_STA_SSID, out_value=NULL)`"
	);

	// Now use `required_size` to retrive the string.
	ESP_GOTO_ON_ERROR(
		nvs_get_str(
			nvs_handle,
			NVS_KEY_STA_SSID,
			(char*) wifi_sta_config.sta.ssid,
			&required_size
		),

		label_free,
		TAG,
		"Error on `nvs_get_str(key=NVS_KEY_STA_SSID)`"
	);

	ESP_GOTO_ON_ERROR(
		nvs_get_str(
			nvs_handle,
			NVS_KEY_STA_PSK,
			NULL,
			&required_size
		),

		label_check_err_nvs_not_found,
		TAG,
		"Error on `nvs_get_str(key=NVS_KEY_STA_PSK, out_value=NULL)`"
	);

	ESP_GOTO_ON_ERROR(
		nvs_get_str(
			nvs_handle,
			NVS_KEY_STA_PSK,
			(char*) wifi_sta_config.sta.password,
			&required_size
		),

		label_free,
		TAG,
		"Error on `nvs_get_str(key=NVS_KEY_STA_PSK)`"
	);

	label_check_err_nvs_not_found:
	if(ret == ESP_ERR_NVS_NOT_FOUND)
		ESP_LOGW(TAG, "No valid Wi-Fi configurations were found in the NVS");

	label_free:
	nvs_close(nvs_handle);
	return ret;
}

esp_err_t __nvs_store_wifi_configs(){
	esp_err_t ret = ESP_OK;

	nvs_handle_t nvs_handle;
	ESP_RETURN_ON_ERROR(
		nvs_new_handle(&nvs_handle, NVS_NAMESPACE),

		TAG,
		"Error on `nvs_new_handle()`"
	);

	ESP_GOTO_ON_ERROR(
		nvs_set_str(
			nvs_handle,
			NVS_KEY_STA_SSID,
			(char*) wifi_sta_config.sta.ssid
		),

		label_free,
		TAG,
		"Error on `nvs_set_str()` (STA_SSID)"
	);

	ESP_GOTO_ON_ERROR(
		nvs_set_str(
			nvs_handle,
			NVS_KEY_STA_PSK,
			(char*) wifi_sta_config.sta.password
		),

		label_free,
		TAG,
		"Error on `nvs_set_str()` (STA_PSK)"
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

esp_err_t __wifi_mode_sta(){

	/**
	 * The connection-to-station async execution pattern is:
	 * - `esp_wifi_start()`
	 * - Wait for a task notification (a simple UNIX signal).
	 * - On the default ESP event loop, `__net_event_callback()` is called with `WIFI_EVENT_STA_START`.
	 * - ...so, `esp_wifi_connect()`
	 * - On the default ESP event loop, '__net_event_callback()' is called with `WIFI_EVENT_STA_DISCONNECTED` or `IP_EVENT_STA_GOT_IP`.
	 * - If `IP_EVENT_STA_GOT_IP`, send the task notification and unlock the calling task.
	 */

	ESP_LOGI(TAG, "(STA mode) connecting to station \"%s\"...", wifi_sta_config.sta.ssid);

	// Handle network interface events on the default event loop.
	esp_netif_create_default_wifi_sta();

	// Station mode.
	ESP_RETURN_ON_ERROR(
		esp_wifi_set_mode(WIFI_MODE_STA),

		TAG,
		"Error on `esp_wifi_set_mode()`"
	);

	// Set configurations.
	if(strcmp(wifi_sta_config.sta.ssid, "") != 0)
		ESP_RETURN_ON_ERROR(
			esp_wifi_set_config(
				WIFI_IF_STA,
				&wifi_sta_config
			),

			TAG,
			"Error on `esp_wifi_set_config()`"
		);

	// Start the async connection procedure.
	ESP_RETURN_ON_ERROR(
		esp_wifi_start(),

		TAG,
		"Error on `esp_wifi_start()`"
	);

	// Block the caller and clear the notification when it's set (`pdTRUE` = act as a binary semaphore).
	ESP_RETURN_ON_FALSE(
		ulTaskNotifyTake(
			pdTRUE,
			pdMS_TO_TICKS(CONFIG_WIFI_STA_CONNECTION_TIMEOUT_SECONDS * 1000)
		) > 0,

		ESP_ERR_WIFI_NOT_CONNECT,
		TAG,
		"(STA mode) setup failed`"
	);

	ESP_LOGI(TAG, "(STA mode) setup ok");
	return ESP_OK;
}

esp_err_t __wifi_mode_smart_config(){

	ESP_LOGI(TAG, "(SmartConfig mode) waiting for EspTouch app...");
	return ESP_OK;
}

void __net_event_callback(void* event_handler_arg, esp_event_base_t event_base, int32_t event_id, void* event_data){

	// Wi-Fi events.
	if(event_base == WIFI_EVENT)
		switch(event_id){
			case WIFI_EVENT_STA_START: {
				ESP_LOGI(TAG, "Wi-Fi station mode initialized; connecting...");
				ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_connect());
			} break;

			case WIFI_EVENT_STA_DISCONNECTED: {
				xSemaphoreTake(__network_ready_semaphore, 0);

				wifi_event_sta_disconnected_t *event = event_data;
				ESP_LOGI(TAG, "Wi-Fi station mode disconnected ((wifi_err_reason_t) reason=%u)", event->reason);

				// `esp_wifi_disconnect()` not called.
				if(event->reason != WIFI_REASON_ASSOC_LEAVE){
					ESP_LOGI(TAG, "reconnecting...");
					ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_connect());
				}
			} break;

			case WIFI_EVENT_AP_START: {
				ESP_LOGI(TAG, "Wi-Fi soft AP mode initialized");
				__unblock_caller();
			} break;

			case WIFI_EVENT_AP_STACONNECTED: {
				wifi_event_ap_staconnected_t* event = event_data;
				ESP_LOGI(TAG, "Station " MACSTR " join, AID=%d", MAC2STR(event->mac), event->aid);
			} break;

			case WIFI_EVENT_AP_STADISCONNECTED: {
				wifi_event_ap_stadisconnected_t* event = event_data;
				ESP_LOGI(TAG, "Station " MACSTR " leave, AID=%d", MAC2STR(event->mac), event->aid);
			} break;

			default:
				break;
		}

	// IP events.
	else if(event_base == IP_EVENT)
		switch(event_id){
			case IP_EVENT_STA_GOT_IP: {
				xSemaphoreGive(__network_ready_semaphore);

				ip_event_got_ip_t* event = event_data;
				ESP_LOGI(TAG, "DHCP data received");
				ESP_LOGI(TAG, "IP: " IPSTR, IP2STR(&event->ip_info.ip));
				ESP_LOGI(TAG, "Netmask: " IPSTR, IP2STR(&event->ip_info.netmask));
				ESP_LOGI(TAG, "Gateway: " IPSTR, IP2STR(&event->ip_info.gw));

				__unblock_caller();
			} break;

			case IP_EVENT_AP_STAIPASSIGNED: {
				ip_event_ap_staipassigned_t* event = event_data;
				ESP_LOGI(
					TAG, "Station " MACSTR " got IP " IPSTR,
					MAC2STR(event->mac), IP2STR(&event->ip)
				);
			} break;

			default:
				break;
		}
	
	// SmartConfig events.
	else if(event_base == SC_EVENT)
		switch(event_id){
			// case IP_EVENT_STA_GOT_IP: {
			// 	xSemaphoreGive(__network_ready_semaphore);

			// 	ip_event_got_ip_t* event = event_data;
			// 	ESP_LOGI(TAG, "DHCP data received");
			// 	ESP_LOGI(TAG, "IP: " IPSTR, IP2STR(&event->ip_info.ip));
			// 	ESP_LOGI(TAG, "Netmask: " IPSTR, IP2STR(&event->ip_info.netmask));
			// 	ESP_LOGI(TAG, "Gateway: " IPSTR, IP2STR(&event->ip_info.gw));

			// 	__unblock_caller();
			// } break;

			// case IP_EVENT_AP_STAIPASSIGNED: {
			// 	ip_event_ap_staipassigned_t* event = event_data;
			// 	ESP_LOGI(
			// 		TAG, "Station " MACSTR " got IP " IPSTR,
			// 		MAC2STR(event->mac), IP2STR(&event->ip)
			// 	);
			// } break;

			default:
				break;
		}
}

/************************************************************************************************************
* Public Functions Definitions
 ************************************************************************************************************/

esp_err_t wifi_setup(){

	ESP_RETURN_ON_FALSE(
		nvs_available(),

		ESP_ERR_NVS_NOT_INITIALIZED,
		TAG,
		"Error: NVS not initialized"
	);

	// Save current task handle.
	__wifi_task_handle = xTaskGetCurrentTaskHandle();
	__network_ready_semaphore = xSemaphoreCreateBinary();

	ESP_RETURN_ON_FALSE(
		__network_ready_semaphore != NULL,

		ESP_ERR_NO_MEM,
		TAG,
		"Error on `xSemaphoreCreateBinary()`"
	);

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

	// Wi-Fi driver initialization.
	wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
	ESP_RETURN_ON_ERROR(
		esp_wifi_init(&wifi_init_config),

		TAG,
		"Error on `esp_wifi_init()`"
	);

	// Register events to the default event loop.
	// Wi-Fi
	ESP_RETURN_ON_ERROR(
		esp_event_handler_instance_register(
			WIFI_EVENT,
			ESP_EVENT_ANY_ID,
			__net_event_callback,
			NULL,
			NULL
		),

		TAG,
		"Error on `esp_event_handler_instance_register(event_base=WIFI_EVENT)`"
	);

	// IP
	ESP_RETURN_ON_ERROR(
		esp_event_handler_instance_register(
			IP_EVENT,
			IP_EVENT_STA_GOT_IP,
			__net_event_callback,
			NULL,
			NULL
		),

		TAG,
		"Error on `esp_event_handler_instance_register(event_base=IP_EVENT)`"
	);

	// SmartConfig
	ESP_RETURN_ON_ERROR(
		esp_event_handler_instance_register(
			SC_EVENT,
			ESP_EVENT_ANY_ID,
			__net_event_callback,
			NULL,
			NULL
		),

		TAG,
		"Error on `esp_event_handler_instance_register(event_base=SC_EVENT)`"
	);

	// Load Wi-Fi credentials from NVS.
	esp_err_t ret = __nvs_load_wifi_configs();

	// No Wi-Fi settings stored in the NVS.
	if(ret == ESP_ERR_NVS_NOT_FOUND)
		ESP_RETURN_ON_ERROR(
			__wifi_mode_smart_config(),

			TAG,
			"Error on `__wifi_mode_smart_config()`"
		);

	else {
		ESP_RETURN_ON_ERROR(
			ret,

			TAG,
			"Error on `__nvs_load_wifi_configs()`"
		);

		ret = __wifi_mode_sta();

		// Wi-Fi station connection failed.
		if(ret == ESP_ERR_WIFI_NOT_CONNECT)
			ESP_RETURN_ON_ERROR(
				__wifi_mode_smart_config(),

				TAG,
				"Error on `__wifi_mode_smart_config()`"
			);

		else
			ESP_RETURN_ON_ERROR(
				ret,

				TAG,
				"Error on `__wifi_mode_sta()`"
			);
	}

	// Reset current task handle.
	__wifi_task_handle = NULL;

	return ESP_OK;
}

bool wifi_network_available(){
	return (
		!__is_initialized() ?
		false :
		uxSemaphoreGetCount(__network_ready_semaphore)
	);
}

esp_err_t wifi_power_save_mode(bool power_save_mode_enabled){
	return (
		!__is_initialized() ?
		ESP_OK :	// No errors are thrown if the library is not initialized.
		esp_wifi_set_ps(
			power_save_mode_enabled ?
			WIFI_PS_MIN_MODEM :
			WIFI_PS_NONE
		)
	);
}
