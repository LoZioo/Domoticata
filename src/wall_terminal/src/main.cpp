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
#include <conf_const.h>
#include <conf_var.h>

extern "C" {
	#include <button_states.h>
}

// !!! VEDERE SE SI PUO' INSERIRE IL CONTROLLO DI PARITA' O UN QUALSIASI CONTROLLO ERRORI SOFTWARE.

// !!! SPIEGARE NELL'HEADER DEL MAIN COMU USARE IL PROGETTO
// (FLASH BOOTLOADER, CALIBRAZIONE OSCCAL, RILEVAZIONE VALORI BOTTONI,
// INSERIMENTO NUMERO BOTTONI DESIDERATI IN CONST E IN analog_button_read ED INFINE FLASH DEL MAIN).

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

#define is_master_byte(b)		((b & 0x80) != 0)
#define get_master_byte(b)	(b & 0x7F)

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

// Last button press instant.
static uint32_t last_button_press_ms = 0;

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

// static uint32_t button_task_t0;
bool button_task();

static uint32_t uart_task_t0;
bool uart_task();

/* Generic functions */

/**
 * @brief Poll the ADC and convert the read value to a button ID.
 * @return A member of `button_id_t` from `button_states.h`: `BUTTON_1`, `BUTTON_2`, ...
 */
button_id_t analog_button_read(analog_pin_t adc_pin);

/**
 * @brief Send the button states to the control unit.
 */
void send_button_states();

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

	button_task();
	uart_task();

	/* USER CODE END Loop */
}

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 2 */

void GPIO_setup(){
	pinMode(CONF_GPIO_PWM_A, OUTPUT);
	pinMode(CONF_GPIO_PWM_B, OUTPUT);
	// !!! pinMode(CONF_GPIO_UART_DE_RE, OUTPUT);
	pinMode(CONF_GPIO_UART_RX_TX, OUTPUT);
}

void UART_setup(){

	// Recall previously stored `OSCCAL` calibration value.
	uint8_t cal = EEPROM.read(0);

	if(cal < 0x80)
		OSCCAL = cal;
}

bool button_task(){

	static uint8_t press_count;
	button_id_t button = analog_button_read(CONF_GPIO_ADC);

	// If a button was pressed.
	if(button != BUTTON_NONE){

		// Update the last button press instant.
		last_button_press_ms = millis();

		// Update the current button state based on it's previous state.
		switch(get_button_state(button)){
			case BUTTON_STATE_IDLE:
				set_button_state(button, BUTTON_STATE_PRESSED);
				press_count = 1;

				// !!! DEBUG
				analogWrite(button-1, 40);
				break;

			case BUTTON_STATE_PRESSED:
			case BUTTON_STATE_DOUBLE_PRESSED:
				press_count++;
				break;

			default:
				break;
		}

		if(press_count == 2){
			set_button_state(button, BUTTON_STATE_DOUBLE_PRESSED);

			// !!! DEBUG
			analogWrite(button-1, 255);
		}

		else if(ul_utils_between(press_count, 3, CONF_TIME_BTN_HELD_TICKS - 1)){
			set_button_state(button, BUTTON_STATE_PRESSED);

			// !!! DEBUG
			analogWrite(button-1, 40);
		}

		else if(press_count == CONF_TIME_BTN_HELD_TICKS){
			set_button_state(button, BUTTON_STATE_HELD);

			// !!! DEBUG
			analogWrite(button-1, 255);
		}

		// Debouncer.
		ul_utils_delay_nonblock(CONF_TIME_BTN_DEBOUNCER_MS, millis, &uart_task_t0, uart_task);
	}

	// Continue eventual non-blocking delay.
	return true;
}

bool uart_task(){

	if(Serial.available()){
		uint8_t b = Serial.read();

		/**
		 * If:
		 * 	-	I read a master byte.
		 * 	-	It's my turn on the bus.
		 * 	-	Some button was pressed.
		 * 	-	The lock time elapsed after the last button press.
		 *
		 * Then send the button states.
		 */
		if(
			is_master_byte(b) &&
			get_master_byte(b) == get_master_byte(CONF_UART_DEVICE_ID) &&
			get_button_states() != 0 &&
			millis() - last_button_press_ms >= CONF_TIME_BTN_LOCK_MS
		)
			send_button_states();
	}

	// Continue eventual non-blocking delay.
	return true;
}

button_id_t analog_button_read(analog_pin_t adc_pin){

	uint16_t adc_val;
	button_id_t button;

	adc_val = analogRead(adc_pin);
	button = BUTTON_NONE;

	if(adc_val < CONF_BTN_VALID_EDGE){

		// !!! METTERE IF COME DEFINE
		if(ul_utils_between(adc_val, VAL_BTN_1_LOWER_THR, VAL_BTN_1_UPPER_THR))
			button = BUTTON_1;

		else if(ul_utils_between(adc_val, VAL_BTN_2_LOWER_THR, VAL_BTN_2_UPPER_THR))
			button = BUTTON_2;
	}

	return button;
}

void send_button_states(){

	// Reply with my ID to get the master's attention.
	Serial.write(CONF_UART_DEVICE_ID);

	// Encoding in 3 bytes instead of 2 (the MSb of every byte must be 0 because a slave is talking).
	uint16_t button_states = get_button_states();

	for(uint8_t i=0; i<3; i++)
		Serial.write(ul_utils_get_bit_group(button_states, 7, i));

	// Button states are now reset.
	reset_button_states();

	// !!! DEBUG
	analogWrite(CONF_GPIO_PWM_A, 0);
	analogWrite(CONF_GPIO_PWM_B, 0);
}

/* USER CODE END 2 */

/* Private user code for ISR (Interrupt Service Routines) --------------------*/
/* USER CODE BEGIN ISR */

/* USER CODE END ISR */
