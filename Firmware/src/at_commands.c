#include <zephyr.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include "at_commands.h"
#include "comms.h"

#include <logging/log.h>
#define LOG_LEVEL LOG_LEVEL_DBG
LOG_MODULE_REGISTER(at_commands);

#define CMD_TIMEOUT K_MSEC(2000)
#define CMD_REBOOT_TIMEOUT K_MSEC(15000)

/* this is a helper buffer to keep track of the input from the UART. The buffer
   is quite short and can't keep track of longer strings. It is mainly concerned
   with catching +NSONMI URCs from the modem */
#define B_SIZE 10

struct buf
{
    uint8_t data[B_SIZE];
    uint8_t index;
    uint16_t size;
};

void b_reset(struct buf *rb)
{
    rb->index = 0;
    rb->size = 0;
    memset(rb->data, 0, B_SIZE);
}

void b_init(struct buf *rb)
{
    b_reset(rb);
}
void b_add(struct buf *rb, uint8_t b)
{
    if (rb->index < B_SIZE)
    {
        rb->data[rb->index++] = b;
    }
    rb->size++;
}

bool b_is(struct buf *rb, const char *str, const size_t len)
{
    if (strncmp((char *)rb->data, str, len) == 0)
    {
        return true;
    }
    return false;
}

bool b_is_urc(struct buf *rb)
{
    return (rb->size > 0 && rb->data[0] == '+');
}

// Callbacks for the input processing. The context is used to maintain variables
// between the invocations and are passed by the decode_input function below.

// The end of line callback - called every time an end-of-line character is found
typedef void (*eol_callback_t)(void *ctx, struct buf *rb, bool is_urc);

// Character callback - called for every character in the input. Both the character callback
// and the EOL callback is called when the processing reaches an end of line character
typedef void (*char_callback_t)(void *ctx, struct buf *rb, char b, bool is_urc, bool is_space);

// Each decoder is largely the same so we use a strategy pattern for each. The char
// callback is called for each character input and the EOL callback is called when a
// new line is found. The buffer will contain the *first* 9 characters of the line
// so it might be truncated.
int decode_input(int32_t timeout, void *ctx, char_callback_t char_cb, eol_callback_t eol_cb)
{
    if (timeout < 0) {
        timeout = -timeout;
    }
    struct buf rb;
    b_init(&rb);
    uint8_t b, prev = ' ';
    bool is_urc = false;

    while (modem_read(&b, timeout))
    {
        if (b == '+' && rb.size == 0)
        {
            is_urc = true;
        }
        b_add(&rb, b);
        if (char_cb)
        {
            char_cb(ctx, &rb, b, is_urc, isspace(b));
        }
        if (rb.size >= 4)
        {
            if (b_is(&rb, "OK\r\n", 4))
            {
                return AT_OK;
            }
            if (b_is(&rb, "ERROR\r\n", 7))
            {
                return AT_ERROR;
            }
        }
        if (prev == '\r' && b == '\n')
        {
            if (eol_cb)
            {
                eol_cb(ctx, &rb, is_urc);
            }
            b_reset(&rb);
            is_urc = false;
        }
        // Additional URCs to support:
        //  - CEREG
        //  - NPSMR
        //  - CSCON
        //  - UFOTAS

        prev = b;
    }
    return AT_TIMEOUT;
}

// Decode AT+NRB responses. It just waits for OK or ERROR with a slightly
// longer timeout than the default commands.
int atnrb_decode()
{
    return decode_input(CMD_REBOOT_TIMEOUT, NULL, NULL, NULL);
}

// AT responses - wait for OK (ERROR is quite rare here but it is handled)
#define at_decode() decode_input(CMD_TIMEOUT, NULL, NULL, NULL)

// Decode response for AT+NSOCL (close socket). There is no return from this
// command, just OK or ERROR.
int atnsocl_decode()
{
    return at_decode();
}

// Decode the CGPADDR response. The in_address flag says if we're in the address
// string (ie past the +CGPADDR: string) and copies the bytes into the address
// buffer. When the eol callback is set the address flag is reset

struct cgp_ctx
{
    bool in_address;
    uint8_t addrindex;
    char *address;
    char *buffer;
    size_t *len;
    uint8_t i;
};

void cgpaddr_eol(void *ctx, struct buf *rb, bool is_urc)
{
    struct cgp_ctx *c = (struct cgp_ctx *)ctx;
    if (c->in_address)
    {
        bool in_str = false;
        uint8_t n = 0;
        for (int i = 0; i < (c->i); i++)
        {
            if (in_str && c->buffer[i] == '\"')
            {
                in_str = false;
            }
            if (in_str)
            {
                c->address[n++] = c->buffer[i];
            }
            if (!in_str && (c->buffer[i] == '\"'))
            {
                in_str = true;
            }
        }
        c->address[n] = 0;
        *c->len = n;
    }
    c->in_address = false;
}

void cgpaddr_char(void *ctx, struct buf *rb, char b, bool is_urc, bool is_space)
{
    struct cgp_ctx *c = (struct cgp_ctx *)ctx;
    if (c->in_address && !is_space)
    {
        c->buffer[c->i++] = (char)b;
    }
    if (is_urc && rb->size == 9 && b_is(rb, "+CGPADDR:", 9))
    {
        c->in_address = true;
    }
}

int atcgpaddr_decode(char *address, size_t *len)
{
    char buffer[20];
    memset(buffer, 0, sizeof(buffer));
    struct cgp_ctx ctx = {
        .in_address = false,
        .address = address,
        .len = len,
        .buffer = buffer,
        .i = 0,
    };
    return decode_input(CMD_TIMEOUT, &ctx, cgpaddr_char, cgpaddr_eol);
}

// Decode NSCR responses. This is fairly straightforward since there's only
// a single digit that is returned. We'll pass the pointer to the return
// function as the context and assign it when the line ends.

void nsocr_eol(void *ctx, struct buf *rb, bool is_urc)
{
    if (!is_urc && rb->size > 2)
    {
        int *sockfd = (int *)ctx;
        *sockfd = atoi((const char *)rb->data);
    }
}

int atnsocr_decode(int *sockfd)
{
    *sockfd = -2;
    return decode_input(CMD_TIMEOUT, sockfd, NULL, nsocr_eol);
}

// Decode SOST responses. Also quite simple since everything fits into
// the entire 9-byte buffer so we just process the line at EOL.

struct nsost_ctx
{
    int *sockfd;
    size_t *len;
};

void nsost_eol(void *ctx, struct buf *rb, bool is_urc)
{
    struct nsost_ctx *c = (struct nsost_ctx *)ctx;
        // Socket descriptor is *always* a single digit, ie the comma would be
        // the 2nd char
    if (!is_urc && *c->len == 0 && rb->size > 2 && rb->data[1] == ',') {
        *c->sockfd = (int)(rb->data[0] - '0');
        if (*c->sockfd >= 7) {
            LOG_ERR("Socket fd should be <= 6 but is '%c'", rb->data[0]);
        }
        *c->len = atoi(rb->data + 2);
    }
}

int atnsost_decode(int *sock_fd, size_t *sent)
{
    struct nsost_ctx ctx = {
        .sockfd = sock_fd,
        .len = sent,
    };
    return decode_input(CMD_TIMEOUT, &ctx, NULL, nsost_eol);
}

// Decode NSORF responses. Each field is decoded separately and stored off in
// a temporary buffer (except the data field which might be large).
#define FROM_HEX(x) (x - '0' > 9 ? x - 'A' + 10 : x - '0')

#define MAX_FIELD_SIZE 16

struct nsorf_ctx
{
    int *sockfd;
    char *ip;
    int *port;
    uint8_t *data;
    size_t *remaining;
    size_t *received;
    int fieldno;
    int fieldindex;
    char field[MAX_FIELD_SIZE];
    int dataidx;
};

void nsorf_eol(void *ctx, struct buf *rb, bool is_urc)
{
    struct nsorf_ctx *c = (struct nsorf_ctx *)ctx;
    if (!is_urc && c->fieldindex > 0)
    {
        *c->remaining = atoi(c->field);
    }
}

void nsorf_char(void *ctx, struct buf *rb, char b, bool is_urc, bool is_space)
{
    if (is_urc || is_space)
    {
        return;
    }
    struct nsorf_ctx *c = (struct nsorf_ctx *)ctx;

    switch (b)
    {
    case ',':
        c->fieldindex = 0;
        switch (c->fieldno)
        {
        case 0:
            *c->sockfd = atoi(c->field);
            break;
        case 1:
            strcpy(c->ip, c->field);
            break;
        case 2:
            *c->port = atoi(c->field);
            break;
        case 3:
            // ignore
            break;
        case 4:
            // ignore
            break;
        default:
            // Should not encounter field #5 here
            LOG_ERR("Too many fields (%d) in response\n", c->fieldno);
            return;
        }
        memset(c->field, 0, MAX_FIELD_SIZE);
        c->fieldno++;
        break;
    case '\"':
        break;
    default:
        c->field[c->fieldindex++] = b;
        if (c->fieldno == 4 && c->fieldindex == 2)
        {
            c->data[c->dataidx++] = (FROM_HEX(c->field[0]) << 4 | FROM_HEX(c->field[1]));
            (*c->received)++;
            c->fieldindex = 0;
        }
        break;
    }
}

int atnsorf_decode(int *sockfd, char *ip, int *port, uint8_t *data, size_t *received, size_t *remaining)
{
    struct nsorf_ctx ctx = {
        .sockfd = sockfd,
        .ip = ip,
        .port = port,
        .data = data,
        .remaining = remaining,
        .fieldno = 0,
        .fieldindex = 0,
        .dataidx = 0,
        .received = received,
    };
    return decode_input(CMD_TIMEOUT, &ctx, nsorf_char, nsorf_eol);
}

// Decode AT+CPSMS responses. This just waits for ERROR or OK
int atcpsms_decode()
{
    return at_decode();
}


// Decode AT+CIMI responses.
struct cimi_ctx
{
    char *imsi;
    uint8_t index;
    bool done;
};

void cimi_char(void *ctx, struct buf *rb, char b, bool is_urc, bool is_space)
{
    struct cimi_ctx *c = (struct cimi_ctx *)ctx;
    if (!c->done && !is_urc && !is_space)
    {
        struct cimi_ctx *c = (struct cimi_ctx *)ctx;
        c->imsi[c->index++] = b;
    }
}

void cimi_eol(void *ctx, struct buf *rb, bool is_urc)
{
    struct cimi_ctx *c = (struct cimi_ctx *)ctx;
    if (!c->done && c->index > 0)
    {
        c->imsi[c->index] = 0;
        c->done = true;
    }
}

int atcimi_decode(char *imsi)
{
    struct cimi_ctx ctx = {
        .imsi = imsi,
        .index = 0,
        .done = false,
    };
    return decode_input(CMD_TIMEOUT, &ctx, cimi_char, cimi_eol);
}