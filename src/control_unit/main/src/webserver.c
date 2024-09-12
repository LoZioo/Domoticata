/** @file webserver.c
 *  @brief  Created on: Sep 9, 2024
 *          Davide Scalisi
 *
 * @copyright [2024] Davide Scalisi *
 * @copyright All Rights Reserved. *
 *
*/

/************************************************************************************************************
* Included files
************************************************************************************************************/

#include <webserver.h>

/************************************************************************************************************
* Private Defines
************************************************************************************************************/

#define LOG_TAG	"webserver"

#define __route(__uri, __method, __handler)	{ \
	.uri			= (__uri), \
	.method		= (__method), \
	.handler	= (__handler), \
	.user_ctx	= NULL \
}

// Webserver routes.
#define ROUTES	{ \
	__route("/hello",	HTTP_GET,	__route_hello), \
	__route("/hello1",	HTTP_GET,	__route_hello), \
}

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

static void __log_http_request(httpd_req_t *req);

static esp_err_t __route_hello(httpd_req_t *req);

/************************************************************************************************************
* Private Functions Definitions
 ************************************************************************************************************/

void __log_http_request(httpd_req_t *req){
	ESP_LOGI(
		TAG,
		"%s %s",
		req->method == -1 ? "Unsupported HTTP method" : http_method_str(req->method),
		req->uri
	);
}

esp_err_t __route_hello(httpd_req_t *req){
	__log_http_request(req);

	uint8_t res[] = "Hello World!";
	ESP_RETURN_ON_ERROR(
		httpd_resp_send(
			req,
			(char*) res,
			HTTPD_RESP_USE_STRLEN
		),

		TAG,
		"Error on `httpd_resp_send()`"
	);

	return ESP_OK;
}

/************************************************************************************************************
* Public Functions Definitions
 ************************************************************************************************************/

esp_err_t webserver_setup(){

	ESP_RETURN_ON_FALSE(
		wifi_network_available(),

		ESP_ERR_INVALID_STATE,
		TAG,
		"Error: network service not available"
	);

	// Webserver daemon configurations.
	httpd_config_t webserver_config = HTTPD_DEFAULT_CONFIG();
	httpd_handle_t webserver = NULL;

	webserver_config.stack_size = CONFIG_WEBSERVER_TASK_STACK_SIZE_BYTES;
	webserver_config.task_priority = CONFIG_WEBSERVER_TASK_PRIORITY;
	webserver_config.core_id = CONFIG_WEBSERVER_TASK_CORE_AFFINITY;
	webserver_config.server_port = CONFIG_WEBSERVER_LISTEN_PORT;

	ESP_RETURN_ON_ERROR(
		httpd_start(
			&webserver,
			&webserver_config
		),

		TAG,
		"Error on `httpd_start()`"
	);

	// Webserver routes registration.
	httpd_uri_t routes[] = ROUTES;
	uint32_t routes_len = sizeof(routes) / sizeof(httpd_uri_t);

	for(uint32_t i=0; i<routes_len; i++)
		ESP_RETURN_ON_ERROR(
			httpd_register_uri_handler(
				webserver,
				&routes[i]
			),

			TAG,
			"Error on `httpd_register_uri_handler(i=%lu)`",
			i
		);

	return ESP_OK;
}
