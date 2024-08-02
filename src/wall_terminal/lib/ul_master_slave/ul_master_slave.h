/** @file ul_master_slave.h
 *  @brief  Created on: Aug 1, 2024
 *          Davide Scalisi
 *
 * 					Description:	Minimal library implementing a simple non-ambiguous
 * 												data encoding which can be sent over an half-duplex
 * 												shared physical bus, like half-duplex RS-485.
 *
 * 					Note:					The library encodes data from 8 bits per byte to
 * 												7 bits per byte, using the MSb to distinguish
 * 												transmissions from the master (MSb = 1) from
 * 												transmissions from slaves (MSb = 0).
 *
 * @copyright [2024] Davide Scalisi *
 * @copyright All Rights Reserved. *
 *
*/

#ifndef INC_UL_MASTER_SLAVE_H_
#define INC_UL_MASTER_SLAVE_H_

/************************************************************************************************************
* Included files
************************************************************************************************************/

// Standard libraries.
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

// UniLibC.
#include <ul_configs.h>
#include <ul_errors.h>
#include <ul_utils.h>

/************************************************************************************************************
* Public Defines
************************************************************************************************************/

// Conditional typings.
#ifdef UL_CONF_MS_UINT8_IS_BUF_SIZE_T
	#define UL_MS_BUF_SIZE_T	uint32_t
#else
	#define UL_MS_BUF_SIZE_T	uint8_t
#endif

/**
 * @brief Check if the given byte was encoded using `ul_ms_encode_master_message()`.
 */
#define ul_ms_is_master_byte(b)	((b & 0x80) != 0)

/**
 * @brief Forces the MSb to 1.
 */
#define ul_ms_encode_master_byte(b)	(b | 0x80)

/**
 * @brief Forces the MSb to 0.
 */
#define ul_ms_decode_master_byte(b)	(b & 0x7F)

/**
 * @brief Forces the MSb to 0.
 */
#define ul_ms_encode_slave_byte(b)	ul_ms_decode_master_byte(b)

/**
 * @brief Forces the MSb to 0.
 */
#define ul_ms_decode_slave_byte(b)	ul_ms_decode_master_byte(b)

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
 * @brief Compute the `dest_buf_size` given the `src_buf_size` for the function `ul_ms_encode_master_message()`.
 * @return The minimum length of `dest_buf` on the function `ul_ms_encode_master/slave_message()`.
 */
extern UL_MS_BUF_SIZE_T ul_ms_compute_encoded_size(UL_MS_BUF_SIZE_T src_buf_size);

/**
 * @brief Compute the `dest_buf_size` given the `src_buf_size` for the function `ul_ms_decode_master_message()`.
 * @return The minimum length of `dest_buf` on the function `ul_ms_decode_master/slave_message()`.
 */
extern UL_MS_BUF_SIZE_T ul_ms_compute_decoded_size(UL_MS_BUF_SIZE_T src_buf_size);

/**
 * @brief Encode a message from the master.
 * @param dest_buf Destination buffer.
 * @param src_buf Source buffer.
 * @param src_buf_size `sizeof(src_buf)`.
 * @note `sizeof(dest_buf)` is computed by `ul_ms_compute_encoded_size(src_buf_size)`.
 */
extern ul_err_t ul_ms_encode_master_message(uint8_t *dest_buf, uint8_t *src_buf, UL_MS_BUF_SIZE_T src_buf_size);

/**
 * @brief Decode a message from the master.
 * @param dest_buf Destination buffer.
 * @param src_buf Source buffer.
 * @param src_buf_size `sizeof(src_buf)`.
 * @note `sizeof(dest_buf)` is computed by `ul_ms_compute_decoded_size(src_buf_size)`.
 */
extern ul_err_t ul_ms_decode_master_message(uint8_t *dest_buf, uint8_t *src_buf, UL_MS_BUF_SIZE_T src_buf_size);

/**
 * @brief Encode a message from a slave.
 * @param dest_buf Destination buffer.
 * @param src_buf Source buffer.
 * @param src_buf_size `sizeof(src_buf)`.
 * @note `sizeof(dest_buf)` is computed by `ul_ms_compute_encoded_size(src_buf_size)`.
 */
extern ul_err_t ul_ms_encode_slave_message(uint8_t *dest_buf, uint8_t *src_buf, UL_MS_BUF_SIZE_T src_buf_size);

/**
 * @brief Decode a message from a slave.
 * @param dest_buf Destination buffer.
 * @param src_buf Source buffer.
 * @param src_buf_size `sizeof(src_buf)`.
 * @note `sizeof(dest_buf)` is computed by `ul_ms_compute_decoded_size(src_buf_size)`.
 */
extern ul_err_t ul_ms_decode_slave_message(uint8_t *dest_buf, uint8_t *src_buf, UL_MS_BUF_SIZE_T src_buf_size);

#endif  /* INC_UL_MASTER_SLAVE_H_ */
