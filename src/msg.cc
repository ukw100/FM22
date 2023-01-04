/*------------------------------------------------------------------------------------------------------------------------
 * msg.cc - message functions
 *------------------------------------------------------------------------------------------------------------------------
 * Copyright (c) 2022-2023 Frank Meyer - frank(at)uclock.de
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
 *------------------------------------------------------------------------------------------------------------------------
 */
#include <stdio.h>
#include <stdint.h>
#include <string>
#include "userio.h"
#include "loco.h"
#include "s88.h"
#include "dcc.h"
#include "serial.h"
#include "msg.h"
#include "debug.h"

#define MSG_ADC                             0x03    // todo: renumber
#define MSG_RC1                             0x04
#define MSG_RC2                             0x05
#define MSG_POM_CV                          0x06
#define MSG_PGM_CV                          0x07
#define MSG_LOCO_RC2_RATE                   0x08
#define MSG_S88                             0x09
#define MSG_RCL                             0x0A

#define MSG_DEBUG_MESSAGE                   0x20

#define GET8(p,o)                           (*((p) + o))
#define GET16(p,o)                          ((*((p) + o) << 8) + (*((p) + 1 + o)))

#define MAX_MSG_SIZE                        128

#define MSG_FRAME_START                     0xFF            // Start of Text
#define MSG_FRAME_END                       0xFE            // End of Text
#define MSG_FRAME_ESCAPE                    0xFD            // Data Link Escape
#define MSG_FRAME_ESCAPE_OFFSET             0xF0            // Escape offset value
#define MSG_FRAME_STOP                      0xFC            // Stop transmission
#define MSG_FRAME_CONTINUE                  0xFB            // Continue transmission

#define MSG_STATE_WAIT_FOR_FRAME_START      0
#define MSG_STATE_WAIT_FOR_LEN              1
#define MSG_STATE_WAIT_FOR_FRAME_END        2

void
MSG::adc (uint8_t * bufp, uint_fast8_t len)
{
    if (len == 4)
    {
        bool    b_on = GET8(bufp, 1);
        DCC::adc_value = GET16(bufp, 2) / 2;

        if (b_on)
        {
            if (! DCC::booster_is_on)
            {
                Debug::printf (DEBUG_LEVEL_VERBOSE, "msg_adc (): booster is already on\n");
                UserIO::booster_on (false);
                DCC::booster_is_on = 1;
            }
        }
        else
        {
            if (DCC::booster_is_on)
            {
                Debug::printf (DEBUG_LEVEL_VERBOSE, "msg_adc (): perhaps shortcut or STM32 reset: adc=%u\n", DCC::adc_value);
                UserIO::booster_off (false);
                DCC::booster_is_on = 0;
            }
        }
    }
}

void
MSG::rc1 (uint8_t * bufp, uint_fast8_t len)
{
    if (len == 3)
    {
        DCC::rc1_value = GET16(bufp, 1);
    }
}

void
MSG::rc2 (uint8_t * bufp, uint_fast8_t len)
{
    if (len >= 1)
    {
        uint_fast8_t    nbytes = len - 1;
        uint_fast8_t    bitpos;
        uint_fast8_t    idx;
        uint_fast16_t   n_locos = Loco::get_n_locos ();

        for (idx = 0; idx < nbytes; idx++)
        {
            for (bitpos = 0; bitpos < 8; bitpos++)
            {
                if (bufp[1 + idx] & (1 << bitpos))
                {
                    Loco::set_online (8 * idx + bitpos, 1);
                }
                else
                {
                    Loco::set_online (8 * idx + bitpos, 0);
                }
            }
        }

        while (idx < n_locos)
        {
            for (bitpos = 0; bitpos < 8; bitpos++)
            {
                Loco::set_online (8 * idx + bitpos, 0);
            }

            idx++;
        }
    }
}

void
MSG::rcl (uint8_t * bufp, uint_fast8_t len)
{
    if (len >= 4)
    {
        uint_fast8_t    nbytes          = (len - 1) / 3;
        uint_fast8_t    idx             = 0;
        uint_fast16_t   loco_idx;
        uint_fast8_t    location;

        bufp++;

        while (idx < nbytes)
        {
            loco_idx = *bufp++ << 8;
            loco_idx |= *bufp++;
            location = *bufp++;

            Loco::set_rcllocation (loco_idx, location);

            Debug::printf (DEBUG_LEVEL_VERBOSE, "MSG::rcl: loco=%d location=%d\n", loco_idx, location);

            idx += 3;
        }
    }
}

void
MSG::pom_cv (uint8_t * bufp, uint_fast8_t len)
{
    if (len == 6)
    {
        DCC::pom_cv.addr        = GET16 (bufp, 1);
        DCC::pom_cv.cv          = GET16 (bufp, 3);
        DCC::pom_cv.cv_value    = GET8(bufp, 5);
        DCC::pom_cv.valid       = 1;
    }
}

void
MSG::pgm_cv (uint8_t * bufp, uint_fast8_t len)
{
    if (len == 4)
    {
        DCC::pgm_cv.cv          = GET16 (bufp, 1);
        DCC::pgm_cv.cv_value    = GET8(bufp, 3);
        DCC::pgm_cv.valid       = 1;
    }
}

void
MSG::loco_rc2_rate (uint8_t * bufp, uint_fast8_t len)
{
    if (len == 4)
    {
        uint_fast16_t   loco_idx = GET16(bufp, 1);
        uint_fast8_t    rc2_rate = GET8(bufp, 3);

        Loco::set_rc2_rate (loco_idx, rc2_rate);
    }
}

void
MSG::s88 (uint8_t * bufp, uint_fast8_t len)
{
    if (len >= 2)
    {
        uint_fast8_t    idx;
        uint_fast8_t    n_bytes = GET8(bufp, 1);

        for (idx = 0; idx < n_bytes && idx < n_bytes; idx++)
        {
            uint_fast8_t   nstatus = GET8(bufp, idx + 2);
            Debug::printf (DEBUG_LEVEL_VERBOSE, "MSG::s88: idx=%u nstatus=%u\n", idx, nstatus);
            S88::set_newstate_byte (idx, nstatus);
        }
    }
}

void
MSG::debug_message (uint8_t * bufp, uint_fast8_t len)
{
    if (len >= 1)
    {
        time_t  now = time ((time_t *) NULL);
        struct  tm * tmp = localtime (&now);

        len--;
        bufp++;

        ::printf ("%04d-%02d-%02d %02d:%02d:%02d: STM32: ", tmp->tm_year + 1900, tmp->tm_mon + 1, tmp->tm_mday, tmp->tm_hour, tmp->tm_min, tmp->tm_sec);

        while (len-- > 0)
        {
            putchar (*bufp);
            bufp++;
        }

        putchar ('\n');
    }
}

void
MSG::msg (uint8_t * buf, uint_fast8_t len)
{
    switch (buf[0])
    {
        case MSG_ADC:                       MSG::adc (buf, len);                                break;
        case MSG_RC1:                       MSG::rc1 (buf, len);                                break;
        case MSG_RC2:                       MSG::rc2 (buf, len);                                break;
        case MSG_POM_CV:                    MSG::pom_cv (buf, len);                             break;
        case MSG_PGM_CV:                    MSG::pgm_cv (buf, len);                             break;
        case MSG_LOCO_RC2_RATE:             MSG::loco_rc2_rate (buf, len);                      break;
        case MSG_S88:                       MSG::s88 (buf, len);                                break;
        case MSG_RCL:                       MSG::rcl (buf, len);                                break;
        case MSG_DEBUG_MESSAGE:             MSG::debug_message (buf, len);                      break;

        default:
            Debug::printf (DEBUG_LEVEL_NORMAL, "msg: invalid msg: 0x%02X\n", buf[0]);
            break;
    }
}

void
MSG::flush_msg (void)
{
    uint_fast8_t    ch;

    while (Serial::poll (&ch))
    {
        ;
    }
}

void
MSG::read_msg (void)
{
    static uint8_t          buf[MAX_MSG_SIZE];
    static uint_fast8_t     bufidx = 0;
    static uint_fast8_t     length = 0;
    static uint_fast8_t     msg_state = MSG_STATE_WAIT_FOR_FRAME_START;
    static bool             escape = false;
    uint_fast8_t            ch;

    while (Serial::poll (&ch))
    {
        if (msg_state == MSG_STATE_WAIT_FOR_FRAME_START)
        {
            if (ch == MSG_FRAME_START)
            {
                msg_state   = MSG_STATE_WAIT_FOR_LEN;
                bufidx      = 0;
                escape      = false;
                length      = 0;
            }
            else if (ch == MSG_FRAME_STOP)
            {
                DCC::channel_stopped = 1;
            }
            else if (ch == MSG_FRAME_CONTINUE)
            {
                DCC::channel_stopped = 0;
            }
            else
            {
                Debug::printf (DEBUG_LEVEL_NORMAL, "read_msg: error: ch=0x%02X\n", ch);
            }
        }
        else if (msg_state == MSG_STATE_WAIT_FOR_LEN)
        {
            if (length < MAX_MSG_SIZE)
            {
                msg_state = MSG_STATE_WAIT_FOR_FRAME_END;
                length = ch;
            }
            else
            {
                if (ch == MSG_FRAME_START)
                {
                    msg_state   = MSG_STATE_WAIT_FOR_LEN;
                    bufidx      = 0;
                    escape      = false;
                    length      = 0;
                }
                else
                {
                    msg_state = MSG_STATE_WAIT_FOR_FRAME_START;
                }

                Debug::printf (DEBUG_LEVEL_NORMAL, "read_msg: expected length, got 0x%02X\n", ch);
            }
        }
        else if (msg_state == MSG_STATE_WAIT_FOR_FRAME_END)
        {
            if (ch == MSG_FRAME_ESCAPE)
            {
                escape = true;
            }
            else
            {
                if (escape)
                {
                    ch += MSG_FRAME_ESCAPE_OFFSET;
                    escape = false;
                }

                if (length > 0)
                {
                    buf[bufidx++] = ch;
                    length--;
                }
                else
                {
                    if (ch == MSG_FRAME_END)
                    {
                        MSG::msg (buf, bufidx);
                    }
                    else
                    {
                        if (ch == MSG_FRAME_START)
                        {
                            msg_state   = MSG_STATE_WAIT_FOR_LEN;
                            bufidx      = 0;
                            escape      = false;
                            length      = 0;
                        }

                        Debug::printf (DEBUG_LEVEL_NORMAL, "read_msg: expected MSG_FRAME_END, got 0x%02X\n", ch);
                    }

                    msg_state = MSG_STATE_WAIT_FOR_FRAME_START;
                }
            }
        }
    }
}
