/** @file const.h
 *  @brief  Created on: July 28, 2024
 *          Davide Scalisi
 *
 * 					Description:	Domoticata wall terminal configurations.
 *
 * @copyright [2024] Davide Scalisi *
 * @copyright All Rights Reserved. *
 *
*/

#ifndef INC_CONST_H_
#define INC_CONST_H_

// Misc
#define ADC_MAX		((1 << 10) - 1)
#define PWM_MAX		0xFF

// GPIO
#define CONF_GPIO_PWM_A				0
#define CONF_GPIO_PWM_B				1
#define CONF_GPIO_ADC					A1
#define CONF_GPIO_UART_DE_RE	-1

// Analog buttons
#define CONF_BTN_VALID_EDGE				(ADC_MAX - 100)
#define CONF_BTN_VALID_INTERVAL		20

// Values from `analog_button_tuner.cpp`.
#define CONF_BTN_1_MEAN		79
#define CONF_BTN_2_MEAN		143

#define VAL_BTN_1_LOWER_THR			(CONF_BTN_1_MEAN - CONF_BTN_VALID_INTERVAL)
#define VAL_BTN_1_UPPER_THR			(CONF_BTN_1_MEAN + CONF_BTN_VALID_INTERVAL)

#define VAL_BTN_2_LOWER_THR			(CONF_BTN_2_MEAN - CONF_BTN_VALID_INTERVAL)
#define VAL_BTN_2_UPPER_THR			(CONF_BTN_2_MEAN + CONF_BTN_VALID_INTERVAL)

// Timings
#define CONF_TIME_BTN_DEBOUNCER_MS		200		// Button delay time after pressed.
#define CONF_TIME_BTN_HELD_TICKS			5			// At this number of ticks, the button will be considered held; the minimum hold time is `CONF_HOLD_BTN_TICKS` * `CONF_TIME_BTN_DEBOUNCER_MS`.
#define CONF_TIME_BTN_LOCK_TIME_MS		500		// Minimum time that must pass from the last button click to send the current button states.

#endif  /* INC_UL_CONFIGS_H_ */
