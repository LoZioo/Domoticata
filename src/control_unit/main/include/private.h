/** @file private.h
 *  @brief  Created on: Sep 14, 2024
 *          Davide Scalisi
 *
 * 					Description:	Private common header.
 *
 * @copyright [2024] Davide Scalisi *
 * @copyright All Rights Reserved. *
 *
*/

#ifndef INC_PRIVATE_H_
#define INC_PRIVATE_H_

/************************************************************************************************************
* Included files
************************************************************************************************************/

/************************************************************************************************************
* Public Defines
************************************************************************************************************/

#define assert_param_notnull(ptr)	\
	ESP_RETURN_ON_FALSE( \
		(ptr) != NULL, \
		ESP_ERR_INVALID_ARG, \
		TAG, \
		"Error: `" #ptr "` is NULL" \
	)

#define assert_param_size_ok(size)	\
	ESP_RETURN_ON_FALSE( \
		(size) > 0, \
		ESP_ERR_INVALID_ARG, \
		TAG, \
		"Error: `" #size "` is 0" \
	)

/************************************************************************************************************
* Public Types Definitions
************************************************************************************************************/

#endif  /* INC_PRIVATE_H_ */
