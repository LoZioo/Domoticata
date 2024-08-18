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
* Public Types Definitions
************************************************************************************************************/

/**
 * @brief Assign to a single button state/trimmer rotation, up to two zones.
 * @note If you want to leave a zone unmapped, set it to 0.
 */
typedef union __attribute__((__packed__)) {
	struct {
		uint8_t zone_1: 4;
		uint8_t zone_2: 4;
	} zones;

	uint8_t raw;
} rs485_keymap_t;

/************************************************************************************************************
* Public Defines
************************************************************************************************************/

// `rs485_keymap.h` helper.
#define Z(z1, z2)( \
	(rs485_keymap_t){ \
		.zones.zone_1 = z1, \
		.zones.zone_2 = z2 \
	} \
)

/**
 * f: (device_id x button_id x button_state) -> (zone_1 x zone_2)
 *
 * {	(button 0x00)
 * 	{ Z( 0,  0), Z( 0,  0), Z( 0,  0) }		(pressed, double pressed, held)
 * }
 *
 * Note: 0 means not mapped.
 */
#define RS485_KEYMAP_BUTTON	{ \
	{ \
		{ Z( 0,  0), Z( 0,  0), Z( 0,  0) }, \
		{ Z( 0,  0), Z( 0,  0), Z( 0,  0) }, \
		{ Z( 0,  0), Z( 0,  0), Z( 0,  0) } \
	}, \
	{ \
		{ Z( 1,  2), Z( 7,  8), Z( 0,  0) }, \
		{ Z( 3,  4), Z( 9, 10), Z( 0,  0) }, \
		{ Z( 5,  6), Z(11, 12), Z( 0,  0) } \
	}, \
	{ \
		{ Z( 0,  0), Z( 0,  0), Z( 0,  0) }, \
		{ Z( 0,  0), Z( 0,  0), Z( 0,  0) }, \
		{ Z( 0,  0), Z( 0,  0), Z( 0,  0) } \
	}, \
	{ \
		{ Z( 0,  0), Z( 0,  0), Z( 0,  0) }, \
		{ Z( 0,  0), Z( 0,  0), Z( 0,  0) }, \
		{ Z( 0,  0), Z( 0,  0), Z( 0,  0) } \
	}, \
	{ \
		{ Z( 0,  0), Z( 0,  0), Z( 0,  0) }, \
		{ Z( 0,  0), Z( 0,  0), Z( 0,  0) }, \
		{ Z( 0,  0), Z( 0,  0), Z( 0,  0) } \
	}, \
	{ \
		{ Z( 0,  0), Z( 0,  0), Z( 0,  0) }, \
		{ Z( 0,  0), Z( 0,  0), Z( 0,  0) }, \
		{ Z( 0,  0), Z( 0,  0), Z( 0,  0) } \
	}, \
	{ \
		{ Z( 0,  0), Z( 0,  0), Z( 0,  0) }, \
		{ Z( 0,  0), Z( 0,  0), Z( 0,  0) }, \
		{ Z( 0,  0), Z( 0,  0), Z( 0,  0) } \
	}, \
	{ \
		{ Z( 0,  0), Z( 0,  0), Z( 0,  0) }, \
		{ Z( 0,  0), Z( 0,  0), Z( 0,  0) }, \
		{ Z( 0,  0), Z( 0,  0), Z( 0,  0) } \
	}, \
	{ \
		{ Z( 0,  0), Z( 0,  0), Z( 0,  0) }, \
		{ Z( 0,  0), Z( 0,  0), Z( 0,  0) }, \
		{ Z( 0,  0), Z( 0,  0), Z( 0,  0) } \
	}, \
	{ \
		{ Z( 0,  0), Z( 0,  0), Z( 0,  0) }, \
		{ Z( 0,  0), Z( 0,  0), Z( 0,  0) }, \
		{ Z( 0,  0), Z( 0,  0), Z( 0,  0) } \
	}, \
	{ \
		{ Z( 0,  0), Z( 0,  0), Z( 0,  0) }, \
		{ Z( 0,  0), Z( 0,  0), Z( 0,  0) }, \
		{ Z( 0,  0), Z( 0,  0), Z( 0,  0) } \
	}, \
	{ \
		{ Z( 0,  0), Z( 0,  0), Z( 0,  0) }, \
		{ Z( 0,  0), Z( 0,  0), Z( 0,  0) }, \
		{ Z( 0,  0), Z( 0,  0), Z( 0,  0) } \
	}, \
	{ \
		{ Z( 0,  0), Z( 0,  0), Z( 0,  0) }, \
		{ Z( 0,  0), Z( 0,  0), Z( 0,  0) }, \
		{ Z( 0,  0), Z( 0,  0), Z( 0,  0) } \
	} \
}

/**
 * f: (device_id) -> (zone_1 x zone_2)
 * Note: 0 means not mapped.
 */
#define RS485_KEYMAP_TRIMMER	{ \
	Z( 0,  0), \
	Z( 1, 12), \
	Z( 0,  0), \
	Z( 0,  0), \
	Z( 0,  0), \
	Z( 0,  0), \
	Z( 0,  0), \
	Z( 0,  0), \
	Z( 0,  0), \
	Z( 0,  0), \
	Z( 0,  0), \
	Z( 0,  0), \
	Z( 0,  0) \
}

#endif  /* INC_RS485_KEYMAP_H_ */
