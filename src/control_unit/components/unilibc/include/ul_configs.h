/** @file ul_configs.h
 *  @brief  Created on: June 30, 2024
 *          Davide Scalisi
 *
 * 					Description:	DS framework configurations.
 *
 * @copyright [2024] Davide Scalisi *
 * @copyright All Rights Reserved. *
 *
*/

#ifndef INC_UL_CONFIGS_H_
#define INC_UL_CONFIGS_H_

/************************************************************************************************************
* ul_errors.h
************************************************************************************************************/

#define UL_CONF_ERRORS_PRINT_DEBUG						// Comment to disable the text debug output.

/************************************************************************************************************
* ul_master_slave.h
************************************************************************************************************/

#define UL_CONF_MS_UINT8_IS_BUF_SIZE_T				// Comment to set the max buffer sizes from (2^32)-1 to (2^8)-1 and save some RAM.

/************************************************************************************************************
* ul_master_slave.h
************************************************************************************************************/

#define UL_CRC_CRC8_POLYNOMIAL				0x07		// Standard CRC8 configurations.
#define UL_CRC_CRC8_INITIAL_VALUE			0x00
#define UL_CRC_CRC8_FINAL_XOR_VALUE		0x00

#endif  /* INC_UL_CONFIGS_H_ */
