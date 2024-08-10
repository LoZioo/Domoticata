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
#define PWM_MAX		0xFF

// GPIO
#define CONFIG_GPIO_PWM_A				0
#define CONFIG_GPIO_PWM_B				1
#define CONFIG_GPIO_ADC					A1
#define CONFIG_GPIO_UART_RX_TX	4
#define CONFIG_GPIO_UART_DE_RE	3

// UART
#define CONFIG_UART_TX_MODE_DELAY_US	50		// Microseconds to stabilize the RS-485 bus after pulling high the DE/~RE pin.

// Timings
#define CONFIG_TIME_BTN_DEBOUNCER_MS	200		// Button delay time after pressed.
#define CONFIG_TIME_BTN_HELD_TICKS		5			// At this number of ticks, the button will be considered held; the minimum hold time is `CONFIG_HOLD_BTN_TICKS` * `CONFIG_TIME_BTN_DEBOUNCER_MS`.
#define CONFIG_TIME_BTN_LOCK_MS				400		// Minimum time that must pass from the last button click to send the current button states.

/**
 * Analog buttons
 * Note: used inside `analog_button_read()`.
 */
#define CONFIG_BTN_VALID_EDGE				(ADC_MAX - 100)		// Threshold under where one button press is detected.
#define CONFIG_BTN_VALID_INTERVAL		20								// Circular +-interval around the button mean value.

// LED PWM values (8-bit).
#define CONFIG_PWM_BTN_IDLE							31
#define CONFIG_PWM_BTN_PRESSED					0
#define CONFIG_PWM_BTN_DOUBLE_PRESSED		255
#define CONFIG_PWM_BTN_HOLD							255

#endif  /* INC_CONF_CONST_H_ */
