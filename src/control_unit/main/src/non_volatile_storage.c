/** @file non_volatile_storage.c
 *  @brief  Created on: Sep 6, 2024
 *          Davide Scalisi
 *
 * @copyright [2024] Davide Scalisi *
 * @copyright All Rights Reserved. *
 *
*/

/************************************************************************************************************
* Included files
************************************************************************************************************/

#include <non_volatile_storage.h>
#include <private.h>

/************************************************************************************************************
* Private Defines
************************************************************************************************************/

#define LOG_TAG	"non_volatile_storage"

/************************************************************************************************************
* Private Types Definitions
 ************************************************************************************************************/

/************************************************************************************************************
* Private Variables
 ************************************************************************************************************/

static const char *TAG = LOG_TAG;
static bool __nvs_available = false;

/************************************************************************************************************
* Private Functions Prototypes
 ************************************************************************************************************/

/************************************************************************************************************
* Private Functions Definitions
 ************************************************************************************************************/

/************************************************************************************************************
* Public Functions Definitions
 ************************************************************************************************************/

esp_err_t nvs_setup(){

	// Initialize NVS partition (Non-Volatile Storage).
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

	__nvs_available = true;
	return ESP_OK;
}

bool nvs_available(){
	return __nvs_available;
}

esp_err_t nvs_new_handle(nvs_handle_t *nvs_handle, const char *nvs_namespace){
	assert_param_notnull(nvs_handle);
	assert_param_notnull(nvs_namespace);

	ESP_RETURN_ON_FALSE(
		__nvs_available,

		ESP_ERR_NVS_NOT_INITIALIZED,
		TAG,
		"Error: NVS not initialized"
	);

	ESP_RETURN_ON_ERROR(
		nvs_open(
			nvs_namespace,
			NVS_READWRITE,
			nvs_handle
		),

		TAG,
		"Error on `nvs_open()`"
	);

	return ESP_OK;
}
