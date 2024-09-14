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
#include <private.h>

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

// Valid Wi-Fi station configurations loaded.
static bool __valid_sta_configurations;

// SmartConfig ACK.
static struct {
	bool send_ack;
	uint8_t token;
	uint8_t cellphone_ip[4];
} __smartconfig_ack = {
	.send_ack = false
};

/************************************************************************************************************
* Private Functions Prototypes
 ************************************************************************************************************/

/**
 * @return `ESP_ERR_WIFI_NOT_CONNECT` on station connection fail.
 */
static esp_err_t __wifi_mode_sta();
static esp_err_t __wifi_mode_smart_config();

static void __net_event_callback(void* event_handler_arg, esp_event_base_t event_base, int32_t event_id, void* event_data);

/************************************************************************************************************
* Private Functions Definitions
 ************************************************************************************************************/

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

	// Get configs from NVS.
	wifi_config_t wifi_sta_config;
	ESP_RETURN_ON_ERROR(
		esp_wifi_get_config(
			WIFI_IF_STA,
			&wifi_sta_config
		),

		TAG,
		"Error on `esp_wifi_get_config()`"
	);

	__valid_sta_configurations = (strcmp((char*) wifi_sta_config.sta.ssid, "") != 0);

	// Wi-Fi configurations not found.
	if(__valid_sta_configurations)
		ESP_LOGI(TAG, "(STA mode) saved SSID is \"%s\"", wifi_sta_config.sta.ssid);

	else
		ESP_LOGW(TAG, "(STA mode) no saved credentials in NVS");

	// Station mode.
	ESP_RETURN_ON_ERROR(
		esp_wifi_set_mode(WIFI_MODE_STA),

		TAG,
		"Error on `esp_wifi_set_mode()`"
	);

	// Start the async connection procedure.
	ESP_RETURN_ON_ERROR(
		esp_wifi_start(),

		TAG,
		"Error on `esp_wifi_start()`"
	);

	// Block the caller and clear the notification when it's set (`pdTRUE` = act as a binary semaphore).
	if(
		!__valid_sta_configurations ||
		ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(CONFIG_WIFI_STA_CONNECTION_TIMEOUT_SECONDS * 1000)) == 0
	){
		__valid_sta_configurations = false;

		ESP_LOGE(TAG, "(STA mode) connection failed");
		ESP_RETURN_ON_ERROR(
			esp_wifi_stop(),

			TAG,
			"Error on `esp_wifi_stop()`"
		);

		return ESP_ERR_WIFI_NOT_CONNECT;
	}

	ESP_LOGI(TAG, "(STA mode) connected");
	return ESP_OK;
}

esp_err_t __wifi_mode_smart_config(){

	ESP_LOGI(TAG, "(SmartConfig mode) start provisioning...");
	ESP_RETURN_ON_ERROR(
		esp_wifi_start(),

		TAG,
		"Error on `esp_wifi_start()`"
	);

	ESP_RETURN_ON_ERROR(
		esp_smartconfig_set_type(SC_TYPE_ESPTOUCH),

		TAG,
		"Error on `esp_smartconfig_set_type()`"
	);

	smartconfig_start_config_t smartconfig_start_config =
		SMARTCONFIG_START_CONFIG_DEFAULT();

	ESP_RETURN_ON_ERROR(
		esp_smartconfig_start(&smartconfig_start_config),

		TAG,
		"Error on `esp_smartconfig_start()`"
	);

	ESP_LOGI(TAG, "(SmartConfig mode) waiting for EspTouch app...");
	while(ulTaskNotifyTake(pdTRUE, portMAX_DELAY) == 0);

	ESP_RETURN_ON_ERROR(
		esp_smartconfig_stop(),

		TAG,
		"Error on `esp_smartconfig_stop()`"
	);

	ESP_RETURN_ON_ERROR(
		esp_wifi_stop(),

		TAG,
		"Error on `esp_wifi_stop()`"
	);

	return ESP_OK;
}

void __net_event_callback(void* event_handler_arg, esp_event_base_t event_base, int32_t event_id, void* event_data){

	// Wi-Fi events.
	if(event_base == WIFI_EVENT)
		switch(event_id){
			case WIFI_EVENT_STA_START: {
				if(__valid_sta_configurations)
					ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_connect());
			} break;

			case WIFI_EVENT_STA_DISCONNECTED: {
				xSemaphoreTake(__network_ready_semaphore, 0);

				wifi_event_sta_disconnected_t *event = event_data;
				ESP_LOGW(TAG, "(STA mode) disconnected ((wifi_err_reason_t) reason=%u)", event->reason);

				// `esp_wifi_disconnect()` not called.
				if(event->reason != WIFI_REASON_ASSOC_LEAVE){
					ESP_LOGI(TAG, "(STA mode) reconnecting...");
					ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_connect());
				}
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

			default:
				break;
		}

	// SmartConfig events.
	else if(event_base == SC_EVENT)
		switch(event_id){
			case SC_EVENT_SCAN_DONE: {
				ESP_LOGI(TAG, "(SmartConfig mode) scan done");
			} break;

			case SC_EVENT_FOUND_CHANNEL: {
				ESP_LOGI(TAG, "(SmartConfig mode) found channel");
			} break;

			case SC_EVENT_GOT_SSID_PSWD: {
				smartconfig_event_got_ssid_pswd_t *event = event_data;
				ESP_LOGI(TAG, "(SmartConfig mode) got SSID and password");

				wifi_config_t wifi_sta_config;
				memcpy(
					wifi_sta_config.sta.ssid,
					event->ssid,
					sizeof(wifi_sta_config.sta.ssid)
				);

				memcpy(
					wifi_sta_config.sta.password,
					event->password,
					sizeof(wifi_sta_config.sta.password)
				);

				// Save Wi-Fi configurations to NVS.
				ESP_ERROR_CHECK_WITHOUT_ABORT(
					esp_wifi_set_config(
						WIFI_IF_STA,
						&wifi_sta_config
					)
				);

				// When connected, send the SmartConfig ACK.
				__smartconfig_ack.send_ack = true;
				__smartconfig_ack.token = event->token;

				memcpy(
					__smartconfig_ack.cellphone_ip,
					event->cellphone_ip,
					sizeof(__smartconfig_ack.cellphone_ip)
				);

				__unblock_caller();
			} break;

			case SC_EVENT_SEND_ACK_DONE: {
				ESP_LOGI(TAG, "(SmartConfig mode) provisioning done");
				sc_send_ack_stop();
			} break;

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

	// Handle network interface events on the default event loop.
	esp_netif_create_default_wifi_sta();

	esp_err_t ret;
	do {

		// Wi-Fi station connection attempt.
		ret = __wifi_mode_sta();

		// Wi-Fi station connection failed.
		if(ret == ESP_ERR_WIFI_NOT_CONNECT){

			ESP_LOGE(
				TAG, "`__wifi_mode_sta()` return code is %u (%s)",
				ret, esp_err_to_name(ret)
			);

			ESP_RETURN_ON_ERROR(
				__wifi_mode_smart_config(),

				TAG,
				"Error on `__wifi_mode_smart_config()`"
			);
		}

		else
			ESP_RETURN_ON_ERROR(
				ret,

				TAG,
				"Error on `__wifi_mode_sta()`"
			);

	} while(ret != ESP_OK);

	// Send SmartConfig ACK if needed.
	if(__smartconfig_ack.send_ack)
		ESP_RETURN_ON_ERROR(
			sc_send_ack_start(
				SC_TYPE_ESPTOUCH,
				__smartconfig_ack.token,
				__smartconfig_ack.cellphone_ip
			),

			TAG,
			"Error on `sc_send_ack_start()`"
		);

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
