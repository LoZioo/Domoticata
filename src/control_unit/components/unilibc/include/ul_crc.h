/** @file ul_crc.h
 *  @brief  Created on: Aug 3, 2024
 *          Davide Scalisi
 *
 * 					Description:	CRC computation functions.
 * 					Note:					http://www.sunshine2k.de/coding/javascript/crc/crc_js.html
 *
 * @copyright [2024] Davide Scalisi *
 * @copyright All Rights Reserved. *
 *
*/

#ifndef INC_UL_CRC_H_
#define INC_UL_CRC_H_

/************************************************************************************************************
* Included files
************************************************************************************************************/

// Standard libraries.
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

// UniLibC libraries.
#include <ul_configs.h>

/************************************************************************************************************
* Public Defines
************************************************************************************************************/

/**
 * @brief Compare the received CRC value to the locally computed one.
 * @param ul_crc_function The CRC function (e.g. `ul_crc_crc8`).
 * @param data The data buffer.
 * @param len `sizeof(data)`.
 * @param received_crc The received CRC value.
 * @return `true` if the CRC matches, `false` otherwise.
 */
#define ul_crc_ok(ul_crc_function, data, len, received_crc)( \
	ul_crc_function(data, len) == received_crc \
)

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
 * @brief Compute the CRC-8 of a buffer.
 * @param data The data buffer.
 * @param len `sizeof(data)`.
 * @return The computed CRC-8.
 * @note You can found the CRC-8 configurations on the `ul_configs.h` header.
 */
extern uint8_t ul_crc_crc8(const uint8_t *data, uint32_t len);

#endif  /* INC_UL_CRC_H_ */
