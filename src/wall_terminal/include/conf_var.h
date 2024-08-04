/** @file conf_var.h
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

// RS-485 ID up to 127 or 0x7F.
#define CONFIG_UART_DEVICE_ID		0x05

/**
 * Averages from `analog_button_tuner.cpp`.
 * You can define up to 8 average values (up to 8 analog buttons).
 */
#define CONFIG_BTN_1_AVG	79
#define CONFIG_BTN_2_AVG	143

#endif  /* INC_CONF_VAR_H_ */
