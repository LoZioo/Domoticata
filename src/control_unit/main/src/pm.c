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

/************************************************************************************************************
* Private Defines
************************************************************************************************************/

#define LOG_TAG	"pm"

// `adc_continuous_read()` timeout value (40ms = two 50Hz cycles).
#define ADC_CONTINUOUS_READ_TIMEOUT_MS	40

/**
 * @brief Convert the needed samples length for the "PowerMonitor" feature to the corresponding total byte length for the allocation of the `esp_adc/adc_continuous.h` driver's buffer.
 * @return `samples_len` * 2 channels * 2 bytes/sample.
 * @note The resulting number must be a multiple of `SOC_ADC_DIGI_DATA_BYTES_PER_CONV` on `soc/soc_caps.h`.
 */
#define __pm_samples_len_to_buf_size(samples_len)( \
	samples_len * 4 \
)

/************************************************************************************************************
* Private Types Definitions
 ************************************************************************************************************/

/************************************************************************************************************
* Private Variables
 ************************************************************************************************************/

static const char *TAG = LOG_TAG;
static TaskHandle_t __pm_task_handle;
static adc_continuous_handle_t __adc_handle;
static ul_pm_handler_t *__pm;

/************************************************************************************************************
* Private Functions Prototypes
 ************************************************************************************************************/

static esp_err_t __adc_driver_setup();
static esp_err_t __pm_code_setup();
static esp_err_t __pm_task_setup();

static void __pm_task(void *parameters);

static uint16_t __pm_get_sample(ul_pm_sample_type_t sample_type, uint32_t index, void *context);
static bool IRAM_ATTR __adc_conversion_done(adc_continuous_handle_t adc_handle, const adc_continuous_evt_data_t *edata, void *user_data);

/************************************************************************************************************
* Private Functions Definitions
 ************************************************************************************************************/

esp_err_t __adc_driver_setup(){

	/**
	 * Driver pre-initialization configurations.
	 * Please read the descriprion of `esp_adc/adc_continuous.h`.
	 */
	adc_continuous_handle_cfg_t adc_memory_config = {
		.max_store_buf_size = __pm_samples_len_to_buf_size(CONFIG_PM_ADC_SAMPLES),
		.conv_frame_size = __pm_samples_len_to_buf_size(CONFIG_PM_ADC_SAMPLES),
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

		.v_correction_factor = 1,
		.i_correction_factor = 1,

		.sample_callback = __pm_get_sample
	};

	ul_err_t ul_ret = ul_pm_begin(pm_init, &__pm);

	ESP_RETURN_ON_FALSE(
		ul_ret == UL_OK,

		ESP_ERR_INVALID_STATE,
		TAG,
		"Error %d on `ul_pm_begin()`",
		ul_ret
	);

	return ESP_OK;
}

esp_err_t __pm_task_setup(){

	BaseType_t ret_val = xTaskCreatePinnedToCore(
		__pm_task,
		LOG_TAG"_task",
		CONFIG_PM_TASK_STACK_SIZE_BYTES,
		NULL,
		CONFIG_PM_TASK_PRIORITY,
		&__pm_task_handle,

		#ifdef CONFIG_PM_TASK_CORE_AFFINITY_APPLICATION
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

void __pm_task(void *parameters){

	ESP_LOGI(TAG, "Started");

	/* Variables */

	// `ESP_GOTO_ON_ERROR()` return code.
	esp_err_t ret;

	// UniLibC return code.
	ul_err_t ul_ret;

	// Timings.
	int64_t t0;

	/**
	 * Must be multiple of 4.
	 * Note: `sizeof(adc_digi_output_data_t)` = `sizeof(uint16_t)`
	 */
	static adc_digi_output_data_t samples[
		__pm_samples_len_to_buf_size(
			CONFIG_PM_ADC_SAMPLES
		)
	];

	uint32_t read_size;

	// `ul_pm_evaluate()` results.
	ul_pm_results_t pm_res;

	/* Code */

	ESP_LOGI(TAG, "Sampling from ADC");

	/* Infinite loop */
	for(;;){
		t0 = millis();
		ret = ESP_OK;

		// Flush old samples.
		ESP_GOTO_ON_ERROR(
			adc_continuous_flush_pool(__adc_handle),

			task_error,
			TAG,
			"Error on `adc_continuous_flush_pool()`"
		);

		// Start the sample acquisition.
		ESP_GOTO_ON_ERROR(
			adc_continuous_start(__adc_handle),

			task_error,
			TAG,
			"Error on `adc_continuous_start()`"
		);

		// Wait for the ISR and then clear the notification (`pdTRUE`).
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

		// Read the acquired samples from the driver.
		ESP_GOTO_ON_ERROR(
			adc_continuous_read(
				__adc_handle,
				(uint8_t*) samples,
				__pm_samples_len_to_buf_size(CONFIG_PM_ADC_SAMPLES),
				&read_size,
				ADC_CONTINUOUS_READ_TIMEOUT_MS
			),

			task_error,
			TAG,
			"Error on `adc_continuous_read()`"
		);

		// Stop the sample acquisition.
		ESP_GOTO_ON_ERROR(
			adc_continuous_stop(__adc_handle),

			task_error,
			TAG,
			"Error on `adc_continuous_stop()`"
		);

		// Conversion.
		ul_ret = ul_pm_evaluate(
			__pm,
			samples,
			CONFIG_PM_ADC_SAMPLES,
			&pm_res
		);

		ESP_GOTO_ON_FALSE(
			ul_ret == UL_OK,

			ESP_ERR_INVALID_STATE,
			task_error,
			TAG,
			"Error %d on `ul_pm_evaluate()`",
			ul_ret
		);

		// !!! DEBUG
		printf("samples: { ");
		for(uint32_t i=0; i<16; i++){

			printf("(%d, %d)",
				samples[i].type1.channel,
				samples[i].type1.data
			);

			if(i < 15)
				printf(", ");
		}
		printf(" }\n");
		printf("read_size: %lu\n", read_size);
		printf("samples_size: %u\n", __pm_samples_len_to_buf_size(CONFIG_PM_ADC_SAMPLES));
		printf("\n");
		// !!! DEBUG

		// !!! DEBUG
		printf("Voltage:\n");
		printf("  V_pos_peak: %.2f\n", pm_res.v_pos_peak);
		printf("  V_neg_peak: %.2f\n", pm_res.v_neg_peak);
		printf("  V_pp: %.2f\n", pm_res.v_pp);
		printf("  V_rms: %.2f\n\n", pm_res.v_rms);

		// printf("Current:\n");
		// printf("  I_pos_peak: %.2f\n", pm_res.i_pos_peak);
		// printf("  I_neg_peak: %.2f\n", pm_res.i_neg_peak);
		// printf("  I_pp: %.2f\n", pm_res.i_pp);
		// printf("  I_rms: %.2f\n\n", pm_res.i_rms);

		// printf("Power:\n");
		// printf("  P_va: %.2f\n", pm_res.p_va);
		// printf("  P_var: %.2f\n", pm_res.p_var);
		// printf("  P_w: %.2f\n", pm_res.p_w);
		// printf("  P_pf: %.2f\n\n", pm_res.p_pf);
		// !!! DEBUG

		// Delay before continuing.
		goto task_continue;

		task_error:
		ESP_ERROR_CHECK_WITHOUT_ABORT(ret);

		task_continue:
		delay_remainings(1000, t0);
	}
}

uint16_t __pm_get_sample(ul_pm_sample_type_t sample_type, uint32_t index, void *context){

	// !!! SISTEMARE FREQUENZA CAMPIONAMENTO E NUMERO CAMPIONI PER INCLUDERE PIU' DI UN CICLO DI 50Hz
	// !!! SISTEMARE RECUPERO CAMPIONI: I CANALI NON SONO MESSI NECESSARIAMENTE SEMPRE NELLO STESSO ORDINE
	index *= 2;
	if(sample_type == UL_PM_SAMPLE_TYPE_VOLTAGE)
		index++;

	return ((adc_digi_output_data_t*) context)[index].type1.data;
}

bool __adc_conversion_done(adc_continuous_handle_t adc_handle, const adc_continuous_evt_data_t *edata, void *user_data){

	BaseType_t must_yield = pdFALSE;
	vTaskNotifyGiveFromISR(__pm_task_handle, &must_yield);

	return (must_yield == pdTRUE);
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
