#pragma once

#define UART_COMMS 1
//#define I2C_COMMS 1

/**
 * @brief Callback for receive notifications.
 */
typedef void (*recv_callback_t)(int fd, size_t bytes);

/**
 * @brief Set callback function for new data notifications. This function is
 *        called whenever a +NSOMNI message is received from the modem.
 * @note  Only a single callback can be registered.
 */
void receive_callback(recv_callback_t receive_cb);

/**
 * @brief Initialize communications
 */
void modem_init(void);

/**
 * @brief Writes a string to the modem.
 * @param *cmd: The string to send
 */
void modem_write(const char *cmd);

/**
 * @brief Read a single character from the modem.
 */
bool modem_read(uint8_t *b, int32_t timeout);

/**
 * @brief check if modem is ready and online (ie check if there's an assigned IP address)
 */
bool modem_is_ready();

/**
 * Restart modem
 */
void modem_restart();