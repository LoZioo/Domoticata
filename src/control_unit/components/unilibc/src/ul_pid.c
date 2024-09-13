/** @file ul_pid.c
 *  @brief  Created on: Sep 13, 2024
 *          Davide Scalisi
 *
 * @copyright [2024] Davide Scalisi *
 * @copyright All Rights Reserved. *
 *
*/

/************************************************************************************************************
* Included files
************************************************************************************************************/

#include <lib.h>

/************************************************************************************************************
* Private Defines
************************************************************************************************************/

/************************************************************************************************************
* Private Types Definitions
 ************************************************************************************************************/

/************************************************************************************************************
* Private Variables
 ************************************************************************************************************/

// A private variable.
static uint8_t *__ptr_to_something = NULL;

/************************************************************************************************************
* Private Functions Prototypes
 ************************************************************************************************************/

/**
 * @brief A private function.
*/
static void __private_func(ul_pid_handle_t *self);

/************************************************************************************************************
* Private Functions Definitions
 ************************************************************************************************************/

void __private_func(ul_pid_handle_t *self){
}

/************************************************************************************************************
* Public Functions Definitions
 ************************************************************************************************************/

ul_err_t ul_pid_begin(ul_pid_init_t *init, ul_pid_handle_t **returned_handle){
	assert_param_notnull(init);
	assert_param_notnull(returned_handle);

	/* Instance configurations */

	ul_err_t ret = UL_OK;
	ul_pid_handle_t *self = malloc(sizeof(ul_pid_handle_t));
	UL_RETURN_ON_FALSE(
		self != NULL,

		UL_ERR_NO_MEM,
		"Error on `malloc(size=%lu)`",
		sizeof(ul_pid_handle_t)
	);

	self->init = *init;

	/* Parameters check */

	UL_GOTO_ON_FALSE(
		init->non_zero_value > 0,

		UL_ERR_INVALID_ARG,
		label_error,
		"Error: `init->non_zero_value` is 0"
	);

	/* Init configurations */

	self->a_value = 1;

	*returned_handle = self;
	return ret;

	label_error:
	free(self);
	return ret;
}

void ul_pid_end(ul_pid_handle_t *self){
	free(self);
}
