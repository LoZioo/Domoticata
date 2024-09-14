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

// Webserver static routes.
#define STATIC_ROUTES	{ \
	__route("/",	HTTP_GET,	__route_root), \
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

/**
 * @note Remember to free every dynamically allocated string.
 */
static esp_err_t __list_webserver_files_rec(ul_linked_list_handle_t *webserver_files, char *full_path, struct stat *path_stat, char **str_ptr);
static esp_err_t __list_webserver_files(ul_linked_list_handle_t **webserver_files);

/**
 * @brief Webserver static routes registration.
 * @note `STATIC_ROUTES` routes.
 */
static esp_err_t __register_static_routes();

/**
 * @brief Webserver dynamic routes registration.
 * @note Filesystem routes.
 */
static esp_err_t __register_dynamic_routes();

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

esp_err_t __list_webserver_files_rec(ul_linked_list_handle_t *webserver_files, char *full_path, struct stat *path_stat, char **str_ptr){
	esp_err_t ret = ESP_OK;

	DIR *dir = opendir(full_path);
	ESP_RETURN_ON_FALSE(
		dir != NULL,

		ESP_ERR_NOT_FOUND,
		TAG,
		"Error on `opendir(name=\"%s\")` (errno=%d)",
		full_path, errno
	);

	strncat(
		full_path,
		"/",
		PATH_MAX - 2
	);

	struct dirent *entry;
	uint16_t original_len = strlen(full_path);

	while((entry = readdir(dir)) != NULL){
		if(ul_utils_either(
			strcmp(entry->d_name, "."),
			strcmp(entry->d_name, ".."),
			==, 0
		))
			continue;

		full_path[original_len] = '\0';
		strncat(
			full_path,
			entry->d_name,
			PATH_MAX - strlen(entry->d_name) - 1
		);

		// Check if it is a file or a directory.
		ESP_GOTO_ON_FALSE(
			stat(full_path, path_stat) >= 0,

			ESP_FAIL,
			label_closedir,
			TAG,
			"Error on `stat(pathname=\"%s\")` (errno=%d)",
			full_path, errno
		);

		// File.
		if(S_ISREG(path_stat->st_mode)){

			/**
			 * Dynamically allocate a new string containing the file's full path.
			 * Exclude the filesystem path and only include the URI.
			 */
			*str_ptr = strndup(&full_path[WEBSERVER_ROOT_FOLDER_LEN], PATH_MAX);

			ESP_GOTO_ON_ERROR(
				ul_errors_to_esp_err(
					ul_linked_list_add(
						webserver_files,
						str_ptr
					)
				),

				label_closedir,
				TAG,
				"Error on `ul_linked_list_add(full_path=\"%s\")`",
				full_path
			);
		}

		// Directory.
		else if(S_ISDIR(path_stat->st_mode))
			ESP_GOTO_ON_ERROR(
				__list_webserver_files_rec(
					webserver_files,
					full_path,
					path_stat,
					str_ptr
				),

				label_closedir,
				TAG,
				"Error on `__list_webserver_files_rec(full_path=\"%s\")`",
				full_path
			);
	}

	label_closedir:
	closedir(dir);
	return ret;
}

esp_err_t __list_webserver_files(ul_linked_list_handle_t **webserver_files){

	ul_linked_list_init_t webserver_files_init = {
		.element_size = sizeof(char*)
	};

	ESP_RETURN_ON_ERROR(
		ul_errors_to_esp_err(
			ul_linked_list_begin(
				&webserver_files_init,
				webserver_files
			)
		),

		TAG,
		"Error on `ul_linked_list_begin()`"
	);

	char full_path[PATH_MAX] = WEBSERVER_ROOT_FOLDER;
	struct stat path_stat;
	char *str_ptr;

	ESP_RETURN_ON_ERROR(
		__list_webserver_files_rec(
			*webserver_files,
			full_path,
			&path_stat,
			&str_ptr
		),

		TAG,
		"Error on `__list_webserver_files_rec()`"
	);

	return ESP_OK;
}

esp_err_t __register_static_routes(){

	httpd_uri_t static_routes[] = STATIC_ROUTES;
	uint32_t static_routes_len = sizeof(static_routes) / sizeof(httpd_uri_t);

	for(uint32_t i=0; i<static_routes_len; i++){

		ESP_LOGI(
			TAG, "Registering static route: (%s) %s",
			http_method_str(static_routes[i].method),
			static_routes[i].uri
		);

		ESP_RETURN_ON_ERROR(
			httpd_register_uri_handler(
				__webserver_handle,
				&static_routes[i]
			),

			TAG,
			"Error on `httpd_register_uri_handler(i=%lu)`",
			i
		);
	}

	return ESP_OK;
}

esp_err_t __register_dynamic_routes(){

	ul_linked_list_handle_t *dynamic_routes_list;
	ESP_RETURN_ON_ERROR(
		__list_webserver_files(&dynamic_routes_list),

		TAG,
		"Error on `__list_webserver_files()`"
	);

	uint32_t dynamic_routes_len;
	ESP_RETURN_ON_ERROR(
		ul_errors_to_esp_err(
			ul_linked_list_len(
				dynamic_routes_list,
				&dynamic_routes_len
			)
		),

		TAG,
		"Error on `ul_linked_list_len()`"
	);

	char *dynamic_routes_uri[dynamic_routes_len];
	ESP_RETURN_ON_ERROR(
		ul_errors_to_esp_err(
			ul_linked_list_to_arr(
				dynamic_routes_list,
				dynamic_routes_uri
			)
		),

		TAG,
		"Error on `ul_linked_list_end()`"
	);

	httpd_uri_t dynamic_route = {
		.method = HTTP_GET,
		.handler = __route_send_file
	};

	for(uint32_t i=0; i<dynamic_routes_len; i++){
		dynamic_route.uri = dynamic_routes_uri[i];

		ESP_LOGI(
			TAG, "Registering dynamic route: (%s) %s",
			http_method_str(dynamic_route.method),
			dynamic_route.uri
		);

		ESP_RETURN_ON_ERROR(
			httpd_register_uri_handler(
				__webserver_handle,
				&dynamic_route
			),

			TAG,
			"Error on `httpd_register_uri_handler(i=%lu)`",
			i
		);

		// Free dynamically allocated strings by `__list_webserver_files_rec()`.
		free(dynamic_routes_uri[i]);
	}

	// Free the entire linked list.
	ESP_RETURN_ON_ERROR(
		ul_errors_to_esp_err(
			ul_linked_list_end(dynamic_routes_list)
		),

		TAG,
		"Error on `ul_linked_list_end()`"
	);

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

	ESP_RETURN_ON_ERROR(
		httpd_start(
			&__webserver_handle,
			&webserver_config
		),

		TAG,
		"Error on `httpd_start()`"
	);

	ESP_RETURN_ON_ERROR(
		__register_static_routes(),

		TAG,
		"Error on `__register_static_routes()`"
	);

	ESP_RETURN_ON_ERROR(
		__register_dynamic_routes(),

		TAG,
		"Error on `__register_dynamic_routes()`"
	);

	return ESP_OK;
}
