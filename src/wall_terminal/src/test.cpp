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
 * @brief Return values of `analog_button_read()`.
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

static uint16_t __button_states;
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

	reset_button_states();
	set_button_state(BUTTON_2, BUTTON_STATE_PRESSED);
	set_button_state(BUTTON_5, BUTTON_STATE_HOLD);
	set_button_state(BUTTON_8, BUTTON_STATE_DOUBLE_PRESSED);

	Serial.println(get_button_state(BUTTON_2));
	Serial.println(get_button_state(BUTTON_5));
	Serial.println(get_button_state(BUTTON_8));

	/* USER CODE END 1 */
}

/**
 * @brief Put your main code here, to run repeatedly.
*/
void loop(){
	/* Infinite loop */
	/* USER CODE BEGIN Loop */

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

/* USER CODE END 2 */

/* Private user code for ISR (Interrupt Service Routines) --------------------*/
/* USER CODE BEGIN ISR */

/* USER CODE END ISR */
