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
#include <ul_private.h>
#include <ul_configs.h>
#include <ul_errors.h>

/************************************************************************************************************
* Public Defines
************************************************************************************************************/

/************************************************************************************************************
* Public Types Definitions
************************************************************************************************************/

/**
 * @brief A callback.
 * @param parameter A parameter.
*/
typedef void (*ul_pid_callback_t)(uint8_t parameter);

// Instance configurations.
typedef struct {

	/* GPIO */

	/* Variables and pointers */

	uint8_t non_zero_value;

	/* Callbacks */

} ul_pid_init_t;

// Instance handle.
typedef struct {

	/* Instance configurations */

	ul_pid_init_t init;

	/* Instance state */

	uint8_t a_value;

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

#endif  /* INC_UL_PID_H_ */
