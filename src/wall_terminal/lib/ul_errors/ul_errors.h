/** @file ul_errors.h
 *  @brief  Created on: June 20, 2024
 *          Author: Davide Scalisi
 *          Description: Library to handle asserts and errors inspired by `esp_err.h` from ESP-IDF.
 *
 * @copyright [2024] Davide Scalisi *
 * @copyright All Rights Reserved. *
 *
*/

#ifndef INC_UL_ERRORS_H_
#define INC_UL_ERRORS_H_

/************************************************************************************************************
* Included files
************************************************************************************************************/

// Standard libraries.
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

// UniLibC.
#include <ul_configs.h>

/************************************************************************************************************
* Public Defines
************************************************************************************************************/

#ifdef UL_CONF_ERRORS_PRINT_DEBUG

/**
 * @brief Check the error code and terminate the program in case the code is not `UL_OK`.
 * Prints the error code, error location, and the failed statement to serial output.
 *
 * @note Terminate the program by calling `abort()`.
 */
#define UL_ERRORS_CHECK(x){ \
	ul_err_t rc = (x); \
	if(rc != UL_OK) \
		__ul_errors_check_failed(rc, __FILE__, __LINE__, __ASSERT_FUNC, #x); \
}

/**
 * @brief Check and print the error code, error location, and the failed statement to serial output.
 * @note Does not terminate the program.
 */
#define UL_ERRORS_CHECK_WITHOUT_ABORT(x){ \
	ul_err_t rc = (x); \
	if(rc != UL_OK) \
		__ul_errors_check_failed_without_abort(rc, __FILE__, __LINE__, __ASSERT_FUNC, #x); \
}

/**
 * @brief Check the error code. If the code is not `UL_OK`, it prints the message and returns.
 */
#define UL_RETURN_ON_ERROR(x, format, ...){ \
	ul_err_t rc = (x); \
	if(rc != UL_OK){ \
		if(__ul_errors_printf != NULL) \
			__ul_errors_printf("%s(%d): " format, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
		return rc; \
	} \
}

/**
 * @brief A version of `UL_RETURN_ON_ERROR()` macro that can be called from ISR.
 */
#define UL_RETURN_ON_ERROR_ISR(x, format, ...){ \
	ul_err_t rc = (x); \
	if(rc != UL_OK){ \
		if(__ul_errors_printf_isr != NULL) \
			__ul_errors_printf_isr("%s(%d): " format, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
		return rc; \
	} \
}

/**
 * @brief Check the error code. If the code is not `UL_OK`, it prints the message,
 * sets the local variable `ret` to `x`, and then exits by jumping to `goto_tag`.
 */
#define UL_GOTO_ON_ERROR(x, goto_tag, format, ...){ \
	ul_err_t rc = (x); \
	if(rc != UL_OK){ \
		if(__ul_errors_printf != NULL) \
			__ul_errors_printf("%s(%d): " format, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
		ret = rc; \
		goto goto_tag; \
	} \
}

/**
 * @brief A version of `UL_GOTO_ON_ERROR()` macro that can be called from ISR.
 */
#define UL_GOTO_ON_ERROR_ISR(x, goto_tag, format, ...){ \
	ul_err_t rc = (x); \
	if(rc != UL_OK){ \
		if(__ul_errors_printf_isr != NULL) \
			__ul_errors_printf_isr("%s(%d): " format, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
		ret = rc; \
		goto goto_tag; \
	} \
}

/**
 * @brief Check the condition. If the condition is not `true`, it prints the message
 * and returns with the supplied `err_code`.
 */
#define UL_RETURN_ON_FALSE(c, err_code, format, ...){ \
	if(!(c)){ \
		if(__ul_errors_printf != NULL) \
			__ul_errors_printf("%s(%d): " format, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
		return err_code; \
	} \
}

/**
 * @brief A version of `UL_RETURN_ON_FALSE()` macro that can be called from ISR.
 */
#define UL_RETURN_ON_FALSE_ISR(c, err_code, format, ...){ \
	if(!(c)){ \
		if(__ul_errors_printf_isr != NULL) \
			__ul_errors_printf_isr("%s(%d): " format, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
		return err_code; \
	} \
}

/**
 * @brief Check the condition. If the condition is not `true`, it prints the message,
 * sets the local variable `ret` to the supplied `err_code`, and then exits by jumping to `goto_tag`.
 */
#define UL_GOTO_ON_FALSE(c, err_code, goto_tag, format, ...){ \
	if(!(c)){ \
		if(__ul_errors_printf != NULL) \
			__ul_errors_printf("%s(%d): " format, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
		ret = err_code; \
		goto goto_tag; \
	} \
}

/**
 * @brief A version of `UL_GOTO_ON_FALSE()` macro that can be called from ISR.
 */
#define UL_GOTO_ON_FALSE_ISR(c, err_code, goto_tag, format, ...){ \
	if(!(c)){ \
		if(__ul_errors_printf_isr != NULL) \
			__ul_errors_printf_isr("%s(%d): " format, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
		ret = err_code; \
		goto goto_tag; \
	} \
}

#else /* UL_CONF_ERRORS_PRINT_DEBUG */

/**
 * @brief Check the error code and terminate the program in case the code is not `UL_OK`.
 * @note Terminate the program by calling `abort()`.
 */
#define UL_ERRORS_CHECK(x){ \
	ul_err_t rc = (x); \
	if(rc != UL_OK) \
		abort(); \
}

/**
 * @brief A stub.
 */
#define UL_ERRORS_CHECK_WITHOUT_ABORT(x)		x

/**
 * @brief Check the error code. If the code is not `UL_OK`, it returns.
 */
#define UL_RETURN_ON_ERROR(x, format, ...){ \
	ul_err_t rc = (x); \
	if(rc != UL_OK) \
		return rc; \
}

/**
 * @brief A version of `UL_RETURN_ON_ERROR()` macro that can be called from ISR.
 */
#define UL_RETURN_ON_ERROR_ISR(x, format, ...)	UL_RETURN_ON_ERROR(x, format, ...)

/**
 * @brief Check the error code. If the code is not `UL_OK`,
 * it sets the local variable `ret` to `x`, and then exits by jumping to `goto_tag`.
 */
#define UL_GOTO_ON_ERROR(x, goto_tag, format, ...){ \
	ul_err_t rc = (x); \
	if(rc != UL_OK){ \
		ret = rc; \
		goto goto_tag; \
	} \
}

/**
 * @brief A version of `UL_GOTO_ON_ERROR()` macro that can be called from ISR.
 */
#define UL_GOTO_ON_ERROR_ISR(x, goto_tag, format, ...)	UL_GOTO_ON_ERROR(x, goto_tag, format, ...)

/**
 * @brief Check the condition. If the condition is not `true`,
 * it returns with the supplied `err_code`.
 */
#define UL_RETURN_ON_FALSE(c, err_code, format, ...){ \
	if(!(c)) \
		return err_code; \
}

/**
 * @brief A version of `UL_RETURN_ON_FALSE()` macro that can be called from ISR.
 */
#define UL_RETURN_ON_FALSE_ISR(c, err_code, format, ...)	UL_RETURN_ON_FALSE(c, err_code, format, ...)

/**
 * @brief Check the condition. If the condition is not `true`,
 * it sets the local variable `ret` to the supplied `err_code`, and then exits by jumping to `goto_tag`.
 */
#define UL_GOTO_ON_FALSE(c, err_code, goto_tag, format, ...){ \
	if(!(c)){ \
		ret = err_code; \
		goto goto_tag; \
	} \
}

/**
 * @brief A version of `UL_GOTO_ON_FALSE()` macro that can be called from ISR.
 */
#define UL_GOTO_ON_FALSE_ISR(c, err_code, goto_tag, format, ...)	UL_GOTO_ON_FALSE(c, err_code, goto_tag, format, ...)

#endif /* UL_CONF_ERRORS_PRINT_DEBUG */

/************************************************************************************************************
* Public Types Definitions
************************************************************************************************************/

// Enumeration of the errors.
enum {
	UL_OK,										// Generic `ul_err_t` code indicating success.
	UL_FAIL,									// Generic `ul_err_t` code indicating failure.

	UL_ERR_NO_MEM,						// Out of memory.
	UL_ERR_INVALID_ARG,				// Invalid argument.
	UL_ERR_INVALID_STATE,			// Invalid state.
	UL_ERR_INVALID_SIZE,			// Invalid size.
	UL_ERR_NOT_FOUND,					// Requested resource not found.
	UL_ERR_NOT_SUPPORTED,			// Operation or feature not supported.
	UL_ERR_TIMEOUT,						// Operation timed out.
	UL_ERR_INVALID_RESPONSE,	// Received response was invalid.
	UL_ERR_INVALID_CRC,				// CRC or checksum was invalid.
	UL_ERR_INVALID_VERSION,		// Version was invalid.
	UL_ERR_INVALID_MAC,				// MAC address was invalid.
	UL_ERR_NOT_FINISHED,			// Operation has not fully completed.
	UL_ERR_NOT_ALLOWED				// Operation is not allowed.
};

// Type of the errors.
typedef uint8_t ul_err_t;

/**
 * @brief Callback to hook the default and ISR printf functions.
*/
typedef int (*ul_errors_printf_callback_t)(const char *format, ...);

/************************************************************************************************************
* Public Variables Prototypes
************************************************************************************************************/

#ifdef UL_CONF_ERRORS_PRINT_DEBUG

extern ul_errors_printf_callback_t __ul_errors_printf;
extern ul_errors_printf_callback_t __ul_errors_printf_isr;

#endif /* UL_CONF_ERRORS_PRINT_DEBUG */

/************************************************************************************************************
* Public Functions Prototypes
************************************************************************************************************/

#ifdef UL_CONF_ERRORS_PRINT_DEBUG

extern void __ul_errors_check_failed(ul_err_t rc, const char *file, int line, const char *function, const char *expression) __attribute__((__noreturn__));
extern void __ul_errors_check_failed_without_abort(ul_err_t rc, const char *file, int line, const char *function, const char *expression);

/**
 * @brief Initialize the library.
 * @param default_printf_callback Default printf callback.
 * @param isr_printf_callback ISR printf callback. If `NULL` is passed, `isr_printf_callback` will also be `default_printf_callback`.
 * @return `true` if the initialization parameters are valid, `false` otherwise.
 */
extern bool ul_errors_begin(ul_errors_printf_callback_t default_printf_callback, ul_errors_printf_callback_t isr_printf_callback);

#else /* UL_CONF_ERRORS_PRINT_DEBUG */

/**
 * @brief A stub.
 */
extern bool ul_errors_begin(ul_errors_printf_callback_t default_printf_callback, ul_errors_printf_callback_t isr_printf_callback);

#endif /* UL_CONF_ERRORS_PRINT_DEBUG */
#endif /* INC_UL_ERRORS_H_ */
