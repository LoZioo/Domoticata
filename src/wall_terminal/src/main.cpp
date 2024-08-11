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
	#include <ul_button_states.h>
	#include <ul_master_slave.h>
	#include <ul_crc.h>
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

/**
 * @brief Read current button states.
 * @return A member of `ul_bs_button_id_t` from `button_states.h`: `UL_BS_BUTTON_1`, `UL_BS_BUTTON_2`, ...
 */
#define button_read()( \
	(ul_bs_button_id_t)( \
		(UL_BS_BUTTON_NONE) | \
		(!digitalRead(CONFIG_GPIO_BTN_1)) | \
		(!digitalRead(CONFIG_GPIO_BTN_2) << 1) \
	) \
)

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

// Last performed action instant.
uint32_t last_action_ms = 0;

// ADC read value + is changed flag.
struct __attribute__((__packed__)){
	int16_t value: 15;
	bool is_changed: 1;
} adc;

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
	pinMode(CONFIG_GPIO_BTN_1, INPUT_PULLUP);
	pinMode(CONFIG_GPIO_BTN_2, INPUT_PULLUP);
	pinMode(CONFIG_GPIO_UART_DE_RE, OUTPUT);
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

	static uint8_t press_count;

	// Sample everything.
	ul_bs_button_id_t button = button_read();
	int16_t adc_tmp = analogRead(CONFIG_GPIO_ADC);

	// If the trimmer was moved.
	if(ul_utils_in_range(adc_tmp, adc.value, CONFIG_ADC_TRIMMER_DETECT)){
		last_action_ms = millis();

		adc.value = adc_tmp;
		adc.is_changed = true;
	}

	// If a button was pressed.
	if(button != UL_BS_BUTTON_NONE){
		last_action_ms = millis();

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
		 * 	-	The trimmer was rotated or some button was pressed.
		 * 	-	The lock time elapsed after the last performed action.
		 *
		 * Then send the button states.
		 */
		if(
			ul_ms_is_master_byte(b) &&
			ul_ms_decode_master_byte(b) == ul_ms_decode_master_byte(CONFIG_UART_DEVICE_ID) &&
			(
				adc.is_changed ||
				ul_bs_get_button_states() != 0
			) &&
			millis() - last_action_ms >= CONFIG_TIME_LAST_ACTION_LOCK_MS
		){
			send_states();

			// States are now reset.
			adc.is_changed = false;
			ul_bs_reset_button_states();
		}
	}

	// Continue eventual non-blocking delay.
	return true;
}

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
		.trimmer_val = (uint16_t) adc.value,
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
