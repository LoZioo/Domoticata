/** @file ul_button_states.c
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

#include <ul_button_states.h>
#include <ul_private.h>

/************************************************************************************************************
* Private Defines
************************************************************************************************************/

/************************************************************************************************************
* Private Types Definitions
 ************************************************************************************************************/

/************************************************************************************************************
* Private Variables
 ************************************************************************************************************/

static uint16_t __button_states = 0;

/************************************************************************************************************
* Private Functions Prototypes
 ************************************************************************************************************/

static uint8_t __btn_bit_shift(ul_bs_button_id_t button);
static uint16_t __btn_bit_mask(ul_bs_button_id_t button);

/************************************************************************************************************
* Private Functions Definitions
 ************************************************************************************************************/

uint8_t __btn_bit_shift(ul_bs_button_id_t button){
	return (button * 2 - 2);
}

uint16_t __btn_bit_mask(ul_bs_button_id_t button){
	return (3 << __btn_bit_shift(button));
}

/************************************************************************************************************
* Public Functions Definitions
 ************************************************************************************************************/

ul_bs_button_state_t ul_bs_get_button_state(ul_bs_button_id_t button){
	return (
		( __button_states & __btn_bit_mask(button) ) >>
		__btn_bit_shift(button)
	);
}

void ul_bs_set_button_state(ul_bs_button_id_t button, ul_bs_button_state_t state){
	__button_states = (
		( __button_states & ~__btn_bit_mask(button) ) |
		( state << __btn_bit_shift(button) )
	);
}

void ul_bs_reset_button_states(){
	__button_states = 0;
}

uint16_t ul_bs_get_button_states(){
	return __button_states;
}

void ul_bs_set_button_states(uint16_t button_states){
	__button_states = button_states;
}
