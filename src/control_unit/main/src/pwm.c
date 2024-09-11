/** @file pwm.c
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

#include <pwm.h>

/************************************************************************************************************
* Private Defines
************************************************************************************************************/

#define LOG_TAG	"pwm"
// #define LOG_STUB

// `__pwm_queue` max length in number of elements.
#define PWM_QUEUE_BUFFER_LEN_ELEMENTS		20

/**
 * @brief Statement to check if the library was initialized.
 */
#define __is_initialized()( \
	__pwm_task_handle != NULL \
)

/************************************************************************************************************
* Private Types Definitions
 ************************************************************************************************************/

typedef struct __attribute__((__packed__)) {

	// PWM output.
	uint8_t pwm_port: 1;
	uint8_t pwm_channel: 3;

	// Up to `PWM_DUTY_MAX`.
	uint16_t target_duty: PWM_BIT_RES;

	// Up to 262144ms.
	uint32_t fade_time_ms: 18;

} pwm_data_t;

/************************************************************************************************************
* Private Variables
 ************************************************************************************************************/

static const char *TAG = LOG_TAG;

static TaskHandle_t __pwm_task_handle = NULL;
static QueueHandle_t __pwm_queue;

/************************************************************************************************************
* Private Functions Prototypes
 ************************************************************************************************************/

/**
 * @param pwm_port Optional, can be `NULL`.
 * @param pwm_channel Optional, can be `NULL`.
 * @param pwm_gpio Optional, can be `NULL`.
 */
static bool __zone_to_pwm_out(zone_t zone, uint8_t *pwm_port, uint8_t *pwm_channel, uint8_t *pwm_gpio);

static esp_err_t __ledc_driver_setup();
static esp_err_t __pwm_task_setup();

static void __pwm_task(void *parameters);

/************************************************************************************************************
* Private Functions Definitions
 ************************************************************************************************************/

bool __zone_to_pwm_out(zone_t zone, uint8_t *pwm_port, uint8_t *pwm_channel, uint8_t *pwm_gpio){

	uint8_t pwm_ports[] = ZONE_PWM_PORTS;
	uint8_t pwm_channels[] = ZONE_PWM_CHANNELS;
	uint8_t pwm_gpios[] = ZONE_PWM_GPIO;

	zone_t pwm_zones[] = ZONE_PWM_ZONES;
	uint8_t i = 0;

	while(i < ZONE_PWM_LEN){
		if(pwm_zones[i] == zone)
			break;

		i++;
	}

	if(pwm_zones[i] == zone){
		if(pwm_port != NULL)
			*pwm_port = pwm_ports[i];

		if(pwm_channel != NULL)
			*pwm_channel = pwm_channels[i];

		if(pwm_gpio != NULL)
			*pwm_gpio = pwm_gpios[i];

		return true;
	}

	return false;
}

esp_err_t __ledc_driver_setup(){

	/* LEDC base config */

	ledc_timer_config_t ledc_tim_config = {
		.freq_hz = CONFIG_PWM_FREQUENCY_HZ,
		.duty_resolution = PWM_BIT_RES,
		.timer_num = LEDC_TIMER_1,
		.clk_cfg = LEDC_AUTO_CLK
	};

	// Timer setup for both ports.
	for(uint8_t i=0; i<LEDC_SPEED_MODE_MAX; i++){
		ledc_tim_config.speed_mode = i;

		ESP_RETURN_ON_ERROR(
			ledc_timer_config(&ledc_tim_config),

			TAG,
			"Error on `ledc_timer_config(speed_mode=%u)`",
			i
		);
	}

	/**
	 * LEDC hardware:
	 * 	-	One peripheral.
	 * 	-	Two ports (high/low speed).
	 * 	-	Eight PWM channel per port.
	 */
	ledc_channel_config_t ledc_ch_config = {
		.intr_type = LEDC_INTR_DISABLE,
		.timer_sel = LEDC_TIMER_1,
		.duty = 0,
		.hpoint = 0,
		.flags.output_invert = false
	};

	uint8_t pwm_ports[] = ZONE_PWM_PORTS;
	uint8_t pwm_channels[] = ZONE_PWM_CHANNELS;
	uint8_t pwm_gpios[] = ZONE_PWM_GPIO;

	// Channel setup for both ports.
	for(uint8_t i=0; i<ZONE_PWM_LEN; i++){
		ledc_ch_config.speed_mode = pwm_ports[i];
		ledc_ch_config.channel = pwm_channels[i];
		ledc_ch_config.gpio_num = pwm_gpios[i];

		ESP_RETURN_ON_ERROR(
			ledc_channel_config(&ledc_ch_config),

			TAG,
			"Error on `ledc_channel_config(pwm_gpio=%u, speed_mode=%u, channel=%u)`",
			ledc_ch_config.gpio_num, ledc_ch_config.speed_mode, ledc_ch_config.channel
		);
	}

	/* LEDC fade config */

	ESP_RETURN_ON_ERROR(
		ledc_fade_func_install(0),

		TAG,
		"Error on `ledc_fade_func_install()`"
	);

	return ESP_OK;
}

esp_err_t __pwm_task_setup(){

	__pwm_queue = xQueueCreate(
		PWM_QUEUE_BUFFER_LEN_ELEMENTS,
		sizeof(pwm_data_t)
	);

	ESP_RETURN_ON_FALSE(
		__pwm_queue != NULL,

		ESP_ERR_NO_MEM,
		TAG,
		"Error: unable to allocate `__pwm_queue`"
	);

	BaseType_t ret_val = xTaskCreatePinnedToCore(
		__pwm_task,
		LOG_TAG"_task",
		CONFIG_PWM_TASK_STACK_SIZE_BYTES,
		NULL,
		CONFIG_PWM_TASK_PRIORITY,
		&__pwm_task_handle,
		CONFIG_PM_TASK_CORE_AFFINITY
	);

	ESP_RETURN_ON_FALSE(
		ret_val == pdPASS,

		ESP_ERR_INVALID_STATE,
		TAG,
		"Error %d: unable to spawn \""LOG_TAG"_task\"",
		ret_val
	);

	return ESP_OK;
}

void __pwm_task(void *parameters){

	ESP_LOGI(TAG, "Started");

	/* Variables */

	// `ESP_GOTO_ON_ERROR()` return code.
	esp_err_t ret __attribute__((unused));

	// PWM incoming data.
	pwm_data_t pwm_data;

	/* Code */

	/* Infinite loop */
	for(;;){
		ret = ESP_OK;

		// Waiting for `pwm_write_zone()` requests.
		if(xQueueReceive(__pwm_queue, &pwm_data, portMAX_DELAY) == pdFALSE)
			continue;

		#ifdef LOG_STUB
		ESP_LOGW(TAG, "LOG_STUB");
		ESP_LOGI(TAG, "PWM zone: %u", pwm_data.zone);
		ESP_LOGI(TAG, "Target duty: %u", pwm_data.target_duty);
		ESP_LOGI(TAG, "Fade time %ums", pwm_data.fade_time_ms);

		// Avoid "label 'task_continue' defined but not used" compiler error.
		if(0) goto task_continue;
		#else

		// Set PWM parameters.
		ESP_GOTO_ON_ERROR(
			ledc_set_fade_with_time(
				pwm_data.pwm_port,
				pwm_data.pwm_channel,
				pwm_data.target_duty,
				pwm_data.fade_time_ms
			),

			task_continue,
			TAG,
			"Error on `ledc_set_fade_with_time(speed_mode=%u, channel=%u, target_duty=%u, max_fade_time_ms=%u)`",
			pwm_data.pwm_port, pwm_data.pwm_channel, pwm_data.target_duty, pwm_data.fade_time_ms
		);

		// Fade start.
		ESP_GOTO_ON_ERROR(
			ledc_fade_start(
				pwm_data.pwm_port,
				pwm_data.pwm_channel,
				LEDC_FADE_NO_WAIT
			),

			task_continue,
			TAG,
			"Error on `ledc_fade_start(speed_mode=%u, channel=%u)`",
			pwm_data.pwm_port, pwm_data.pwm_channel
		);

		#endif

		task_continue:
	}
}

/************************************************************************************************************
* Public Functions Definitions
 ************************************************************************************************************/

esp_err_t pwm_setup(){

	ESP_RETURN_ON_ERROR(
		__ledc_driver_setup(),

		TAG,
		"Error on `__ledc_driver_setup()`"
	);

	ESP_RETURN_ON_ERROR(
		__pwm_task_setup(),

		TAG,
		"Error on `__pwm_task_setup()`"
	);

	return ESP_OK;
}

esp_err_t pwm_write_zone(uint8_t zone, uint16_t target_duty, uint16_t fade_time_ms){

	ESP_RETURN_ON_FALSE(
		__is_initialized(),

		ESP_ERR_INVALID_STATE,
		TAG,
		"Error: library not initialized"
	);

	uint8_t pwm_port, pwm_channel;

	// The zone is is not a PWM zone.
	if(!__zone_to_pwm_out(zone, &pwm_port, &pwm_channel, NULL))
		return ESP_ERR_NOT_SUPPORTED;

	pwm_data_t pwm_data = {
		.pwm_port = pwm_port,
		.pwm_channel = pwm_channel,
		.target_duty = target_duty,
		.fade_time_ms = fade_time_ms
	};

	ESP_RETURN_ON_FALSE(
		xQueueSend(__pwm_queue, &pwm_data, 0) == pdTRUE,

		ESP_ERR_INVALID_STATE,
		TAG,
		"Error: `pwm_queue` is full"
	);

	return ESP_OK;
}
