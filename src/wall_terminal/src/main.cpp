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

// Misc
#define ADC_MAX		((1 << 10) - 1)

// GPIO
#define CONF_GPIO_PWM_A				0
#define CONF_GPIO_PWM_B				1
#define CONF_GPIO_ADC					A1
#define CONF_GPIO_UART_DE_RE	-1

// Analog buttons
#define CONF_BTN_VALID_EDGE				(ADC_MAX - 100)
#define CONF_BTN_VALID_INTERVAL		10

#define CONF_BTN_0_MEAN		79
#define CONF_BTN_1_MEAN		143

#define VAL_BTN_0_LOWER_THR		(CONF_BTN_0_MEAN - CONF_BTN_VALID_INTERVAL)
#define VAL_BTN_0_UPPER_THR		(CONF_BTN_0_MEAN + CONF_BTN_VALID_INTERVAL)

#define VAL_BTN_1_LOWER_THR		(CONF_BTN_1_MEAN - CONF_BTN_VALID_INTERVAL)
#define VAL_BTN_1_UPPER_THR		(CONF_BTN_1_MEAN + CONF_BTN_VALID_INTERVAL)

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

	static int8_t button;
	button = analog_button_read(CONF_GPIO_ADC);

	if(button != BUTTON_NONE){
		Serial.println(button);
		delay(100);
	}

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

/* USER CODE END ISR */