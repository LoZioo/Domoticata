/** @file const.h
 *  @brief  Created on: July 28, 2024
 *          Davide Scalisi
 *
 * 					Description:	Domoticata wall terminal variable configurations.
 *
 * @copyright [2024] Davide Scalisi *
 * @copyright All Rights Reserved. *
 *
*/

#ifndef INC_CONF_VAR_H_
#define INC_CONF_VAR_H_

// UART
#define CONF_UART_DEVICE_ID		0x06		// RS-485 ID.

// Averages from `analog_button_tuner.cpp`.
#define CONF_BTN_1_AVG	79
#define CONF_BTN_2_AVG	143

#define VAL_BTN_1_LOWER_THR			(CONF_BTN_1_AVG - CONF_BTN_VALID_INTERVAL)
#define VAL_BTN_1_UPPER_THR			(CONF_BTN_1_AVG + CONF_BTN_VALID_INTERVAL)

#define VAL_BTN_2_LOWER_THR			(CONF_BTN_2_AVG - CONF_BTN_VALID_INTERVAL)
#define VAL_BTN_2_UPPER_THR			(CONF_BTN_2_AVG + CONF_BTN_VALID_INTERVAL)

#endif  /* INC_CONF_VAR_H_ */
