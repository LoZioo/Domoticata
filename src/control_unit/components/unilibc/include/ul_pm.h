/** @file ul_pm.h
 *  @brief  Created on: June 30, 2024
 *          Davide Scalisi
 *
 * 					Description:	Library to convert AC voltage and current samples to power data.
 *
 * @copyright [2024] Davide Scalisi *
 * @copyright All Rights Reserved. *
 *
*/

#ifndef INC_UL_PM_H_
#define INC_UL_PM_H_

/************************************************************************************************************
* Included files
************************************************************************************************************/

// Standard libraries.
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

// UniLibC.
#include <ul_configs.h>
#include <ul_errors.h>

/************************************************************************************************************
* Public Defines
************************************************************************************************************/

/************************************************************************************************************
* Public Types Definitions
************************************************************************************************************/

#ifndef UL_CONFIG_PM_DOUBLE_BUFFER

typedef enum __attribute__((__packed__)) {

	UL_PM_SAMPLE_TYPE_VOLTAGE,
	UL_PM_SAMPLE_TYPE_CURRENT

} ul_pm_sample_type_t;

/**
 * @brief Callback to retrieve the samples needed for `ul_pm_evaluate()`.
 * @param sample_type `UL_PM_SAMPLE_TYPE_VOLTAGE` or `UL_PM_SAMPLE_TYPE_CURRENT`.
 * @param index Index of the sample wanted by the library.
 * @param context A generic user context to be passed to this callback by the library; leave it to `NULL` if unused.
 * @return The raw read sample.
 * @note Only the first `ul_pm_init_t::sample_resolution_bits` will be considered.
*/
typedef uint16_t (*ul_pm_sample_callback_t)(ul_pm_sample_type_t sample_type, uint32_t index, void *context);

#endif

// Evaluation results.
typedef struct __attribute__((__packed__)) {

	float v_pos_peak;
	float v_neg_peak;
	float v_pp;
	float v_rms;

	float i_pos_peak;
	float i_neg_peak;
	float i_pp;
	float i_rms;

	float p_va;
	float p_var;
	float p_w;
	float p_pf;

} ul_pm_results_t;

// Instance configurations.
typedef struct {

	// Relevant bits of the single sample.
	uint8_t sample_resolution_bits;

	// ADC max voltage.
	float adc_vcc_v;

	/**
	 * Transformer's gain (measured with the voltage divider attached to the secondary).
	 * `Vout / Vin`
	 */
	float v_transformer_gain;

	// Voltage divider upper resistor.
	float v_divider_r1_ohm;

	// Voltage divider lower resistor.
	float v_divider_r2_ohm;

	/**
	 * Current clamp gain.
	 * `Iout / Iin`
	 */
	float i_clamp_gain;

	// Current clamp resistor's value (Ohm).
	uint16_t i_clamp_resistor_ohm;

	// Set to 1 if unused.
	float v_correction_factor;

	// Set to 1 if unused.
	float i_correction_factor;

	#ifndef UL_CONFIG_PM_DOUBLE_BUFFER

	// Callback to retrieve the samples needed for `ul_pm_evaluate()`.
	ul_pm_sample_callback_t sample_callback;

	#endif

} ul_pm_init_t;

// Instance handler.
typedef struct {

	/* Instance configurations */

	ul_pm_init_t init;

	/* Instance state */

	// Formulas constants.
	float k_v, k_i;

} ul_pm_handler_t;

/************************************************************************************************************
* Public Variables Prototypes
************************************************************************************************************/

/************************************************************************************************************
* Public Functions Prototypes
************************************************************************************************************/

/**
 * @brief Create a new instance.
 */
extern ul_err_t ul_pm_begin(ul_pm_init_t init, ul_pm_handler_t **returned_handler);

/**
 * @brief Free the allocated resources.
*/
extern void ul_pm_end(ul_pm_handler_t *self);

#ifdef UL_CONFIG_PM_DOUBLE_BUFFER

/**
 * @brief Evaluate the provided data.
 * @param v_samples AC voltage samples.
 * @param i_samples AC current samples.
 * @param samples_len Number of samples acquired.
 * @param res Where to store the evaluated result.
 */
extern ul_err_t ul_pm_evaluate(ul_pm_handler_t *self, uint16_t *v_samples, uint16_t *i_samples, uint32_t samples_len, ul_pm_results_t *res);

#else

/**
 * @brief Evaluate the provided data.
 * @param samples_len Number of samples acquired.
 * @param sample_callback_context A generic user context to be passed to the `ul_pm_sample_callback_t`; leave it to `NULL` if unused.
 * @param res Where to store the evaluated result.
 */
extern ul_err_t ul_pm_evaluate(ul_pm_handler_t *self, void *sample_callback_context, uint32_t samples_len, ul_pm_results_t *res);

#endif

#endif  /* INC_UL_PM_H_ */
