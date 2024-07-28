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

// Header.
#include <const.h>

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/**
 * @brief Button numbers.
 * @note Return values of `analog_button_read()`.
 */
enum {
	BUTTON_NONE = 0,
	BUTTON_1,
	BUTTON_2,
	BUTTON_3,
	BUTTON_4,
	BUTTON_5,
	BUTTON_6,
	BUTTON_7,
	BUTTON_8
};

/**
 * @brief Button states (4-bits).
 */
enum {
	BUTTON_STATE_IDLE,
	BUTTON_STATE_PRESSED,
	BUTTON_STATE_DOUBLE_PRESSED,
	BUTTON_STATE_HOLD,
};

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* Non-blocking delay */

static uint32_t button_t0, uart_t0;
#define delay_nonblock(ms, t0, background_routine) \
	t0 = millis(); \
	while(millis() - t0 < ms) \
		background_routine()

/* Button management */

static uint16_t __button_states = 0;
#define __btn_bit_shift(button)	(button * 2 - 2)
#define __btn_bit_mask(button)		(3 << __btn_bit_shift(button))

#define reset_button_states() \
	__button_states = 0

/**
 * @brief Set the button state of the specified button.
 * @param button `BUTTON_1`, `BUTTON_2`, ...
 * @param state `BUTTON_STATE_PRESSED`, `BUTTON_STATE_DOUBLE_PRESSED`, ...
 */
#define set_button_state(button, state) \
	__button_states = \
	( \
		__button_states & \
		~( (uint16_t) __btn_bit_mask(button) ) \
	) | ( state << __btn_bit_shift(button) )

/**
 * @brief Get the button state of the specified button.
 * @param button `BUTTON_1`, `BUTTON_2`, ...
 * @return `BUTTON_STATE_PRESSED`, `BUTTON_STATE_DOUBLE_PRESSED`, ...
 */
#define get_button_state(button) ( \
	( \
		__button_states & \
		__btn_bit_mask(button) \
	) >> __btn_bit_shift(button) \
)

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

// !!! METTERE IN EEPROM
uint8_t device_id = 'x';

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/**
 * @brief GPIO initialization.
 */
void GPIO_setup();

/**
 * @brief GPIO initialization.
 */
void UART_setup();

/**
 * @brief Poll the ADC and convert the read value to a button.
 */
uint8_t analog_button_read(analog_pin_t adc_pin);

/**
 * @note No delays allowed inside; treat it like an ISR.
 */
void button_task();

/**
 * @note No delays allowed inside; treat it like an ISR.
 */
void uart_task();

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

	delay_nonblock(CONF_LOOP_PERIOD_MS, button_t0, button_task);

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

uint8_t analog_button_read(analog_pin_t adc_pin){

	static uint16_t adc_val;
	static uint8_t button;

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

/* USER CODE END 2 */

/* Private user code for ISR (Interrupt Service Routines) --------------------*/
/* USER CODE BEGIN ISR */

void button_task(){

	static int8_t button;
	button = analog_button_read(CONF_GPIO_ADC);

	if(button != BUTTON_NONE){

		// !!! IMPLEMENTARE RILEVAZIONE DOUBLE PRESS E HOLD
		set_button_state(button, BUTTON_STATE_PRESSED);
		delay_nonblock(CONF_DEBOUNCE_TIME_MS, uart_t0, uart_task);
	}

	uart_task();
}

void uart_task(){

	if(Serial.available() && Serial.read() == device_id){
		Serial.write(device_id);
		Serial.write(' ');

		Serial.print(get_button_state(BUTTON_1));
		Serial.print(F(", "));
		Serial.print(get_button_state(BUTTON_2));
		Serial.println();

		reset_button_states();
	}
}

/* USER CODE END ISR */
