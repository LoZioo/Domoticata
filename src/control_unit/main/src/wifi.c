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

/************************************************************************************************************
* Private Functions Prototypes
 ************************************************************************************************************/

static esp_err_t __nvs_setup();
static esp_err_t __wifi_netif_up();

static void __net_event_callback(void* event_handler_arg, esp_event_base_t event_base, int32_t event_id, void* event_data);

/************************************************************************************************************
* Private Functions Definitions
 ************************************************************************************************************/

esp_err_t __nvs_setup(){

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

	return ESP_OK;
}

esp_err_t __wifi_netif_up(){

	/**
	 * `NULL` -> wifi_sta:	first call
	 * wifi_sta -> wifi_ap:	other calls
	 */
	static esp_netif_t *netif_handle = NULL;
	bool wifi_sta = (netif_handle == NULL);

	// If not first time, destroy the previously created wifi_sta netif.
	if(!wifi_sta){
		ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_disconnect());
		ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_stop());
		esp_netif_destroy_default_wifi(netif_handle);
	}

	// WiFi configurations.
	wifi_config_t wifi_sta_config = (
		wifi_sta ?

		(wifi_config_t){
			.sta.ssid = CONFIG_STA_SSID,
			.sta.password = CONFIG_STA_PSK
		} :

		(wifi_config_t){
			.ap = {
				.ssid = CONFIG_WIFI_SOFTAP_SSID,
				.ssid_len = strlen(CONFIG_WIFI_SOFTAP_SSID),
				.password = CONFIG_WIFI_SOFTAP_PSK,
				.authmode = WIFI_AUTH_WPA2_PSK,
				.max_connection = CONFIG_WIFI_SOFTAP_MAX_CONNECTIONS,
				.pmf_cfg.required = true
			}
		}
	);

	// Handle network interface events on the default event loop.
	netif_handle = (
		wifi_sta ?
		esp_netif_create_default_wifi_sta() :
		esp_netif_create_default_wifi_ap()
	);

	// Station mode.
	ESP_RETURN_ON_ERROR(
		esp_wifi_set_mode(
			wifi_sta ?
			WIFI_MODE_STA :
			WIFI_MODE_AP
		),

		TAG,
		"Error on `esp_wifi_set_mode()`"
	);

	// Set configurations.
	ESP_RETURN_ON_ERROR(
		esp_wifi_set_config(
			wifi_sta ?
			WIFI_IF_STA :
			WIFI_IF_AP,
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

	return ESP_OK;
}

void __net_event_callback(void* event_handler_arg, esp_event_base_t event_base, int32_t event_id, void* event_data){

	// WiFi events.
	if(event_base == WIFI_EVENT)
		switch(event_id){
			case WIFI_EVENT_STA_START: {
				ESP_LOGI(TAG, "WiFi station mode initialized; connecting...");
				ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_connect());
			} break;

			case WIFI_EVENT_STA_DISCONNECTED: {
				xSemaphoreTake(__network_ready_semaphore, 0);

				wifi_event_sta_disconnected_t *event = event_data;
				ESP_LOGI(TAG, "WiFi station mode disconnected ((wifi_err_reason_t) reason=%u)", event->reason);

				// `esp_wifi_disconnect()` not called.
				if(event->reason != WIFI_REASON_ASSOC_LEAVE){
					ESP_LOGI(TAG, "reconnecting...");
					ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_connect());
				}
			} break;

			case WIFI_EVENT_AP_START: {
				ESP_LOGI(TAG, "WiFi soft AP mode initialized");
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
}

/************************************************************************************************************
* Public Functions Definitions
 ************************************************************************************************************/

esp_err_t wifi_setup(){

	__wifi_task_handle = xTaskGetCurrentTaskHandle();
	__network_ready_semaphore = xSemaphoreCreateBinary();

	ESP_RETURN_ON_FALSE(
		__network_ready_semaphore != NULL,

		ESP_ERR_NO_MEM,
		TAG,
		"Error on `xSemaphoreCreateBinary()`"
	);

	ESP_RETURN_ON_ERROR(
		__nvs_setup(),

		TAG,
		"Error on `__nvs_setup()`"
	);

	/**
	 * The async execution pattern is:
	 * - `esp_wifi_start()`
	 * - Wait for a task notification (a simple UNIX signal).
	 * - On the default ESP event loop, `__net_event_callback()` is called with `WIFI_EVENT_STA_START`.
	 * - ...so, `esp_wifi_connect()`
	 * - On the default ESP event loop, '__net_event_callback()' is called with `WIFI_EVENT_STA_DISCONNECTED` or `IP_EVENT_STA_GOT_IP`.
	 * - If `IP_EVENT_STA_GOT_IP`, send the task notification and unlock the calling task.
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

	// WiFi driver initialization.
	wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
	ESP_RETURN_ON_ERROR(
		esp_wifi_init(&wifi_init_config),

		TAG,
		"Error on `esp_wifi_init()`"
	);

	// Register events to the default event loop.
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

	// First call: STA mode.
	ESP_RETURN_ON_ERROR(
		__wifi_netif_up(),

		TAG,
		"Error on `__wifi_netif_up()` (WiFi station mode)"
	);

	ESP_LOGI(TAG, "Station SSID: \"" CONFIG_STA_SSID "\"");

	/**
	 * Block the caller and clear the notification when it's set (`pdTRUE` = act as a binary semaphore).
	 * If a timeout occurs, set the wifi_ap mode.
	 */
	if(ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(CONFIG_WIFI_STA_CONNECTION_TIMEOUT_SECONDS * 1000)) == 0){
		ESP_LOGW(TAG, "WiFi station mode setup failed; starting WiFi soft AP mode setup");
		ESP_RETURN_ON_ERROR(
			__wifi_netif_up(),

			TAG,
			"Error on `__wifi_netif_up()` (WiFi soft AP mode)"
		);

		ESP_LOGI(TAG, "Soft AP SSID: \"" CONFIG_WIFI_SOFTAP_SSID "\"");

		// Block the caller.
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		ESP_LOGI(TAG, "WiFi soft AP mode setup ok");
	}

	else
		ESP_LOGI(TAG, "WiFi station mode setup ok");

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
