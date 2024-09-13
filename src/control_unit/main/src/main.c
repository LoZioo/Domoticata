/* USER CODE BEGIN Header */
/**
	******************************************************************************
	* @file           : main.c
	* @brief          : main program body.
	******************************************************************************
	* @attention
	*
	* Copyright (c) [2024] Davide Scalisi *
	* All rights reserved.
	*
	* This software is licensed under terms that can be found in the LICENSE file
	* in the root directory of this software component.
	* If no LICENSE file comes with this software, it is provided AS-IS.
	*
	******************************************************************************
	*/
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
/* USER CODE BEGIN Includes */

// Standard libraries.
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

// Platform libraries.
#include <esp_err.h>
#include <esp_check.h>
#include <esp_log.h>

// Project libraries.
#include <main.h>
#include <gpio.h>
#include <rs485.h>
#include <pwm.h>
#include <wifi.h>
#include <fs.h>
#include <webserver.h>
#include <ota.h>
#include <pm.h>

// !!!
#include <ul_linked_list.h>

// !!! SISTEMARE IL REBOOT IN CASO DI CRASH NEL MENUCONFIG SOTTO IL MENU Trace memory
// !!! SISTEMARE PRIORITA' TASK, CORE DI ALLOCAZIONE E STACK ALLOCATO NEL MENUCONFIG

// !!! OTTIMIZZARE CODICE ZONE.H

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

const char *TAG = "main";

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

void linked_list_test();

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
 * @brief App entry point.
*/
void app_main(){

	/* MCU Configuration--------------------------------------------------------*/
	/* USER CODE BEGIN SysInit */

	ESP_LOGI(TAG, "Started");

	// ESP_LOGI(TAG, "gpio_setup()");
	// ESP_ERROR_CHECK(gpio_setup());

	// ESP_LOGI(TAG, "pwm_setup()");
	// ESP_ERROR_CHECK(pwm_setup());

	// ESP_LOGI(TAG, "rs485_setup()");
	// ESP_ERROR_CHECK(rs485_setup());

	// ESP_LOGI(TAG, "nvs_setup()");	// !!!
	// ESP_ERROR_CHECK(nvs_setup());

	// ESP_LOGI(TAG, "wifi_setup()");
	// ESP_ERROR_CHECK(wifi_setup());

	// ESP_LOGI(TAG, "fs_setup()");		// !!!
	// ESP_ERROR_CHECK(fs_setup());

	// ESP_LOGI(TAG, "webserver_setup()");	// !!!
	// ESP_ERROR_CHECK(webserver_setup());

	// ESP_LOGI(TAG, "pm_setup()");
	// ESP_ERROR_CHECK(pm_setup());

	/* USER CODE END SysInit */

	/* USER CODE BEGIN Init */

	/* USER CODE END Init */

	/* USER CODE BEGIN 1 */

	linked_list_test();

	ESP_LOGI(TAG, "Completed");
	return;

	/* Infinite loop */
	// for(;;){
	// 	delay(1);
	// }
	/* USER CODE END 1 */
}

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 2 */

void delay_remainings(int32_t ms, int64_t initial_timestamp_ms){
	ms -= millis() - initial_timestamp_ms;
	if(ms > 0)
		delay(ms);
}

void log_hash(const char *TAG, const char* label, uint8_t *hash, uint16_t hash_len){

	char str[hash_len * 2 + 1];
	str[sizeof(str) - 1] = 0;

	for(uint16_t i=0; i<hash_len; i++)
		snprintf(&str[i * 2], hash_len, "%02x", hash[i]);

	ESP_LOGI(TAG, "%s: %s", label, str);
}

// #define USE_PTR_ARR

#ifdef USE_PTR_ARR
	#define CONVERSION_FUNCTION_TO_USE	ul_linked_list_to_arr_ptr(list, (void**) ptr_arr)
	#define PRINT_FUNCTION_TO_USE				print_ptr_arr()
#else
	#define CONVERSION_FUNCTION_TO_USE	ul_linked_list_to_arr(list, arr)
	#define PRINT_FUNCTION_TO_USE				print_arr()
#endif

#define log_list(title)	\
	UL_ERROR_CHECK(ul_linked_list_len(list, &len)); \
	printf("### %s\n", title); \
	printf("len: %lu\n", len); \
	UL_ERROR_CHECK(CONVERSION_FUNCTION_TO_USE) \
	PRINT_FUNCTION_TO_USE;

#define spacer() \
	printf("----------------------------------------\n\n");

void reset_ptr_arr();
void print_ptr_arr();

void reset_arr();
void print_arr();

static bool ul_linked_list_predicate_callback(void *user_context, uint32_t index, void *element);

// Predicate operand.
uint32_t k = 12;
#define PREDICATE_OPERATOR	<

ul_linked_list_handle_t *list;
ul_linked_list_init_t ul_linked_list_init = {
	.element_size = sizeof(uint32_t),
	.user_context = &k
};

#define ARR_LEN	20
uint32_t *ptr_arr[ARR_LEN];
uint32_t arr[ARR_LEN];

#define V_LEN	3
uint32_t n, v[V_LEN] = {100, 150, 200};
uint32_t len;

void linked_list_test(){
	ul_errors_begin(printf, NULL);

	UL_ERROR_CHECK(ul_linked_list_begin(&ul_linked_list_init, &list));
	spacer();

	reset_arr();
	reset_ptr_arr();
	log_list("Virgin array");

	spacer();

	n = 10;
	UL_ERROR_CHECK(ul_linked_list_add(list, &n));
	log_list("Add 10");

	n = 20;
	UL_ERROR_CHECK(ul_linked_list_add(list, &n));
	log_list("Add 20");

	n = 30;
	UL_ERROR_CHECK(ul_linked_list_add(list, &n));
	log_list("Add 30");

	spacer();

	UL_ERROR_CHECK(ul_linked_list_add_arr(list, v, V_LEN));
	log_list("Add v");

	spacer();

	n = 9;
	UL_ERROR_CHECK(ul_linked_list_append(list, &n));
	log_list("Append 9");

	n = 8;
	UL_ERROR_CHECK(ul_linked_list_append(list, &n));
	log_list("Append 8");

	n = 7;
	UL_ERROR_CHECK(ul_linked_list_append(list, &n));
	log_list("Append 7");

	spacer();

	UL_ERROR_CHECK(ul_linked_list_append_arr(list, v, V_LEN));
	log_list("Append v");

	spacer();
	log_list("memcmp search");

	uint32_t index;

	n = 9;
	UL_ERROR_CHECK_WITHOUT_ABORT(ul_linked_list_search_element(list, &n, &index));

	printf("<< search_element(%lu): %lu >>\n", n, index);
	printf("<< includes(%lu): %d >>\n\n", n, ul_linked_list_includes(list, &n));

	n = 1;
	UL_ERROR_CHECK_WITHOUT_ABORT(ul_linked_list_search_element(list, &n, &index));

	printf("<< search_element(%lu): %lu >>\n", n, index);
	printf("<< includes(%lu): %d >>\n\n", n, ul_linked_list_includes(list, &n));

	spacer();
	log_list("custom search");

	UL_ERROR_CHECK_WITHOUT_ABORT(ul_linked_list_search(list, ul_linked_list_predicate_callback, &index));
	printf("<< search(%lu): %lu >>\n", k, index);
	printf("<< any(%lu): %d >>\n\n", k, ul_linked_list_any(list, ul_linked_list_predicate_callback));

	spacer();

	UL_ERROR_CHECK(ul_linked_list_delete(list, 4));
	log_list("Delete list[4]");

	UL_ERROR_CHECK(ul_linked_list_delete(list, 0));
	log_list("Delete list[0]");

	spacer();

	index = 2;
	UL_ERROR_CHECK(ul_linked_list_get(list, index, &n));

	n += 10;
	UL_ERROR_CHECK(ul_linked_list_set(list, index, &n));

	printf("<< list[%lu]: %lu >>\n\n", index, n);
	log_list("Get/Set test: list[2] += 10");

	spacer();

	reset_ptr_arr();
	UL_ERROR_CHECK(ul_linked_list_reset(list));
	log_list("Reset");

	spacer();
}


void reset_ptr_arr(){
	static bool first_time = true;

	if(!first_time)
		for(int i=0; i<ARR_LEN; i++)
			free(ptr_arr[i]);

	for(int i=0; i<ARR_LEN; i++){
		ptr_arr[i] = (uint32_t*) malloc(sizeof(uint32_t));
		*ptr_arr[i] = 0;
	}

	first_time = false;
}

void print_ptr_arr(){
	UL_ERROR_CHECK(ul_linked_list_len(list, &len));

	printf("[");
	for(uint32_t i=0; i<len; i++){
		printf("%d", (int) *ptr_arr[i]);

		if(i < len-1)
			printf(", ");
	}
	printf("]\n\n");
}

void reset_arr(){
	for(int i=0; i<ARR_LEN; i++)
		arr[i] = 0;
}

void print_arr(){
	UL_ERROR_CHECK(ul_linked_list_len(list, &len));

	printf("[");
	for(uint32_t i=0; i<len; i++){
		printf("%d", (int) arr[i]);

		if(i < len-1)
			printf(", ");
	}
	printf("]\n\n");
}

bool ul_linked_list_predicate_callback(void *user_context, uint32_t index, void *element){
	uint32_t n = *(uint32_t*) element;
	uint32_t k = *(uint32_t*) user_context;

	return (n PREDICATE_OPERATOR k);
}

/* USER CODE END 2 */

/* Private user code for ISR (Interrupt Service Routines) --------------------*/
/* USER CODE BEGIN ISR */

/* USER CODE END ISR */
