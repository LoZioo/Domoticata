/**
 * !!! RENDERE CONFIGURABILI A RUNTIME TUTTE QUESTE CONFIGURAZIONI E RIMUOVERE QUESTO HEADER
 * Possibili configurazioni da aggiungere:
 * 	-	Frequenza PWM
 * 	-	Fade time
 * 	-	Soglia potenza cicalino
 */

// Transformer: 232.9V vs 14.29V
// Applique: a total of 12.19V for 4 led @ 248mA

// LEDC PWM max duty.
#define CONFIG_PWM_DUTY_MAX		((1 << CONFIG_PWM_BIT_RES) - 1)

// Max number of buttons per wall terminal.
#define CONFIG_BUTTONS_MAX_NUMBER_PER_TERMINAL	UL_BS_BUTTON_3

/**
 * Number of wall terminals on the RS-485 bus (from 0 to 127).
 * Set to 0 to disable the RS-485 slave polling
 */
#define CONFIG_RS485_WALL_TERMINALS_COUNT		13

// Response timeout on the poll phase.
#define CONFIG_RS485_WALL_TERMINAL_POLL_TIMEOUT_MS	30

// Response timeout on the data exchange phase.
#define CONFIG_RS485_WALL_TERMINAL_CONN_TIMEOUT_MS	100

// Number of PWM channels on the physical board.
#define CONFIG_PWM_INDEXES	13

// Default LEDC PWM value at startup when a button is pressed.
#define CONFIG_PWM_DEFAULT	512

// !!! VEDERE SE GIRANDO IL TRIMMER, IL LED RISPONDE BENE E NON "A GRADINI"
// !!! NEL CASO, IMPLEMENTARE CALCOLO DEL DELTA TRA UN CAMPIONE RICEVUTO E IL SUCCESSIVO (AVENTI LO STESSO DEVICE ID)
// !!! TUTTO CIO' NEL CASO E' DA FARE IN `handle_trimmer_change()`
#define CONFIG_PWM_FADE_TIME_MS		500

#define CONFIG_PWM_GAMMA_CORRECTION		2.2

/**
 * f: (device_id x button_id x button state) -> pwm_index
 *
 * Row n: wall terminal with `device_id` = n.
 * Coloumn n: button n.
 *
 * Element 1: button pressed.
 * Element 2: button double pressed.
 * Element 3: button held.
 *
 * Note: -1 means not mapped.
 */
#define CONFIG_BUTTON_MATRIX	\
{ \
	{{  0,  3,  2 }, { -1, -1, -1 }, { -1, -1, -1 }}, \
	{{  1,  4,  5 }, {  6,  7,  8 }, {  9, 10, 11 }}, \
	{{ -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 }}, \
	{{ -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 }}, \
	{{ -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 }}, \
	{{ -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 }}, \
	{{ -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 }}, \
	{{ -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 }}, \
	{{ -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 }}, \
	{{ -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 }}, \
	{{ -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 }}, \
	{{ -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 }}, \
	{{ -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 }}, \
}

/**
 * f: device_id -> pwm_index
 * Note: -1 means not mapped.
 */
#define CONFIG_TRIMMER_MATRIX	\
	{ -1, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }
