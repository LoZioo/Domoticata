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

#define WEBSERVER_ROOT_FOLDER				FS_LITTLEFS_BASE_PATH CONFIG_WEBSERVER_VFS_ROOT_FOLDER
#define WEBSERVER_ROOT_FOLDER_LEN		(sizeof(WEBSERVER_ROOT_FOLDER) - 1)

#define __str_ends_with2(str, len, str_end) \
	(strcasecmp(&str[len - sizeof(str_end) + 1], str_end) == 0)

#define __str_ends_with(str, str_end) \
	__str_ends_with2(str, strlen(str), str_end)

#define __route(__uri, __method, __handler)	{ \
	.uri			= (__uri), \
	.method		= (__method), \
	.handler	= (__handler), \
	.user_ctx	= NULL \
}

// Webserver routes.
#define ROUTES	{ \
	__route("/",	HTTP_GET,	__route_root), \
	__route("/update/*",	HTTP_GET,	__route_ota_update), \
	__route("/*",	HTTP_GET,	__route_send_text_file), \
}

/************************************************************************************************************
* Private Types Definitions
 ************************************************************************************************************/

typedef struct {
	char *uri;
	char *query;
	uint32_t uri_len;
	uint32_t query_len;
} decoded_uri_t;

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
static decoded_uri_t __decode_uri(httpd_req_t *req);
static esp_ip4_addr_t __get_sender_ipv4(httpd_req_t *req);

static esp_err_t __set_content_type_from_file_type(httpd_req_t *req, const char *filename);

/**
 * @brief Send the requested file from VFS.
 */
static esp_err_t __route_send_text_file(httpd_req_t *req);
static esp_err_t __route_root(httpd_req_t *req);
static esp_err_t __route_ota_update(httpd_req_t *req);

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

decoded_uri_t __decode_uri(httpd_req_t *req){
	decoded_uri_t uri;

	uri.uri = (char*) req->uri;
	uri.query_len = httpd_req_get_url_query_len(req);
	uri.uri_len = strlen(uri.uri) - uri.query_len;
	uri.query = &(uri.uri[uri.uri_len]);

	if(uri.uri[uri.uri_len - 1] == '?')
		uri.uri[--uri.uri_len] = '\0';

	return uri;
}

esp_ip4_addr_t __get_sender_ipv4(httpd_req_t *req){
	esp_ip4_addr_t ret = {
		.addr = 0
	};

	int sockfd = httpd_req_to_sockfd(req);
	struct sockaddr_in6 addr;
	socklen_t addr_len = sizeof(addr);

	if(
		sockfd == -1 ||
		getpeername(sockfd, (struct sockaddr*) &addr, &addr_len) < 0
	)
		return ret;

	if(addr.sin6_family == AF_INET6){
		uint8_t *ipv6_addr = ul_utils_cast_to_mem(addr.sin6_addr);
		ret.addr = ntohl(
			(ipv6_addr[12] << 24) |
			(ipv6_addr[13] << 16) |
			(ipv6_addr[14] << 8)  |
			(ipv6_addr[15])
		);
	}

	else if(addr.sin6_family == AF_INET) {
		struct sockaddr_in *ipv4_addr = (struct sockaddr_in*) &addr;
		ret.addr = ntohl(ipv4_addr->sin_addr.s_addr);
	}

	return ret;
}

esp_err_t __set_content_type_from_file_type(httpd_req_t *req, const char *filename){
	uint32_t len = strlen(filename);

	if(__str_ends_with2(filename, len, ".pdf"))
		return httpd_resp_set_type(req, "application/pdf");

	else if(
		__str_ends_with2(filename, len, ".html") ||
		__str_ends_with2(filename, len, ".htm")
	)
		return httpd_resp_set_type(req, "text/html");

	else if(__str_ends_with2(filename, len, ".css"))
		return httpd_resp_set_type(req, "text/css");

	else if(__str_ends_with2(filename, len, ".js"))
		return httpd_resp_set_type(req, "text/javascript");

	else if(__str_ends_with2(filename, len, ".xml"))
		return httpd_resp_set_type(req, "application/xml");

	else if(__str_ends_with2(filename, len, ".json"))
		return httpd_resp_set_type(req, "application/json");

	else if(__str_ends_with2(filename, len, ".gif"))
		return httpd_resp_set_type(req, "image/gif");

	else if(__str_ends_with2(filename, len, ".jpeg"))
		return httpd_resp_set_type(req, "image/jpeg");

	else if(__str_ends_with2(filename, len, ".png"))
		return httpd_resp_set_type(req, "image/png");

	else if(__str_ends_with2(filename, len, ".ico"))
		return httpd_resp_set_type(req, "image/x-icon");

	return httpd_resp_set_type(req, "text/plain");
}

esp_err_t __route_send_text_file(httpd_req_t *req){
	esp_err_t ret = ESP_OK;
	__log_http_request(req);

	decoded_uri_t uri = __decode_uri(req);
	char *buffer = NULL;
	FILE *file;

	{
		char full_path[
			WEBSERVER_ROOT_FOLDER_LEN +
			uri.uri_len + 1
		];

		snprintf(
			full_path, sizeof(full_path),
			WEBSERVER_ROOT_FOLDER "%s",
			uri.uri
		);

		file = fopen(full_path, "r");
		ESP_GOTO_ON_FALSE(
			file != NULL,

			ESP_ERR_NOT_FOUND,
			label_error_404,
			TAG,
			"Error on `fopen(filename=\"%s\")` (errno=%d)",
			full_path, errno
		);
	}

	ESP_GOTO_ON_ERROR(
		__set_content_type_from_file_type(
			req, uri.uri
		),

		label_error_500,
		TAG,
		"Error on `__set_content_type_from_file_type(filename=\"%s\")`",
		uri.uri
	);

	buffer = malloc(CONFIG_WEBSERVER_FILE_LINE_BUFFER_LEN_BYTES);
	ESP_GOTO_ON_FALSE(
		buffer != NULL,

		ESP_ERR_NO_MEM,
		label_error_500,
		TAG,
		"Error on `malloc(size=%u)`",
		CONFIG_WEBSERVER_FILE_LINE_BUFFER_LEN_BYTES
	);

	while(fgets(buffer, sizeof(buffer), file) != NULL)
		ESP_GOTO_ON_ERROR(
			httpd_resp_sendstr_chunk(
				req, buffer
			),

			label_error_500,
			TAG,
			"Error on `httpd_resp_sendstr_chunk(str=\"%s\")`",
			buffer
		);

	ESP_GOTO_ON_ERROR(
		httpd_resp_sendstr_chunk(
			req, NULL
		),

		label_error_500,
		TAG,
		"Error on `httpd_resp_sendstr_chunk(str=NULL)`"
	);

	label_cleanup:
	free(buffer);
	fclose(file);
	return ret;

	label_error_404:
	ESP_ERROR_CHECK_WITHOUT_ABORT(httpd_resp_send_404(req));
	goto label_cleanup;

	label_error_500:
	ESP_ERROR_CHECK_WITHOUT_ABORT(httpd_resp_send_500(req));
	goto label_cleanup;
}

esp_err_t __route_root(httpd_req_t *req){
	esp_err_t ret = ESP_OK;

	ESP_GOTO_ON_ERROR(
		httpd_resp_set_status(req, "302 Found"),

		label_error,
		TAG,
		"Error on `httpd_resp_set_status()`"
	);

	ESP_GOTO_ON_ERROR(
		httpd_resp_set_hdr(req, "Location", "/index.html"),

		label_error,
		TAG,
		"Error on `httpd_resp_set_hdr()`"
	);

	ESP_GOTO_ON_ERROR(
		httpd_resp_send(req, NULL, 0),

		label_error,
		TAG,
		"Error on `httpd_resp_send()`"
	);

	return ret;

	label_error:
	ESP_ERROR_CHECK_WITHOUT_ABORT(httpd_resp_send_500(req));
	return ret;
}

esp_err_t __route_ota_update(httpd_req_t *req){
	esp_err_t ret = ESP_OK;
	__log_http_request(req);

	decoded_uri_t uri = __decode_uri(req);
	esp_ip4_addr_t ota_server_ip =
		__get_sender_ipv4(req);

	ESP_GOTO_ON_FALSE(
		ota_server_ip.addr != 0,

		ESP_FAIL,
		label_error_500,
		TAG,
		"Error on `__get_sender_ipv4()`"
	);

	bool update_fs;
	if(__str_ends_with(
		uri.uri,
		"/fs"
	))
		update_fs = true;

	else if(__str_ends_with(
		uri.uri,
		"/fw"
	))
		update_fs = false;

	else
		goto label_error_400;

	char ota_server_ip_str[WIFI_IPV4_ADDR_STR_LEN];
	snprintf(
		ota_server_ip_str,
		sizeof(ota_server_ip_str),
		IPSTR,
		IP2STR(&ota_server_ip)
	);

	if(update_fs)
		ESP_GOTO_ON_ERROR(
			httpd_resp_sendstr_chunk(req, "Updating filesystem..."),

			label_error_500,
			TAG,
			"Error on `httpd_resp_sendstr_chunk()`"
		);

	else
		ESP_GOTO_ON_ERROR(
			httpd_resp_sendstr_chunk(req, "Updating firmware..."),

			label_error_500,
			TAG,
			"Error on `httpd_resp_sendstr_chunk()`"
		);

	ESP_GOTO_ON_ERROR(
		httpd_resp_sendstr_chunk(req, "<br>OTA webserver IP: "),

		label_error_500,
		TAG,
		"Error on `httpd_resp_sendstr_chunk()`"
	);

	ESP_GOTO_ON_ERROR(
		httpd_resp_sendstr_chunk(req, ota_server_ip_str),

		label_error_500,
		TAG,
		"Error on `httpd_resp_sendstr_chunk()`"
	);

	ESP_GOTO_ON_ERROR(
		httpd_resp_sendstr_chunk(req, NULL),

		label_error_500,
		TAG,
		"Error on `httpd_resp_sendstr_chunk(str=NULL)`"
	);

	if(update_fs){

		ESP_RETURN_ON_ERROR(
			ota_update_fs(ota_server_ip),

			TAG,
			"Error on `ota_update_fs()`"
		);

		ESP_LOGI(TAG, "Mounting the new filesystem partition");
		ESP_RETURN_ON_ERROR(
			fs_partition_swap(),

			TAG,
			"Error on `fs_partition_swap()`"
		);
	}

	else {

		ESP_RETURN_ON_ERROR(
			ota_update_fw(ota_server_ip),

			TAG,
			"Error on `ota_update_fw()`"
		);

		ESP_LOGI(TAG, "Rebooting to the newly updated firmware");
		esp_restart();
	}

	return ret;

	label_error_400:
	ESP_ERROR_CHECK_WITHOUT_ABORT(httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, NULL));

	label_error_500:
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
