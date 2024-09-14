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

// #define UL_CONFIG_ERRORS_PRINT_DEBUG								// Comment to disable the text debug output.
#define UL_CONFIG_ERRORS_DISABLE_ESP_IDF_SHIMS			// Comment to enable the UniLibC to ESP-IDF errors shims.

/************************************************************************************************************
* ul_pm.h
************************************************************************************************************/

#define UL_CONFIG_PM_DOUBLE_BUFFER									// Comment to disable the classic PowerMonitor double sample buffer mode. Instead, the callback sample selection mode will be used.

/************************************************************************************************************
* ul_master_slave.h
************************************************************************************************************/

// #define UL_CONFIG_MS_UINT8_IS_BUF_SIZE_T						// Comment to set the max buffer sizes from (2^32)-1 to (2^8)-1 and save some RAM.

/************************************************************************************************************
* ul_crc.h
************************************************************************************************************/

#define UL_CONFIG_CRC_CRC8_POLYNOMIAL				0x07		// Standard CRC8 configurations.
#define UL_CONFIG_CRC8_INITIAL_VALUE				0x00
#define UL_CONFIG_CRC_CRC8_FINAL_XOR_VALUE	0x00

#endif  /* INC_UL_CONFIGS_H_ */
