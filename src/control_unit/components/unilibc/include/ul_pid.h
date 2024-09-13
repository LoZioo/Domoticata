/** @file ul_pid.h
 *  @brief  Created on: Sep 13, 2024
 *          Davide Scalisi
 *
 * 					Description:	Simple PID implementation.
 *
 * @copyright [2024] Davide Scalisi *
 * @copyright All Rights Reserved. *
 *
*/

#ifndef INC_UL_PID_H_
#define INC_UL_PID_H_

/************************************************************************************************************
* Included files
************************************************************************************************************/

// Standard libraries.
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

// UniLibC libraries.
#include <ul_configs.h>
#include <ul_errors.h>

/************************************************************************************************************
* Public Defines
************************************************************************************************************/

// !!! COPIARE MACRO

/************************************************************************************************************
* Public Types Definitions
************************************************************************************************************/

// Instance configurations.
typedef struct {

	/**
	 * Sampling period (`PID_evaluate()` calling period).
	 * @note Measured in seconds!
	*/
	float dt;

	// PID coefficients.
	float kp, ki, kd;

	// Saturation threshold.
	float sat;

	// Abilitation of the anti windup optimization.
	bool anti_windup;

} ul_pid_init_t;

// Instance handle.
typedef struct {

	/* Instance configurations */

	ul_pid_init_t init;

	/* Instance state */

	// Integral accumulator.
	float integral;

	// Needed for calculating the derivative contribution.
	float prev_err;

	// Saturation flag.
	bool in_saturation;

	// Enable flag: if it's false, `PID_evaluate()` does nothing.
	bool enabled;

} ul_pid_handle_t;

/************************************************************************************************************
* Public Variables Prototypes
************************************************************************************************************/

/************************************************************************************************************
* Public Functions Prototypes
************************************************************************************************************/

/**
 * @brief Create a new instance.
 */
extern ul_err_t ul_pid_begin(ul_pid_init_t *init, ul_pid_handle_t **returned_handle);

/**
 * @brief Free the allocated resources.
*/
extern void ul_pid_end(ul_pid_handle_t *self);

/**
 * @brief Reset the PID.
*/
extern ul_err_t ul_pid_reset(ul_pid_handle_t *self);

/**
 * @brief Enable PID evaluation.
 * @note Unfreeze the PID.
*/
extern ul_err_t ul_pid_enable(ul_pid_handle_t *self);

/**
 * @brief Disable PID evaluation.
 * @note Freeze the PID.
*/
extern ul_err_t ul_pid_disable(ul_pid_handle_t *self);

/**
 * @brief Must be called 1/dt times per second.
 * @param err The current error.
 * @param res Pointer to where to store the evaluation result.
*/
extern ul_err_t ul_pid_evaluate(ul_pid_handle_t *self, float err, float *res);

#endif  /* INC_UL_PID_H_ */
