/** @file dup.h
 *  @brief  Created on: July 29, 2024
 *          Davide Scalisi
 *
 * 					Description:	Minimal library to implement the comunication between the Domoticata control unit and the wall terminals.
 * 					Notes:				"DUP" stands for Domoticata UART Protocol.
 *
 * @copyright [2024] Davide Scalisi *
 * @copyright All Rights Reserved. *
 *
*/

#ifndef INC_DUP_H_
#define INC_DUP_H_

/************************************************************************************************************
* Included files
************************************************************************************************************/

// Standard libraries.
#include <stdlib.h>
#include <stdint.h>

/************************************************************************************************************
* Public Defines
************************************************************************************************************/

#define DUP_ID_CU					0
#define DUP_ID_BROADCAST	32

/************************************************************************************************************
* Public Types Definitions
************************************************************************************************************/

/**
 * @brief DUP commands: up to 8 commands and 30 device IDs (effectively 30 + 1 + 1, where 0 is the `CU`, 1 to 30 are the `WTs` and 31 is the broadcast). The command is trasmitted as is: `iiiiiccc`, where `iiiii` is the device ID and `ccc` is the command. Eventual parameters are transmitted just after this byte.
 * @note Control unit: `CU`, Wall terminal `WT`. The parameters are trasmitted in LSB-first mode, so read them from left to right.
 */
typedef enum __attribute__((packed)) {

	DUP_COMMAND_POLL,							// (`CU`	->	`WT`) [ Default command when the `CU` polls the `WTs`. The requested `WT` can answer to the `CU` with an ACK to signal that it's processing the received command. ]

	DUP_COMMAND_ACK,							// (`CU`	<->	`WT`)
	DUP_COMMAND_NACK,							// (`CU`	<->	`WT`)

	DUP_COMMAND_BUTTON_STATES,		// (`CU`	<->	`WT`)	[ Req. parameters: `N/A`, res. parameters: `bbbbbbbb bbbbbbbb`, where `bbbbbbbb bbbbbbbb` are the eight button states from the `button_states` library. ]
	DUP_COMMAND_SET_PWM,					// (`CU`	->	`WT`)	[ Parameters: `vvvvvvvc`, where `vvvvvvv` is the PWM percentage value and `c` is the PWM channel. ]

} dup_command_t;

/**
 * @brief DUP encoded header (device ID + command).
 */
typedef uint8_t dup_encoded_header_t;

/**
 * @brief DUP decoded header (device ID + command).
 */
typedef struct __attribute__((packed)) {

	// Device ID: `DUP_ID_CU`, `DUP_ID_BROADCAST` or an ID from 1 to 30.
	uint8_t device_id;

	// A member of `dup_command_t`.
	dup_command_t command;

} dup_decoded_header_t;

/************************************************************************************************************
* Public Variables Prototypes
************************************************************************************************************/

/************************************************************************************************************
* Public Functions Prototypes
************************************************************************************************************/

/**
 * @brief Encode the specified DUP header.
 */
extern dup_encoded_header_t dup_encode_header(dup_decoded_header_t header);

/**
 * @brief Decode the specified DUP header.
 */
extern dup_decoded_header_t dup_decode_header(dup_encoded_header_t header);

/**
 * @brief Encode the specified `DUP_COMMAND_SET_PWM` parameters.
 * @param pwm_channel The selected PWM channel: can be only 0 or 1.
 * @param pwm_percentage The PWM percentage from 0% to 100%.
 * @return The encoded parameters.
 */
extern uint8_t dup_encode_parameters_command_set_pwm(uint8_t pwm_channel, uint8_t pwm_percentage);

/**
 * @brief Decode the specified `DUP_COMMAND_SET_PWM` parameters.
 * @param encoded_parameters The encoded parameters from `dup_encode_parameters_command_set_pwm()`.
 * @param pwm_channel A pointer where to store the specified PWM channel.
 * @param pwm_percentage A pointer where to store the specified PWM percentage.
 */
extern void dup_decode_parameters_command_set_pwm(uint8_t encoded_parameters, uint8_t *pwm_channel, uint8_t *pwm_percentage);

#endif  /* INC_DUP_H_ */
