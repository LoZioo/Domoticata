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
#include <private.h>

/************************************************************************************************************
* Private Defines
************************************************************************************************************/

#define LOG_TAG	"webserver"

#define WEBSERVER_ROOT_FOLDER				FS_LITTLEFS_BASE_PATH "/www"
#define WEBSERVER_ROOT_FOLDER_LEN		(sizeof(WEBSERVER_ROOT_FOLDER) - 1)

// `__route_send_file()` `line_buffer` size.
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
	__route("/*",	HTTP_GET,	__route_send_file), \
}

/************************************************************************************************************
* Private Types Definitions
 ************************************************************************************************************/

/************************************************************************************************************
* Private Variables
 ************************************************************************************************************/

static const char *TAG = LOG_TAG;
static httpd_handle_t __webserver_handle = NULL;

/************************************************************************************************************
* Private Functions Prototypes
 ************************************************************************************************************/

static esp_err_t __register_routes();
static void __log_http_request(httpd_req_t *req);

/**
 * @brief Send the specified file from VFS.
 * @note The path is `req->uri`.
 */
static esp_err_t __route_send_file(httpd_req_t *req);
static esp_err_t __route_root(httpd_req_t *req);

/************************************************************************************************************
* Private Functions Definitions
 ************************************************************************************************************/

esp_err_t __register_routes(){

	httpd_uri_t routes[] = ROUTES;
	uint32_t routes_len = sizeof(routes) / sizeof(httpd_uri_t);

	for(uint32_t i=0; i<routes_len; i++){

		ESP_LOGI(
			TAG, "Registering route: (%s) %s",
			http_method_str(routes[i].method),
			routes[i].uri
		);

		ESP_RETURN_ON_ERROR(
			httpd_register_uri_handler(
				__webserver_handle,
				&routes[i]
			),

			TAG,
			"Error on `httpd_register_uri_handler(i=%lu)`",
			i
		);
	}

	return ESP_OK;
}

void __log_http_request(httpd_req_t *req){
	ESP_LOGI(
		TAG,
		"(%s) %s",
		req->method == -1 ? "Unsupported HTTP method" : http_method_str(req->method),
		req->uri
	);
}

esp_err_t __route_send_file(httpd_req_t *req){
	esp_err_t ret = ESP_OK;
	__log_http_request(req);

	char full_path[WEBSERVER_ROOT_FOLDER_LEN + strlen(req->uri) + 1];
	snprintf(
		full_path,
		sizeof(full_path),
		WEBSERVER_ROOT_FOLDER "%s",
		req->uri
	);

	FILE *file = fopen(full_path, "r");
	ESP_GOTO_ON_FALSE(
		file != NULL,

		ESP_ERR_NOT_FOUND,
		label_error_404,
		TAG,
		"Error on `fopen(filename=\"%s\")` (errno=%d)",
		full_path, errno
	);

	char line_buffer[LINE_BUFFER_SIZE];
	while(fgets(line_buffer, sizeof(line_buffer), file) != NULL)
		ESP_GOTO_ON_ERROR(
			httpd_resp_sendstr_chunk(
				req,
				line_buffer
			),

			label_error_500,
			TAG,
			"Error on `httpd_resp_sendstr_chunk(str=\"%s\")`",
			line_buffer
		);

	ESP_GOTO_ON_ERROR(
		httpd_resp_sendstr_chunk(
			req,
			NULL
		),

		label_error_500,
		TAG,
		"Error on `httpd_resp_sendstr_chunk(str=\"%s\")`",
		line_buffer
	);

	label_fclose:
	fclose(file);
	return ret;

	label_error_404:
	ESP_ERROR_CHECK_WITHOUT_ABORT(httpd_resp_send_404(req));
	goto label_fclose;

	label_error_500:
	ESP_ERROR_CHECK_WITHOUT_ABORT(httpd_resp_send_500(req));
	goto label_fclose;
}

esp_err_t __route_root(httpd_req_t *req){
	esp_err_t ret = ESP_OK;

	ESP_GOTO_ON_ERROR(
		httpd_resp_set_status(req, "302 Found"),

		label_error,
		TAG,
		"Error on `httpd_resp_set_status()"
	);

	ESP_GOTO_ON_ERROR(
		httpd_resp_set_hdr(req, "Location", "/index.html"),

		label_error,
		TAG,
		"Error on `httpd_resp_set_hdr()"
	);

	ESP_GOTO_ON_ERROR(
		httpd_resp_send(req, NULL, 0),

		label_error,
		TAG,
		"Error on `httpd_resp_send()"
	);

	return ret;

	label_error:
	ESP_ERROR_CHECK_WITHOUT_ABORT(httpd_resp_send_500(req));
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

	webserver_config.stack_size = CONFIG_WEBSERVER_TASK_STACK_SIZE_BYTES;
	webserver_config.task_priority = CONFIG_WEBSERVER_TASK_PRIORITY;
	webserver_config.core_id = CONFIG_WEBSERVER_TASK_CORE_AFFINITY;
	webserver_config.server_port = CONFIG_WEBSERVER_LISTEN_PORT;

	/**
	 * Use the URI wildcard matching function in order to
	 * allow the same handler to respond to multiple different
	 * target URIs which match the wildcard scheme.
	 */
	webserver_config.uri_match_fn = httpd_uri_match_wildcard;

	ESP_RETURN_ON_ERROR(
		httpd_start(
			&__webserver_handle,
			&webserver_config
		),

		TAG,
		"Error on `httpd_start()`"
	);

	ESP_RETURN_ON_ERROR(
		__register_routes(),

		TAG,
		"Error on `__register_routes()`"
	);

	return ESP_OK;
}
