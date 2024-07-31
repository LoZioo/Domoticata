/** @file ul_utils.h
 *  @brief  Created on: June 20, 2024
 *          Author: Davide Scalisi
 *          Description: Simple set of utility functions.
 *
 * @copyright [2024] Davide Scalisi *
 * @copyright All Rights Reserved. *
 *
*/

#ifndef INC_UTILS_H_
#define INC_UTILS_H_

/************************************************************************************************************
* Included files
************************************************************************************************************/

// Standard libraries.
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>

// UniLibC.
#include <ul_errors.h>

/************************************************************************************************************
* Public Defines
************************************************************************************************************/

/* Conditional types */

#ifdef __AVR_ATtiny13A__
	#define millis_ret_t	uint32_t
#else
	#define millis_ret_t	unsigned long
#endif

/* Constants */

#define UL_UTILS_DEG_TO_RAD		0.017453292519943295769236907684886
#define UL_UTILS_RAD_TO_DEG		57.295779513082320876798154814105

/* Math */

/**
 * @brief Converts degrees to radians.
*/
#define ul_utils_radians(deg) \
	(((float) deg) * UL_UTILS_DEG_TO_RAD)

/**
 * @brief Converts radians to degrees.
*/
#define ul_utils_degrees(rad) \
	(((float) rad) * UL_UTILS_RAD_TO_DEG)

/**
 * @brief Constrains `x` between `low` and `high`.
*/
#define	ul_utils_constrain(x, low, high)( \
	(x) < (low) ? \
	(low) : \
	( \
		(x) > (high) ? \
		(high) : \
		(x) \
	) \
)

/* Bitwise */

/**
 * @brief Get a specific group of bits from a number.
 * @param n The source number.
 * @param s Size of the group.
 * @param i Get the i-th group (from left to right).
 * @return The extracted bits saved from the LSb to the bit of index `s`.
 */
#define ul_utils_get_bit_group(n, s, i)( \
	((n) >> ((s) * (i))) & ((1U << (s)) - 1) \
)

/**
 * @brief Set a specific group of bits to a number.
 * @param n The source number.
 * @param v The new bits saved from the LSb to the bit of index `s`.
 * @param s Size of the group.
 * @param i Save into the i-th group (from left to right).
 * @return `v` with the specified group of bits altered.
 */
#define ul_utils_set_bit_group(n, v, s, i) ( \
	((n) & ~(((1U << (s)) - 1) << ((s) * (i)))) | \
	(((v) & ((1U << (s)) - 1)) << ((s) * (i))) \
)

/* Contition statements */

/**
 * @brief Check if `x` is in the interval [`low`, `high`].
 * @return The contition statement.
*/
#define ul_utils_between(x, low, high)( \
	((x) >= (low)) && \
	((x) <= (high)) \
)

/**
 * @brief Traslates to the logical statement `(x op z) and (y op z)`
 * @param op The logical operator to be used (>, ==, &&...).
*/
#define ul_utils_both(x, y, op, z)( \
	((x) op (z)) && \
	((y) op (z)) \
)

/**
 * @brief Traslates to the logical statement `(x op z) or (y op z)`
 * @param op The logical operator to be used (>, ==, &&...).
*/
#define ul_utils_either(x, y, op, z)( \
	((x) op (z)) || \
	((y) op (z)) \
)

/**
 * @brief Traslates to the logical statement `not(x op z) and not(y op z)`
 * @param op The logical operator to be used (>, ==, &&...).
*/
#define ul_utils_neither(x, y, op, z)( \
	!ul_utils_either(x, y, op, z) \
)

/* Type cast */

/**
 * @brief Cast the memory allocated for the buffer `buf` to a specific type.
 * @param buf Pointer to the memory buffer (a generic array of bytes).
 * @param type The type name (`float`, `int16_t`, ...).
 * @note It's your responsibility to check if `sizeof(buf) >= sizeof(type)` to avoid segmentation faults.
*/
#define ul_utils_cast_to_type(buf, type)( \
	*(type*) (buf) \
)

/**
 * @brief Cast the memory allocated for the variable `mem` to an array of bytes.
 * @param mem The generic variable.
*/
#define ul_utils_cast_to_mem(mem)( \
 (uint8_t*) &(mem) \
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

/* Math */

/**
 * @brief Shift the integer number `n` from the discrete range [`in_min`, `in_max`] to [`out_min`, `out_max`].
*/
extern int ul_utils_map_int(int n, int in_min, int in_max, int out_min, int out_max);

/**
 * @brief Shift the real number `x` from the continuous range [`in_min`, `in_max`] to [`out_min`, `out_max`].
*/
extern float ul_utils_map_float(float x, float in_min, float in_max, float out_min, float out_max);

/**
 * @brief Multiple arguments XOR.
*/
extern uint8_t ul_utils_multi_xor(uint32_t argc, ...);

/**
 * @brief Module operator with positive and also negative numbers.
*/
extern int ul_utils_mod(int a, int b);

/**
 * @brief Angle normalization between [-PI, PI].
*/
extern float ul_utils_normalize_angle(float a);

/* Time */

/**
 * @brief Freeze the caller execution for at least `ms` milliseconds. If you want, you can internally interrupt the delay by returning `false` from the `background_routine()`.
 * @param ms The number of milliseconds to pause.
 * @param millis_routine This routine must return the number of milliseconds passed when called.
 * @param time_counter A pointer to a variable needed to keep track of the passing time. You should keep one associated with each `background_routine()`.
 * @param background_routine The background routine that must be executed repeatedly during the time delay. Return `true` if the the delay must go on, otherwise return `false` to explicitly interrupt the delay.
 * @return `true` if at least `ms` milliseconds have passed. `false` if the delay was explicitly interrupted by returning `false` from the `background_routine()` or if there are some parameter errors.
 * @note No explicit blocking time delays are allowed inside the `background_routine()`: treat it like an ISR.
 */
bool ul_utils_delay_nonblock(uint16_t ms, millis_ret_t (*millis_routine)(), uint32_t *time_counter, bool (*background_routine)());

/* Contitions */

/**
 * @brief Check if `x` is equal to, at least, one passed variable parameter.
 * @param argc The number of passed variable parameters.
*/
extern bool ul_utils_in(int x, uint8_t argc, ...);

/* Strings */

/**
 * @brief Converts a floating point number to its string representation.
 * @param x The floating point to be converted.
 * @param str The destination string; ensure it's big enough.
 * @param afterpoint The number of decimal places.
 * @return `ul_err_t`
*/
extern ul_err_t ul_utils_ftoa(float x, char *str, uint8_t afterpoint);

/**
 * @brief Converts an `uint32_t` to its binary string representation.
 * @param n The `uint32_t` to be converted.
 * @param str The string where to store the binary representation; ensure it's big enough.
 * @param byte_len The number of bytes to represent (from 1 to 4).
 * @return `ul_err_t`
 */
extern ul_err_t ul_utils_int_to_bin_str(uint32_t n, char *str, uint8_t byte_len);

/**
 * @brief Converts a bytes buffer to its string representation.
 * @param buf The buffer of bytes.
 * @param size `sizeof(buf)`
 * @param str The destination string; ensure it's big enough.
 * @param base The base of the number; supported values are 2, 10 and 16.
 * @return `ul_err_t`
*/
extern ul_err_t ul_utils_buf_to_str(uint8_t *buf, uint32_t size, char *str, uint8_t base);

/* Arrays */

/**
 * This function slices the given buffer into parts of the specified size and returns
 * each slice in the provided buffer. Subsequent calls to the function can be made
 * to continue extracting slices from the remaining data by set buffer to `NULL`.
 *
 * @param buf Pointer to the input buffer.
 * @param size Size of the input buffer.
 * @param slice Pointer to the output buffer (slice).
 * @param slice_size Pointer to the wanted size of the slice; modified to the actual slice size.
 * @return Return `true` if a slice is successfully extracted; `false` if no data remains or, on the first call, if there are some parameters errors.
 *
 * @note	To use this function, initialize the buffer and call the function with the buffer,
 *				its size, an output buffer (slice), and the desired slice size. Subsequent calls
 *				can be made with the buffer set to `NULL` to continue extracting slices.
 *
 * @example
 *	uint8_t buf[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
 *	uint32_t slice_size = 4;
 *	uint8_t slice[slice_size];
 *
 *	bool process_slice = ul_utils_slice_buf(buf, sizeof(buf), slice, &slice_size);
 *	while(process_slice){
 *		// ...
 *
 *		printf("[");
 *		for(int i=0; i<slice_size; i++)
 *			printf("%d, ", slice[i]);
 *		printf("]\n");
 *
 *		process_slice = ul_utils_slice_buf(NULL, 0, slice, &slice_size);
 *	}
 */
extern bool ul_utils_slice_buf(uint8_t *buf, uint32_t size, uint8_t *slice, uint32_t *slice_size);

/**
 * @brief Encode a variable series of arguments into a buffer (call this function from within another variable arguments function).
 * @param argv The destination buffer; ensure it's big enough.
 * @param argc The number of variable arguments.
 * @param args The `va_list` after `va_start()` and before `va_end()`.
 * @return `ul_err_t`
*/
extern ul_err_t ul_utils_build_buf_from_uint8_va_list(uint8_t *argv, uint32_t argc, va_list args);

/**
 * @brief Encode a variable series of arguments into a buffer.
 * @param argv The destination buffer; ensure it's big enough.
 * @param argc The number of variable arguments.
 * @return `ul_err_t`
*/
extern ul_err_t ul_utils_build_buf_from_uint8_vargs(uint8_t *argv, uint32_t argc, ...);

#endif /* INC_UTILS_H_ */
