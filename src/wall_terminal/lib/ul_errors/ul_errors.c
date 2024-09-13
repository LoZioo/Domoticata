/** @file ul_errors.c
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

#include <ul_errors.h>

/************************************************************************************************************
* Private Defines
************************************************************************************************************/

#ifdef UL_CONFIG_ERRORS_PRINT_DEBUG

// Shortcuts.
#define printf_def	__ul_errors_printf
#define printf_isr	__ul_errors_printf_isr

// Suppress the warning emitted at linking time.
#ifdef __AVR_ATmega2560__
	#define __builtin_ret_addr(n)		NULL
#else
	#define __builtin_ret_addr			__builtin_return_address
#endif

/**
 * @brief Check if `ptr` is not `NULL`.
 * @param ptr The pointer to be checked.
 * @param ... The value to be returned by the function in case of this assert fails.
*/
#define __assert_notnull(ptr, ...) \
	if(ptr == NULL) \
		return __VA_ARGS__

#define SERIAL_CLEAR() \
	for(int i=0; i<10; i++) \
			printf_def("\n"); \
		for(int i=0; i<25; i++) \
			printf_def("-"); \
		printf_def(" BEGIN APP LOG "); \
		for(int i=0; i<25; i++) \
			printf_def("-"); \
		printf_def("\n")

#endif /* UL_CONFIG_ERRORS_PRINT_DEBUG */

/************************************************************************************************************
* Private Types Definitions
 ************************************************************************************************************/

/************************************************************************************************************
* Private Variables
 ************************************************************************************************************/

/************************************************************************************************************
* Public Variables
 ************************************************************************************************************/

#ifdef UL_CONFIG_ERRORS_PRINT_DEBUG

ul_errors_printf_callback_t __ul_errors_printf = NULL;
ul_errors_printf_callback_t __ul_errors_printf_isr = NULL;

#endif /* UL_CONFIG_ERRORS_PRINT_DEBUG */

/************************************************************************************************************
* Private Functions Prototypes
 ************************************************************************************************************/

#ifdef UL_CONFIG_ERRORS_PRINT_DEBUG

static void __ul_errors_check_failed_print(ul_err_t rc, const char *file, int line, const char *function, const char *expression);

#endif /* UL_CONFIG_ERRORS_PRINT_DEBUG */

/************************************************************************************************************
* Private Functions Definitions
 ************************************************************************************************************/

#ifdef UL_CONFIG_ERRORS_PRINT_DEBUG

void __ul_errors_check_failed_print(ul_err_t rc, const char *file, int line, const char *function, const char *expression){
	__assert_notnull(printf_def);

	printf_def("\nUL_ERROR_CHECK failed: ul_err_t #%u at %p\n", rc, __builtin_ret_addr(0));
	printf_def("file: \"%s\" line %d\nfunc: %s\nexpression: %s\n", file, line, function, expression);
}

#endif /* UL_CONFIG_ERRORS_PRINT_DEBUG */

/************************************************************************************************************
* Public Functions Definitions
 ************************************************************************************************************/

/* Helpers */

#ifdef UL_CONFIG_ERRORS_PRINT_DEBUG

void __ul_errors_check_failed(ul_err_t rc, const char *file, int line, const char *function, const char *expression){
	__ul_errors_check_failed_print(rc, file, line, function, expression);
	abort();
}

void __ul_errors_check_failed_without_abort(ul_err_t rc, const char *file, int line, const char *function, const char *expression){
	__ul_errors_check_failed_print(rc, file, line, function, expression);
}

#endif /* UL_CONFIG_ERRORS_PRINT_DEBUG */

bool ul_errors_begin(ul_errors_printf_callback_t default_printf_callback, ul_errors_printf_callback_t isr_printf_callback){
	#ifdef UL_CONFIG_ERRORS_PRINT_DEBUG
	__assert_notnull(default_printf_callback, false);

	printf_def = default_printf_callback;
	printf_isr = (isr_printf_callback == NULL ? default_printf_callback : isr_printf_callback);

	SERIAL_CLEAR();

	#endif /* UL_CONFIG_ERRORS_PRINT_DEBUG */
	return true;
}
