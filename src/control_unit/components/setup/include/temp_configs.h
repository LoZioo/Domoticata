/**
 * !!! RENDERE CONFIGURABILI A RUNTIME TUTTE QUESTE CONFIGURAZIONI E RIMUOVERE QUESTO HEADER
 * Possibili configurazioni da aggiungere:
 * 	-	Frequenza PWM
 * 	-	Fade time
 * 	-	Soglia potenza cicalino
 */

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
