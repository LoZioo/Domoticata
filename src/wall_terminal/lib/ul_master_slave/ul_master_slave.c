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

/************************************************************************************************************
* Private Types Definitions
 ************************************************************************************************************/

/************************************************************************************************************
* Private Variables
 ************************************************************************************************************/

/************************************************************************************************************
* Private Functions Prototypes
 ************************************************************************************************************/

static ul_err_t __encode_message(uint8_t *dest_buf, uint8_t *src_buf, UL_MS_BUF_SIZE_T src_buf_size, bool master);
static ul_err_t __decode_message(uint8_t *dest_buf, uint8_t *src_buf, UL_MS_BUF_SIZE_T src_buf_size, bool master);

/************************************************************************************************************
* Private Functions Definitions
 ************************************************************************************************************/

ul_err_t __encode_message(uint8_t *dest_buf, uint8_t *src_buf, UL_MS_BUF_SIZE_T src_buf_size, bool master){
	assert_param_notnull(dest_buf);
	assert_param_notnull(src_buf);
	assert_param_size_ok(src_buf_size);

	uint32_t bit_index = 0;
	uint8_t encoded_byte, j;
	UL_MS_BUF_SIZE_T i, dest_buf_size = ul_ms_compute_encoded_size(src_buf_size);

	memset(dest_buf, 0, dest_buf_size);

	for(i=0; i<dest_buf_size; i++){
		encoded_byte = 0;

		for(j=0; j<7; j++)
			if(bit_index / 8 < src_buf_size){
				encoded_byte |= ((src_buf[bit_index / 8] >> (bit_index % 8)) & 0x01) << j;
				bit_index++;
			}

		if(master)
			encoded_byte |= 0x80;

		dest_buf[i] = encoded_byte;
	}

	return UL_OK;
}

ul_err_t __decode_message(uint8_t *dest_buf, uint8_t *src_buf, UL_MS_BUF_SIZE_T src_buf_size, bool master){
	assert_param_notnull(dest_buf);
	assert_param_notnull(src_buf);
	assert_param_size_ok(src_buf_size);

	uint32_t bit_index = 0;
	uint8_t decoded_byte, j;
	UL_MS_BUF_SIZE_T i, dest_buf_size = ul_ms_compute_decoded_size(src_buf_size);

	memset(dest_buf, 0, dest_buf_size);

	for(i=0; i<src_buf_size; i++)
		for(j=0; j<7; j++)
			if(bit_index / 8 < dest_buf_size){
				decoded_byte = (src_buf[i] >> j) & 0x01;

				if(!master)
					decoded_byte &= 0x7F;

				dest_buf[bit_index / 8] |= decoded_byte << (bit_index % 8);
				bit_index++;
			}

	return UL_OK;
}

/************************************************************************************************************
* Public Functions Definitions
 ************************************************************************************************************/

UL_MS_BUF_SIZE_T ul_ms_compute_encoded_size(UL_MS_BUF_SIZE_T src_buf_size){
	assert_param_size_ok(src_buf_size);

	// Bit size.
	src_buf_size *= 8;

	return (
		(src_buf_size / 7) +
		(src_buf_size % 7 != 0)
	);
}

UL_MS_BUF_SIZE_T ul_ms_compute_decoded_size(UL_MS_BUF_SIZE_T src_buf_size){
	assert_param_size_ok(src_buf_size);
	return ((src_buf_size * 7) / 8);
}

ul_err_t ul_ms_encode_master_message(uint8_t *dest_buf, uint8_t *src_buf, UL_MS_BUF_SIZE_T src_buf_size){
	return __encode_message(dest_buf, src_buf, src_buf_size, true);
}

ul_err_t ul_ms_decode_master_message(uint8_t *dest_buf, uint8_t *src_buf, UL_MS_BUF_SIZE_T src_buf_size){
	return __decode_message(dest_buf, src_buf, src_buf_size, true);
}

ul_err_t ul_ms_encode_slave_message(uint8_t *dest_buf, uint8_t *src_buf, UL_MS_BUF_SIZE_T src_buf_size){
	return __encode_message(dest_buf, src_buf, src_buf_size, false);
}

ul_err_t ul_ms_decode_slave_message(uint8_t *dest_buf, uint8_t *src_buf, UL_MS_BUF_SIZE_T src_buf_size){
	return __decode_message(dest_buf, src_buf, src_buf_size, false);
}
