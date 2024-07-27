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

ul_err_t analog_button_begin(analog_button_init_t init, analog_button_handler_t **returned_handler){

	UL_RETURN_ON_FALSE(
		returned_handler != NULL,
		UL_ERR_INVALID_ARG,
		"Error: `returned_handler` is NULL"
	);

	/* Instance configurations */

	ul_err_t ret = UL_OK;

	analog_button_handler_t *self = (analog_button_handler_t*) malloc(sizeof(analog_button_handler_t));
	UL_GOTO_ON_FALSE(
		self != NULL,
		UL_ERR_NO_MEM,
		ul_pm_begin_err,
		"Error on `malloc(sizeof(analog_button_handler_t)`"
	);

	self->init = init;

	/* Parameters check */

	// UL_GOTO_ON_FALSE(
	// 	self->init.non_zero_value > 0,
	// 	UL_ERR_INVALID_ARG,
	// 	ul_pm_begin_free,
	// 	"Error: `self->init.non_zero_value` is 0"
	// );

	/* Init configurations */

	// self->a_value = 1;

	*returned_handler = self;
	return ret;

// ul_pm_begin_free:
// 	free(self);

ul_pm_begin_err:
	return ret;
}

void analog_button_end(analog_button_handler_t *self){
	free(self);
}
