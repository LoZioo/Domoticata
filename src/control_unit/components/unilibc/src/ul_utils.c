/** @file ul_utils.c
 *  @brief  Created on: June 20, 2024
 *          Author: Davide Scalisi
 *
 * @copyright [2024] Davide Scalisi *
 * @copyright All Rights Reserved. *
 *
*/

/************************************************************************************************************
* Included files
************************************************************************************************************/

#include <ul_utils.h>

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

/* Math */

int ul_utils_map_int(int n, int in_min, int in_max, int out_min, int out_max){
	return (n - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

float ul_utils_map_float(float x, float in_min, float in_max, float out_min, float out_max){
	return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

uint8_t ul_utils_multi_xor(uint32_t argc, ...){
	if(argc == 0)
		return 0;

	va_list argv;
	va_start(argv, argc);

	uint8_t res = 0;
	for(uint32_t i=0; i<argc; i++)
		res ^= (uint8_t) va_arg(argv, int);

	va_end(argv);
	return res;
}

int ul_utils_mod(int a, int b){
	int res = a % b;
	return res<0 ? res+b : res;
}

float ul_utils_normalize_angle(float rad){
	while(rad > M_PI)
		rad -= 2*M_PI;
	while(rad < -M_PI)
		rad += 2*M_PI;
	return rad;
}

/* Time */

bool ul_utils_delay_nonblock(uint16_t ms, millis_ret_t (*millis_routine)(), uint32_t *time_counter, bool (*background_routine)()){

	if(
		millis_routine == NULL ||
		time_counter == NULL ||
		background_routine == NULL
	)
		return false;

	bool background_routine_return_value;
	*time_counter = millis_routine();

	do background_routine_return_value = background_routine();
	while(
		background_routine_return_value &&
		millis_routine() - (*time_counter) < ms
	);

	return background_routine_return_value;
}

/* Contitions */

bool ul_utils_in(int x, uint8_t argc, ...){
	if(argc == 0)
		return false;

	va_list argv;
	va_start(argv, argc);

	int i = 0;
	bool found = false;

	while(i<argc && !found){
		if( x == ((int) va_arg(argv, int)) )
			found = true;

		else
			i++;
	}

	va_end(argv);
	return found;
}

/* Strings */

ul_err_t ul_utils_ftoa(float x, char *str, uint8_t afterpoint){
	UL_RETURN_ON_FALSE(str != NULL, UL_ERR_INVALID_ARG, "Error: `str` is NULL");

	int ipart = (int) x;
	float fpart = x - (float) ipart;

	itoa(ipart, str, 10);
	strcat(str, ".");

	float multiplier = 1.0;
	for(uint8_t i=0; i<afterpoint; i++)
		multiplier *= 10.0;

	int fpart_int = (int) (fpart * multiplier);
	itoa(fpart_int, str + strlen(str), 10);

	return UL_OK;
}

ul_err_t ul_utils_int_to_bin_str(uint32_t n, char *str, uint8_t byte_len){
	UL_RETURN_ON_FALSE(str != NULL, UL_ERR_INVALID_ARG, "Error: `str` is NULL");
	UL_RETURN_ON_FALSE(!ul_utils_between(byte_len, 1, 4), UL_ERR_INVALID_ARG, "Error: `byte_len` is not between 1 and 4");

	// Assuming str is large enough to hold the binary representation.
	str[(byte_len * 9) - 1] = '\0';				// Set the last character of the string as the null terminator.

	for(int i=byte_len*8 - 1; i>=0; --i){
		str[i + i/8] = (n & 1) + '0';				// Get the least significant bit and convert it to a character.
		n >>= 1;														// Right-shift to move to the next bit.

		// Insert a space after each byte.
		if(i % 8 == 0 && i > 0)
			str[i + i/8 - 1] = ' ';
	}

	return UL_OK;
}

ul_err_t ul_utils_buf_to_str(uint8_t *buf, uint32_t size, char *str, uint8_t base){
	UL_RETURN_ON_FALSE(buf != NULL, UL_ERR_INVALID_ARG, "Error: `buf` is NULL");
	UL_RETURN_ON_FALSE(str != NULL, UL_ERR_INVALID_ARG, "Error: `str` is NULL");
	UL_RETURN_ON_FALSE(!ul_utils_in(base, 3, 2, 10, 16), UL_ERR_INVALID_ARG, "Error: `base` can be only {2, 10, 16}");

	strcpy(str, "[");
	char tmp[10];

	for(uint32_t i=0; i<size; i++){
		switch(base){
			case 2:
				UL_RETURN_ON_ERROR(
					ul_utils_int_to_bin_str(buf[i], tmp, 1),
					"Error on `ul_utils_int_to_bin_str()`"
				);
				break;

			case 10:
				sprintf(tmp, "%u", buf[i]);
				break;

			case 16:
				sprintf(tmp, "%02X", buf[i]);
				break;

			default:
				break;
		}

		strcat(str, tmp);
		if(i < size-1)
			strcat(str, ", ");
	}

	strcat(str, "]");
	return UL_OK;
}

/* Arrays */

bool ul_utils_slice_buf(uint8_t *buf, uint32_t size, uint8_t *slice, uint32_t *slice_size){
	if(ul_utils_either(slice, slice_size, ==, NULL))
		return false;

	static uint8_t *current_position = NULL;
	static size_t remaining_size = 0;

	// If it's the first call, initialize the static data.
	if(buf != NULL){
		current_position = buf;
		remaining_size = size;
	}

	// Check if there is data remaining in the buffer.
	if(current_position == NULL || remaining_size == 0)
		return false;

	// Calculate the size of the next slice.
	size_t next_slice_size = (*slice_size < remaining_size) ? *slice_size : remaining_size;

	// Copy data from the current location to the slice buffer.
	memcpy(slice, current_position, next_slice_size);

	// Update current position and remaining size.
	current_position += next_slice_size;
	remaining_size -= next_slice_size;

	// Update the actual slice size.
	*slice_size = next_slice_size;

	// Extraction complete.
	return true;
}

ul_err_t ul_utils_build_buf_from_uint8_va_list(uint8_t *argv, uint32_t argc, va_list args){
	UL_RETURN_ON_FALSE(argv != NULL, UL_ERR_INVALID_ARG, "Error: `argv` is NULL");

	for(uint32_t i=0; i<argc; i++)
		argv[i] = (uint8_t) va_arg(args, int);	// Every integer variable argument is automatically promoted to the `int` type.

	return UL_OK;
}

ul_err_t ul_utils_build_buf_from_uint8_vargs(uint8_t *argv, uint32_t argc, ...){
	va_list args;
	va_start(args, argc);

	UL_RETURN_ON_ERROR(
		ul_utils_build_buf_from_uint8_va_list(argv, argc, args),
		"Error on `ul_utils_build_buf_from_uint8_va_list()`"
	);

	va_end(args);
	return UL_OK;
}
