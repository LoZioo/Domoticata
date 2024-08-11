/** @file conf_const.h
 *  @brief  Created on: July 28, 2024
 *          Davide Scalisi
 *
 * 					Description:	Domoticata wall terminal constant configurations.
 *
 * @copyright [2024] Davide Scalisi *
 * @copyright All Rights Reserved. *
 *
*/

#include <conf_var.h>

#ifndef INC_CONF_CONST_H_
#define INC_CONF_CONST_H_

// Misc
#define ADC_MAX		((1 << 10) - 1)

// GPIO
#define CONFIG_GPIO_BTN_1				1
#define CONFIG_GPIO_BTN_2				0
#define CONFIG_GPIO_ADC					A1
#define CONFIG_GPIO_UART_RX_TX	4
#define CONFIG_GPIO_UART_DE_RE	3

// ADC
#define CONFIG_ADC_TRIMMER_DETECT			5			// +- steps to check whether the potentiometer was turned or not.

// UART
#define CONFIG_UART_TX_MODE_DELAY_US	50		// Microseconds to stabilize the RS-485 bus after pulling high the DE/~RE pin.

// Timings
#define CONFIG_TIME_BTN_DEBOUNCER_MS	200		// Button delay time after pressed.
#define CONFIG_TIME_BTN_HELD_TICKS		5			// At this number of ticks, the button will be considered held; the minimum hold time is `CONFIG_HOLD_BTN_TICKS` * `CONFIG_TIME_BTN_DEBOUNCER_MS`.
#define CONFIG_TIME_BTN_LOCK_MS				400		// Minimum time that must pass from the last button press to send the current states.

#endif  /* INC_CONF_CONST_H_ */
