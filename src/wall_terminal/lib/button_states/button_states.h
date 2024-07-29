/** @file button_states.h
 *  @brief  Created on: July 29, 2024
 *          Davide Scalisi
 *
 * 					Description:	Minimal library to store up to 8 button states.
 *
 * @copyright [2024] Davide Scalisi *
 * @copyright All Rights Reserved. *
 *
*/

#ifndef INC_BUTTON_STATES_H_
#define INC_BUTTON_STATES_H_

/************************************************************************************************************
* Included files
************************************************************************************************************/

// Standard libraries.
#include <stdint.h>

/************************************************************************************************************
* Public Defines
************************************************************************************************************/

/************************************************************************************************************
* Public Types Definitions
************************************************************************************************************/

/**
 * @brief Button IDs.
 */
typedef enum {
	BUTTON_NONE = 0,
	BUTTON_1,
	BUTTON_2,
	BUTTON_3,
	BUTTON_4,
	BUTTON_5,
	BUTTON_6,
	BUTTON_7,
	BUTTON_8
} button_id_t;

/**
 * @brief Button states (2-bits).
 */
typedef enum {
	BUTTON_STATE_IDLE = 0,
	BUTTON_STATE_PRESSED,
	BUTTON_STATE_DOUBLE_PRESSED,
	BUTTON_STATE_HELD,
} button_state_t;

/************************************************************************************************************
* Public Variables Prototypes
************************************************************************************************************/

/************************************************************************************************************
* Public Functions Prototypes
************************************************************************************************************/

/**
 * @brief Get the button state of the specified button.
 * @param button A member of `button_id_t`: `BUTTON_1`, `BUTTON_2`, ...
 * @return A member of `button_state_t`: `BUTTON_STATE_PRESSED`, `BUTTON_STATE_DOUBLE_PRESSED`, ...
 */
extern uint8_t get_button_state(uint8_t button);

/**
 * @brief Set the button state of the specified button.
 * @param button A member of `button_id_t`: `BUTTON_1`, `BUTTON_2`, ...
 * @param state A member of `button_state_t`: `BUTTON_STATE_PRESSED`, `BUTTON_STATE_DOUBLE_PRESSED`, ...
 */
extern void set_button_state(uint8_t button, uint8_t state);

/**
 * @brief Reset all the button states to `BUTTON_STATE_IDLE`.
 */
extern void reset_button_states();

/**
 * @return A copy of the current raw button states.
 */
extern uint16_t save_button_states();

/**
 * @brief Restore the button states to the specified values.
 * @param button_states Raw button states from `save_button_states()`.
 */
extern void load_button_states(uint16_t button_states);

#endif  /* INC_BUTTON_STATES_H_ */
