/** @file ul_private.h
 *  @brief  Created on: Sep 13, 2024
 *          Davide Scalisi
 *
 * 					Description:	UniLibC private common header.
 *
 * @copyright [2024] Davide Scalisi *
 * @copyright All Rights Reserved. *
 *
*/

#ifndef INC_UL_PRIVATE_H_
#define INC_UL_PRIVATE_H_

/************************************************************************************************************
* Included files
************************************************************************************************************/

/************************************************************************************************************
* Public Defines
************************************************************************************************************/

#define assert_param_notnull(ptr)	\
	UL_RETURN_ON_FALSE( \
		(ptr) != NULL, \
		UL_ERR_INVALID_ARG, \
		"Error: `" #ptr "` is NULL" \
	)

#define assert_param_size_ok(size)	\
	UL_RETURN_ON_FALSE( \
		(size) > 0, \
		UL_ERR_INVALID_ARG, \
		"Error: `" #size "` is 0" \
	)

/************************************************************************************************************
* Public Types Definitions
************************************************************************************************************/

#endif  /* INC_UL_PRIVATE_H_ */
