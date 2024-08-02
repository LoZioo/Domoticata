/** @file ul_master_slave.c
 *  @brief  Created on: Aug 1, 2024
 *          Davide Scalisi
 *
 * @copyright [2024] Davide Scalisi *
 * @copyright All Rights Reserved. *
 *
*/

/************************************************************************************************************
* Included files
************************************************************************************************************/

#include <ul_master_slave.h>

/************************************************************************************************************
* Private Defines
************************************************************************************************************/

/**
 * @brief Size assert for the encode/decode functions and the size computation functions.
 */
#define __size_assert() \
	UL_RETURN_ON_FALSE( \
		src_buf_size > 0, \
		UL_ERR_INVALID_ARG, \
		"Error: `src_buf_size` is 0" \
	)

/**
 * @brief Pointer asserts for the encode/decode functions.
 */
#define __ptr_asserts() \
	UL_RETURN_ON_FALSE( \
		dest_buf != NULL, \
		UL_ERR_INVALID_ARG, \
		"Error: `dest_buf` is NULL" \
	); \
	UL_RETURN_ON_FALSE( \
		src_buf != NULL, \
		UL_ERR_INVALID_ARG, \
		"Error: `src_buf` is NULL" \
	)

/**
 * @brief Force the MSb to 1.
 */
#define __encode_master_byte(b)	(b | 0x80)

/**
 * @brief Force the MSb to 0.
 */
#define __decode_master_byte(b)	(b & 0x7F)

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

uint32_t ul_ms_compute_encoded_size(uint32_t src_buf_size){
	__size_assert();

	// Bit size.
	src_buf_size *= 8;

	return (
		(src_buf_size / 7) +
		(src_buf_size % 7 != 0)
	);
}

uint32_t ul_ms_compute_decoded_size(uint32_t src_buf_size){
	__size_assert();
	return ((src_buf_size * 7) / 8);
}

ul_err_t ul_ms_encode_master_message(uint8_t *dest_buf, uint8_t *src_buf, uint32_t src_buf_size){
	__size_assert();
	__ptr_asserts();

	uint8_t tmp;
	for(uint32_t i=0; i<ul_ms_compute_encoded_size(src_buf_size); i++){
		tmp = ul_utils_get_bit_group()
	}

	return UL_OK;
}

ul_err_t ul_ms_decode_master_message(uint8_t *dest_buf, uint8_t *src_buf, uint32_t src_buf_size){
	__size_assert();
	__ptr_asserts();

	return UL_OK;
}

ul_err_t ul_ms_encode_slave_message(uint8_t *dest_buf, uint8_t *src_buf, uint32_t src_buf_size){
	__size_assert();
	__ptr_asserts();

	return UL_OK;
}

ul_err_t ul_ms_decode_slave_message(uint8_t *dest_buf, uint8_t *src_buf, uint32_t src_buf_size){
	__size_assert();
	__ptr_asserts();

	return UL_OK;
}
