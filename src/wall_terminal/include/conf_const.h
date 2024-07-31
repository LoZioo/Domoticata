/** @file const.h
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
#define CONF_GPIO_PWM_A				0
#define CONF_GPIO_PWM_B				1
#define CONF_GPIO_ADC					A1
#define CONF_GPIO_UART_DE_RE	-1
#define CONF_GPIO_UART_RX_TX	4

// UART
#define CONF_UART_DEVICE_RX_ID	(CONF_UART_DEVICE_ID & 0x7F)
#define CONF_UART_DEVICE_TX_ID	(CONF_UART_DEVICE_ID | 0x80)

// Timings
#define CONF_TIME_BTN_DEBOUNCER_MS	200		// Button delay time after pressed.
#define CONF_TIME_BTN_HELD_TICKS		5			// At this number of ticks, the button will be considered held; the minimum hold time is `CONF_HOLD_BTN_TICKS` * `CONF_TIME_BTN_DEBOUNCER_MS`.
#define CONF_TIME_BTN_LOCK_MS				500		// Minimum time that must pass from the last button click to send the current button states.

/**
 * Analog buttons
 * Note: used inside `analog_button_read()`.
 */
#define CONF_BTN_VALID_EDGE				(ADC_MAX - 100)		// Threshold under where one button press is detected.
#define CONF_BTN_VALID_INTERVAL		20								// Circular +-interval around the button mean value.

#ifdef CONF_BTN_1_AVG
	#define VAL_BTN_1_LOWER_THR			(CONF_BTN_1_AVG - CONF_BTN_VALID_INTERVAL)
	#define VAL_BTN_1_UPPER_THR			(CONF_BTN_1_AVG + CONF_BTN_VALID_INTERVAL)
#endif

#ifdef CONF_BTN_2_AVG
	#define VAL_BTN_2_LOWER_THR			(CONF_BTN_2_AVG - CONF_BTN_VALID_INTERVAL)
	#define VAL_BTN_2_UPPER_THR			(CONF_BTN_2_AVG + CONF_BTN_VALID_INTERVAL)
#endif

#ifdef CONF_BTN_3_AVG
	#define VAL_BTN_3_LOWER_THR			(CONF_BTN_3_AVG - CONF_BTN_VALID_INTERVAL)
	#define VAL_BTN_3_UPPER_THR			(CONF_BTN_3_AVG + CONF_BTN_VALID_INTERVAL)
#endif

#ifdef CONF_BTN_3_AVG
	#define VAL_BTN_3_LOWER_THR			(CONF_BTN_3_AVG - CONF_BTN_VALID_INTERVAL)
	#define VAL_BTN_3_UPPER_THR			(CONF_BTN_3_AVG + CONF_BTN_VALID_INTERVAL)
#endif

#ifdef CONF_BTN_4_AVG
	#define VAL_BTN_4_LOWER_THR			(CONF_BTN_4_AVG - CONF_BTN_VALID_INTERVAL)
	#define VAL_BTN_4_UPPER_THR			(CONF_BTN_4_AVG + CONF_BTN_VALID_INTERVAL)
#endif

#ifdef CONF_BTN_5_AVG
	#define VAL_BTN_5_LOWER_THR			(CONF_BTN_5_AVG - CONF_BTN_VALID_INTERVAL)
	#define VAL_BTN_5_UPPER_THR			(CONF_BTN_5_AVG + CONF_BTN_VALID_INTERVAL)
#endif

#ifdef CONF_BTN_6_AVG
	#define VAL_BTN_6_LOWER_THR			(CONF_BTN_6_AVG - CONF_BTN_VALID_INTERVAL)
	#define VAL_BTN_6_UPPER_THR			(CONF_BTN_6_AVG + CONF_BTN_VALID_INTERVAL)
#endif

#ifdef CONF_BTN_7_AVG
	#define VAL_BTN_7_LOWER_THR			(CONF_BTN_7_AVG - CONF_BTN_VALID_INTERVAL)
	#define VAL_BTN_7_UPPER_THR			(CONF_BTN_7_AVG + CONF_BTN_VALID_INTERVAL)
#endif

#ifdef CONF_BTN_8_AVG
	#define VAL_BTN_8_LOWER_THR			(CONF_BTN_8_AVG - CONF_BTN_VALID_INTERVAL)
	#define VAL_BTN_8_UPPER_THR			(CONF_BTN_8_AVG + CONF_BTN_VALID_INTERVAL)
#endif

#endif  /* INC_CONF_CONST_H_ */
