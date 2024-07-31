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

/**
 * RS-485 ID up to 127 or 0x7F.
 * The MSb is used to distinguish between RX from the control unit and TX to the control unit.
 */
#define CONF_UART_DEVICE_ID		0x06

// Averages from `analog_button_tuner.cpp`.
#define CONF_BTN_1_AVG	79
#define CONF_BTN_2_AVG	143

#endif  /* INC_CONF_VAR_H_ */
