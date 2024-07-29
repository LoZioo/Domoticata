/** @file button_states.c
 *  @brief  Created on: July 29, 2024
 *          Davide Scalisi
 *
 * @copyright [2024] Davide Scalisi *
 * @copyright All Rights Reserved. *
 *
*/

/************************************************************************************************************
* Included files
************************************************************************************************************/

#include <button_states.h>

/************************************************************************************************************
* Private Defines
************************************************************************************************************/

/************************************************************************************************************
* Private Types Definitions
 ************************************************************************************************************/

/************************************************************************************************************
* Private Variables
 ************************************************************************************************************/

static raw_button_states_t __button_states = 0;

/************************************************************************************************************
* Private Functions Prototypes
 ************************************************************************************************************/

static uint8_t __btn_bit_shift(button_id_t button);
static uint8_t __btn_bit_mask(button_id_t button);

/************************************************************************************************************
* Private Functions Definitions
 ************************************************************************************************************/

uint8_t __btn_bit_shift(button_id_t button){
	return (button * 2 - 2);
}

uint8_t __btn_bit_mask(button_id_t button){
	return (3 << __btn_bit_shift(button));
}

/************************************************************************************************************
* Public Functions Definitions
 ************************************************************************************************************/

button_state_t get_button_state(button_id_t button){
	return (
		(
			__button_states &
			__btn_bit_mask(button)
		) >> __btn_bit_shift(button)
	);
}

void set_button_state(button_id_t button, button_state_t state){
	__button_states = (
		(
			__button_states &
			~( (uint16_t) __btn_bit_mask(button) )
		) | ( state << __btn_bit_shift(button) )
	);
}

void reset_button_states(){
	__button_states = 0;
}

raw_button_states_t save_button_states(){
	return __button_states;
}

void load_button_states(raw_button_states_t button_states){
	__button_states = button_states;
}
