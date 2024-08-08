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

/************************************************************************************************************
* Private Defines
************************************************************************************************************/

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

ul_err_t ul_pm_begin(ul_pm_init_t init, ul_pm_handler_t **returned_handler){

	UL_RETURN_ON_FALSE(
		returned_handler != NULL,
		UL_ERR_INVALID_ARG,
		"Error: `returned_handler` is NULL"
	);

	/* Instance configurations */

	ul_err_t ret = UL_OK;

	ul_pm_handler_t *self = (ul_pm_handler_t*) malloc(sizeof(ul_pm_handler_t));
	UL_GOTO_ON_FALSE(
		self != NULL,
		UL_ERR_NO_MEM,
		ul_pm_begin_err,
		"Error on `malloc(sizeof(ul_pm_handler_t))`"
	);

	self->init = init;

	/* Parameters check */

	UL_GOTO_ON_FALSE(
		self->init.sample_resolution_bits > 0,
		UL_ERR_INVALID_ARG,
		ul_pm_begin_free,
		"Error: `self->init.sample_resolution_bits` is 0"
	);

	UL_GOTO_ON_FALSE(
		self->init.adc_vcc_v > 0,
		UL_ERR_INVALID_ARG,
		ul_pm_begin_free,
		"Error: `self->init.adc_vcc_v` is less or equal to 0"
	);

	UL_GOTO_ON_FALSE(
		self->init.v_transformer_gain > 0,
		UL_ERR_INVALID_ARG,
		ul_pm_begin_free,
		"Error: `self->init.v_transformer_gain` is less or equal to 0"
	);

	UL_GOTO_ON_FALSE(
		self->init.v_divider_r1_ohm > 0,
		UL_ERR_INVALID_ARG,
		ul_pm_begin_free,
		"Error: `self->init.v_divider_r1_ohm` is less or equal to 0"
	);

	UL_GOTO_ON_FALSE(
		self->init.v_divider_r2_ohm > 0,
		UL_ERR_INVALID_ARG,
		ul_pm_begin_free,
		"Error: `self->init.v_divider_r2_ohm` is less or equal to 0"
	);

	UL_GOTO_ON_FALSE(
		self->init.i_clamp_gain > 0,
		UL_ERR_INVALID_ARG,
		ul_pm_begin_free,
		"Error: `init.i_clamp_gain` is less or equal to 0"
	);

	UL_GOTO_ON_FALSE(
		self->init.i_clamp_resistor_ohm > 0,
		UL_ERR_INVALID_ARG,
		ul_pm_begin_free,
		"Error: `init.i_clamp_resistor_ohm` is 0"
	);

	/* Init configurations */

	float k = self->init.adc_vcc_v / (1 << self->init.sample_resolution_bits);
	self->k_v = self->init.v_correction_factor * k * (self->init.v_divider_r1_ohm + self->init.v_divider_r2_ohm) / (self->init.v_transformer_gain * self->init.v_divider_r2_ohm);
	self->k_i = self->init.i_correction_factor * k / (self->init.i_clamp_gain * self->init.i_clamp_resistor_ohm);

	*returned_handler = self;
	return ret;

ul_pm_begin_free:
	free(self);

ul_pm_begin_err:
	return ret;
}

void ul_pm_end(ul_pm_handler_t *self){
	free(self);
}

ul_err_t ul_pm_evaluate(ul_pm_handler_t *self, uint16_t *v_samples, uint16_t *i_samples, uint32_t len, ul_pm_results_t *res){
	UL_RETURN_ON_FALSE(self != NULL, UL_ERR_INVALID_ARG, "Error: `self` is NULL");
	UL_RETURN_ON_FALSE(v_samples != NULL, UL_ERR_INVALID_ARG, "Error: `v_samples` is NULL");
	UL_RETURN_ON_FALSE(i_samples != NULL, UL_ERR_INVALID_ARG, "Error: `i_samples` is NULL");
	UL_RETURN_ON_FALSE(len > 0, UL_ERR_INVALID_ARG, "Error: `len` is 0");
	UL_RETURN_ON_FALSE(res != NULL, UL_ERR_INVALID_ARG, "Error: `res` is NULL");

	// Knuth running mean.
	float v_samples_avg = 0, i_samples_avg = 0;

	for(int i=0; i<len; i++){
		v_samples_avg += (v_samples[i] - v_samples_avg) / (i + 1);
		i_samples_avg += (i_samples[i] - i_samples_avg) / (i + 1);
	}

	float v_quadratic_sum = 0, i_quadratic_sum = 0;
	float instant_power_sum = 0;

	res->v_pos_peak = res->i_pos_peak = -0xFFFF;
	res->v_neg_peak = res->i_neg_peak = 0xFFFF;

	for(int i=0; i<len; i++){

		// Remove the average from the sample and convert to AC voltage/current.
		float v_val = ((float) v_samples[i] - v_samples_avg) * self->k_v;
		float i_val = ((float) i_samples[i] - i_samples_avg) * self->k_i;

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

	res->v_rms = sqrt(v_quadratic_sum / len);
	res->i_rms = sqrt(i_quadratic_sum / len);

	res->p_va = res->v_rms * res->i_rms;
	res->p_w = instant_power_sum / len;
	res->p_var = sqrt(pow(res->p_va, 2) - pow(res->p_w, 2));
	res->p_pf = res->p_w / res->p_va;

	return UL_OK;
}
