/** @file ul_linked_list.c
 *  @brief  Created on: Sep 13, 2024
 *          Davide Scalisi
 *
 * @copyright [2024] Davide Scalisi *
 * @copyright All Rights Reserved. *
 *
*/

/************************************************************************************************************
* Included files
************************************************************************************************************/

#include <ul_linked_list.h>
#include <ul_private.h>

/************************************************************************************************************
* Private Defines
************************************************************************************************************/

/**
 * @brief Reset the linked list to an empty linked list.
*/
#define __reset_instance(self) \
	self->head = NULL; \
	self->length = 0

/**
 * @brief Free the space allocated by `__new_node()` on `node` and `node->data`.
*/
#define __delete_node(self, node) \
	free(node->data); \
	free(node); \
	self->length--

/**
 * @brief Get the pointer to the tail node of the linked list.
 * @param node_at_tail Pointer to where to store a pointer to the tail element.
*/
#define __get_tail_node(self, node_at_tail) \
	__get_node_at_index(self, self->length > 0 ? self->length - 1 : 0, node_at_tail)

/************************************************************************************************************
* Private Types Definitions
 ************************************************************************************************************/

/************************************************************************************************************
* Private Variables
 ************************************************************************************************************/

/************************************************************************************************************
* Private Functions Prototypes
 ************************************************************************************************************/

/**
 * @brief Dinamically allocate a new node.
 * @note `element` content is copied using a `memcpy()` on a new dinamically allocated space.
*/
static ul_err_t __new_node(ul_linked_list_handle_t *self, void *element, ul_linked_list_node_t **new_node);

/**
 * @brief Append to the `*parent` node, the `*new_node` node.
*/
static void __append_node(ul_linked_list_handle_t *self, ul_linked_list_node_t **parent, ul_linked_list_node_t **new_node);

/**
 * @brief Search an element inside the linked list.
 * @param index Pointer to where to store the index of the searched element.
 * @note If `element` is `NULL`, `predicate_callback()` will be used and if `predicate_callback` is `NULL`, `memcmp()` will be used.
*/
static ul_err_t __search(ul_linked_list_handle_t *self, void *element, ul_linked_list_predicate_callback_t predicate_callback, uint32_t *searched_index);

/**
 * @brief Get a pointer to the node at the specified index of the linked list.
 * @param node_at_index Pointer to where to store a pointer to the element with the specified index.
*/
static ul_err_t __get_node_at_index(ul_linked_list_handle_t *self, uint32_t index, ul_linked_list_node_t **node_at_index);

/************************************************************************************************************
* Private Functions Definitions
 ************************************************************************************************************/

ul_err_t __new_node(ul_linked_list_handle_t *self, void *element, ul_linked_list_node_t **new_node){
	ul_err_t ret = UL_OK;

	ul_linked_list_node_t *node = malloc(sizeof(ul_linked_list_node_t));
	UL_RETURN_ON_FALSE(
		node != NULL,

		UL_ERR_NO_MEM,
		"Error on `malloc(size=%lu)`",
		sizeof(ul_linked_list_node_t)
	);

	node->data = malloc(self->init.element_size);
	UL_GOTO_ON_FALSE(
		node->data != NULL,

		UL_ERR_NO_MEM,
		label_error,
		"Error on `malloc(size=%lu)`",
		self->init.element_size
	);

	memcpy(node->data, element, self->init.element_size);
	node->next = NULL;

	*new_node = node;
	return ret;

	label_error:
	free(node);
	return ret;
}

void __append_node(ul_linked_list_handle_t *self, ul_linked_list_node_t **parent, ul_linked_list_node_t **new_node){
	(*new_node)->next = *parent;
	*parent = *new_node;
	self->length++;
}

ul_err_t __search(ul_linked_list_handle_t *self, void *element, ul_linked_list_predicate_callback_t predicate_callback, uint32_t *searched_index){

	UL_RETURN_ON_FALSE(
		ul_utils_either(
			element, predicate_callback,
			!=, NULL
		),

		UL_ERR_INVALID_ARG,
		"Error: inconsistent parameters"
	);

	// Empty list.
	if(self->head == NULL)
		return UL_ERR_NOT_FOUND;

	ul_linked_list_node_t *current = self->head;
	uint32_t index = 0;

	while(current != NULL){
		if(
			(
				// `memcmp()` search.
				element != NULL &&
				predicate_callback == NULL &&
				memcmp(current->data, element, self->init.element_size) == 0

			) || (

				// Predicate search.
				element == NULL &&
				predicate_callback != NULL &&
				predicate_callback(self->init.user_context, index, current->data)

			)
		){
			*searched_index = index;
			return UL_OK;
		}

		else {
			current = current->next;
			index++;
		}
	}

	return UL_ERR_NOT_FOUND;
}

ul_err_t __get_node_at_index(ul_linked_list_handle_t *self, uint32_t index, ul_linked_list_node_t **node_at_index){

	ul_linked_list_node_t *current = self->head;
	for(uint32_t i=0; i<index; i++){

		// Check if there's a next before accessing it.
		UL_RETURN_ON_FALSE(
			current != NULL,

			UL_ERR_INVALID_ARG,
			"Error: `index` out of bound"
		);

		current = current->next;
	}

	*node_at_index = current;
	return UL_OK;
}

/************************************************************************************************************
* Public Functions Definitions
 ************************************************************************************************************/

ul_err_t ul_linked_list_begin(ul_linked_list_init_t *init, ul_linked_list_handle_t **returned_handle){
	assert_param_notnull(init);
	assert_param_notnull(returned_handle);

	/* Instance configurations */

	ul_err_t ret = UL_OK;
	ul_linked_list_handle_t *self = malloc(sizeof(ul_linked_list_handle_t));

	UL_RETURN_ON_FALSE(
		self != NULL,

		UL_ERR_NO_MEM,
		"Error on `malloc(size=%lu)`",
		sizeof(ul_linked_list_handle_t)
	);

	self->init = *init;

	/* Parameters check */

	UL_GOTO_ON_FALSE(
		init->element_size > 0,

		UL_ERR_INVALID_ARG,
		label_error,
		"Error: `init->element_size` is 0"
	);

	/* Init configurations */

	__reset_instance(self);

	*returned_handle = self;
	return ret;

	label_error:
	free(self);
	return ret;
}

ul_err_t ul_linked_list_end(ul_linked_list_handle_t *self){
	assert_param_notnull(self);

	UL_RETURN_ON_ERROR(
		ul_linked_list_reset(self),
		"Error on `ul_linked_list_reset()`"
	);

	free(self);
	return UL_OK;
}

ul_err_t ul_linked_list_reset(ul_linked_list_handle_t *self){
	assert_param_notnull(self);

	// Deallocate every single node with its data.
	ul_linked_list_node_t *next, *current = self->head;

	while(current != NULL){
		next = current->next;
		__delete_node(self, current);
		current = next;
	}

	__reset_instance(self);
	return UL_OK;
}

/* Generic */

ul_err_t ul_linked_list_len(ul_linked_list_handle_t *self, uint32_t *len){
	assert_param_notnull(self);
	assert_param_notnull(len);

	*len = self->length;
	return UL_OK;
}

ul_err_t ul_linked_list_add(ul_linked_list_handle_t *self, void *element){
	assert_param_notnull(self);
	assert_param_notnull(element);

	ul_linked_list_node_t *node;
	UL_RETURN_ON_ERROR(
		__new_node(self, element, &node),
		"Error on `__new_node()`"
	);

	__append_node(self, &(self->head), &node);
	return UL_OK;
}

ul_err_t ul_linked_list_add_arr(ul_linked_list_handle_t *self, void *arr, uint32_t len){
	assert_param_notnull(self);
	assert_param_notnull(arr);
	assert_param_size_ok(len);

	ul_linked_list_node_t *node;
	for(int i=len-1; i>=0; i--){

		UL_RETURN_ON_ERROR(
			__new_node(
				self,
				&((uint8_t*) arr)[i * self->init.element_size],
				&node
			),

			"Error on `__new_node()`"
		);

		__append_node(self, &(self->head), &node);
	}

	return UL_OK;
}

ul_err_t ul_linked_list_append(ul_linked_list_handle_t *self, void *element){
	assert_param_notnull(self);
	assert_param_notnull(element);

	ul_linked_list_node_t *tail;
	UL_RETURN_ON_ERROR(
		__get_tail_node(self, &tail),
		"Error on `__get_tail_node()`"
	);

	/**
	 * Handle empty list case (head = tail).
	 * if empty, append to the head, else to the tail.
	*/
	ul_linked_list_node_t **parent = (
		self->head == NULL ?
		&(self->head) :
		&(tail->next)
	);

	ul_linked_list_node_t *node;
	UL_RETURN_ON_ERROR(
		__new_node(self, element, &node),

		"Error on `__new_node()`"
	);

	__append_node(self, parent, &node);
	return UL_OK;
}

ul_err_t ul_linked_list_append_arr(ul_linked_list_handle_t *self, void *arr, uint32_t len){
	assert_param_notnull(self);
	assert_param_notnull(self);
	assert_param_size_ok(len);

	ul_linked_list_node_t *tail;
	UL_RETURN_ON_ERROR(
		__get_tail_node(self, &tail),
		"Error on `__get_tail_node()`"
	);

	/**
	 * Handle empty list case (head = tail).
	 * if empty, append to the head, else to the tail.
	*/
	ul_linked_list_node_t **parent = (
		self->head == NULL ?
		&(self->head) :
		&(tail->next)
	);

	ul_linked_list_node_t *node;
	for(int i=len-1; i>=0; i--){

		UL_RETURN_ON_ERROR(
			__new_node(
				self,
				&((uint8_t*) arr)[i * self->init.element_size],
				&node
			),

			"Error on `__new_node()`"
		);

		__append_node(self, parent, &node);
	}

	return UL_OK;
}

ul_err_t ul_linked_list_delete(ul_linked_list_handle_t *self, uint32_t index){
	assert_param_notnull(self);

	// Check if the index exists and if the list is not empty.
	UL_RETURN_ON_FALSE(
		index < self->length,

		UL_ERR_INVALID_ARG,
		"Error: `index` is greater or equal to `self->length`"
	);

	ul_linked_list_node_t *node;

	// Case index = 0
	if(index == 0){

		// Save the node for deallocation purposes.
		node = self->head;

		// Detach the node.
		self->head = node->next;
	}

	// Case index > 0
	else {
		index--;

		ul_linked_list_node_t *parent;
		UL_RETURN_ON_ERROR(
			__get_node_at_index(self, index, &parent),

			"Error on `__get_node_at_index(index=%lu)`",
			index
		);

		// Save the node for deallocation purposes.
		node = parent->next;

		// Detach the node.
		parent->next = node->next;
	}

	// Deallocate the node.
	__delete_node(self, node);

	return UL_OK;
}

ul_err_t ul_linked_list_search(ul_linked_list_handle_t *self, ul_linked_list_predicate_callback_t predicate_callback, uint32_t *index){
	assert_param_notnull(self);
	assert_param_notnull(predicate_callback);
	assert_param_notnull(index);
	return __search(self, NULL, predicate_callback, index);
}

ul_err_t ul_linked_list_search_element(ul_linked_list_handle_t *self, void *element, uint32_t *index){
	assert_param_notnull(self);
	assert_param_notnull(element);
	assert_param_notnull(index);
	return __search(self, element, NULL, index);
}

bool ul_linked_list_includes(ul_linked_list_handle_t *self, void *element){
	assert_param_notnull(self);
	assert_param_notnull(element);

	uint32_t index;
	ul_err_t ret =
		__search(self, element, NULL, &index);

	if(ret == UL_OK)
		return true;

	else if(ret == UL_ERR_NOT_FOUND)
		return false;

	// Error occurred.
	UL_ERROR_CHECK_WITHOUT_ABORT(ret);
	return false;
}

bool ul_linked_list_any(ul_linked_list_handle_t *self, ul_linked_list_predicate_callback_t predicate_callback){
	assert_param_notnull(self);
	assert_param_notnull(predicate_callback);

	uint32_t index;
	ul_err_t ret =
		__search(self, NULL, predicate_callback, &index);

	if(ret == UL_OK)
		return true;

	else if(ret == UL_ERR_NOT_FOUND)
		return false;

	// Error occurred.
	UL_ERROR_CHECK_WITHOUT_ABORT(ret);
	return false;
}

ul_err_t ul_linked_list_to_arr(ul_linked_list_handle_t *self, void *arr){
	assert_param_notnull(self);
	assert_param_notnull(arr);

	ul_linked_list_node_t *current = self->head;
	uint32_t index = 0;

	while(current != NULL){

		memcpy(
			&((uint8_t*) arr)[index * self->init.element_size],
			current->data,
			self->init.element_size
		);

		current = current->next;
		index++;
	}

	return UL_OK;
}

ul_err_t ul_linked_list_to_arr_ptr(ul_linked_list_handle_t *self, void **arr_ptr){
	assert_param_notnull(self);
	assert_param_notnull(arr_ptr);

	ul_linked_list_node_t *current = self->head;
	uint32_t index = 0;

	while(current != NULL){

		// You can address directly `arr_ptr` because the sizeof(void*) is constant.
		arr_ptr[index] = current->data;

		current = current->next;
		index++;
	}

	return UL_OK;
}

/* Getter */

ul_err_t ul_linked_list_get(ul_linked_list_handle_t *self, uint32_t index, void *element){
	assert_param_notnull(self);
	assert_param_notnull(element);

	// Check if the index exists and if the list is not empty.
	UL_RETURN_ON_FALSE(
		index < self->length,

		UL_ERR_INVALID_ARG,
		"Error: `index` is greater or equal to `self->length`"
	);

	ul_linked_list_node_t *node;
	UL_RETURN_ON_ERROR(
		__get_node_at_index(self, index, &node),

		"Error on `__get_node_at_index(index=%lu)`",
		index
	);

	memcpy(element, node->data, self->init.element_size);
	return UL_OK;
}

/* Setter */

ul_err_t ul_linked_list_set(ul_linked_list_handle_t *self, uint32_t index, void *element){
	assert_param_notnull(self);
	assert_param_notnull(element);

	// Check if the index exists and if the list is not empty.
	UL_RETURN_ON_FALSE(
		index < self->length,

		UL_ERR_INVALID_ARG,
		"Error: `index` is greater or equal to `self->length`"
	);

	ul_linked_list_node_t *node;
	UL_RETURN_ON_ERROR(
		__get_node_at_index(self, index, &node),

		"Error on `__get_node_at_index(index=%lu)`",
		index
	);

	memcpy(node->data, element, self->init.element_size);
	return UL_OK;
}

ul_err_t ul_linked_list_set_user_context(ul_linked_list_handle_t *self, void *user_context){
	assert_param_notnull(self);

	self->init.user_context = user_context;
	return UL_OK;
}
