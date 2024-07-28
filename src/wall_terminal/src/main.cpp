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
	#include <ul_analog_button.h>
}

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

// GPIO
#define CONF_GPIO_PWM_A				0
#define CONF_GPIO_PWM_B				1
#define CONF_GPIO_ADC					A1
#define CONF_GPIO_UART_DE_RE	-1

// Analog buttons
#define CONF_BTN_1_MEAN	80
#define CONF_BTN_2_MIN	145

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

ul_analog_button_handler_t *abtn;

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
 * @brief Analog button event callback.
 * @param button_id The registered button ID that triggered the callback.
 * @param edge `ul_analog_button_edge_t` The edge that triggered the callback.
*/
void ul_analog_button_event_callback(uint8_t button_id, uint8_t edge);

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

	ul_analog_button_init_t abtn_init = {
		.adc_res_bits = 10,
		.adc_read_callback = []() -> uint16_t { return analogRead(CONF_GPIO_ADC); },
		.event_callback = ul_analog_button_event_callback
	};

	ul_analog_button_begin(abtn_init, &abtn);

	/* USER CODE END Init */

	/* USER CODE BEGIN 1 */

	Serial.println(F("Ready"));

	/* USER CODE END 1 */
}

/**
 * @brief Put your main code here, to run repeatedly.
*/
void loop(){

	/* Infinite loop */
	/* USER CODE BEGIN Loop */

	ul_analog_button_evaluate(abtn);
	delay(1);

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

void ul_analog_button_event_callback(uint8_t button_id, uint8_t edge){
	// Serial.print(F("id: "));
	// Serial.print(button_id);
	// Serial.print(F(", edge: "));

	// switch(edge){
	// 	case UL_ANALOG_BUTTON_EDGE_RISING:
	// 		Serial.print(F("rising"));
	// 		break;

	// 	case UL_ANALOG_BUTTON_EDGE_FALLING:
	// 		Serial.print(F("falling"));
	// 		break;

	// 	case UL_ANALOG_BUTTON_EDGE_BOTH:
	// 		Serial.print(F("both"));
	// 		break;

	// 	default:
	// 		Serial.print(F("???"));
	// 		break;
	// }
}

/* USER CODE END 2 */

/* Private user code for ISR (Interrupt Service Routines) --------------------*/
/* USER CODE BEGIN ISR */

/* USER CODE END ISR */