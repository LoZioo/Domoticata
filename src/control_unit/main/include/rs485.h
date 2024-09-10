/** @file rs485.h
 *  @brief  Created on: Aug 17, 2024
 *          Davide Scalisi
 *
 * 					Description:	RS485 code.
 *
 * @copyright [2024] Davide Scalisi *
 * @copyright All Rights Reserved. *
 *
*/

#ifndef INC_RS485_H_
#define INC_RS485_H_

/************************************************************************************************************
* Included files
************************************************************************************************************/

// Standard libraries.
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

// Platform libraries.
#include <esp_err.h>
#include <esp_check.h>
#include <esp_log.h>

#include <freertos/FreeRTOS.h>
#include <driver/uart.h>

// UniLibC libraries.
#include <ul_errors.h>
#include <ul_utils.h>
#include <ul_button_states.h>
#include <ul_master_slave.h>
#include <ul_crc.h>

// Project libraries.
#include <main.h>
#include <zone.h>
#include <gpio.h>
#include <pwm.h>

/************************************************************************************************************
* Public Defines
************************************************************************************************************/

/************************************************************************************************************
* Public Types Definitions
************************************************************************************************************/

/************************************************************************************************************
* Public Variables Prototypes
************************************************************************************************************/

/************************************************************************************************************
* Public Functions Prototypes
************************************************************************************************************/

/**
 * @brief Initialize the library.
 */
extern esp_err_t rs485_setup();

#endif  /* INC_RS485_H_ */
