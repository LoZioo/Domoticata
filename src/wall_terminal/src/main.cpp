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
	#include <ul_button_states.h>
	#include <ul_master_slave.h>
	#include <ul_crc.h>
}

// Project libraries.
#include <conf_const.h>
#include <conf_var.h>

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

/**
 * @brief Helper.
 */
#define analog_button_if(dest_var, adc_val, btn_interval, btn_avg, button_id) \
	if(ul_utils_between(adc_val, btn_avg - btn_interval, btn_avg + btn_interval)) \
		dest_var = button_id

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
 * @return A member of `ul_bs_button_id_t` from `button_states.h`: `BUTTON_1`, `BUTTON_2`, ...
 */
ul_bs_button_id_t analog_button_read(analog_pin_t adc_pin);

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
	ul_bs_button_id_t button = analog_button_read(CONF_GPIO_ADC);

	// If a button was pressed.
	if(button != BUTTON_NONE){

		// Update the last button press instant.
		last_button_press_ms = millis();

		// Update the current button state based on it's previous state.
		switch(ul_bs_get_button_state(button)){
			case BUTTON_STATE_IDLE:
				ul_bs_set_button_state(button, BUTTON_STATE_PRESSED);
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
			ul_bs_set_button_state(button, BUTTON_STATE_DOUBLE_PRESSED);

			// !!! DEBUG
			analogWrite(button-1, 255);
		}

		else if(ul_utils_between(press_count, 3, CONF_TIME_BTN_HELD_TICKS - 1)){
			ul_bs_set_button_state(button, BUTTON_STATE_PRESSED);

			// !!! DEBUG
			analogWrite(button-1, 40);
		}

		else if(press_count == CONF_TIME_BTN_HELD_TICKS){
			ul_bs_set_button_state(button, BUTTON_STATE_HELD);

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
			ul_ms_is_master_byte(b) &&
			ul_ms_decode_master_byte(b) == ul_ms_decode_master_byte(CONF_UART_DEVICE_ID) &&
			ul_bs_get_button_states() != 0 &&
			millis() - last_button_press_ms >= CONF_TIME_BTN_LOCK_MS
		)
			send_button_states();
	}

	// Continue eventual non-blocking delay.
	return true;
}

ul_bs_button_id_t analog_button_read(analog_pin_t adc_pin){

	uint16_t adc_val;
	ul_bs_button_id_t button;

	adc_val = analogRead(adc_pin);
	button = BUTTON_NONE;

	if(adc_val < CONF_BTN_VALID_EDGE){
		#ifdef CONF_BTN_1_AVG
			analog_button_if(button, adc_val, CONF_BTN_VALID_INTERVAL, CONF_BTN_1_AVG, BUTTON_1);
		#endif
		#ifdef CONF_BTN_2_AVG
			else analog_button_if(button, adc_val, CONF_BTN_VALID_INTERVAL, CONF_BTN_2_AVG, BUTTON_2);
		#endif
		#ifdef CONF_BTN_3_AVG
			else analog_button_if(button, adc_val, CONF_BTN_VALID_INTERVAL, CONF_BTN_3_AVG, BUTTON_3);
		#endif
		#ifdef CONF_BTN_4_AVG
			else analog_button_if(button, adc_val, CONF_BTN_VALID_INTERVAL, CONF_BTN_4_AVG, BUTTON_4);
		#endif
		#ifdef CONF_BTN_5_AVG
			else analog_button_if(button, adc_val, CONF_BTN_VALID_INTERVAL, CONF_BTN_5_AVG, BUTTON_5);
		#endif
		#ifdef CONF_BTN_6_AVG
			else analog_button_if(button, adc_val, CONF_BTN_VALID_INTERVAL, CONF_BTN_6_AVG, BUTTON_6);
		#endif
		#ifdef CONF_BTN_7_AVG
			else analog_button_if(button, adc_val, CONF_BTN_VALID_INTERVAL, CONF_BTN_7_AVG, BUTTON_7);
		#endif
		#ifdef CONF_BTN_8_AVG
			else analog_button_if(button, adc_val, CONF_BTN_VALID_INTERVAL, CONF_BTN_8_AVG, BUTTON_8);
		#endif
	}

	return button;
}

void send_button_states(){

	// Reply with my ID to get the master's attention.
	Serial.write(ul_ms_encode_slave_byte(CONF_UART_DEVICE_ID));

	// button_states + CRC.
	uint8_t data[3];

	ul_utils_cast_to_type(data, uint16_t) = ul_bs_get_button_states();
	data[sizeof(data) - 1] = ul_crc_crc8(data, 2);

	/**
	 * Encoding in 4 bytes instead of 3 (the MSb of every byte must be 0 because a slave is talking).
	 * Equivalent of a low memory version of `ul_ms_encode_slave_message()` from `ul_master_slave.h`.
	 */
	for(uint8_t i=0; i<4; i++)
		Serial.write(
			ul_ms_encode_slave_byte(
				ul_utils_get_bit_group(
					ul_utils_cast_to_type(data, uint32_t),
					7, i
				)
			)
		);

	// Button states are now reset.
	ul_bs_reset_button_states();

	// !!! DEBUG
	analogWrite(CONF_GPIO_PWM_A, 0);
	analogWrite(CONF_GPIO_PWM_B, 0);
}

/* USER CODE END 2 */

/* Private user code for ISR (Interrupt Service Routines) --------------------*/
/* USER CODE BEGIN ISR */

/* USER CODE END ISR */
