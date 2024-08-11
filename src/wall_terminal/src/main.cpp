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
	*
	*	To use this source code, you need to follow these steps first:
	*		- Open the Arduino IDE.
	*		- Flash the bootloader with "9.6 MHz internal osc." selected.
	*		- Select the `env:oscillator_tuner` PlatformIO environment.
	*		- Upload the sketch.
	*		- Open the serial monitor and spam 'x' to detect the right `OSCCAL`.
	*		- Open the `conf_var.h` header, update the `CONFIG_UART_DEVICE_ID`.
	*		- Select the `env:main` PlatformIO environment.
	*		- Upload the sketch.
	*		- Enjoy.
	*
	******************************************************************************
	*/
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
/* USER CODE BEGIN Includes */

// Platform libraries.
#include <Arduino.h>
#include <EEPROM.h>

// UniLibC libraries.
extern "C" {
	#include <ul_errors.h>
	#include <ul_utils.h>
	#include <ul_master_slave.h>
	#include <ul_crc.h>

	#ifndef CONFIG_HW_NO_BTN
		#include <ul_button_states.h>
	#endif
}

// Project libraries.
#include <conf_const.h>
#include <conf_var.h>

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

#ifdef INTERRUPT_SERIAL_RX
	#define uart_available()	purx_dataready()
	#define uart_read_byte()	pu_read()
#else
	#define uart_available()	false
	#define uart_read_byte()	purx()
#endif

#define uart_write_byte(b) \
	putx(b)

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

// Last button press instant.
#ifndef CONFIG_HW_NO_BTN
uint32_t last_button_press_ms = 0;
#endif

// ADC is changed flag.
#ifdef CONFIG_HW_TRIMMER
bool adc_is_changed = false;
#endif

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* Setup routines */

/**
 * @brief GPIO initialization.
 */
void GPIO_setup();

/**
 * @brief UART initialization.
 */
void UART_setup();

/* Background tasks */

bool sample_task();
bool send_task();

/* Generic functions */

#ifndef CONFIG_HW_NO_BTN

/**
 * @brief Read current button states.
 * @return A member of `ul_bs_button_id_t` from `button_states.h`: `UL_BS_BUTTON_1`, `UL_BS_BUTTON_2`, ...
 */
ul_bs_button_id_t button_read();

#endif

void uart_rx_mode();
void uart_tx_mode();

/**
 * @brief Send current button states and trimmer value to the control unit.
 * @note This function will reset all states after sending them.
 */
void send_states();

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
}

/**
 * @brief Put your main code here, to run repeatedly.
*/
void loop(){

	/* Infinite loop */
	/* USER CODE BEGIN Loop */

	sample_task();
	send_task();

	/* USER CODE END Loop */
}

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 2 */

void GPIO_setup(){
	pinMode(CONFIG_GPIO_UART_DE_RE, OUTPUT);

	#ifdef CONFIG_HW_BTN_1
		pinMode(CONFIG_GPIO_BTN_1, INPUT_PULLUP);
	#endif

	#ifdef CONFIG_HW_BTN_2
		pinMode(CONFIG_GPIO_BTN_2, INPUT_PULLUP);
	#endif
}

void UART_setup(){

	// Recall previously stored `OSCCAL` calibration value.
	uint8_t cal = EEPROM.read(0);

	if(cal < 0x80)
		OSCCAL = cal;

	// RS-485 listen mode.
	uart_rx_mode();
}

bool sample_task(){

	#ifndef CONFIG_HW_NO_BTN
	static uint8_t press_count;
	#endif

	// Sample trimmer.
	#ifdef CONFIG_HW_TRIMMER
	static int16_t adc_last_value = 0;
	#endif

	// Sample buttons.
	#ifndef CONFIG_HW_NO_BTN
	ul_bs_button_id_t button = button_read();
	#endif

	#ifdef CONFIG_HW_TRIMMER
	int16_t adc_value = analogRead(CONFIG_GPIO_ADC);

	// If the trimmer was moved.
	if(!ul_utils_in_range(adc_value, adc_last_value, CONFIG_ADC_TRIMMER_DETECT)){
		adc_last_value = adc_value;
		adc_is_changed = true;
	}
	#endif

	// If a button was pressed.
	#ifndef CONFIG_HW_NO_BTN
	if(button != UL_BS_BUTTON_NONE){
		last_button_press_ms = millis();

		// Update the current button state based on it's previous state.
		switch(ul_bs_get_button_state(button)){
			case UL_BS_BUTTON_STATE_IDLE:
				ul_bs_set_button_state(button, UL_BS_BUTTON_STATE_PRESSED);
				press_count = 1;
				break;

			case UL_BS_BUTTON_STATE_PRESSED:
			case UL_BS_BUTTON_STATE_DOUBLE_PRESSED:
				press_count++;
				break;

			default:
				break;
		}

		if(press_count == 2)
			ul_bs_set_button_state(button, UL_BS_BUTTON_STATE_DOUBLE_PRESSED);

		else if(ul_utils_between(press_count, 3, CONFIG_TIME_BTN_HELD_TICKS - 1))
			ul_bs_set_button_state(button, UL_BS_BUTTON_STATE_PRESSED);

		else if(press_count == CONFIG_TIME_BTN_HELD_TICKS)
			ul_bs_set_button_state(button, UL_BS_BUTTON_STATE_HELD);

		// Debouncer.
		ul_utils_delay_nonblock(CONFIG_TIME_BTN_DEBOUNCER_MS, millis, send_task);
	}
	#endif

	// Continue eventual non-blocking delay.
	return true;
}

bool send_task(){

	if(uart_available()){
		uint8_t b = uart_read_byte();

		/**
		 * If:
		 * 	-	I read a master byte.
		 * 	-	It's my turn on the bus.
		 * 	-	The trimmer was rotated or:
		 * 	-	Some button was pressed and...
		 * 	-	...the lock time elapsed after the last button press.
		 *
		 * Then send the states and immediately reset them.
		 */
		if(
			ul_ms_is_master_byte(b) &&
			ul_ms_decode_master_byte(b) == ul_ms_decode_master_byte(CONFIG_UART_DEVICE_ID) &&
			(
				#ifdef CONFIG_HW_TRIMMER
					adc_is_changed
				#endif

				#if defined(CONFIG_HW_TRIMMER) && !defined(CONFIG_HW_NO_BTN)
					||
				#endif

				#ifndef CONFIG_HW_NO_BTN
					(
						ul_bs_get_button_states() != 0 &&
						millis() - last_button_press_ms >= CONFIG_TIME_BTN_LOCK_MS
					)
				#endif
			)
		){
			send_states();

			// States are now reset.
			#ifdef CONFIG_HW_TRIMMER
			adc_is_changed = false;
			#endif

			#ifndef CONFIG_HW_NO_BTN
			ul_bs_reset_button_states();
			#endif
		}
	}

	// Continue eventual non-blocking delay.
	return true;
}

#ifndef CONFIG_HW_NO_BTN
ul_bs_button_id_t button_read(){
	uint8_t pinb = PINB;
	return (ul_bs_button_id_t)(
		UL_BS_BUTTON_NONE
		#ifdef CONFIG_HW_BTN_1
			| (!(pinb & _BV(CONFIG_GPIO_BTN_1)))
		#endif
		#ifdef CONFIG_HW_BTN_2
			| (!(pinb & _BV(CONFIG_GPIO_BTN_2)) << 1)
		#endif
	);
}
#endif

void uart_rx_mode(){
	digitalWrite(CONFIG_GPIO_UART_DE_RE, LOW);
}

void uart_tx_mode(){
	digitalWrite(CONFIG_GPIO_UART_DE_RE, HIGH);
	delayMicroseconds(CONFIG_UART_TX_MODE_DELAY_US);
}

void send_states(){
	uart_tx_mode();

	// Reply with my ID to get the master's attention.
	uart_write_byte(ul_ms_encode_slave_byte(CONFIG_UART_DEVICE_ID));

	struct __attribute__((__packed__)){
		uint16_t trimmer_val: 10;
		uint16_t button_states: 6;
		uint8_t crc8;
	} data = {
		.trimmer_val = (uint16_t) analogRead(CONFIG_GPIO_ADC),
		.button_states = ul_bs_get_button_states(),
		.crc8 = ul_crc_crc8(ul_utils_cast_to_mem(data), 2)
	};

	/**
	 * Encoding in 4 bytes instead of 3 (the MSb of every byte must be 0 because a slave is talking).
	 * Equivalent of a low memory version of `ul_ms_encode_slave_message()` from `ul_master_slave.h`.
	 */
	for(uint8_t i=0; i<4; i++)
		uart_write_byte(
			ul_ms_encode_slave_byte(
				ul_utils_get_bit_group(
					ul_utils_cast_to_type(&data, uint32_t),
					7, i
				)
			)
		);

	uart_rx_mode();
}

/* USER CODE END 2 */

/* Private user code for ISR (Interrupt Service Routines) --------------------*/
/* USER CODE BEGIN ISR */

/* USER CODE END ISR */
