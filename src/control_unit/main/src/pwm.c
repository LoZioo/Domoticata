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

// `__pwm_queue` max length in number of elements.
#define PWM_QUEUE_BUFFER_LEN_ELEMENTS		10

#define PWM_CH_TO_GPIO_INIT	{ \
	CONFIG_GPIO_FAN, \
	CONFIG_GPIO_LED_1, \
	CONFIG_GPIO_LED_2, \
	CONFIG_GPIO_LED_3, \
	CONFIG_GPIO_LED_4, \
	CONFIG_GPIO_LED_5, \
	CONFIG_GPIO_LED_6, \
	CONFIG_GPIO_LED_7, \
	CONFIG_GPIO_LED_8, \
	CONFIG_GPIO_LED_9, \
	CONFIG_GPIO_LED_10, \
	CONFIG_GPIO_LED_11, \
	CONFIG_GPIO_LED_12 \
}

/**
 * @brief Statement to check if the library was initialized.
 */
#define __is_initialized()( \
	__pwm_task_handle != NULL \
)

/**
 * @return `ledc_mode_t` given the zone.
 * @note 0: Fan controller, 1-12: LEDs.
 */
#define __pwm_get_port(zone)( \
	zone < LEDC_CHANNEL_MAX ? \
	LEDC_HIGH_SPEED_MODE : \
	LEDC_LOW_SPEED_MODE \
)

/**
 * @return `ledc_channel_t` given the zone.
 * @note 0: Fan controller, 1-12: LEDs.
 */
#define __pwm_get_channel(zone)( \
	(ledc_channel_t) (zone % LEDC_CHANNEL_MAX) \
)

/************************************************************************************************************
* Private Types Definitions
 ************************************************************************************************************/

/************************************************************************************************************
* Private Variables
 ************************************************************************************************************/

static const char *TAG = LOG_TAG;

static TaskHandle_t __pwm_task_handle = NULL;
static QueueHandle_t __pwm_queue;

/************************************************************************************************************
* Private Functions Prototypes
 ************************************************************************************************************/

static esp_err_t __ledc_driver_setup();
static esp_err_t __pwm_task_setup();

static void __pwm_task(void *parameters);

/************************************************************************************************************
* Private Functions Definitions
 ************************************************************************************************************/

esp_err_t __ledc_driver_setup(){

	/* LEDC base config */

	ledc_timer_config_t ledc_tim_config = {
		.freq_hz = CONFIG_PWM_FREQUENCY_HZ,
		.duty_resolution = (ledc_timer_bit_t) PWM_BIT_RES,
		.timer_num = LEDC_TIMER_1,
		.clk_cfg = LEDC_AUTO_CLK
	};

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

	uint8_t pwm_ch_to_gpio[] = PWM_CH_TO_GPIO_INIT;

	// Channel setup for both ports.
	for(uint8_t i=0; i<sizeof(pwm_ch_to_gpio); i++){
		ledc_ch_config.gpio_num = pwm_ch_to_gpio[i];
		ledc_ch_config.speed_mode = __pwm_get_port(i);
		ledc_ch_config.channel = __pwm_get_channel(i);

		ESP_RETURN_ON_ERROR(
			ledc_channel_config(&ledc_ch_config),
			TAG,
			"Error on `ledc_channel_config(gpio_num=%u, speed_mode=%u, channel=%u)`",
			pwm_ch_to_gpio[i], __pwm_get_port(i), __pwm_get_channel(i)
		);
	}

	/* LEDC fade config */

	ledc_fade_func_install(0);

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
		"Error: unable to allocate \"__pwm_queue\""
	);

	BaseType_t ret_val = xTaskCreatePinnedToCore(
		__pwm_task,
		LOG_TAG"_task",
		CONFIG_PWM_TASK_STACK_SIZE_BYTES,
		NULL,
		CONFIG_PWM_TASK_PRIORITY,
		&__pwm_task_handle,

		#ifdef CONFIG_PWM_TASK_CORE_AFFINITY_APPLICATION
			ESP_APPLICATION_CORE
		#else
			ESP_PROTOCOL_CORE
		#endif
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
	esp_err_t ret;

	// PWM incoming data.
	pwm_data_t pwm;

	/* Code */

	/* Infinite loop */
	for(;;){
		ret = ESP_OK;

		// Waiting for `pwm_write()` requests.
		if(xQueueReceive(__pwm_queue, &pwm, portMAX_DELAY) == pdFALSE)
			goto task_continue;

		// Set PWM parameters.
		ESP_GOTO_ON_ERROR(
			ledc_set_fade_with_time(
				__pwm_get_port(pwm.zone),
				__pwm_get_channel(pwm.zone),
				pwm.target_duty,
				pwm.fade_time_ms
			),

			task_error,
			TAG,
			"Error on `ledc_set_fade_with_time(speed_mode=%u, channel=%u, target_duty=%u, max_fade_time_ms=%u)`",
			__pwm_get_port(pwm.zone), __pwm_get_channel(pwm.zone), pwm.target_duty, pwm.fade_time_ms
		);

		// Fade start.
		ESP_GOTO_ON_ERROR(
			ledc_fade_start(
				__pwm_get_port(pwm.zone),
				__pwm_get_channel(pwm.zone),
				LEDC_FADE_NO_WAIT
			),

			task_error,
			TAG,
			"Error on `ledc_fade_start(speed_mode=%u, channel=%u)`",
			__pwm_get_port(pwm.zone), __pwm_get_channel(pwm.zone)
		);

		// Delay before continuing.
		goto task_continue;

		task_error:
		ESP_ERROR_CHECK_WITHOUT_ABORT(ret);

		task_continue:
		delay(1);
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

esp_err_t pwm_write(uint8_t zone, uint16_t target_duty, uint16_t fade_time_ms){

	ESP_RETURN_ON_FALSE(
		__is_initialized(),

		ESP_ERR_INVALID_STATE,
		TAG,
		"Error: library not initialized"
	);

	ESP_RETURN_ON_FALSE(
		zone < PWM_ZONES,

		ESP_ERR_INVALID_ARG,
		TAG,
		"Error: `zone` must be between 0 and %u",
		PWM_ZONES - 1
	);

	if(target_duty > PWM_DUTY_MAX)
		target_duty = PWM_DUTY_MAX;

	pwm_data_t pwm_data = {
		.zone = zone,
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
