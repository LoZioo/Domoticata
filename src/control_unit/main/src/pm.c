/** @file pm.c
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

#include <pm.h>
#include <private.h>

/************************************************************************************************************
* Private Defines
************************************************************************************************************/

#define LOG_TAG	"pm"
// #define LOG_RESULTS

// Voltage and current channels.
#define ADC_CHANNELS	2
#define ADC_BYTES_PER_SAMPLE	sizeof(adc_digi_output_data_t)

// This number must be a multiple of `SOC_ADC_DIGI_DATA_BYTES_PER_CONV` on `soc/soc_caps.h`.
#define ADC_BUF_SIZE_BYTES	( \
	ADC_CHANNELS * ADC_BYTES_PER_SAMPLE * CONFIG_PM_ADC_SAMPLES \
)

/**
 * `adc_continuous_read()` timeout value (e.g. 200ms = ten 50Hz cycles).
 * Period of time in which the read data becomes obsolete.
 */
#define ADC_CONTINUOUS_READ_TIMEOUT_MS	( \
	(CONFIG_PM_ADC_SAMPLES * 1000) / CONFIG_PM_ADC_SAMPLE_RATE \
)

/**
 * @brief Statement to check if the library was initialized.
 */
#define __is_initialized()( \
	__pm_task_handle != NULL \
)

/************************************************************************************************************
* Private Types Definitions
 ************************************************************************************************************/

/************************************************************************************************************
* Private Variables
 ************************************************************************************************************/

static const char *TAG = LOG_TAG;

static TaskHandle_t __pm_task_handle = NULL;
static adc_continuous_handle_t __adc_handle;
static ul_pm_handle_t *__pm_handle;

// Voltage and current channels.
static struct __attribute__((__packed__)) {

	adc_channel_t v_channel: 4;
	adc_channel_t i_channel: 4;

} adc_channels;

// `ul_pm_evaluate()` results.
static ul_pm_results_t __pm_res;
static SemaphoreHandle_t __pm_res_mutex;

// Sample buffer.
static uint8_t __buffer[ADC_BUF_SIZE_BYTES];
static uint8_t __v_sample_offset, __i_sample_offset;

/************************************************************************************************************
* Private Functions Prototypes
 ************************************************************************************************************/

static esp_err_t __adc_driver_setup();
static esp_err_t __pm_code_setup();
static esp_err_t __pm_task_setup();

static uint16_t __pm_get_sample(void *user_context, ul_pm_sample_type_t sample_type, uint32_t index);
static bool __adc_conversion_done(adc_continuous_handle_t adc_handle, const adc_continuous_evt_data_t *edata, void *user_data);

static void __pm_task(void *parameters);

/************************************************************************************************************
* Private Functions Definitions
 ************************************************************************************************************/

esp_err_t __adc_driver_setup(){

	/**
	 * Driver pre-initialization configurations.
	 * Please read the descriprion of `esp_adc/adc_continuous.h`.
	 */
	adc_continuous_handle_cfg_t adc_memory_config = {
		.max_store_buf_size = ADC_BUF_SIZE_BYTES,
		.conv_frame_size = ADC_BUF_SIZE_BYTES,
		.flags.flush_pool = false
	};

	ESP_RETURN_ON_ERROR(
		adc_continuous_new_handle(&adc_memory_config, &__adc_handle),
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

		// Save channel number.
		switch(i){

			// Voltage channel.
			case 0:
				adc_channels.v_channel = adc_channel;
				break;

			// Current channel.
			case 1:
				adc_channels.i_channel = adc_channel;
				break;

			default:
				break;
		}

		// Channel configurations.
		adc_channel_config[i].channel = adc_channel;
		adc_channel_config[i].unit = ADC_UNIT_1;
		adc_channel_config[i].atten = ADC_ATTEN_DB_12;
		adc_channel_config[i].bit_width = ADC_BITWIDTH_12;
	}

	// Driver global configurations.
	adc_continuous_config_t adc_digital_config = {
		.sample_freq_hz = CONFIG_PM_ADC_SAMPLE_RATE,
		.conv_mode = ADC_CONV_SINGLE_UNIT_1,
		.format = ADC_DIGI_OUTPUT_FORMAT_TYPE1,
		.pattern_num = sizeof(adc_gpio),
		.adc_pattern = adc_channel_config
	};

	ESP_RETURN_ON_ERROR(
		adc_continuous_config(
			__adc_handle,
			&adc_digital_config
		),

		TAG,
		"Error on `adc_continuous_config()`"
	);

	adc_continuous_evt_cbs_t adc_callbacks = {
		.on_conv_done = __adc_conversion_done
	};

	ESP_RETURN_ON_ERROR(
		adc_continuous_register_event_callbacks(
			__adc_handle, &adc_callbacks, NULL
		),

		TAG,
		"Error on `adc_continuous_register_event_callbacks()`"
	);

	return ESP_OK;
}

esp_err_t __pm_code_setup(){

	ul_pm_init_t pm_init = {
		.adc_vcc_v = 3,
		.adc_value_at_adc_vcc = 3840,

		.v_transformer_gain = 0.06136,		// 232.9V : 14.29V
		.v_divider_r1_ohm = 10000,
		.v_divider_r2_ohm = 680,

		.i_clamp_gain = 0.0005,						// 100A : 50mA
		.i_clamp_resistor_ohm = 120,

		.v_rms_threshold = 10,
		.i_rms_threshold = 0.05,

		.v_correction_factor = 1,
		.i_correction_factor = 1,

		.sample_callback = __pm_get_sample
	};

	ESP_RETURN_ON_ERROR(
		ul_errors_to_esp_err(
			ul_pm_begin(
				&pm_init,
				&__pm_handle
			)
		),

		TAG,
		"Error on `ul_pm_begin()`"
	);

	return ESP_OK;
}

esp_err_t __pm_task_setup(){

	__pm_res_mutex = xSemaphoreCreateMutex();
	ESP_RETURN_ON_FALSE(
		__pm_res_mutex != NULL,

		ESP_ERR_NO_MEM,
		TAG,
		"Error: unable to allocate `__pm_res_mutex`"
	);

	BaseType_t ret_val = xTaskCreatePinnedToCore(
		__pm_task,
		LOG_TAG "_task",
		CONFIG_PM_TASK_STACK_SIZE_BYTES,
		NULL,
		CONFIG_PM_TASK_PRIORITY,
		&__pm_task_handle,
		CONFIG_PM_TASK_CORE_AFFINITY
	);

	ESP_RETURN_ON_FALSE(
		ret_val == pdPASS,

		ESP_ERR_INVALID_STATE,
		TAG,
		"Error %d: unable to spawn \"" LOG_TAG "_task\"",
		ret_val
	);

	return ESP_OK;
}

uint16_t __pm_get_sample(void *user_context, ul_pm_sample_type_t sample_type, uint32_t index){

	index = (index * 2) + (
		sample_type == UL_PM_SAMPLE_TYPE_VOLTAGE ?
		__v_sample_offset :
		__i_sample_offset
	);

	return ((adc_digi_output_data_t*) __buffer)[index].type1.data;
}

bool __adc_conversion_done(adc_continuous_handle_t adc_handle, const adc_continuous_evt_data_t *edata, void *user_data){

	BaseType_t must_yield = pdFALSE;
	vTaskNotifyGiveFromISR(__pm_task_handle, &must_yield);

	return (must_yield == pdTRUE);
}

void __pm_task(void *parameters){

	ESP_LOGI(TAG, "Started");

	/* Variables */

	// `ESP_GOTO_ON_ERROR()` return code.
	esp_err_t ret __attribute__((unused));

	// Sample buffer read length.
	uint32_t read_len;

	/* Code */

	ESP_LOGI(TAG, "Sampling from ADC");

	/* Infinite loop */
	for(;;){
		ret = ESP_OK;

		// Flush old samples.
		ESP_GOTO_ON_ERROR(
			adc_continuous_flush_pool(__adc_handle),

			task_continue,
			TAG,
			"Error on `adc_continuous_flush_pool()`"
		);

		// Start the sample acquisition.
		ESP_GOTO_ON_ERROR(
			adc_continuous_start(__adc_handle),

			task_continue,
			TAG,
			"Error on `adc_continuous_start()`"
		);

		// Wait for the ISR and then clear the notification (`pdTRUE`).
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

		// Read the acquired samples from the driver.
		ESP_GOTO_ON_ERROR(
			adc_continuous_read(
				__adc_handle,
				__buffer,
				ADC_BUF_SIZE_BYTES,
				&read_len,
				ADC_CONTINUOUS_READ_TIMEOUT_MS
			),

			task_continue,
			TAG,
			"Error on `adc_continuous_read()`"
		);

		// Stop the sample acquisition.
		ESP_GOTO_ON_ERROR(
			adc_continuous_stop(__adc_handle),

			task_continue,
			TAG,
			"Error on `adc_continuous_stop()`"
		);

		// Sample order.
		if(((adc_digi_output_data_t*) __buffer)[0].type1.channel == adc_channels.v_channel){
			__v_sample_offset = 0;
			__i_sample_offset = 1;
		}

		else {
			__v_sample_offset = 1;
			__i_sample_offset = 0;
		}

		if(xSemaphoreTake(__pm_res_mutex, pdMS_TO_TICKS(ADC_CONTINUOUS_READ_TIMEOUT_MS)) == pdFALSE)
			continue;

		ESP_GOTO_ON_ERROR(
			ul_errors_to_esp_err(
				ul_pm_evaluate(
					__pm_handle,
					NULL,
					read_len / (ADC_CHANNELS * ADC_BYTES_PER_SAMPLE),
					&__pm_res
				)
			),

			task_continue,
			TAG,
			"Error on `ul_pm_evaluate()`"
		);

		xSemaphoreGive(__pm_res_mutex);

		#ifdef LOG_RESULTS
		ESP_LOGW(TAG, "LOG_RESULTS");

		ESP_LOGI(TAG, "Voltage:");
		ESP_LOGI(TAG, "  V_pos_peak: %.2f", __pm_res.v_pos_peak);
		ESP_LOGI(TAG, "  V_neg_peak: %.2f", __pm_res.v_neg_peak);
		ESP_LOGI(TAG, "  V_pp: %.2f", __pm_res.v_pp);
		ESP_LOGI(TAG, "  V_rms: %.2f", __pm_res.v_rms);

		ESP_LOGI(TAG, "Current:");
		ESP_LOGI(TAG, "  I_pos_peak: %.2f", __pm_res.i_pos_peak);
		ESP_LOGI(TAG, "  I_neg_peak: %.2f", __pm_res.i_neg_peak);
		ESP_LOGI(TAG, "  I_pp: %.2f", __pm_res.i_pp);
		ESP_LOGI(TAG, "  I_rms (mA): %.2f", __pm_res.i_rms * 1000);

		ESP_LOGI(TAG, "Power:");
		ESP_LOGI(TAG, "  P_va: %.2f", __pm_res.p_va);
		ESP_LOGI(TAG, "  P_w: %.2f", __pm_res.p_w);
		ESP_LOGI(TAG, "  P_var: %.2f", __pm_res.p_var);
		ESP_LOGI(TAG, "  P_pf: %.2f", __pm_res.p_pf);

		delay(1000);
		#endif

		task_continue:
	}
}

/************************************************************************************************************
* Public Functions Definitions
 ************************************************************************************************************/

esp_err_t pm_setup(){

	ESP_RETURN_ON_ERROR(
		__adc_driver_setup(),

		TAG,
		"Error on `__adc_driver_setup()`"
	);

	ESP_RETURN_ON_ERROR(
		__pm_code_setup(),

		TAG,
		"Error on `__pm_code_setup()`"
	);

	ESP_RETURN_ON_ERROR(
		__pm_task_setup(),

		TAG,
		"Error on `__pm_task_setup()`"
	);

	return ESP_OK;
}

esp_err_t pm_get_results(ul_pm_results_t *ul_pm_results){
	assert_param_notnull(ul_pm_results);

	ESP_RETURN_ON_FALSE(
		__is_initialized(),

		ESP_ERR_INVALID_STATE,
		TAG,
		"Error: library not initialized"
	);

	ESP_RETURN_ON_FALSE(
		xSemaphoreTake(
			__pm_res_mutex,
			pdMS_TO_TICKS(ADC_CONTINUOUS_READ_TIMEOUT_MS)
		) == pdTRUE,

		ESP_ERR_TIMEOUT,
		TAG,
		"Error: unable to take `__pm_res_mutex`"
	);

	*ul_pm_results = __pm_res;
	xSemaphoreGive(__pm_res_mutex);

	return ESP_OK;
}
