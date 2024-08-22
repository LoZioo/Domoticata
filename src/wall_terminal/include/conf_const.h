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

/**
 * Device hardware configurations.
 * Note: the 3rd button is attached between `CONFIG_GPIO_BTN_1`, `CONFIG_GPIO_BTN_2` and GND using two diodes.
 */
// #define CONFIG_HW_BTN_1		// Button attached between `CONFIG_GPIO_BTN_1` and GND.
// #define CONFIG_HW_BTN_2		// Button attached between `CONFIG_GPIO_BTN_2` and GND.
// #define CONFIG_HW_TRIMMER	// Trimmer attached to `CONFIG_GPIO_ADC`.

// Situational hardware configurations.
#if CONFIG_RS485_DEVICE_ID == 0
	#define CONFIG_HW_BTN_1
	#define CONFIG_HW_TRIMMER
#elif CONFIG_RS485_DEVICE_ID == 1
	#define CONFIG_HW_BTN_1
	#define CONFIG_HW_BTN_2
	#define CONFIG_HW_TRIMMER
#elif CONFIG_RS485_DEVICE_ID == 2
	#define CONFIG_HW_BTN_1
#elif CONFIG_RS485_DEVICE_ID == 3
	#define CONFIG_HW_BTN_1
	#define CONFIG_HW_BTN_2
#elif CONFIG_RS485_DEVICE_ID == 4
	#define CONFIG_HW_BTN_1
	#define CONFIG_HW_BTN_2
	#define CONFIG_HW_TRIMMER
#elif CONFIG_RS485_DEVICE_ID == 5
	#define CONFIG_HW_BTN_1
	#define CONFIG_HW_BTN_2
	#define CONFIG_HW_TRIMMER
#elif CONFIG_RS485_DEVICE_ID == 6
	#define CONFIG_HW_BTN_1
	#define CONFIG_HW_BTN_2
	#define CONFIG_HW_TRIMMER
#elif CONFIG_RS485_DEVICE_ID == 7
	#define CONFIG_HW_BTN_1
	#define CONFIG_HW_BTN_2
	#define CONFIG_HW_TRIMMER
#elif CONFIG_RS485_DEVICE_ID == 8
	#define CONFIG_HW_BTN_1
	#define CONFIG_HW_TRIMMER
#elif CONFIG_RS485_DEVICE_ID == 9
	#define CONFIG_HW_BTN_1
	#define CONFIG_HW_TRIMMER
#elif CONFIG_RS485_DEVICE_ID == 10
	#define CONFIG_HW_BTN_1
#elif CONFIG_RS485_DEVICE_ID == 11
	#define CONFIG_HW_BTN_1
	#define CONFIG_HW_BTN_2
	#define CONFIG_HW_TRIMMER
#elif CONFIG_RS485_DEVICE_ID == 12
	#define CONFIG_HW_BTN_1
	#define CONFIG_HW_TRIMMER
#else
	#error Device ID does not correspond to any hardware configuration.
#endif

#if !defined(CONFIG_HW_BTN_1) && !defined(CONFIG_HW_BTN_2)
	#define CONFIG_HW_NO_BTN
	#define ul_bs_get_button_states() 0

	#ifndef CONFIG_HW_TRIMMER
		#error Hardware configuration error.
	#endif
#endif

#ifndef CONFIG_HW_TRIMMER
	#define analogRead(pin)	0
#endif

#endif  /* INC_CONF_CONST_H_ */
