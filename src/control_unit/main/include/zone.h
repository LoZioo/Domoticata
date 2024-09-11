/** @file zone.h
 *  @brief  Created on: Aug 17, 2024
 *          Davide Scalisi
 *
 * 					Description:	RS485 button and trimmer maps.
 *
 * @copyright [2024] Davide Scalisi *
 * @copyright All Rights Reserved. *
 *
*/

#ifndef INC_ZONE_H_
#define INC_ZONE_H_

/************************************************************************************************************
* Included files
************************************************************************************************************/

// Standard libraries.
#include <stdint.h>
#include <stdbool.h>

/************************************************************************************************************
* Public Types Definitions
************************************************************************************************************/

// Available zones; `sizeof(zone_t)` must be 1!
typedef enum __attribute__((__packed__)) {
	ZONE_UNMAPPED,

	ZONE_LED_1,
	ZONE_LED_2,
	ZONE_LED_3,
	ZONE_LED_4,
	ZONE_LED_5,
	ZONE_LED_6,
	ZONE_LED_7,
	ZONE_LED_8,
	ZONE_LED_9,
	ZONE_LED_10,
	ZONE_LED_11,
	ZONE_LED_12,

	ZONE_RELAY_1,
	ZONE_RELAY_2,
	ZONE_RELAY_3,
	ZONE_RELAY_4,

	ZONE_FAN_CONTROLLER,
	ZONE_BUZZER,

	ZONE_MAX
} zone_t;

/************************************************************************************************************
* Public Defines
************************************************************************************************************/

/**
 * @brief Library internal helper.
 */
#define __zone_same(zone)	\
	(zone), (zone), (zone)

#define ZONE_DIGITAL_LEN	7
#define ZONE_PWM_LEN			11

// Zones that must be controlled by a digital output.
#define ZONE_DIGITAL_ZONES	{ \
	ZONE_LED_7, \
	ZONE_LED_8, \
	ZONE_RELAY_1, \
	ZONE_RELAY_2, \
	ZONE_RELAY_3, \
	ZONE_RELAY_4, \
	ZONE_BUZZER \
}

// Corresponding GPIO map for `ZONE_DIGITAL_ZONES`.
#define ZONE_DIGITAL_GPIO	{ \
	CONFIG_GPIO_LED_7, \
	CONFIG_GPIO_LED_8, \
	CONFIG_GPIO_RELAY_1, \
	CONFIG_GPIO_RELAY_2, \
	CONFIG_GPIO_RELAY_3, \
	CONFIG_GPIO_RELAY_4, \
	CONFIG_GPIO_ALARM \
}

// Zones that can be controlled by a PWM output.
#define ZONE_PWM_ZONES	{ \
	ZONE_LED_1, \
	ZONE_LED_2, \
	ZONE_LED_3, \
	ZONE_LED_4, \
	ZONE_LED_5, \
	ZONE_LED_6, \
	ZONE_LED_9, \
	ZONE_LED_10, \
	ZONE_LED_11, \
	ZONE_LED_12, \
	ZONE_FAN_CONTROLLER \
}

// Corresponding GPIO map for `ZONE_PWM_ZONES`.
#define ZONE_PWM_GPIO	{ \
	CONFIG_GPIO_LED_1, \
	CONFIG_GPIO_LED_2, \
	CONFIG_GPIO_LED_3, \
	CONFIG_GPIO_LED_4, \
	CONFIG_GPIO_LED_5, \
	CONFIG_GPIO_LED_6, \
	CONFIG_GPIO_LED_9, \
	CONFIG_GPIO_LED_10, \
	CONFIG_GPIO_LED_11, \
	CONFIG_GPIO_LED_12, \
	CONFIG_GPIO_FAN \
}

// Corresponding PWM ports (speed modes) map for `ZONE_PWM_ZONES`.
#define ZONE_PWM_PORTS	{ \
	LEDC_HIGH_SPEED_MODE, \
	LEDC_HIGH_SPEED_MODE, \
	LEDC_HIGH_SPEED_MODE, \
	LEDC_HIGH_SPEED_MODE, \
	LEDC_HIGH_SPEED_MODE, \
	LEDC_HIGH_SPEED_MODE, \
	LEDC_HIGH_SPEED_MODE, \
	LEDC_HIGH_SPEED_MODE, \
	LEDC_LOW_SPEED_MODE, \
	LEDC_LOW_SPEED_MODE, \
	LEDC_LOW_SPEED_MODE \
}

// Corresponding PWM channels map for `ZONE_PWM_ZONES`.
#define ZONE_PWM_CHANNELS	{ \
	LEDC_CHANNEL_0, \
	LEDC_CHANNEL_1, \
	LEDC_CHANNEL_2, \
	LEDC_CHANNEL_3, \
	LEDC_CHANNEL_4, \
	LEDC_CHANNEL_5, \
	LEDC_CHANNEL_6, \
	LEDC_CHANNEL_7, \
	LEDC_CHANNEL_0, \
	LEDC_CHANNEL_1, \
	LEDC_CHANNEL_2 \
}

/**
 * f: (device_id x button_id x button_state) -> (zone)
 *
 * 	(device_id 0) [that's the wall terminal 0]
 * {	(button_id 1) [that's the first button of the wall terminal 0]
 * 	{ (x, y, z) }		(pressed, double pressed, held)
 * }
 */
#define ZONE_BUTTONS	{ \
	{ \
		{ __zone_same(ZONE_LED_4)	}, \
		{ __zone_same(ZONE_UNMAPPED)	}, \
		{ __zone_same(ZONE_UNMAPPED)	} \
	}, \
	{ \
		{ __zone_same(ZONE_LED_2)	}, \
		{ __zone_same(ZONE_LED_1)	}, \
		{ __zone_same(ZONE_UNMAPPED)	} \
	}, \
	{ \
		{ __zone_same(ZONE_LED_3)	}, \
		{ __zone_same(ZONE_UNMAPPED)	}, \
		{ __zone_same(ZONE_UNMAPPED)	} \
	}, \
	{ \
		{ __zone_same(ZONE_LED_9)	}, \
		{ __zone_same(ZONE_LED_2)	}, \
		{ __zone_same(ZONE_UNMAPPED)	} \
	}, \
	{ \
		{ __zone_same(ZONE_LED_2)	}, \
		{ __zone_same(ZONE_LED_10)	}, \
		{ __zone_same(ZONE_UNMAPPED)	} \
	}, \
	{ \
		{ __zone_same(ZONE_LED_6)	}, \
		{ __zone_same(ZONE_LED_7)	}, \
		{ __zone_same(ZONE_UNMAPPED)	} \
	}, \
	{ \
		{ __zone_same(ZONE_LED_8)	}, \
		{ __zone_same(ZONE_LED_7)	}, \
		{ __zone_same(ZONE_UNMAPPED)	} \
	}, \
	{ \
		{ __zone_same(ZONE_LED_5)	}, \
		{ __zone_same(ZONE_LED_7)	}, \
		{ __zone_same(ZONE_UNMAPPED)	} \
	}, \
	{ \
		{ __zone_same(ZONE_LED_7)	}, \
		{ __zone_same(ZONE_LED_1)	}, \
		{ __zone_same(ZONE_UNMAPPED)	} \
	}, \
	{ \
		{ __zone_same(ZONE_LED_6)	}, \
		{ __zone_same(ZONE_UNMAPPED)	}, \
		{ __zone_same(ZONE_UNMAPPED)	} \
	}, \
	{ \
		{ __zone_same(ZONE_LED_7)	}, \
		{ __zone_same(ZONE_UNMAPPED)	}, \
		{ __zone_same(ZONE_UNMAPPED)	} \
	}, \
	{ \
		{ __zone_same(ZONE_LED_1)	}, \
		{ __zone_same(ZONE_LED_2)	}, \
		{ __zone_same(ZONE_LED_4)	} \
	}, \
	{ \
		{ __zone_same(ZONE_LED_6)	}, \
		{ __zone_same(ZONE_UNMAPPED)	}, \
		{ __zone_same(ZONE_UNMAPPED)	} \
	} \
}

// f: (device_id) -> (zone)
#define ZONE_TRIMMERS	{ \
	ZONE_LED_4, \
	ZONE_LED_2, \
	ZONE_UNMAPPED, \
	ZONE_UNMAPPED, \
	ZONE_LED_10, \
	ZONE_LED_6, \
	ZONE_UNMAPPED, \
	ZONE_LED_5, \
	ZONE_UNMAPPED, \
	ZONE_LED_6, \
	ZONE_UNMAPPED, \
	ZONE_LED_1, \
	ZONE_LED_6 \
}

#endif  /* INC_ZONE_H_ */
