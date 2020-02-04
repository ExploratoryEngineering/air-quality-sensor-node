/*
 * Copyright (c) 2018 Foundries.io
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_LEVEL LOG_LEVEL_INF
#include <logging/log.h>
LOG_MODULE_REGISTER(n2_offload);

#include <stdio.h>
#include <stdbool.h>
#include <zephyr/types.h>
#include <errno.h>
#include <zephyr.h>
#include <device.h>
#include <init.h>
#include <net/net_offload.h>
#include <net/socket_offload.h>
#include <stdio.h>

#include "config.h"
#include "comms.h"
#include "at_commands.h"

// The maximum number of sockets in SARA N2 is 7
#define MDM_MAX_SOCKETS 7
#define INVALID_FD -1
#define MAX_RECEIVE 512

struct n2_socket
{
    int id;
    int in_use;
    bool connected;
    int local_port;
    ssize_t incoming_len;
    void *remote_addr;
    ssize_t remote_len;
};

static struct n2_socket sockets[MDM_MAX_SOCKETS];

static int next_free_port = 6000;

#define CMD_BUFFER_SIZE 64
static char modem_command_buffer[CMD_BUFFER_SIZE];

#define CMD_TIMEOUT 2000

#define TO_HEX(i) (i <= 9 ? '0' + i : 'A' - 10 + i)
#define S_TO_I(s) (s - 100)
#define I_TO_S(i) (i + 100)
#define VALID_SOCKET(s) (s >= 100 && s <= (100 + MDM_MAX_SOCKETS) && sockets[s-100].in_use)

static struct k_sem mdm_sem;

/**
 * @brief Clear socket state
 */
static void clear_socket(int sock_fd)
{
    sockets[sock_fd].id = -1;
    sockets[sock_fd].connected = false;
    sockets[sock_fd].in_use = false;
    sockets[sock_fd].local_port = 0;
    sockets[sock_fd].incoming_len = 0;
    sockets[sock_fd].remote_len = 0;
    if (sockets[sock_fd].remote_addr != NULL)
    {
        k_free(sockets[sock_fd].remote_addr);
        sockets[sock_fd].remote_addr = NULL;
    }
}

static int offload_close(int sfd)
{
    if (!VALID_SOCKET(sfd))
    {
        return -EINVAL;
    }
    int sock_fd = S_TO_I(sfd);
    k_sem_take(&mdm_sem, K_FOREVER);
    sprintf(modem_command_buffer, "AT+NSOCL=%d\r", sockets[sock_fd].id);
    modem_write(modem_command_buffer);

    if (atnsocl_decode() != AT_OK)
    {
        k_sem_give(&mdm_sem);
        return -ENOMEM;
    }
    clear_socket(sock_fd);
    k_sem_give(&mdm_sem);
    return 0;
}

static int offload_connect(int sfd, const struct sockaddr *addr,
                           socklen_t addrlen)
{
    if (!VALID_SOCKET(sfd))
    {
        return -EINVAL;
    }
    int sock_fd = S_TO_I(sfd);
    k_sem_take(&mdm_sem, K_FOREVER);
    // Find matching socket, then check if it created on the modem. It shouldn't be created
    if (!sockets[sock_fd].in_use)
    {
        k_sem_give(&mdm_sem);
        return -EISCONN;
    }

    sockets[sock_fd].connected = true;
    sockets[sock_fd].remote_addr = k_malloc(addrlen);
    memcpy(sockets[sock_fd].remote_addr, addr, addrlen);
    k_sem_give(&mdm_sem);
    return 0;
}

static int offload_poll(struct pollfd *fds, int nfds, int msecs)
{
    // Not *quite* how it should behave but close enough.
    if (msecs > 0) {
        k_sleep(msecs);
    }
    k_sem_take(&mdm_sem, K_FOREVER);
    for (int i = 0; i < nfds; i++)
    {
        if (!VALID_SOCKET(fds[i].fd))
        {
            fds[i].revents = POLLNVAL;
            continue;
        }
        fds[i].revents = POLLOUT;
        if (sockets[S_TO_I(fds[i].fd)].incoming_len > 0)
        {
            fds[i].revents |= POLLIN;
        }
    }
    k_sem_give(&mdm_sem);
    return 0;
}

static int offload_recvfrom(int sfd, void *buf, short int len,
                            short int flags, struct sockaddr *from,
                            socklen_t *fromlen)
{
    ARG_UNUSED(flags);
    if (!VALID_SOCKET(sfd))
    {
        return -EINVAL;
    }
    int sock_fd = S_TO_I(sfd);
    k_sem_take(&mdm_sem, K_FOREVER);

    // Now here's an interesting bit of information: If you send AT+NSORF *before*
    // you receive the +NSONMI URC from the module you'll get just three fields
    // in return: socket, data, remaining. IT WOULD HAVE BEEN REALLY NICE IF THE
    // DOCUMENTATION INCLUDED THIS.

    if (sockets[sock_fd].incoming_len == 0)
    {
        k_sem_give(&mdm_sem);
        errno = EWOULDBLOCK;
        return 0;
    }

    // Use NSORF to read incoming data.
    memset(modem_command_buffer, 0, sizeof(modem_command_buffer));
    if (len > MAX_RECEIVE) {
        len = MAX_RECEIVE;
    }
    sprintf(modem_command_buffer, "AT+NSORF=%d,%d\r", sockets[sock_fd].id, len);
    modem_write(modem_command_buffer);

    char ip[16];
    int port = 0;
    size_t remain = 0;
    int sockfd = 0;
    size_t received = 0;

    int res = atnsorf_decode(&sockfd, ip, &port, buf, &received, &remain);
    if (res == AT_OK)
    {
        if (received == 0)
        {
            k_sem_give(&mdm_sem);
            return 0;
        }
        if (fromlen != NULL)
        {
            *fromlen = sizeof(struct sockaddr_in);
        }
        if (from != NULL)
        {
            ((struct sockaddr_in *)from)->sin_family = AF_INET;
            ((struct sockaddr_in *)from)->sin_port = htons(port);
            inet_pton(AF_INET, ip, &((struct sockaddr_in *)from)->sin_addr);
        }
        sockets[sock_fd].incoming_len = remain;
        k_sem_give(&mdm_sem);
        return received;
    }
    k_sem_give(&mdm_sem);
    errno = -ENOMEM;
    return -ENOMEM;
}

static int offload_recv(int sfd, void *buf, size_t max_len, int flags)
{
    ARG_UNUSED(flags);

    if (!VALID_SOCKET(sfd))
    {
        return -EINVAL;
    }
    int sock_fd = S_TO_I(sfd);
    k_sem_take(&mdm_sem, K_FOREVER);
    if (!sockets[sock_fd].connected)
    {
        k_sem_give(&mdm_sem);
        return -EINVAL;
    }

    if (sockets[sock_fd].incoming_len == 0 && ((flags & MSG_DONTWAIT) == MSG_DONTWAIT))
    {
        k_sem_give(&mdm_sem);
        errno = EWOULDBLOCK;
        return 0;
    }

    int curcount = sockets[sock_fd].incoming_len;
    k_sem_give(&mdm_sem);

    while (curcount == 0)
    {
        // busy wait for data
        k_sleep(1000);
        k_sem_take(&mdm_sem, K_FOREVER);
        curcount = sockets[sock_fd].incoming_len;
        k_sem_give(&mdm_sem);
    }
    return offload_recvfrom(sfd, buf, max_len, flags, NULL, NULL);
}

static int offload_sendto(int sfd, const void *buf, size_t len,
                          int flags, const struct sockaddr *to,
                          socklen_t tolen)
{
    if (!VALID_SOCKET(sfd))
    {
        return -EINVAL;
    }

    if (len > CONFIG_N2_MAX_PACKET_SIZE)
    {
        return -EINVAL;
    }
    int sock_fd = S_TO_I(sfd);
    k_sem_take(&mdm_sem, K_FOREVER);

    struct sockaddr_in *toaddr = (struct sockaddr_in *)to;

    char addr[64];
    if (!inet_ntop(AF_INET, &toaddr->sin_addr, addr, 128))
    {
        // couldn't read address. Bail out
        k_sem_give(&mdm_sem);
        return -EINVAL;
    }

    sprintf(modem_command_buffer,
            "AT+NSOST=%d,\"%s\",%d,%d,\"",
            sockets[sock_fd].id, addr,
            ntohs(toaddr->sin_port),
            len);

    modem_write(modem_command_buffer);

    char byte[3];
    for (int i = 0; i < len; i++)
    {
        byte[0] = TO_HEX((((const char *)buf)[i] >> 4));
        byte[1] = TO_HEX((((const char *)buf)[i] & 0xF));
        byte[2] = 0;
        modem_write(byte);
    }

    modem_write("\"\r");

    int written = len;
    int fd = -1;
    size_t sent = 0;
    switch (atnsost_decode(&fd, &sent))
    {
    case AT_OK:
        break;
    case AT_ERROR:
        written = -ENOMEM;
        break;
    case AT_TIMEOUT:
        written = -ENOMEM;
        break;
    }
    k_sem_give(&mdm_sem);

    return written;
}

static int offload_send(int sfd, const void *buf, size_t len, int flags)
{
    if (!VALID_SOCKET(sfd))
    {
        return -EINVAL;
    }
    int sock_fd = S_TO_I(sfd);
    k_sem_take(&mdm_sem, K_FOREVER);

    if (!sockets[sock_fd].connected)
    {
        k_sem_give(&mdm_sem);
        return -ENOTCONN;
    }
    k_sem_give(&mdm_sem);
    int ret = offload_sendto(sfd, buf, len, flags,
                             sockets[sock_fd].remote_addr, sockets[sock_fd].remote_len);
    return ret;
}

static int offload_socket(int family, int type, int proto)
{
    if (family != AF_INET)
    {
        return -EAFNOSUPPORT;
    }
    if (type != SOCK_DGRAM)
    {
        return -ENOTSUP;
    }
    if (proto != IPPROTO_UDP)
    {
        return -ENOTSUP;
    }

    k_sem_take(&mdm_sem, K_FOREVER);
    int fd = INVALID_FD;
    for (int i = 0; i < MDM_MAX_SOCKETS; i++) {
        if (!sockets[i].in_use) {
            fd = i;
            break;
        }
    }
    if (fd == INVALID_FD) {
        k_sem_give(&mdm_sem);
        return -ENOMEM;
    }
    sockets[fd].local_port = next_free_port;
    sprintf(modem_command_buffer, "AT+NSOCR=\"DGRAM\",17,%d,1\r", sockets[fd].local_port);
    modem_write(modem_command_buffer);

    int sockfd = -1;
    if (atnsocr_decode(&sockfd) == AT_OK)
    {
        sockets[fd].id = sockfd;
        sockets[fd].in_use = true;
        next_free_port++;
        k_sem_give(&mdm_sem);
        return I_TO_S(fd);
    }
    k_sem_give(&mdm_sem);
    return -ENOMEM;
}

// We're only interested in socket(), close(), connect(), poll()/POLLIN, send() and recvfrom()
// since that's what the lwm2m client/coap library uses.
// bind(), accept(), fctl(), freeaddrinfo(), getaddrinfo(), setsockopt(),
// getsockopt() and listen() is not implemented
static const struct socket_offload n2_socket_offload = {
    .socket = offload_socket,
    .close = offload_close,
    .connect = offload_connect,
    .poll = offload_poll,
    .recv = offload_recv,
    .recvfrom = offload_recvfrom,
    .send = offload_send,
    .sendto = offload_sendto,
};

static int dummy_offload_get(sa_family_t family,
                             enum net_sock_type type,
                             enum net_ip_protocol ip_proto,
                             struct net_context **context)
{
    return -ENOTSUP;
}

// Zephyr doesn't like a null offload so we'll use a dummy offload here.
static struct net_offload offload_funcs = {
    .get = dummy_offload_get,
};

// Offload the interface. This will set the dummy offload functions then
// the socket offloading.
static void offload_iface_init(struct net_if *iface)
{
    for (int i = 0; i < MDM_MAX_SOCKETS; i++)
    {
        sockets[i].id = -1;
        sockets[i].remote_addr = NULL;
    }
    iface->if_dev->offload = &offload_funcs;
    socket_offload_register(&n2_socket_offload);
}

static struct net_if_api api_funcs = {
    .init = offload_iface_init,
};

static void receive_cb(int fd, size_t bytes)
{
    k_sem_take(&mdm_sem, K_FOREVER);
    for (int i = 0; i < MDM_MAX_SOCKETS; i++)
    {
        if (sockets[i].id == fd)
        {
            sockets[i].incoming_len += bytes;
        }
    }
    k_sem_give(&mdm_sem);
}

// _init initializes the network offloading
static int n2_init(struct device *dev)
{
    ARG_UNUSED(dev);

    k_sem_init(&mdm_sem, 1, 1);

    receive_callback(receive_cb);

    modem_init();
    return 0;
}

NET_DEVICE_OFFLOAD_INIT(sara_n2, CONFIG_N2_NAME,
                        n2_init, NULL, NULL,
                        CONFIG_N2_INIT_PRIORITY, &api_funcs,
                        CONFIG_N2_MAX_PACKET_SIZE);
