/** @file ul_analog_button.h
 *  @brief  Created on: July 27, 2024
 *          Davide Scalisi
 *
 * 					Description:	Library to poll buttons wired to an ADC and to handle respective events.
 *
 * @copyright [2024] Davide Scalisi *
 * @copyright All Rights Reserved. *
 *
*/

#ifndef INC_UL_ANALOG_BUTTON_H_
#define INC_UL_ANALOG_BUTTON_H_

/************************************************************************************************************
* Included files
************************************************************************************************************/

// Standard libraries.
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

// UniLibC libraries.
#include <ul_errors.h>
#include <ul_utils.h>

/************************************************************************************************************
* Public Defines
************************************************************************************************************/

/************************************************************************************************************
* Public Types Definitions
************************************************************************************************************/

/**
 * @brief Trigger polarity.
 */
typedef enum {

	UL_ANALOG_BUTTON_EDGE_RISING,
	UL_ANALOG_BUTTON_EDGE_FALLING,
	UL_ANALOG_BUTTON_EDGE_BOTH

} ul_analog_button_edge_t;

/**
 * @brief Analog button ADC sampling callback.
 * @return Must return the sampled ADC value.
*/
typedef uint16_t (*ul_analog_button_adc_read_callback_t)();

/**
 * @brief Analog button event callback.
 * @param button_id The registered button ID that triggered the callback.
 * @param edge `ul_analog_button_edge_t` The edge that triggered the callback.
*/
typedef void (*ul_analog_button_event_callback_t)(uint8_t button_id, uint8_t edge);

// Instance configurations.
typedef struct {

	/* Variables and pointers */

	// ADC resolution in bits.
	uint8_t adc_res_bits;

	/* Callbacks */

	/**
	 * @brief The ADC callback.
	 */
	ul_analog_button_adc_read_callback_t adc_read_callback;

	/**
	 * @brief The event callback.
	 */
	ul_analog_button_event_callback_t event_callback;

} ul_analog_button_init_t;

// Instance handler.
typedef struct {

	/* Instance configurations */

	ul_analog_button_init_t init;

	/* Instance state */

	// Current ADC value from the `evaluate()`.
	uint16_t adc_current_val;

	// Needed to implement the edge detector.
	uint16_t adc_last_val;

} ul_analog_button_handler_t;

/************************************************************************************************************
* Public Variables Prototypes
************************************************************************************************************/

/************************************************************************************************************
* Public Functions Prototypes
************************************************************************************************************/

/**
 * @brief Create a new instance.
 */
extern ul_err_t ul_analog_button_begin(ul_analog_button_init_t init, ul_analog_button_handler_t **returned_handler);

/**
 * @brief Free the allocated resources.
*/
extern void ul_analog_button_end(ul_analog_button_handler_t *self);

/**
 * @brief Bind an analog read to a button event.
 * @param adc_mean_val The mean analog value.
 * @param valid_interval The `event_callback` will be triggered if the sampled value is between `adc_mean_val` +- `valid_interval`.
 * @param button_id Button ID to be retrived inside `event_callback`.
 * @param edge `ul_analog_button_edge_t` The edge that must trigger `event_callback`.
 */
extern ul_err_t ul_analog_button_bind(
	ul_analog_button_handler_t *self,
	uint16_t adc_mean_val,
	uint16_t valid_interval,
	uint8_t button_id,
	uint8_t edge
);

/**
 * @brief Update and handle the button's bind events.
 */
extern ul_err_t ul_analog_button_evaluate(ul_analog_button_handler_t *self);

#endif  /* INC_UL_ANALOG_BUTTON_H_ */
