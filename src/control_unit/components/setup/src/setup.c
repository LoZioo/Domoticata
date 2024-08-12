/** @file setup.c
 *  @brief  Created on: Aug 3, 2024
 *          Davide Scalisi
 *
 * @copyright [2024] Davide Scalisi *
 * @copyright All Rights Reserved. *
 *
*/

/************************************************************************************************************
* Included files
************************************************************************************************************/

#include <setup.h>

/************************************************************************************************************
* Private Defines
************************************************************************************************************/

/**
 * @brief Helper.
 */
#define __gpio_to_bit_mask(x)	(1ULL << x)

// `gpio_config_t::pin_bit_mask` for output GPIOs.
#define GPIO_OUT_BIT_MASK	( \
	__gpio_to_bit_mask(CONFIG_GPIO_ALARM) | \
	__gpio_to_bit_mask(CONFIG_GPIO_RELAY_1) | \
	__gpio_to_bit_mask(CONFIG_GPIO_RELAY_2) | \
	__gpio_to_bit_mask(CONFIG_GPIO_RELAY_3) | \
	__gpio_to_bit_mask(CONFIG_GPIO_RELAY_4) \
)

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

/************************************************************************************************************
* Private Types Definitions
 ************************************************************************************************************/

/************************************************************************************************************
* Private Variables
 ************************************************************************************************************/

/************************************************************************************************************
* Private Functions Prototypes
 ************************************************************************************************************/

/************************************************************************************************************
* Private Functions Definitions
 ************************************************************************************************************/

/************************************************************************************************************
* Public Functions Definitions
 ************************************************************************************************************/

esp_err_t GPIO_setup(const char *TAG){

	ESP_RETURN_ON_FALSE(
		TAG != NULL,

		ESP_ERR_INVALID_ARG,
		"???",
		"Error: `TAG` is NULL"
	);

	gpio_config_t io_config = {
		.pin_bit_mask = GPIO_OUT_BIT_MASK,
		.mode = GPIO_MODE_OUTPUT,
		.intr_type = GPIO_INTR_DISABLE,
		.pull_up_en = 0,
		.pull_down_en = 0
	};

	ESP_RETURN_ON_ERROR(
		gpio_config(&io_config),
		TAG,
		"Error on `gpio_config()`"
	);

	return ESP_OK;
}

esp_err_t LEDC_setup(const char *TAG){

	ESP_RETURN_ON_FALSE(
		TAG != NULL,

		ESP_ERR_INVALID_ARG,
		"???",
		"Error: `TAG` is NULL"
	);

	/* LEDC base config */

	ledc_timer_config_t ledc_tim_config = {
		.freq_hz = CONFIG_LEDC_PWM_FREQUENCY_HZ,
		.duty_resolution = (ledc_timer_bit_t) CONFIG_LEDC_PWM_BIT_RES,
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
		ledc_ch_config.speed_mode = pwm_get_port(i);
		ledc_ch_config.channel = pwm_get_channel(i);

		ESP_RETURN_ON_ERROR(
			ledc_channel_config(&ledc_ch_config),
			TAG,
			"Error on `ledc_channel_config(gpio_num=%u, speed_mode=%u, channel=%u)`",
			pwm_ch_to_gpio[i], pwm_get_port(i), pwm_get_channel(i)
		);
	}

	/* LEDC fade config */

	ledc_fade_func_install(0);

	return ESP_OK;
}

esp_err_t UART_setup(const char *TAG){

	ESP_RETURN_ON_FALSE(
		TAG != NULL,

		ESP_ERR_INVALID_ARG,
		"???",
		"Error: `TAG` is NULL"
	);

	uart_config_t uart_config = {
		.baud_rate = CONFIG_UART_BAUD_RATE,
		.data_bits = UART_DATA_8_BITS,
		.parity = UART_PARITY_DISABLE,
		.stop_bits = UART_STOP_BITS_1,
		.flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
		.rx_flow_ctrl_thresh = 122,
		.source_clk = UART_SCLK_DEFAULT,
	};

	ESP_RETURN_ON_ERROR(
		uart_driver_install(
			CONFIG_UART_PORT,
			UART_HW_FIFO_LEN(2) * 2,
			0,
			0,
			NULL,
			0
		),

		TAG,
		"Error on `uart_driver_install()`"
	);

	ESP_RETURN_ON_ERROR(
		uart_param_config(
			CONFIG_UART_PORT,
			&uart_config
		),

		TAG,
		"Error on `uart_param_config()`"
	);

	ESP_RETURN_ON_ERROR(
		uart_set_pin(
			CONFIG_UART_PORT,
			CONFIG_GPIO_UART_TX,
			CONFIG_GPIO_UART_RX,
			CONFIG_GPIO_UART_DE_RE,
			UART_PIN_NO_CHANGE
		),

		TAG,
		"Error on `uart_set_pin()`"
	);

	ESP_RETURN_ON_ERROR(
		uart_set_mode(
			CONFIG_UART_PORT,
			UART_MODE_RS485_HALF_DUPLEX
		),

		TAG,
		"Error on `uart_set_mode()`"
	);

	return ESP_OK;
}

esp_err_t ADC_setup(const char *TAG){

	ESP_RETURN_ON_FALSE(
		TAG != NULL,

		ESP_ERR_INVALID_ARG,
		"???",
		"Error: `TAG` is NULL"
	);

	extern adc_continuous_handle_t adc_handle;
	extern bool IRAM_ATTR adc_conversion_done(
		adc_continuous_handle_t adc_handle,
		const adc_continuous_evt_data_t *edata,
		void *user_data
	);

	/**
	 * Driver pre-initialization configurations.
	 * Please read the descriprion of `esp_adc/adc_continuous.h`.
	 */
	adc_continuous_handle_cfg_t adc_memory_config = {
		.max_store_buf_size = pm_samples_len_to_buf_size(CONFIG_ADC_SAMPLES),
		.conv_frame_size = pm_samples_len_to_buf_size(CONFIG_ADC_SAMPLES),
	};

	ESP_RETURN_ON_ERROR(
		adc_continuous_new_handle(&adc_memory_config, &adc_handle),
		TAG,
		"Error on `adc_continuous_new_handle()`"
	);

	// ADC GPIO pads.
	uint8_t adc_gpio[] = {
		CONFIG_GPIO_AC_V,
		CONFIG_GPIO_AC_I
	};

	// Corresponding ADC unit to `adc_gpio[i]` (must always be `ADC_UNIT_1`).
	adc_unit_t adc_unit;

	// Corresponding ADC channel to `adc_gpio[i]`.
	adc_channel_t adc_channel;

	// Configurations for every specified ADC channel.
	adc_digi_pattern_config_t adc_channel_config[sizeof(adc_gpio)];

	for(uint8_t i=0; i<sizeof(adc_gpio); i++){

		ESP_RETURN_ON_ERROR(
			adc_continuous_io_to_channel(
				adc_gpio[i],
				&adc_unit,
				&adc_channel
			),

			TAG,
			"Error on `adc_continuous_io_to_channel(io_num=%u)`",
			adc_gpio[i]
		);

		ESP_RETURN_ON_FALSE(
			adc_unit == ADC_UNIT_1,

			ESP_ERR_NOT_SUPPORTED,
			TAG,
			"Error: `adc_unit` is not `ADC_UNIT_1` for item #%u",
			i
		);

		// Channel configurations.
		adc_channel_config[i].channel = adc_channel;
		adc_channel_config[i].unit = ADC_UNIT_1;
		adc_channel_config[i].atten = ADC_ATTEN_DB_0;
		adc_channel_config[i].bit_width = ADC_BITWIDTH_12;
	}

	// Driver global configurations.
	adc_continuous_config_t adc_digital_config = {
		.sample_freq_hz = CONFIG_ADC_SAMPLE_RATE,
		.conv_mode = ADC_CONV_SINGLE_UNIT_1,
		.format = ADC_DIGI_OUTPUT_FORMAT_TYPE1,
		.pattern_num = sizeof(adc_gpio),
		.adc_pattern = adc_channel_config
	};

	ESP_RETURN_ON_ERROR(
		adc_continuous_config(
			adc_handle,
			&adc_digital_config
		),

		TAG,
		"Error on `adc_continuous_config()`"
	);

	adc_continuous_evt_cbs_t adc_callbacks = {
		.on_conv_done = adc_conversion_done
	};

	ESP_RETURN_ON_ERROR(
		adc_continuous_register_event_callbacks(
			adc_handle, &adc_callbacks, NULL
		),

		TAG,
		"Error on `adc_continuous_register_event_callbacks()`"
	);

	return ESP_OK;
}

esp_err_t QUEUES_setup(const char *TAG){

	ESP_RETURN_ON_FALSE(
		TAG != NULL,

		ESP_ERR_INVALID_ARG,
		"???",
		"Error: `TAG` is NULL"
	);

	/* pwm_task */

	extern QueueHandle_t pwm_queue;
	pwm_queue = xQueueCreate(10, sizeof(pwm_data_t));

	ESP_RETURN_ON_FALSE(
		pwm_queue != NULL,

		ESP_ERR_NO_MEM,
		TAG,
		"Error: unable to allocate the \"pwm_queue\""
	);

	return ESP_OK;
}

esp_err_t TASKS_setup(const char *TAG){

	ESP_RETURN_ON_FALSE(
		TAG != NULL,

		ESP_ERR_INVALID_ARG,
		"???",
		"Error: `TAG` is NULL"
	);

	extern TaskHandle_t
		rs485_task_handle,
		pwm_task_handle,
		pm_task_handle;

	TaskHandle_t *task_handles[] = {
		&rs485_task_handle,
		&pwm_task_handle,
		&pm_task_handle
	};

	extern void
		rs485_task(void *parameters),
		pwm_task(void *parameters),
		pm_task(void *parameters);

	TaskFunction_t task_routines[] = {
		rs485_task,
		pwm_task,
		pm_task
	};

	char task_names[][12] = {
		"rs485_task",
		"pwm_task",
		"pm_task"
	};

	uint8_t tasks = sizeof(task_handles) / sizeof(TaskHandle_t);
	BaseType_t ret_val;

	for(uint8_t i=0; i<tasks; i++){

		ret_val = xTaskCreatePinnedToCore(
			task_routines[i],
			task_names[i],
			4096,
			NULL,
			tskIDLE_PRIORITY,
			task_handles[i],
			ESP_APPLICATION_CORE
		);

		ESP_RETURN_ON_FALSE(
			ret_val == pdPASS,

			ESP_ERR_INVALID_STATE,
			TAG,
			"Error %d: unable to spawn the \"%s\"",
			ret_val, task_names[i]
		);
	}

	return ESP_OK;
}
