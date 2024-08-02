/** @file ul_master_slave.h
 *  @brief  Created on: Aug 1, 2024
 *          Davide Scalisi
 *
 * 					Description:	Minimal library implementing a simple non-ambiguous
 * 												data encoding which can be sent on an half-duplex
 * 												shared bus, like half-duplex RS-485.
 * 
 * 					Note:					The encoding simply consists forcing the MSb of every byte
 * 												to 1 when the master is transmitting and to 0 when a slave
 * 												is transmitting.
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

// UniLibC.
#include <ul_errors.h>
#include <ul_utils.h>

/************************************************************************************************************
* Public Defines
************************************************************************************************************/

/**
 * @brief Check if the given byte was encoded using `ul_ms_encode_master_message()`.
 */
#define ul_ms_is_master_byte(b)	((b & 0x80) != 0)

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
extern uint32_t ul_ms_compute_encoded_size(uint32_t src_buf_size);

/**
 * @brief Compute the `dest_buf_size` given the `src_buf_size` for the function `ul_ms_decode_master_message()`.
 * @return The minimum length of `dest_buf` on the function `ul_ms_decode_master/slave_message()`.
 */
extern uint32_t ul_ms_compute_decoded_size(uint32_t src_buf_size);

/**
 * @brief Encode a message from the master.
 * @param dest_buf Destination buffer.
 * @param src_buf Source buffer.
 * @param src_buf_size `sizeof(src_buf)`.
 * @note `sizeof(dest_buf)` is computed by `ul_ms_compute_encoded_size(src_buf_size)`.
 */
extern ul_err_t ul_ms_encode_master_message(uint8_t *dest_buf, uint8_t *src_buf, uint32_t src_buf_size);

/**
 * @brief Decode a message from the master.
 * @param dest_buf Destination buffer.
 * @param src_buf Source buffer.
 * @param src_buf_size `sizeof(src_buf)`.
 * @note `sizeof(dest_buf)` is computed by `ul_ms_compute_decoded_size(src_buf_size)`.
 */
extern ul_err_t ul_ms_decode_master_message(uint8_t *dest_buf, uint8_t *src_buf, uint32_t src_buf_size);

/**
 * @brief Encode a message from a slave.
 * @param dest_buf Destination buffer.
 * @param src_buf Source buffer.
 * @param src_buf_size `sizeof(src_buf)`.
 * @note `sizeof(dest_buf)` is computed by `ul_ms_compute_encoded_size(src_buf_size)`.
 */
extern ul_err_t ul_ms_encode_slave_message(uint8_t *dest_buf, uint8_t *src_buf, uint32_t src_buf_size);

/**
 * @brief Decode a message from a slave.
 * @param dest_buf Destination buffer.
 * @param src_buf Source buffer.
 * @param src_buf_size `sizeof(src_buf)`.
 * @note `sizeof(dest_buf)` is computed by `ul_ms_compute_decoded_size(src_buf_size)`.
 */
extern ul_err_t ul_ms_decode_slave_message(uint8_t *dest_buf, uint8_t *src_buf, uint32_t src_buf_size);

#endif  /* INC_UL_MASTER_SLAVE_H_ */
