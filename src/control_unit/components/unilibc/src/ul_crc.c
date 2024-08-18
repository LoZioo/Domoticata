/** @file ul_crc.c
 *  @brief  Created on: Aug 3, 2024
 *          Davide Scalisi
 *
 * @copyright [2024] Davide Scalisi *
 * @copyright All Rights Reserved. *
 *
*/

/************************************************************************************************************
* Included files
************************************************************************************************************/

#include <ul_crc.h>

/************************************************************************************************************
* Private Defines
************************************************************************************************************/

/************************************************************************************************************
* Private Types Definitions
 ************************************************************************************************************/

/************************************************************************************************************
* Private Variables
 ************************************************************************************************************/

/************************************************************************************************************
* Private Functions Prototypes
 ************************************************************************************************************/

/************************************************************************************************************
* Private Functions Definitions
 ************************************************************************************************************/

/************************************************************************************************************
* Public Functions Definitions
 ************************************************************************************************************/

uint8_t ul_crc_crc8(const uint8_t *data, uint32_t len){

	if(
		data == NULL ||
		len == 0
	)
		return 0x00;

	uint8_t bit, crc = UL_CONFIG_CRC8_INITIAL_VALUE;

	for(uint32_t i=0; i<len; i++){
		crc ^= data[i];

		for(bit=0; bit<8; bit++)
			crc = (
				crc & 0x80 ?
				(crc << 1) ^ UL_CONFIG_CRC_CRC8_POLYNOMIAL :
				crc << 1
			);
	}

	return (crc ^ UL_CONFIG_CRC_CRC8_FINAL_XOR_VALUE);
}
