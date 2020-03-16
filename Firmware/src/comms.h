#pragma once

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

/**
 * Restart modem (in a more network friendly way than modem_restart())
 *  NOTE: This function has to be tested while moving between cells - and maybe also renamed at some point.
 */
void modem_restart_without_triggering_network_signalling_storm_but_hopefully_picking_up_the_correct_cell___maybe();

void get_IMEI();