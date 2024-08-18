/** @file rs485_keymap.h
 *  @brief  Created on: Aug 17, 2024
 *          Davide Scalisi
 *
 * 					Description:	RS485 button and trimmer maps.
 *
 * @copyright [2024] Davide Scalisi *
 * @copyright All Rights Reserved. *
 *
*/

#ifndef INC_RS485_KEYMAP_H_
#define INC_RS485_KEYMAP_H_

/************************************************************************************************************
* Public Defines
************************************************************************************************************/

/**
 * f: (device_id x button_id x button_state) -> pwm_index
 *
 * Row n: wall terminal with `device_id` = n.
 * Coloumn n: button n.
 *
 * Element 1: button pressed.
 * Element 2: button double pressed.
 * Element 3: button held.
 *
 * Note: -1 means not mapped.
 */
#define RS485_KEYMAP_BUTTON	\
{ \
	{{  0,  3,  2 }, { -1, -1, -1 }, { -1, -1, -1 }}, \
	{{  1,  4,  5 }, {  6,  7,  8 }, {  9, 10, 11 }}, \
	{{ -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 }}, \
	{{ -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 }}, \
	{{ -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 }}, \
	{{ -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 }}, \
	{{ -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 }}, \
	{{ -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 }}, \
	{{ -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 }}, \
	{{ -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 }}, \
	{{ -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 }}, \
	{{ -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 }}, \
	{{ -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 }}, \
}

/**
 * f: device_id -> pwm_index
 * Note: -1 means not mapped.
 */
#define RS485_KEYMAP_TRIMMER	\
	{ -1, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }

#endif  /* INC_RS485_KEYMAP_H_ */
