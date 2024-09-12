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

// `__send_file()` `line_buffer` size.
#define LINE_BUFFER_SIZE	1024

#define __route(__uri, __method, __handler)	{ \
	.uri			= (__uri), \
	.method		= (__method), \
	.handler	= (__handler), \
	.user_ctx	= NULL \
}

// Webserver routes.
#define ROUTES	{ \
	__route("/",	HTTP_GET,	__route_root), \
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
static void __handle_send_file_error(httpd_req_t *req, esp_err_t ret);
static esp_err_t __send_file(httpd_req_t *req, const char *path);

static esp_err_t __route_root(httpd_req_t *req);

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

void __handle_send_file_error(httpd_req_t *req, esp_err_t ret){
	switch(ret){
		case ESP_ERR_NOT_FOUND:
			httpd_resp_send_404(req);
			break;

		default:
			httpd_resp_send_500(req);
			break;
	}
}

esp_err_t __send_file(httpd_req_t *req, const char *path){

	char full_path[FS_LITTLEFS_BASE_PATH_LEN + strlen(path) + 1];
	sprintf(full_path, FS_LITTLEFS_BASE_PATH "%s", path);

	FILE *file = fopen(full_path, "r");
	ESP_RETURN_ON_FALSE(
		file != NULL,

		ESP_ERR_NOT_FOUND,
		TAG,
		"Error on `fopen(filename=\"%s\")`",
		full_path
	);

	char line_buffer[LINE_BUFFER_SIZE];
	while(fgets(line_buffer, sizeof(line_buffer), file) != NULL)
		ESP_RETURN_ON_ERROR(
			httpd_resp_sendstr_chunk(
				req,
				line_buffer
			),

			TAG,
			"Error on `httpd_resp_sendstr_chunk(str=\"%s\")`",
			line_buffer
		);

	ESP_RETURN_ON_ERROR(
		httpd_resp_sendstr_chunk(
			req,
			NULL
		),

		TAG,
		"Error on `httpd_resp_sendstr_chunk(str=\"%s\")`",
		line_buffer
	);

	fclose(file);
	return ESP_OK;
}

esp_err_t __route_root(httpd_req_t *req){
	__log_http_request(req);

	esp_err_t ret;
	const char *path = "/index.html";

	ESP_GOTO_ON_ERROR(
		__send_file(req, path),

		label_send_file_error,
		TAG,
		"Error on `__send_file(path=\"%s\")`",
		path
	);

	return ESP_OK;

	label_send_file_error:
	__handle_send_file_error(req, ret);
	return ret;
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

	ESP_RETURN_ON_FALSE(
		fs_available(),

		ESP_ERR_INVALID_STATE,
		TAG,
		"Error: filesystem service not initialized"
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
