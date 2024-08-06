/** @file setup.h
 *  @brief  Created on: Aug 3, 2024
 *          Davide Scalisi
 *
 * 					Description:	Project module to initialize onboard peripherals.
 *
 * @copyright [2024] Davide Scalisi *
 * @copyright All Rights Reserved. *
 *
*/

#ifndef INC_SETUP_H_
#define INC_SETUP_H_

/************************************************************************************************************
* Included files
************************************************************************************************************/

// Standard libraries.

// Platform libraries.
#include <esp_err.h>
#include <esp_check.h>

#include <freertos/FreeRTOS.h>

#include <driver/gpio.h>
#include <driver/uart.h>

/************************************************************************************************************
* Public Defines
************************************************************************************************************/

#define ESP_PROTOCOL_CORE			0
#define ESP_APPLICATION_CORE	1

/************************************************************************************************************
* Public Types Definitions
************************************************************************************************************/

/************************************************************************************************************
* Public Variables Prototypes
************************************************************************************************************/

/************************************************************************************************************
* Public Functions Prototypes
************************************************************************************************************/

extern esp_err_t GPIO_setup();
extern esp_err_t UART_setup();
extern esp_err_t TASKS_setup();

#endif  /* INC_SETUP_H_ */
