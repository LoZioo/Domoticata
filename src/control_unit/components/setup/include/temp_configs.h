
// !!! RENDERE CONFIGURABILI A RUNTIME TUTTE QUESTE CONFIGURAZIONI E RIMUOVERE QUESTO HEADER

/**
 * Number of slave on the RS-485 bus (from 0 to 127).
 * Set to 0 to disable the RS-485 slave polling
 */
#define CONFIG_RS485_WALL_TERMINAL_COUNT	12

// Response timeout on the poll phase.
#define CONFIG_RS485_WALL_TERMINAL_POLL_TIMEOUT_MS	30

// Response timeout on the data exchange phase.
#define CONFIG_RS485_WALL_TERMINAL_CONN_TIMEOUT_MS	100
