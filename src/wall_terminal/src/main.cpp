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
	*		- Select the `env:analog_button_tuner` PlatformIO environment.
	*		- Upload the sketch.
	*		- Press the physical button to obtain on the serial monitor, the corresponding ADC value.
	*		- For each read physical button, go to the `conf_var.h` header.
	*		- In that header, define the `CONFIG_BTN_x_AVG` as the read ADC value.
	*		- Note: keep a distance of at least `CONFIG_BTN_VALID_INTERVAL` between the values.
	*		- In the same header, update the `CONFIG_UART_DEVICE_ID`.
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

/**
 * @brief `analog_button_read()` helper.
 */
#define analog_button_if(dest_var, adc_val, btn_interval, btn_avg, button_id) \
	if(ul_utils_between(adc_val, btn_avg - btn_interval, btn_avg + btn_interval)) \
		dest_var = button_id

#ifdef INTERRUPT_SERIAL_RX
	#define uart_available()	purx_dataready()
	#define uart_read_byte()	pu_read()
#else
	#define uart_available()	false
	#define uart_read_byte()	purx()
#endif

#define uart_write_byte(b)	putx(b)

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

bool button_task();
bool uart_task();

/* Generic functions */

void uart_rx_mode();
void uart_tx_mode();

/**
 * @brief Poll the ADC and convert the read value to a button ID.
 * @return A member of `ul_bs_button_id_t` from `button_states.h`: `UL_BS_BUTTON_1`, `UL_BS_BUTTON_2`, ...
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

	// Stabilize the ADC.
	while(millis() < 500)
		analogRead(CONFIG_GPIO_ADC);

	/* USER CODE END Init */
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
	pinMode(CONFIG_GPIO_PWM_A, OUTPUT);
	pinMode(CONFIG_GPIO_PWM_B, OUTPUT);
	pinMode(CONFIG_GPIO_UART_DE_RE, OUTPUT);

	analogWrite(CONFIG_GPIO_PWM_A, CONFIG_PWM_BTN_IDLE);
	analogWrite(CONFIG_GPIO_PWM_B, CONFIG_PWM_BTN_IDLE);
}

void UART_setup(){

	// Recall previously stored `OSCCAL` calibration value.
	uint8_t cal = EEPROM.read(0);

	if(cal < 0x80)
		OSCCAL = cal;

	// RS-485 listen mode.
	uart_rx_mode();
}

bool button_task(){

	static uint8_t press_count;
	ul_bs_button_id_t button = analog_button_read(CONFIG_GPIO_ADC);

	// If a button was pressed.
	if(button != UL_BS_BUTTON_NONE){

		// Update the last button press instant.
		last_button_press_ms = millis();

		// Update the current button state based on it's previous state.
		switch(ul_bs_get_button_state(button)){
			case UL_BS_BUTTON_STATE_IDLE:
				ul_bs_set_button_state(button, UL_BS_BUTTON_STATE_PRESSED);
				analogWrite(button-1, CONFIG_PWM_BTN_PRESSED);
				press_count = 1;
				break;

			case UL_BS_BUTTON_STATE_PRESSED:
			case UL_BS_BUTTON_STATE_DOUBLE_PRESSED:
				press_count++;
				break;

			default:
				break;
		}

		if(press_count == 2){
			ul_bs_set_button_state(button, UL_BS_BUTTON_STATE_DOUBLE_PRESSED);
			analogWrite(button-1, CONFIG_PWM_BTN_DOUBLE_PRESSED);
		}

		else if(ul_utils_between(press_count, 3, CONFIG_TIME_BTN_HELD_TICKS - 1)){
			ul_bs_set_button_state(button, UL_BS_BUTTON_STATE_PRESSED);
			analogWrite(button-1, CONFIG_PWM_BTN_PRESSED);
		}

		else if(press_count == CONFIG_TIME_BTN_HELD_TICKS){
			ul_bs_set_button_state(button, UL_BS_BUTTON_STATE_HELD);
			analogWrite(button-1, CONFIG_PWM_BTN_HOLD);
		}

		// Debouncer.
		ul_utils_delay_nonblock(CONFIG_TIME_BTN_DEBOUNCER_MS, millis, uart_task);
	}

	// Continue eventual non-blocking delay.
	return true;
}

bool uart_task(){

	if(uart_available()){
		uint8_t b = uart_read_byte();

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
			ul_ms_decode_master_byte(b) == ul_ms_decode_master_byte(CONFIG_UART_DEVICE_ID) &&
			ul_bs_get_button_states() != 0 &&
			millis() - last_button_press_ms >= CONFIG_TIME_BTN_LOCK_MS
		){
			send_button_states();

			// Button states are now reset.
			ul_bs_reset_button_states();

			analogWrite(CONFIG_GPIO_PWM_A, CONFIG_PWM_BTN_IDLE);
			analogWrite(CONFIG_GPIO_PWM_B, CONFIG_PWM_BTN_IDLE);
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

ul_bs_button_id_t analog_button_read(analog_pin_t adc_pin){

	uint16_t adc_val;
	ul_bs_button_id_t button;

	adc_val = analogRead(adc_pin);
	button = UL_BS_BUTTON_NONE;

	if(adc_val < CONFIG_BTN_VALID_EDGE){
		#ifdef CONFIG_BTN_1_AVG
			analog_button_if(button, adc_val, CONFIG_BTN_VALID_INTERVAL, CONFIG_BTN_1_AVG, UL_BS_BUTTON_1);
		#endif
		#ifdef CONFIG_BTN_2_AVG
			else analog_button_if(button, adc_val, CONFIG_BTN_VALID_INTERVAL, CONFIG_BTN_2_AVG, UL_BS_BUTTON_2);
		#endif
		#ifdef CONFIG_BTN_3_AVG
			else analog_button_if(button, adc_val, CONFIG_BTN_VALID_INTERVAL, CONFIG_BTN_3_AVG, UL_BS_BUTTON_3);
		#endif
		#ifdef CONFIG_BTN_4_AVG
			else analog_button_if(button, adc_val, CONFIG_BTN_VALID_INTERVAL, CONFIG_BTN_4_AVG, UL_BS_BUTTON_4);
		#endif
		#ifdef CONFIG_BTN_5_AVG
			else analog_button_if(button, adc_val, CONFIG_BTN_VALID_INTERVAL, CONFIG_BTN_5_AVG, UL_BS_BUTTON_5);
		#endif
		#ifdef CONFIG_BTN_6_AVG
			else analog_button_if(button, adc_val, CONFIG_BTN_VALID_INTERVAL, CONFIG_BTN_6_AVG, UL_BS_BUTTON_6);
		#endif
		#ifdef CONFIG_BTN_7_AVG
			else analog_button_if(button, adc_val, CONFIG_BTN_VALID_INTERVAL, CONFIG_BTN_7_AVG, UL_BS_BUTTON_7);
		#endif
		#ifdef CONFIG_BTN_8_AVG
			else analog_button_if(button, adc_val, CONFIG_BTN_VALID_INTERVAL, CONFIG_BTN_8_AVG, UL_BS_BUTTON_8);
		#endif
	}

	return button;
}

void send_button_states(){
	uart_tx_mode();

	// Reply with my ID to get the master's attention.
	uart_write_byte(ul_ms_encode_slave_byte(CONFIG_UART_DEVICE_ID));

	// button_states + CRC.
	uint8_t data[3];

	ul_utils_cast_to_type(data, uint16_t) = ul_bs_get_button_states();
	data[sizeof(data) - 1] = ul_crc_crc8(data, 2);

	/**
	 * Encoding in 4 bytes instead of 3 (the MSb of every byte must be 0 because a slave is talking).
	 * Equivalent of a low memory version of `ul_ms_encode_slave_message()` from `ul_master_slave.h`.
	 */
	for(uint8_t i=0; i<4; i++)
		uart_write_byte(
			ul_ms_encode_slave_byte(
				ul_utils_get_bit_group(
					ul_utils_cast_to_type(data, uint32_t),
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
