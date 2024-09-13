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

#include <ul_pid.h>
#include <ul_private.h>

/************************************************************************************************************
* Private Defines
************************************************************************************************************/

/**
 * @brief Reset the PID.
*/
#define __reset_instance(self) \
	self->integral = self->prev_err = 0; \
	self->in_saturation = false; \
	self->enabled = false

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
		init->dt > 0,

		UL_ERR_INVALID_ARG,
		label_error,
		"Error: `init->dt` is less or equal to 0"
	);

	/* Init configurations */

	__reset_instance(self);

	*returned_handle = self;
	return ret;

	label_error:
	free(self);
	return ret;
}

void ul_pid_end(ul_pid_handle_t *self){
	free(self);
}

ul_err_t ul_pid_reset(ul_pid_handle_t *self){
	assert_param_notnull(self);
	__reset_instance(self);
	return UL_OK;
}

ul_err_t ul_pid_enable(ul_pid_handle_t *self){
	assert_param_notnull(self);
	self->enabled = true;
	return UL_OK;
}

ul_err_t ul_pid_disable(ul_pid_handle_t *self){
	assert_param_notnull(self);
	self->enabled = false;
	return UL_OK;
}

// !!! DA QUI'

ul_err_t ul_pid_evaluate(ul_pid_handle_t *self, float err, float *res){
	return UL_OK;
}
