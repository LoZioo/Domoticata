/** @file ul_configs.h
 *  @brief  Created on: June 30, 2024
 *          Davide Scalisi
 *
 * 					Description:	DS framework configurations.
 *
 * @copyright [2024] Davide Scalisi *
 * @copyright All Rights Reserved. *
 *
*/

#ifndef INC_UL_CONFIGS_H_
#define INC_UL_CONFIGS_H_

/************************************************************************************************************
* ul_errors.h
************************************************************************************************************/

// #define UL_CONF_ERRORS_PRINT_DEBUG										// Comment to disable the text debug output.

/************************************************************************************************************
* ul_analog_button.h
************************************************************************************************************/

#define UL_CONF_ANALOG_BUTTON_MAX_EVENTS			10			// Max len of the `ul_analog_button_handler_t::id_to_value` array.
#define UL_CONF_ANALOG_BUTTON_VALID_INTERVAL	10			// The valid circular interval of the ADC read inside the `evaluate()`.

#endif  /* INC_UL_CONFIGS_H_ */
