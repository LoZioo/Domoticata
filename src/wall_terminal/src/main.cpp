/* USER CODE BEGIN Header */
/**
	******************************************************************************
	* @file           : main.cpp
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

// Platform libraries.
#include <Arduino.h>
#include <EEPROM.h>

// UniLibC libraries.
extern "C" {
	#include <ul_errors.h>
	#include <ul_utils.h>
}

// Project libraries.
#include <const.h>

extern "C" {
	#include <button_states.h>
	#include <dup.h>
}

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/**
 * @brief State machine states.
 */
typedef enum : uint8_t {
	STATE_IDLE,
	STATE_HANDLE_COMMAND,
	STATE_SEND_BUTTON_STATES
} state_t;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

// !!! METTERE IN EEPROM
#define local_device_id		0x06

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

// Current state.
static state_t state = STATE_IDLE;

// DUP decoded header temp buffer.
static dup_decoded_header_t dup_header;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* Setup routines */

/**
 * @brief GPIO initialization.
 */
void GPIO_setup();

/**
 * @brief GPIO initialization.
 */
void UART_setup();

/* Background tasks */

// static uint32_t background_button_task_t0;
bool background_button_task();

static uint32_t background_uart_task_t0;
bool background_uart_task();

/* Generic functions */

void state_handle_command();
void state_send_button_states();

/**
 * @brief Poll the ADC and convert the read value to a button ID.
 * @return A member of `button_id_t` from `button_states.h`: `BUTTON_1`, `BUTTON_2`, ...
 */
button_id_t analog_button_read(analog_pin_t adc_pin);

/**
 * @brief Send `DUP_COMMAND_ACK` to `DUP_ID_CU`.
 * @param nack If `true`, send a `DUP_COMMAND_ACK`; if `false`, send a `DUP_COMMAND_NACK`.
 */
void send_ack(bool ack = true);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
 * @brief Put your setup code here, to run once.
*/
void setup(){

	/* MCU Configuration--------------------------------------------------------*/
	/* USER CODE BEGIN SysInit */

	GPIO_setup();
	UART_setup();

	/* USER CODE END SysInit */

	/* USER CODE BEGIN Init */

	/* USER CODE END Init */

	/* USER CODE BEGIN 1 */

	/* USER CODE END 1 */
}

/**
 * @brief Put your main code here, to run repeatedly.
*/
void loop(){
	/* Infinite loop */
	/* USER CODE BEGIN Loop */

	// State machine.
	switch(state){
		case STATE_IDLE:
			// Run only the background tasks.
			break;

		case STATE_HANDLE_COMMAND:
			state_handle_command();
			break;

		case STATE_SEND_BUTTON_STATES:
			state_send_button_states();
			break;

		default:
			break;
	}

	// Always run them at the end of each loop.
	background_button_task();
	background_uart_task();

	/* USER CODE END Loop */
}

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 2 */

void GPIO_setup(){
	pinMode(CONF_GPIO_PWM_A, OUTPUT);
	pinMode(CONF_GPIO_PWM_B, OUTPUT);
}

void UART_setup(){

	// Recall previously stored `OSCCAL` calibration value.
	uint8_t cal = EEPROM.read(0);

	if(cal < 0x80)
		OSCCAL = cal;
}

bool background_button_task(){

	static button_id_t button;
	static uint8_t button_press_count;

	button = analog_button_read(CONF_GPIO_ADC);

	// If a button was pressed.
	if(button != BUTTON_NONE){

		// Update the current button state based on it's previous state.
		switch(get_button_state(button)){
			case BUTTON_STATE_IDLE:
				set_button_state(button, BUTTON_STATE_PRESSED);
				button_press_count = 1;

				// !!! DEBUG
				analogWrite(button-1, 40);
				break;

			case BUTTON_STATE_PRESSED:
			case BUTTON_STATE_DOUBLE_PRESSED:
				button_press_count++;

				// !!! DEBUG
				analogWrite(button-1, 255);
				break;

			default:
				break;
		}

		if(button_press_count == 2)
			set_button_state(button, BUTTON_STATE_DOUBLE_PRESSED);

		else if(button_press_count > CONF_HELD_BTN_TICKS)
			set_button_state(button, BUTTON_STATE_HELD);

		// Stop the delay only if something is available on the UART.
		return ul_utils_delay_nonblock(CONF_DEBOUNCE_TIME_MS, millis, &background_uart_task_t0, background_uart_task);
	}

	// If no buttons are pressed, reset the press count.
	else
		button_press_count = 0;

	// Continue eventual non-blocking delay.
	return true;
}

bool background_uart_task(){

	if(Serial.available()){
		dup_header = dup_decode_header(Serial.read());

		/**
		 * Force the non-blocking delay interruption to handle the received command.
		 * Only if the command is not a poll command.
		 */
		if(
			(
				dup_header.device_id == local_device_id ||
				dup_header.device_id == DUP_ID_BROADCAST
			)
				&&		// Do not answer to these commands.
			!ul_utils_in(
				dup_header.command, 3,
				DUP_COMMAND_POLL,
				DUP_COMMAND_ACK,
				DUP_COMMAND_NACK
			)
		){

			// ACK to begin the communication.
			send_ack();

			// Specific command handling.
			state = STATE_HANDLE_COMMAND;
			return false;
		}
	}

	// Continue eventual non-blocking delay.
	return true;
}

void state_handle_command(){

	// Handle the received command.
	switch(dup_header.command){
		case DUP_COMMAND_GET_BUTTON_STATES: {

			// !!! CONTROLLARE SE SONO PASSATI 500MS DALL'ULTIMO TASTO PREMUTO

			raw_button_states_t button_states = save_button_states();
			Serial.write(ul_utils_cast_to_mem(button_states), sizeof(button_states));

			reset_button_states();

			// // !!! DEBUG
			analogWrite(CONF_GPIO_PWM_A, 0);
			analogWrite(CONF_GPIO_PWM_B, 0);

			send_ack();
		} break;

		case DUP_COMMAND_SET_PWM: {

			uint8_t pwm_channel, pwm_percentage;
			dup_decode_parameters_command_set_pwm(
				Serial.read_blocking(),
				&pwm_channel,
				&pwm_percentage
			);

			analogWrite(
				pwm_channel == 0 ? CONF_GPIO_PWM_A : CONF_GPIO_PWM_B,
				ul_utils_map_int(
					pwm_percentage <= 100 ? pwm_percentage : 100,
					0, 100, 0, PWM_MAX
				)
			);

			send_ack();
		} break;

		// Command not recognized.
		default:
			send_ack(false);
			break;
	}
}

void state_send_button_states(){}

button_id_t analog_button_read(analog_pin_t adc_pin){

	static uint16_t adc_val;
	static button_id_t button;

	adc_val = analogRead(adc_pin);
	button = BUTTON_NONE;

	if(adc_val < CONF_BTN_VALID_EDGE){
		if(ul_utils_between(adc_val, VAL_BTN_1_LOWER_THR, VAL_BTN_1_UPPER_THR))
			button = BUTTON_1;

		else if(ul_utils_between(adc_val, VAL_BTN_2_LOWER_THR, VAL_BTN_2_UPPER_THR))
			button = BUTTON_2;
	}

	return button;
}

void send_ack(bool ack){
	Serial.write(dup_encode_header((dup_decoded_header_t){
		.device_id = DUP_ID_CU,
		.command = (ack ? DUP_COMMAND_ACK : DUP_COMMAND_NACK)
	}));
}

/* USER CODE END 2 */

/* Private user code for ISR (Interrupt Service Routines) --------------------*/
/* USER CODE BEGIN ISR */

/* USER CODE END ISR */
