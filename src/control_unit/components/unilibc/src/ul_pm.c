/** @file ul_pm.c
 *  @brief  Created on: June 30, 2024
 *          Davide Scalisi
 *
 * @copyright [2024] Davide Scalisi *
 * @copyright All Rights Reserved. *
 *
*/

/************************************************************************************************************
* Included files
************************************************************************************************************/

#include <ul_pm.h>
#include <ul_private.h>

/************************************************************************************************************
* Private Defines
************************************************************************************************************/

// Shims.
#ifdef UL_CONFIG_PM_DOUBLE_BUFFER
	#define v_samples_get(i)	v_samples[i]
	#define i_samples_get(i)	i_samples[i]
#else
	#define v_samples_get(i)	self->init.sample_callback(user_context, UL_PM_SAMPLE_TYPE_VOLTAGE, i)
	#define i_samples_get(i)	self->init.sample_callback(user_context, UL_PM_SAMPLE_TYPE_CURRENT, i)
#endif

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

ul_err_t ul_pm_begin(ul_pm_init_t *init, ul_pm_handle_t **returned_handle){
	assert_param_notnull(init);
	assert_param_notnull(returned_handle);

	/* Instance configurations */

	ul_err_t ret = UL_OK;
	ul_pm_handle_t *self = malloc(sizeof(ul_pm_handle_t));
	UL_RETURN_ON_FALSE(
		self != NULL,

		UL_ERR_NO_MEM,
		"Error on `malloc(size=%lu)`",
		sizeof(ul_pm_handle_t)
	);

	self->init = *init;

	/* Parameters check */

	UL_GOTO_ON_FALSE(
		init->adc_vcc_v > 0,

		UL_ERR_INVALID_ARG,
		label_error,
		"Error: `init->adc_vcc_v` is less or equal to 0"
	);

	UL_GOTO_ON_FALSE(
		init->adc_value_at_adc_vcc > 0,

		UL_ERR_INVALID_ARG,
		label_error,
		"Error: `init->adc_value_at_adc_vcc` is 0"
	);

	UL_GOTO_ON_FALSE(
		init->v_transformer_gain > 0,

		UL_ERR_INVALID_ARG,
		label_error,
		"Error: `init->v_transformer_gain` is less or equal to 0"
	);

	UL_GOTO_ON_FALSE(
		init->v_divider_r1_ohm > 0,

		UL_ERR_INVALID_ARG,
		label_error,
		"Error: `init->v_divider_r1_ohm` is less or equal to 0"
	);

	UL_GOTO_ON_FALSE(
		init->v_divider_r2_ohm > 0,

		UL_ERR_INVALID_ARG,
		label_error,
		"Error: `init->v_divider_r2_ohm` is less or equal to 0"
	);

	UL_GOTO_ON_FALSE(
		init->i_clamp_gain > 0,

		UL_ERR_INVALID_ARG,
		label_error,
		"Error: `init->i_clamp_gain` is less or equal to 0"
	);

	UL_GOTO_ON_FALSE(
		init->i_clamp_resistor_ohm > 0,

		UL_ERR_INVALID_ARG,
		label_error,
		"Error: `init->i_clamp_resistor_ohm` is less or equal to 0"
	);

	UL_GOTO_ON_FALSE(
		init->v_rms_threshold > 0,

		UL_ERR_INVALID_ARG,
		label_error,
		"Error: `init->v_rms_threshold` is less or equal to 0"
	);

	UL_GOTO_ON_FALSE(
		init->i_rms_threshold > 0,

		UL_ERR_INVALID_ARG,
		label_error,
		"Error: `init->i_rms_threshold` is less or equal to 0"
	);

	UL_GOTO_ON_FALSE(
		init->v_correction_factor > 0,

		UL_ERR_INVALID_ARG,
		label_error,
		"Error: `init->v_correction_factor` is less or equal to 0"
	);

	UL_GOTO_ON_FALSE(
		init->i_correction_factor > 0,

		UL_ERR_INVALID_ARG,
		label_error,
		"Error: `init->i_correction_factor` is less or equal to 0"
	);

	#ifndef UL_CONFIG_PM_DOUBLE_BUFFER

	UL_GOTO_ON_FALSE(
		init->sample_callback != NULL,

		UL_ERR_INVALID_ARG,
		label_error,
		"Error: `init->sample_callback` is NULL"
	);

	#endif

	/* Init configurations */

	float resolution = self->init.adc_vcc_v / self->init.adc_value_at_adc_vcc;
	self->k_v = self->init.v_correction_factor * resolution * (self->init.v_divider_r1_ohm + self->init.v_divider_r2_ohm) / (self->init.v_transformer_gain * self->init.v_divider_r2_ohm);
	self->k_i = self->init.i_correction_factor * resolution / (self->init.i_clamp_gain * self->init.i_clamp_resistor_ohm);

	*returned_handle = self;
	return ret;

	label_error:
	free(self);
	return ret;
}

void ul_pm_end(ul_pm_handle_t *self){
	free(self);
}

ul_err_t ul_pm_evaluate(
	ul_pm_handle_t *self,

	#ifdef UL_CONFIG_PM_DOUBLE_BUFFER
		uint16_t *v_samples,
		uint16_t *i_samples,
	#else
		void *user_context,
	#endif

	uint32_t samples_len,
	ul_pm_results_t *res
){

	assert_param_notnull(self);

	#ifdef UL_CONFIG_PM_DOUBLE_BUFFER
	assert_param_notnull(v_samples);
	assert_param_notnull(i_samples);
	#endif

	assert_param_size_ok(samples_len);
	assert_param_notnull(res);

	// Knuth running mean.
	float v_samples_avg = 0, i_samples_avg = 0, tmp;

	for(int i=0; i<samples_len; i++){

		// Saturation.
		tmp = v_samples_get(i);
		if(tmp > self->init.adc_value_at_adc_vcc)
			tmp = self->init.adc_value_at_adc_vcc;

		v_samples_avg += (tmp - v_samples_avg) / (i + 1);

		// Saturation.
		tmp = i_samples_get(i);
		if(tmp > self->init.adc_value_at_adc_vcc)
			tmp = self->init.adc_value_at_adc_vcc;

		i_samples_avg += (tmp - i_samples_avg) / (i + 1);
	}

	float v_quadratic_sum = 0, i_quadratic_sum = 0;
	float instant_power_sum = 0;
	float v_val, i_val;

	res->v_pos_peak = res->i_pos_peak = -0xFFFF;
	res->v_neg_peak = res->i_neg_peak = 0xFFFF;

	for(int i=0; i<samples_len; i++){

		// Remove the average from the sample and convert to AC voltage/current.
		v_val = ((float) v_samples_get(i) - v_samples_avg) * self->k_v;
		i_val = ((float) i_samples_get(i) - i_samples_avg) * self->k_i;

		// Begin computing the RMS.
		v_quadratic_sum += pow(v_val, 2);
		i_quadratic_sum += pow(i_val, 2);

		// Sum of all the instant powers.
		instant_power_sum += v_val * i_val;

		// Find the peaks.
		if(v_val > res->v_pos_peak)
			res->v_pos_peak = v_val;

		if(v_val < res->v_neg_peak)
			res->v_neg_peak = v_val;

		if(i_val > res->i_pos_peak)
			res->i_pos_peak = i_val;

		if(i_val < res->i_neg_peak)
			res->i_neg_peak = i_val;
	}

	res->v_pp = res->v_pos_peak - res->v_neg_peak;
	res->i_pp = res->i_pos_peak - res->i_neg_peak;

	res->v_rms = sqrt(v_quadratic_sum / samples_len);
	res->i_rms = sqrt(i_quadratic_sum / samples_len);

	if(
		res->v_rms < self->init.v_rms_threshold ||
		res->i_rms < self->init.i_rms_threshold
	){
		res->p_va = res->p_w = res->p_var = res->p_pf = 0;

		if(res->v_rms < self->init.v_rms_threshold)
			res->v_rms = 0;

		if(res->i_rms < self->init.i_rms_threshold)
			res->i_rms = 0;
	}

	else {
		res->p_va = res->v_rms * res->i_rms;
		res->p_w = instant_power_sum / samples_len;
		res->p_var = sqrt(pow(res->p_va, 2) - pow(res->p_w, 2));
		res->p_pf = res->p_w / res->p_va;
	}

	return UL_OK;
}
