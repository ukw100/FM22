/*-------------------------------------------------------------------------------------------------------------------------------------------
 * listener.c - listener routines
 *-------------------------------------------------------------------------------------------------------------------------------------------
 * Copyright (c) 2022 Frank Meyer - frank(at)uclock.de
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
#include <stdio.h>
#include <string.h>
#include "listener.h"
#include "listener-uart.h"

#include "booster.h"
#include "dcc.h"
#include "rc-detector.h"
#include "s88.h"
#include "rs485.h"

#define WEBIO_BUFLEN                80

#define MSG_FRAME_START             0xFF            // Start of Text
#define MSG_FRAME_END               0xFE            // End of Text
#define MSG_FRAME_ESCAPE            0xFD            // Data Link Escape
#define MSG_FRAME_STOP              0xFC            // Stop transmission - NOT USED
#define MSG_FRAME_CONTINUE          0xFB            // Continue transmission
#define MSG_FRAME_ESCAPE_OFFSET     0xF0            // Escape offset value

#define MSG_ADC                     0x03            // todo: renumber -> 0x01
#define MSG_RC1                     0x04
#define MSG_RC2                     0x05
#define MSG_POM_CV                  0x06
#define MSG_PGM_CV                  0x07
#define MSG_LOCO_RC2_RATE           0x08
#define MSG_S88                     0x09
#define MSG_RCL                     0x0A
#define MSG_XPOM_CV                 0x0B
#define MSG_DEBUG_MESSAGE           0x20

#define MAX_MSG_SIZE                128

static volatile uint_fast8_t        listener_do_send_continue;                          // set by ISR
static volatile uint_fast8_t        frame_stopped;                                      // checked by ISR
static uint8_t                      last_locations[RC_DETECTOR_MAX_LOCOS];

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * send_msg () - send message
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
send_msg (uint8_t * buf, uint_fast8_t len)
{
    uint8_t   ch;
    uint8_t * bufp = buf;

    listener_putc (MSG_FRAME_START);
    listener_putc (len);

    while (len--)
    {
        ch = *bufp;

        if (ch >= MSG_FRAME_CONTINUE)
        {
            listener_putc (MSG_FRAME_ESCAPE);
            ch -= MSG_FRAME_ESCAPE_OFFSET;
        }
        listener_putc (ch);
        bufp++;
    }

    listener_putc (MSG_FRAME_END);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * listener_set_stop () - set stop flag
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
listener_set_stop (void)
{
    if (! frame_stopped)
    {
        frame_stopped = 1;
    }
}


/*-------------------------------------------------------------------------------------------------------------------------------------------
 * listener_set_continue () - set continue flag
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
listener_set_continue (void)
{
    if (frame_stopped)
    {
        listener_do_send_continue = 1;
        frame_stopped = 0;
    }
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * listener_send_continue () - send continue flag
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
listener_send_continue (void)
{
    if (listener_do_send_continue)
    {
        listener_putc (MSG_FRAME_CONTINUE);
        listener_do_send_continue = 0;
    }
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * listener_send_msg_adc () - send booster and ADC status message
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
listener_send_msg_adc (uint_fast16_t adc)
{
    uint8_t     buf[4];

    buf[0] = MSG_ADC;
    buf[1] = booster_is_on;
    buf[2] = adc >> 8;
    buf[3] = adc & 0xFF;
    send_msg (buf, 4);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * listener_send_msg_rc1 () - send RC1 status message
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
listener_send_msg_rc1 (void)
{
    uint8_t     buf[3];

    uint16_t    rc1 = rc_detector_rc1_addr ();

    buf[0] = MSG_RC1;
    buf[1] = rc1 >> 8;
    buf[2] = rc1 & 0xFF;
    send_msg (buf, 3);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * listener_send_msg_rc2 () - send RC2 status message
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
listener_send_msg_rc2 (void)
{
    uint8_t         buf[64];
    uint_fast16_t   n_rc2infos          = rc_detector_n_rc2infos;
    uint_fast8_t    byte_offset         = 0;
    uint_fast8_t    bit_offset;
    uint_fast16_t   idx;

    buf[0] = MSG_RC2;

    for (idx = 0; idx < n_rc2infos; idx++)
    {
        byte_offset = idx / 8 + 1;
        bit_offset  = idx % 8;

        if (bit_offset == 0)
        {
            buf[byte_offset] = 0;
        }

        if (rc_detector_get_rc2_millis_diff (idx) < 100)
        {
            buf[byte_offset] |= (1 << bit_offset);
        }
    }

    send_msg (buf, byte_offset + 1);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * listener_send_msg_rcl () - send RC-Local status message
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
listener_send_msg_rcl (void)
{
    uint8_t         buf[32];
    uint_fast16_t   n_locations     = rc_detector_n_locations;
    uint_fast8_t    location;
    uint_fast8_t    len             = 0;
    uint_fast16_t   loco_idx;

    buf[0] = MSG_RCL;

    for (loco_idx = 0; loco_idx < n_locations; loco_idx++)
    {
        location = rc_detector_get_loco_location (loco_idx);

        if (location != last_locations[loco_idx])
        {
            if (len < 32 - 4)       // if buffer is big enough, send it now, otherwise send it next time
            {
                len++;
                buf[len] = loco_idx >> 8;
                len++;
                buf[len] = loco_idx & 0xFF;
                len++;
                buf[len] = location;
                last_locations[loco_idx] = location;
            }
            else
            {
                break;
            }
        }
    }

    if (len > 0)
    {
        send_msg (buf, len + 1);
    }
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * listener_send_msg_s88 () - send S88 status message
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
listener_send_msg_s88 (void)
{
    uint_fast8_t    n_bytes     = s88_get_n_bytes ();
    uint8_t         buf[n_bytes + 2];
    uint_fast8_t    idx;

    buf[0] = MSG_S88;
    buf[1] = n_bytes;

    for (idx = 0; idx < n_bytes; idx++)
    {
        uint_fast8_t state = s88_get_status_byte (idx);

        buf[idx + 2] = state;
    }

    send_msg (buf, idx + 2);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * listener_send_msg_pom_cv () - send POM CV value
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
listener_send_msg_pom_cv (uint_fast16_t addr, uint_fast16_t cv, uint_fast8_t cv_value)
{
    uint8_t         buf[6];

    buf[0] = MSG_POM_CV;

    buf[1] = addr >> 8;
    buf[2] = addr & 0xFF;
    buf[3] = cv >> 8;
    buf[4] = cv & 0xFF;
    buf[5] = cv_value;

    send_msg (buf, 6);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * listener_send_msg_pom_cv () - send POM CV value
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
listener_send_msg_xpom_cv (uint_fast16_t addr, uint_fast8_t cv31, uint_fast8_t cv32, uint_fast8_t cv_range, uint8_t * cv_values, uint_fast8_t n)
{
    uint8_t         buf[32];
    uint_fast8_t    i;

    buf[0] = MSG_XPOM_CV;

    buf[1] = addr >> 8;
    buf[2] = addr & 0xFF;
    buf[3] = cv31;
    buf[4] = cv32;
    buf[5] = cv_range;

    n *= 4;

    for (i = 6; i < n + 6; i++)
    {
        buf[i] = *cv_values++;
    }

    send_msg (buf, i);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * listener_send_msg_pgm_cv () - send PGM CV value
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
listener_send_msg_pgm_cv (uint_fast16_t cv, uint_fast8_t cv_value)
{
    uint8_t         buf[4];

    buf[0] = MSG_PGM_CV;

    buf[1] = cv >> 8;
    buf[2] = cv & 0xFF;
    buf[3] = cv_value;

    send_msg (buf, 4);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * send_msg_rc2 () - send ADC message
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
listener_send_msg_rc2_rate (uint_fast16_t loco_idx, uint_fast8_t rc2_rate)
{
    uint8_t         buf[4];

    buf[0] = MSG_LOCO_RC2_RATE;
    buf[1] = loco_idx >> 8;
    buf[2] = loco_idx & 0xFF;
    buf[3] = rc2_rate;

    send_msg (buf, 4);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * listener_send_debug_msg () - send debug message
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
listener_send_debug_msg (char * msg)
{
    uint8_t     buf[MAX_MSG_SIZE];
    uint32_t    len;

    len = strlen (msg);

    if (len >= MAX_MSG_SIZE - 1)
    {
        len = MAX_MSG_SIZE - 1;
    }

    buf[0] = MSG_DEBUG_MESSAGE;

    memcpy (buf + 1, msg, len);
    send_msg (buf, len + 1);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * listener_send_debug_msgf () - send debug message
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
listener_send_debug_msgv (const char * fmt, va_list ap)
{
    static char msg_buf[MAX_MSG_SIZE];

    (void) vsnprintf ((char *) msg_buf, MAX_MSG_SIZE, fmt, ap);
    listener_send_debug_msg (msg_buf);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * listener_send_debug_msgf () - send a formatted message (varargs)
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
listener_send_debug_msgf (const char * fmt, ...)
{
    va_list ap;

    va_start (ap, fmt);
    listener_send_debug_msgv (fmt, ap);
    va_end (ap);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * listener_queue_debug_msg () - put a msg into msg queue - usable for messages sent from ISR
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
#define MSG_QUEUE_SIZE      4
volatile char               msg_queue[MSG_QUEUE_SIZE][MAX_MSG_SIZE];
volatile uint_fast8_t       msg_queue_len[MSG_QUEUE_SIZE];
volatile uint_fast8_t       msg_queue_start;
volatile uint_fast8_t       msg_queue_end;

void
listener_queue_debug_msg (char * msg)
{
    uint_fast8_t    len;
    char *          t;
    char *          s;
    uint_fast8_t    l;

    len = strlen (msg);

    if (len >= MAX_MSG_SIZE - 1)
    {
        len = MAX_MSG_SIZE - 1;
    }

    msg_queue[msg_queue_end][0] = MSG_DEBUG_MESSAGE;

    t = (char *) msg_queue[msg_queue_end] + 1;
    s = msg;
    l = len;

    while (l)       // memcpy() does not reliable work with volatile values
    {
        *t++ = *s++;
        l--;
    }

    msg_queue_len[msg_queue_end] = len;

    msg_queue_end++;

    if (msg_queue_end == MSG_QUEUE_SIZE)
    {
        msg_queue_end = 0;
    }
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * listener_queue_debug_msgv () - queue debug message
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
listener_queue_debug_msgv (const char * fmt, va_list ap)
{
    static char msg_buf[MAX_MSG_SIZE];

    (void) vsnprintf ((char *) msg_buf, MAX_MSG_SIZE, fmt, ap);
    listener_queue_debug_msg (msg_buf);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * listener_queue_debug_msgf () - queue formatted message (varargs)
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
listener_queue_debug_msgf (const char * fmt, ...)
{
    va_list ap;

    va_start (ap, fmt);
    listener_queue_debug_msgv (fmt, ap);
    va_end (ap);
}


/*-------------------------------------------------------------------------------------------------------------------------------------------
 * listener_flush_debug_msg () - flush message queue
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
listener_flush_debug_msg (void)
{
    uint_fast8_t    len;
    uint8_t *          buf;

    while (msg_queue_start != msg_queue_end)
    {
        len = msg_queue_len[msg_queue_start];
        buf = (uint8_t *) msg_queue[msg_queue_start];

        send_msg (buf, len + 1);

        msg_queue_start++;

        if (msg_queue_start == MSG_QUEUE_SIZE)
        {
            msg_queue_start = 0;
        }
    }
}

#define CMD_BOOSTER_ON                  0x01
#define CMD_BOOSTER_OFF                 0x02
#define CMD_SET_MODE                    0x03

#define CMD_PGM_READ_CV                 0x11
#define CMD_PGM_WRITE_CV                0x12
#define CMD_PGM_WRITE_CV_BIT            0x13
#define CMD_PGM_WRITE_ADDRESS           0x14

#define CMD_POM_READ_CV                 0x21
#define CMD_XPOM_READ_CV                0x22
#define CMD_POM_WRITE_CV                0x23
#define CMD_POM_WRITE_CV_BIT            0x24
#define CMD_POM_WRITE_ADDRESS           0x25

#define CMD_LOCO_28                     0x31
#define CMD_LOCO_FUNCTION_F00_F04       0x32
#define CMD_LOCO_FUNCTION_F05_F08       0x33
#define CMD_LOCO_FUNCTION_F09_F12       0x34
#define CMD_LOCO_FUNCTION_F13_F20       0x35
#define CMD_LOCO_FUNCTION_F21_F28       0x36
#define CMD_LOCO                        0x37
#define CMD_LOCO_RC2_RATE               0x38

#define CMD_RESET                       0x41
#define CMD_STOP                        0x42
#define CMD_ESTOP                       0x43
#define CMD_RESET_DECODER               0x44
#define CMD_HARD_RESET_DECODER          0x45

#define CMD_GET_ACK                     0x51

#define CMD_BASE_SWITCH_SET             0x61
#define CMD_BASE_SWITCH_RESET           0x62

#define CMD_S88_SET_N_CONTACTS          0x71

/* 0xF0 - 0xFF reserved for escape characters */

#define GET8(p,o)     (*((p) + o))
#define GET16(p,o)    ((*((p) + o) << 8) + (*((p) + 1 + o)))

#if 0
/*-------------------------------------------------------------------------------------------------------------------------------------------
 * cmd_version () - command: VERSION
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
cmd_version (uint8_t * bufp, uint_fast8_t len)
{
    uint8_t buf[4];
    buf[0] = *bufp;

    if (len == 1)
    {
        buf[1] = 1;
        buf[2] = 0;
        buf[3] = 0;
        cmd_answer_ack (buf, 4);
    }
    else
    {
        cmd_answer_nack (buf);
    }
}
#endif

static void
cmd_booster_on (uint8_t * bufp, uint_fast8_t len)
{
    (void) bufp;

    if (len == 1)
    {
        dcc_booster_on ();
    }
}

static void
cmd_booster_off (uint8_t * bufp, uint_fast8_t len)
{
    (void) bufp;

    if (len == 1)
    {
        dcc_booster_off ();
    }
}

static void
cmd_set_mode (uint8_t * bufp, uint_fast8_t len)
{
    if (len == 2)
    {
        uint_fast8_t    mode = GET8(bufp, 1);
        dcc_set_mode (mode);
    }
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * cmd_pgm_read_cv () - command: PGM_READ_CV
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
cmd_pgm_read_cv (uint8_t * bufp, uint_fast8_t len)
{
    if (len == 3)
    {
        uint_fast16_t   cv = GET16(bufp, 1);
        uint_fast8_t    cv_value;

        if (dcc_pgm_read_cv (&cv_value, cv))
        {
            listener_send_msg_pgm_cv (cv, cv_value);
        }
    }
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * cmd_pgm_write_cv () - command: PGM_WRITE_CV
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
cmd_pgm_write_cv (uint8_t * bufp, uint_fast8_t len)
{
    if (len == 4)
    {
        uint_fast16_t   cv = GET16(bufp, 1);
        uint_fast8_t    cv_value = GET8(bufp, 3);

        if (dcc_pgm_write_cv (cv, cv_value))
        {
            // TODO
        }
    }
}

static void
cmd_pgm_write_cv_bit (uint8_t * bufp, uint_fast8_t len)
{
    if (len == 5)
    {
        uint_fast16_t   cv = GET16(bufp, 1);
        uint_fast8_t    bitpos = GET8(bufp, 3);
        uint_fast8_t    bitvalue = GET8(bufp, 4);

        if (dcc_pgm_write_cv_bit  (cv, bitpos, bitvalue))
        {
            // TODO
        }
    }
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * cmd_pgm_write_address () - command: PGM_WRITE_ADDRESS
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
cmd_pgm_write_address (uint8_t * bufp, uint_fast8_t len)
{
    if (len == 3)
    {
        uint_fast16_t   addr = GET16(bufp, 1);

        if (dcc_pgm_write_address (addr))
        {
            // TODO
        }
    }
}

static void
cmd_pom_read_cv (uint8_t * bufp, uint_fast8_t len)
{
    if (len == 5)
    {
        uint_fast16_t   addr = GET16(bufp, 1);
        uint_fast16_t   cv = GET16(bufp, 3);
        uint_fast8_t    cv_value;

        if (dcc_pom_read_cv (&cv_value, addr, cv))
        {
            listener_send_msg_pom_cv (addr, cv, cv_value);
        }
    }
}

static void
cmd_xpom_read_cv (uint8_t * bufp, uint_fast8_t len)
{
    if (len == 7)
    {
        uint_fast8_t    n           = GET8(bufp, 1);
        uint_fast16_t   addr        = GET16(bufp, 2);
        uint_fast8_t    cv31        = GET8(bufp, 4);
        uint_fast8_t    cv32        = GET8(bufp, 5);
        uint_fast8_t    cv_range    = GET8(bufp, 6);
        uint8_t         cv_values[16];

        if (dcc_xpom_read_cv (cv_values, n, addr, cv31, cv32, cv_range))
        {
            listener_send_msg_xpom_cv (addr, cv31, cv32, cv_range, cv_values, n);
        }
    }
}

static void
cmd_pom_write_cv (uint8_t * bufp, uint_fast8_t len)
{
    if (len == 6)
    {
        uint_fast16_t   addr = GET16(bufp, 1);
        uint_fast16_t   cv = GET16(bufp, 3);
        uint_fast8_t    cv_value = GET8(bufp, 5);

        dcc_pom_write_cv (addr, cv, cv_value);
    }
}

static void
cmd_pom_write_cv_bit (uint8_t * bufp, uint_fast8_t len)
{
    if (len == 7)
    {
        uint_fast16_t   addr = GET16(bufp, 1);
        uint_fast16_t   cv = GET16(bufp, 3);
        uint_fast8_t    bitpos = GET8(bufp, 5);
        uint_fast8_t    bitvalue = GET8(bufp, 6);

        dcc_pom_write_cv_bit  (addr, cv, bitpos, bitvalue);
    }
}

static void
cmd_pom_write_cv_address (uint8_t * bufp, uint_fast8_t len)
{
    if (len == 5)
    {
        uint_fast16_t   oldaddr = GET16(bufp, 1);
        uint_fast16_t   newaddr = GET16(bufp, 3);

        dcc_pom_write_address (oldaddr, newaddr);
    }
}

static void
cmd_loco_28 (uint8_t * bufp, uint_fast8_t len)
{
    if (len == 7)
    {
        uint_fast16_t   loco_idx    = GET16(bufp, 1);
        uint_fast16_t   addr        = GET16(bufp, 3);
        uint_fast8_t    direction   = GET8(bufp, 5);
        uint_fast8_t    speed       = GET8(bufp, 6);

        dcc_loco_28 (loco_idx, addr, direction, speed);
    }
}

static void
cmd_loco_function_f00_f04 (uint8_t * bufp, uint_fast8_t len)
{
    if (len == 6)
    {
        uint_fast16_t   loco_idx    = GET16(bufp, 1);
        uint_fast16_t   addr        = GET16(bufp, 3);
        uint32_t        fmask       = GET8(bufp, 5);

        dcc_loco_function (loco_idx, addr, fmask, DCC_F00_F04_RANGE);
    }
}

static void
cmd_loco_function_f05_f08 (uint8_t * bufp, uint_fast8_t len)
{
    if (len == 6)
    {
        uint_fast16_t   loco_idx    = GET16(bufp, 1);
        uint_fast16_t   addr        = GET16(bufp, 3);
        uint32_t        fmask       = GET8(bufp, 5);

        fmask <<= 5;

        dcc_loco_function (loco_idx, addr, fmask, DCC_F05_F08_RANGE);
    }
}

static void
cmd_loco_function_f09_f12 (uint8_t * bufp, uint_fast8_t len)
{
    if (len == 6)
    {
        uint_fast16_t   loco_idx    = GET16(bufp, 1);
        uint_fast16_t   addr        = GET16(bufp, 3);
        uint32_t        fmask       = GET8(bufp, 5);

        fmask <<= 9;

        dcc_loco_function (loco_idx, addr, fmask, DCC_F09_F12_RANGE);
    }
}

static void
cmd_loco_function_f13_f20 (uint8_t * bufp, uint_fast8_t len)
{
    if (len == 6)
    {
        uint_fast16_t   loco_idx    = GET16(bufp, 1);
        uint_fast16_t   addr        = GET16(bufp, 3);
        uint32_t        fmask       = GET8(bufp, 5);

        fmask <<= 13;

        dcc_loco_function (loco_idx, addr, fmask, DCC_F13_F20_RANGE);
    }
}

static void
cmd_loco_function_f21_f28 (uint8_t * bufp, uint_fast8_t len)
{
    if (len == 6)
    {
        uint_fast16_t   loco_idx    = GET16(bufp, 1);
        uint_fast16_t   addr        = GET16(bufp, 3);
        uint32_t        fmask       = GET8(bufp, 5);

        fmask <<= 21;

        dcc_loco_function (loco_idx, addr, fmask, DCC_F21_F28_RANGE);
    }
}

static void
cmd_loco (uint8_t * bufp, uint_fast8_t len)
{
    if (len >= 7 && len <= 11)
    {
        uint_fast16_t   loco_idx    = GET16(bufp, 1);
        uint_fast16_t   addr        = GET16(bufp, 3);
        uint_fast8_t    direction   = GET8(bufp, 5);
        uint_fast8_t    speed       = GET8(bufp, 6);
        uint32_t        fmask       = 0;
        uint_fast8_t    n           = len - 7;

        if (n >= 1)
        {
            fmask |= GET8(bufp, 7);

            if (n >= 2)
            {
                fmask |= (GET8(bufp, 8) << 8);

                if (n >= 3)
                {
                    fmask |= (GET8(bufp, 9) << 16);

                    if (n >= 4)
                    {
                        fmask |= (GET8(bufp, 10) << 24);
                    }
                }
            }
        }

        dcc_loco (loco_idx, addr, direction, speed, fmask, n);
    }
}

static void
cmd_loco_rc2_rate (uint8_t * bufp, uint_fast8_t len)
{
    if (len == 3)
    {
        uint_fast16_t   loco_idx = GET16(bufp, 1);
        uint_fast8_t    rc2_rate = rc_detector_get_rc2_rate (loco_idx);
        listener_send_msg_rc2_rate (loco_idx, rc2_rate);
    }
}

static void
cmd_reset (uint8_t * bufp, uint_fast8_t len)
{
    (void) bufp;

    if (len == 1)
    {
        dcc_reset ();
    }
}

static void
cmd_stop (uint8_t * bufp, uint_fast8_t len)
{
    (void) bufp;

    if (len == 1)
    {
        dcc_stop ();
    }
}

static void
cmd_estop (uint8_t * bufp, uint_fast8_t len)
{
    (void) bufp;

    if (len == 1)
    {
        dcc_estop ();
    }
}

static void
cmd_reset_decoder (uint8_t * bufp, uint_fast8_t len)
{
    if (len == 3)
    {
        uint_fast16_t   addr    = GET16(bufp, 1);
        dcc_reset_decoder (addr);
    }
}

static void
cmd_hard_reset_decoder (uint8_t * bufp, uint_fast8_t len)
{
    if (len == 3)
    {
        uint_fast16_t   addr    = GET16(bufp, 1);
        dcc_hard_reset_decoder (addr);
    }
}

static void
cmd_get_ack (uint8_t * bufp, uint_fast8_t len)
{
    if (len == 3)
    {
        uint_fast16_t   addr    = GET16(bufp, 1);
        dcc_get_ack (addr);
    }
}

static void
cmd_base_switch_set (uint8_t * bufp, uint_fast8_t len)
{
    if (len == 4)
    {
        uint_fast16_t   addr    = GET16(bufp, 1);
        uint_fast8_t    nswitch = GET8(bufp, 3);
        dcc_base_switch_set (addr, nswitch);
    }
}

static void
cmd_base_switch_reset (uint8_t * bufp, uint_fast8_t len)
{
    if (len == 4)
    {
        uint_fast16_t   addr    = GET16(bufp, 1);
        uint_fast8_t    nswitch = GET8(bufp, 3);
        dcc_base_switch_reset (addr, nswitch);
    }
}

static void
cmd_s88_set_n_contacts (uint8_t * bufp, uint_fast8_t len)
{
    if (len == 3)
    {
        uint_fast16_t     ncontacts = GET16(bufp, 1);
        s88_set_n_contacts (ncontacts);
    }
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * cmd_pgm_statistics () - command: PGM_STATISTICS
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
#if 0 // TODO
static void
cmd_pgm_statistics (uint8_t * bufp, uint_fast8_t len)
{
    uint8_t     buf[15];

    buf[0] = *bufp;

    if (len == 1)
    {
        buf[1]  = dcc_pgm_limit >> 8;
        buf[2]  = dcc_pgm_limit & 0xFF;
        buf[3]  = dcc_pgm_min_lower_value >> 8;
        buf[4]  = dcc_pgm_min_lower_value & 0xFF;
        buf[5]  = dcc_pgm_max_lower_value >> 8;
        buf[6]  = dcc_pgm_max_lower_value & 0xFF;
        buf[7]  = dcc_pgm_min_upper_value >> 8;
        buf[8]  = dcc_pgm_min_upper_value & 0xFF;
        buf[9]  = dcc_pgm_max_upper_value >> 8;
        buf[10] = dcc_pgm_max_upper_value & 0xFF;
        buf[11] = dcc_pgm_min_cnt >> 8;
        buf[12] = dcc_pgm_min_cnt & 0xFF;
        buf[13] = dcc_pgm_max_cnt >> 8;
        buf[14] = dcc_pgm_max_cnt & 0xFF;

        cmd_answer_ack (buf, 15);
    }
}
#endif

#define DEFAULT_TIMEOUT     1000
#define PGM_TIMEOUT         3000

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * cmd () - handle commands
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static uint32_t
cmd (uint8_t * buf, uint_fast8_t len)
{
    uint32_t    timeout = DEFAULT_TIMEOUT;             // timeout for a commant
    // console_printf ("%02X %d\r\n", buf[0], len);

    switch (buf[0])
    {
        case CMD_BOOSTER_ON:                cmd_booster_on (buf, len);                                  break;
        case CMD_BOOSTER_OFF:               cmd_booster_off (buf, len);                                 break;
        case CMD_SET_MODE:                  cmd_set_mode (buf, len);                                    break;

        case CMD_PGM_READ_CV:               cmd_pgm_read_cv (buf, len);         timeout = PGM_TIMEOUT;  break;
        case CMD_PGM_WRITE_CV:              cmd_pgm_write_cv (buf, len);        timeout = PGM_TIMEOUT;  break;
        case CMD_PGM_WRITE_CV_BIT:          cmd_pgm_write_cv_bit (buf, len);    timeout = PGM_TIMEOUT;  break;
        case CMD_PGM_WRITE_ADDRESS:         cmd_pgm_write_address (buf, len);   timeout = PGM_TIMEOUT;  break;

        case CMD_POM_READ_CV:               cmd_pom_read_cv (buf, len);                                 break;
        case CMD_XPOM_READ_CV:              cmd_xpom_read_cv (buf, len);                                break;
        case CMD_POM_WRITE_CV:              cmd_pom_write_cv (buf, len);                                break;
        case CMD_POM_WRITE_CV_BIT:          cmd_pom_write_cv_bit (buf, len);                            break;
        case CMD_POM_WRITE_ADDRESS:         cmd_pom_write_cv_address (buf, len);                        break;

        case CMD_LOCO_28:                   cmd_loco_28 (buf, len);                                     break;
        case CMD_LOCO_FUNCTION_F00_F04:     cmd_loco_function_f00_f04 (buf, len);                       break;
        case CMD_LOCO_FUNCTION_F05_F08:     cmd_loco_function_f05_f08 (buf, len);                       break;
        case CMD_LOCO_FUNCTION_F09_F12:     cmd_loco_function_f09_f12 (buf, len);                       break;
        case CMD_LOCO_FUNCTION_F13_F20:     cmd_loco_function_f13_f20 (buf, len);                       break;
        case CMD_LOCO_FUNCTION_F21_F28:     cmd_loco_function_f21_f28 (buf, len);                       break;
        case CMD_LOCO:                      cmd_loco (buf, len);                                        break;
        case CMD_LOCO_RC2_RATE:             cmd_loco_rc2_rate (buf, len);                               break;

        case CMD_RESET:                     cmd_reset (buf, len);                                       break;
        case CMD_STOP:                      cmd_stop (buf, len);                                        break;
        case CMD_ESTOP:                     cmd_estop (buf, len);                                       break;
        case CMD_RESET_DECODER:             cmd_reset_decoder (buf, len);                               break;
        case CMD_HARD_RESET_DECODER:        cmd_hard_reset_decoder (buf, len);                          break;

        case CMD_GET_ACK:                   cmd_get_ack (buf, len);                                     break;

        case CMD_BASE_SWITCH_SET:           cmd_base_switch_set (buf, len);                             break;
        case CMD_BASE_SWITCH_RESET:         cmd_base_switch_reset (buf, len);                           break;

        case CMD_S88_SET_N_CONTACTS:        cmd_s88_set_n_contacts (buf, len);                          break;
    }
    return timeout;
}

#define CMD_FRAME_START             0xFF            // Start of Text
#define CMD_FRAME_END               0xFE            // End of Text
#define CMD_FRAME_ESCAPE            0xFD            // Data Link Escape
#define CMD_FRAME_ESCAPE_OFFSET     0xF0            // Escape offset value

#define CMD_STATE_WAIT_FOR_FRAME_START      0
#define CMD_STATE_WAIT_FOR_LEN              1
#define CMD_STATE_WAIT_FOR_FRAME_END        2

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * read_cmd () - read a command
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
listener_read_cmd (void)
{
    static uint8_t          buf[WEBIO_BUFLEN + 1];
    static uint_fast8_t     bufidx = 0;
    static uint_fast8_t     length = 0;
    static uint_fast8_t     cmd_state = CMD_STATE_WAIT_FOR_FRAME_START;
    static uint_fast8_t     escape = 0;
    uint_fast8_t            ch;
    uint_fast8_t            timeout = 0;

    while (listener_poll (&ch))
    {
        // console_printf ("ch=%02X\r\n", ch);

        if (cmd_state == CMD_STATE_WAIT_FOR_FRAME_START)
        {
            if (ch == CMD_FRAME_START)
            {
                cmd_state   = CMD_STATE_WAIT_FOR_LEN;
                bufidx      = 0;
                escape      = 0;
                length      = 0;
            }
            else
            {
                // console_printf ("listener_read_cmd: invalid ch: 0x%02X\r\n", ch);
            }
        }
        else if (cmd_state == CMD_STATE_WAIT_FOR_LEN)
        {
            cmd_state = CMD_STATE_WAIT_FOR_FRAME_END;
            length = ch;
        }
        else if (cmd_state == CMD_STATE_WAIT_FOR_FRAME_END)
        {
            if (ch == CMD_FRAME_ESCAPE)
            {
                escape = 1;
            }
            else
            {
                if (escape)
                {
                    ch += CMD_FRAME_ESCAPE_OFFSET;
                    escape = 0;
                }

                if (length)
                {
                    buf[bufidx++] = ch;
                    length--;
                }
                else
                {
                    if (ch == CMD_FRAME_END)
                    {
                        timeout = cmd (buf, bufidx);
                    }

                    cmd_state = CMD_STATE_WAIT_FOR_FRAME_START;
                }
            }
        }
    }

    return (timeout);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * listener_read_rcl () - read from local RC-Detector
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
listener_read_rcl (void)
{
    static uint8_t          buf[8];
    static uint_fast8_t     bufidx = 0;
    static uint_fast8_t     length = 0;
    static uint_fast8_t     cmd_state = CMD_STATE_WAIT_FOR_FRAME_START;
    static uint_fast8_t     escape = 0;
    uint_fast8_t            ch;
    uint_fast8_t            rtc = 0;

    while (rs485_poll (&ch))
    {
        // console_printf ("ch=%02X\r\n", ch);

        if (cmd_state == CMD_STATE_WAIT_FOR_FRAME_START)
        {
            if (ch == CMD_FRAME_START)
            {
                cmd_state   = CMD_STATE_WAIT_FOR_FRAME_END;
                bufidx      = 0;
                escape      = 0;
                length      = 3;
            }
            else
            {
                // console_printf ("listener_read_cmd: invalid ch: 0x%02X\r\n", ch);
            }
        }
        else if (cmd_state == CMD_STATE_WAIT_FOR_FRAME_END)
        {
            if (ch == CMD_FRAME_ESCAPE)
            {
                escape = 1;
            }
            else
            {
                if (escape)
                {
                    ch += CMD_FRAME_ESCAPE_OFFSET;
                    escape = 0;
                }

                if (length)
                {
                    buf[bufidx++] = ch;
                    length--;
                }
                else
                {
                    if (ch == CMD_FRAME_END)
                    {
                        uint_fast8_t    channel_idx = buf[0];
                        uint_fast16_t   addr        = (buf[1] << 8) | buf[2];
                        rc_detector_set_loco_location (addr, channel_idx);
                        rtc = 1;
                    }

                    cmd_state = CMD_STATE_WAIT_FOR_FRAME_START;
                }
            }
        }
    }

    return (rtc);
}
/*-------------------------------------------------------------------------------------------------------------------------------------------
 * listener_msg_init () - initialize listener messages
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
listener_msg_init (void)
{
    uint_fast16_t   loco_idx;
    uint_fast8_t    ch;

    for (loco_idx = 0; loco_idx < RC_DETECTOR_MAX_LOCOS; loco_idx++)
    {
        last_locations[loco_idx] = 0xFF;
    }

    while (rs485_poll (&ch))
    {
    }
}
