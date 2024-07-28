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
	BUTTON_NONE = -1,
	BUTTON_0,
	BUTTON_1
};

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

static uint32_t t0;
#define delay_nonblock(ms) \
	t0 = millis(); \
	while(millis() - t0 < ms) \
		delay_nonblock_tasks()

static uint8_t button_clicks[CONF_BTN_COUNT];
#define reset_button_clicks() \
	memset(button_clicks, 0, CONF_BTN_COUNT)

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

/**
 * @brief Poll the ADC and convert the read value to a button.
 */
int8_t analog_button_read(analog_pin_t adc_pin);

/**
 * @brief `delay_nonblock()` macro background tasks.
 * @note No delays allowed inside; treat it like an ISR.
 */
inline void delay_nonblock_tasks();

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

	reset_button_clicks();

	/* USER CODE END 1 */
}

/**
 * @brief Put your main code here, to run repeatedly.
*/
void loop(){

	/* Infinite loop */
	/* USER CODE BEGIN Loop */

	static uint16_t ms = 0;
	static int8_t button;

	if(ms == 999){
		Serial.println(button_clicks[0]);
		Serial.println(button_clicks[1]);
		Serial.println();
	}

	button = analog_button_read(CONF_GPIO_ADC);

	if(button != BUTTON_NONE){
		button_clicks[button]++;
		delay_nonblock(CONF_DEBOUNCE_TIME_MS);
	}

	delay(1);
	ms = (ms + 1) % 1000;

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

int8_t analog_button_read(analog_pin_t adc_pin){

	static uint16_t adc_val;
	static int8_t button;

	adc_val = analogRead(adc_pin);
	button = BUTTON_NONE;

	if(adc_val < CONF_BTN_VALID_EDGE){
		if(ul_utils_between(adc_val, VAL_BTN_0_LOWER_THR, VAL_BTN_0_UPPER_THR))
			button = BUTTON_0;

		else if(ul_utils_between(adc_val, VAL_BTN_1_LOWER_THR, VAL_BTN_1_UPPER_THR))
			button = BUTTON_1;
	}

	return button;
}

/* USER CODE END 2 */

/* Private user code for ISR (Interrupt Service Routines) --------------------*/
/* USER CODE BEGIN ISR */

void delay_nonblock_tasks(){
}

/* USER CODE END ISR */