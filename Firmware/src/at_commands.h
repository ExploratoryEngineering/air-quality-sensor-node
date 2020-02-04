#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define AT_OK 0
#define AT_ERROR -1
#define AT_TIMEOUT -2

/**
 * @brief  Decode AT+NSORF response. The buffer is read in multiple chunks from
 *         the modem.
 * @return -1 for ERROR response, -2 for timeout, number of bytes decoded otherwise
 * @note   Will swallow URCs and call the appropriate callbacks. The lenght of
 *         the buffer must fit the number of bytes that is returned (it's set in
 *         the NSORF command)
 */
int atnsorf_decode(int *sockfd, char *ip, int *port, uint8_t *data, size_t *received, size_t *remaining);

/**
 * @brief Decode AT+CGPADDR response.
 * @return  0 for OK, -1 for ERROR response, -2 for timeout, lenght of address string otherwise
 * @note Will swallow URCs and call appropriate callbacks.  Address might be "0"
 */
int atcgpaddr_decode(char *address, size_t *len);

/**
 * @brief  Decode AT+NSOCR response. Reads until OK or ERROR is received.
 * @return socket file descriptor for modem, -1 for ERROR response, -2 for timeout
 * @note   Will swallow URCs and call the appropriate callbacks
 */
int atnsocr_decode(int *sockfd);

/**
 * @brief  Decode AT+NSOCL response. Reads until OK or ERROR is received.
 * @return 0 for OK, -1 for ERROR
 * @note   Will swallow URCs and call the appropriate callbacks
 */
int atnsocl_decode();

/**
 * @brief  Decode AT+NSOST response. Reads until OK or ERROR is received.
 * @return 0 for OK, -1 for ERROR, -2 for timeout
 * @note   Will swallow URCs and call the appropriate callbacks
 */
int atnsost_decode(int *sock_fd, size_t *sent);

/**
 * @brief Reads response from AT+NRB command. Reads until OK or ERROR is received.
 * @return 0 for OK, -1 for ERROR response, -2 for timeout, -3 for invalid input
 * @note  Will swallow URCs and call the appropriate callbacks
 */
int atnrb_decode();

/**
 * @brief decode AT+CIMI response from modem
 * @note buffer should have enough room for IMSIs (22 chars)
 */
int atcimi_decode(char *imsi);

/**
 * @brief decode AT+CPSMS response from modem.
 */
int atcpsms_decode();


