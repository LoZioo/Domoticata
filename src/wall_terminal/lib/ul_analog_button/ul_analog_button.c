/** @file ul_analog_button.c
 *  @brief  Created on: July 27, 2024
 *          Davide Scalisi
 *
 * @copyright [2024] Davide Scalisi *
 * @copyright All Rights Reserved. *
 *
*/

/************************************************************************************************************
* Included files
************************************************************************************************************/

#include <ul_analog_button.h>

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

ul_err_t ul_analog_button_begin(ul_analog_button_init_t init, ul_analog_button_handler_t **returned_handler){

	UL_RETURN_ON_FALSE(
		returned_handler != NULL,
		UL_ERR_INVALID_ARG,
		"Error: `returned_handler` is NULL"
	);

	/* Instance configurations */

	ul_err_t ret = UL_OK;

	ul_analog_button_handler_t *self = (ul_analog_button_handler_t*) malloc(sizeof(ul_analog_button_handler_t));
	UL_GOTO_ON_FALSE(
		self != NULL,
		UL_ERR_NO_MEM,
		ul_pm_begin_err,
		"Error on `malloc(sizeof(ul_analog_button_handler_t)`"
	);

	self->init = init;

	/* Parameters check */

	UL_GOTO_ON_FALSE(
		ul_utils_between(
			self->init.adc_res_bits,
			8, 16
		),
		UL_ERR_INVALID_ARG,
		ul_pm_begin_free,
		"Error: `self->init.adc_res_bits` must be between 8 and 16"
	);

	UL_GOTO_ON_FALSE(
		self->init.adc_callback != NULL,
		UL_ERR_INVALID_ARG,
		ul_pm_begin_free,
		"Error: `self->init.adc_callback` is NULL"
	);

	UL_GOTO_ON_FALSE(
		self->init.event_callback != NULL,
		UL_ERR_INVALID_ARG,
		ul_pm_begin_free,
		"Error: `self->init.event_callback` is NULL"
	);

	/* Init configurations */

	self->adc_last_val = (1 << self->init.adc_res_bits) - 1;

	*returned_handler = self;
	return ret;

ul_pm_begin_free:
	free(self);

ul_pm_begin_err:
	return ret;
}

void ul_analog_button_end(ul_analog_button_handler_t *self){
	free(self);
}
