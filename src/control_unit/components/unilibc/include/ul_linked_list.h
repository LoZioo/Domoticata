/** @file ul_linked_list.h
 *  @brief  Created on: Sep 13, 2024
 *          Davide Scalisi
 *
 * 					Description:	Linked list implementation.
 *
 * @copyright [2024] Davide Scalisi *
 * @copyright All Rights Reserved. *
 *
*/

#ifndef INC_UL_LINKED_UL_LINKED_LIST_H_
#define INC_UL_LINKED_UL_LINKED_LIST_H_

/************************************************************************************************************
* Included files
************************************************************************************************************/

// Standard libraries.
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

// UniLibC libraries.
#include <ul_configs.h>
#include <ul_errors.h>
#include <ul_utils.h>

/************************************************************************************************************
* Public Defines
************************************************************************************************************/

/************************************************************************************************************
* Public Types Definitions
************************************************************************************************************/

/**
 * @brief A predicate callback.
 * @param index The index of the current element.
 * @param element The current element.
 * @return Must return `true` if the element match your research criteria, `false` otherwise.
*/
typedef bool (*ul_linked_list_predicate_callback_t)(void *user_context, uint32_t index, void *element);

// Single node struct.
typedef struct ul_linked_list_node_t {
	void *data;
	struct ul_linked_list_node_t *next;

} ul_linked_list_node_t;

// Initialization struct.
typedef struct {

	// Size of the single element of the linked list (bytes).
	size_t element_size;

	/**
	 * This pointer is needed when you want to use `ul_linked_list_predicate_callback_t`.
	 * If you set a value here, you will retrive it in the `user_context` parameter.
	 * This is used when from another library, you want to take into account the current
	 * library handle in the callback evaluation.
	 *
	 * If not needed, you can leave it uninitialized or initialize it to `NULL`.
	*/
	void *user_context;

} ul_linked_list_init_t;

// List handle.
typedef struct {

	/* Instance configurations */

	ul_linked_list_init_t init;

	/* Instance state */

	ul_linked_list_node_t *head;
	uint32_t length;

} ul_linked_list_handle_t;

/************************************************************************************************************
* Public Variables Prototypes
************************************************************************************************************/

/************************************************************************************************************
* Public Functions Prototypes
************************************************************************************************************/

/**
 * @brief Create a new instance.
*/
extern ul_err_t ul_linked_list_begin(ul_linked_list_init_t *init, ul_linked_list_handle_t **returned_handle);

/**
 * @brief Free the allocated resources.
*/
extern ul_err_t ul_linked_list_end(ul_linked_list_handle_t *self);

/**
 * @brief Reset the linked list.
*/
extern ul_err_t ul_linked_list_reset(ul_linked_list_handle_t *self);

/* Generic */

/**
 * @return The length of the linked list.
*/
extern ul_err_t ul_linked_list_len(ul_linked_list_handle_t *self, uint32_t *len);

/**
 * @brief Append a new element to the head of the linked list.
*/
extern ul_err_t ul_linked_list_add(ul_linked_list_handle_t *self, void *element);

/**
 * @brief Append the array elements to the head of the linked list.
*/
extern ul_err_t ul_linked_list_add_arr(ul_linked_list_handle_t *self, void *arr, uint32_t len);

/**
 * @brief Append a new element to the tail of the linked list.
*/
extern ul_err_t ul_linked_list_append(ul_linked_list_handle_t *self, void *element);

/**
 * @brief Append the array elements to the tail of the linked list.
*/
extern ul_err_t ul_linked_list_append_arr(ul_linked_list_handle_t *self, void *arr, uint32_t len);

/**
 * @brief Delete the element at the given index.
*/
extern ul_err_t ul_linked_list_delete(ul_linked_list_handle_t *self, uint32_t index);

/**
 * @brief Search an element inside the linked list by using a custom predicate function.
 * @param predicate_callback The search predicate callback (please refer to `ul_linked_list_predicate_callback_t`).
 * @param index Pointer to where to store the index of the searched element.
*/
extern ul_err_t ul_linked_list_search(ul_linked_list_handle_t *self, ul_linked_list_predicate_callback_t predicate_callback, uint32_t *index);

/**
 * @brief Search an element inside the linked list by using the standard `memcmp()` function.
 * @param index Pointer to where to store the index of the searched element.
*/
extern ul_err_t ul_linked_list_search_element(ul_linked_list_handle_t *self, void *element, uint32_t *index);

/**
 * @brief Check if an element is contained inside the linked list by using the standard `memcmp()` function.
*/
extern bool ul_linked_list_includes(ul_linked_list_handle_t *self, void *element);

/**
 * @brief Check whether at least one element in the linked list complies with the given predicate.
 * @param predicate_callback The search predicate callback (please refer to `ul_linked_list_predicate_callback_t`).
*/
extern bool ul_linked_list_any(ul_linked_list_handle_t *self, ul_linked_list_predicate_callback_t predicate_callback);

/**
 * @brief Check all items in the linked list comply with the given predicate.
 * @param predicate_callback The search predicate callback (please refer to `ul_linked_list_predicate_callback_t`).
 * @warning Function not implemented.
*/
extern ul_err_t ul_linked_list_all(ul_linked_list_handle_t *self, ul_linked_list_predicate_callback_t predicate_callback) __attribute__((weak));

/**
 * @brief Converts the linked list to an array.
 * @warning The array's bytes size must be at least `ul_linked_list_len() * self->init.element_size`.
 * @note This function performs a `memcpy()` from the single linked list element to the single array location.
*/
extern ul_err_t ul_linked_list_to_arr(ul_linked_list_handle_t *self, void *arr);

/**
 * @brief Converts the linked list to an array of pointers.
 * @warning The array's bytes size must be at least `ul_linked_list_len() * sizeof(void*)`.
 * @note This function copies only the memory addresses, not the pointed data.
*/
extern ul_err_t ul_linked_list_to_arr_ptr(ul_linked_list_handle_t *self, void **arr_ptr);

/* Getter */

/**
 * @brief Get the element at the given index.
*/
extern ul_err_t ul_linked_list_get(ul_linked_list_handle_t *self, uint32_t index, void *element);

/* Setter */

/**
 * @brief Set the element at the given index.
*/
extern ul_err_t ul_linked_list_set(ul_linked_list_handle_t *self, uint32_t index, void *element);

/**
 * @brief Set the `user_context` parameter of the `ul_linked_list_predicate_callback_t` callback.
 * @note For more informations, please refer to the `self->init->user_context` description.
*/
extern ul_err_t ul_linked_list_set_user_context(ul_linked_list_handle_t *self, void *user_context);

#endif  /* INC_UL_LINKED_UL_LINKED_LIST_H_ */
