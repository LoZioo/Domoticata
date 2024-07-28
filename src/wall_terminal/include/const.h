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

// GPIO
#define CONF_GPIO_PWM_A				0
#define CONF_GPIO_PWM_B				1
#define CONF_GPIO_ADC					A1
#define CONF_GPIO_UART_DE_RE	-1

// Analog buttons
#define CONF_BTN_VALID_EDGE				(ADC_MAX - 100)
#define CONF_BTN_VALID_INTERVAL		20

// Values from `analog_button_tuner.cpp`.
#define CONF_BTN_0_MEAN		79
#define CONF_BTN_1_MEAN		143

#define VAL_BTN_0_LOWER_THR			(CONF_BTN_0_MEAN - CONF_BTN_VALID_INTERVAL)
#define VAL_BTN_0_UPPER_THR			(CONF_BTN_0_MEAN + CONF_BTN_VALID_INTERVAL)

#define VAL_BTN_1_LOWER_THR			(CONF_BTN_1_MEAN - CONF_BTN_VALID_INTERVAL)
#define VAL_BTN_1_UPPER_THR			(CONF_BTN_1_MEAN + CONF_BTN_VALID_INTERVAL)

// Timings
#define CONF_DEBOUNCE_TIME_MS		250

#endif  /* INC_UL_CONFIGS_H_ */
