/** @file ul_button_states.h
 *  @brief  Created on: July 29, 2024
 *          Davide Scalisi
 *
 * 					Description:	Minimal library to store up to 4 button states for 8 different buttons on only 16-bits.
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
typedef enum __attribute__((packed)) {
	UL_BS_BUTTON_NONE = 0,
	UL_BS_BUTTON_1,
	UL_BS_BUTTON_2,
	UL_BS_BUTTON_3,
	UL_BS_BUTTON_4,
	UL_BS_BUTTON_5,
	UL_BS_BUTTON_6,
	UL_BS_BUTTON_7,
	UL_BS_BUTTON_8
} ul_bs_button_id_t;

/**
 * @brief Button states (2-bits).
 */
typedef enum __attribute__((packed)) {
	UL_BS_BUTTON_STATE_IDLE = 0,
	UL_BS_BUTTON_STATE_PRESSED,
	UL_BS_BUTTON_STATE_DOUBLE_PRESSED,
	UL_BS_BUTTON_STATE_HELD,
} ul_bs_button_state_t;

/************************************************************************************************************
* Public Variables Prototypes
************************************************************************************************************/

/************************************************************************************************************
* Public Functions Prototypes
************************************************************************************************************/

/**
 * @brief Get the button state of the specified button.
 * @param button A member of `ul_bs_button_id_t`: `BUTTON_1`, `BUTTON_2`, ...
 * @return A member of `ul_bs_button_state_t`: `BUTTON_STATE_PRESSED`, `BUTTON_STATE_DOUBLE_PRESSED`, ...
 */
extern ul_bs_button_state_t ul_bs_get_button_state(ul_bs_button_id_t button);

/**
 * @brief Set the button state of the specified button.
 * @param button A member of `ul_bs_button_id_t`: `BUTTON_1`, `BUTTON_2`, ...
 * @param state A member of `ul_bs_button_state_t`: `BUTTON_STATE_PRESSED`, `BUTTON_STATE_DOUBLE_PRESSED`, ...
 */
extern void ul_bs_set_button_state(ul_bs_button_id_t button, ul_bs_button_state_t state);

/**
 * @brief Reset all the button states to `BUTTON_STATE_IDLE`.
 */
extern void ul_bs_reset_button_states();

/**
 * @return A copy of the current raw button states.
 */
extern uint16_t ul_bs_get_button_states();

/**
 * @brief Restore the button states to the specified values.
 * @param button_states Raw button states from `ul_bs_get_button_states()`.
 */
extern void ul_bs_set_button_states(uint16_t button_states);

#endif  /* INC_BUTTON_STATES_H_ */
