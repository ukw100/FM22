/*-------------------------------------------------------------------------------------------------------------------------------------------
 * rc-detector.c - Railcom(TM) detector
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
#include "board-led.h"
#include "dcc.h"
#include "rc-detector.h"
#include "rc-detector-uart.h"
#include "io.h"


#define RC_DETECTOR_ENABLE_PERIPH_CLOCK_CMD  RCC_AHB1PeriphClockCmd

#if defined STM32F407VE
#define RC_DETECTOR_ENABLE_PERIPH            RCC_AHB1Periph_GPIOC
#define RC_DETECTOR_ENABLE_PORT              GPIOC
#define RC_DETECTOR_ENABLE_PIN               GPIO_Pin_3                      // PC3
#elif defined STM32F401CC
#define RC_DETECTOR_ENABLE_PERIPH            RCC_AHB1Periph_GPIOA
#define RC_DETECTOR_ENABLE_PORT              GPIOA
#define RC_DETECTOR_ENABLE_PIN               GPIO_Pin_15                     // PA15
#else
#error unknown STM32
#endif

/*----------------------------------------------------------------------------------------------------------------------------------------------------------------
 *  Hamming Code Table 8/4:
 *
 *  0x00   10101100   0x10   10110010   0x20   01010110   0x30   11000110
 *  0x01   10101010   0x11   10110100   0x21   01001110   0x31   11001100
 *  0x02   10101001   0x12   10111000   0x22   01001101   0x32   01111000
 *  0x03   10100101   0x13   01110100   0x23   01001011   0x33   00010111
 *  0x04   10100011   0x14   01110010   0x24   01000111   0x34   00011011
 *  0x05   10100110   0x15   01101100   0x25   01110001   0x35   00011101
 *  0x06   10011100   0x16   01101010   0x26   11101000   0x36   00011110
 *  0x07   10011010   0x17   01101001   0x27   11100100   0x37   00101110
 *  0x08   10011001   0x18   01100101   0x28   11100010   0x38   00110110
 *  0x09   10010101   0x19   01100011   0x29   11010001   0x39   00111010
 *  0x0A   10010011   0x1A   01100110   0x2A   11001001   0x3A   00100111
 *  0x0B   10010110   0x1B   01011100   0x2B   11000101   0x3B   00101011
 *  0x0C   10001110   0x1C   01011010   0x2C   11011000   0x3C   00101101
 *  0x0D   10001101   0x1D   01011001   0x2D   11010100   0x3D   00110101
 *  0x0E   10001011   0x1E   01010101   0x2E   11010010   0x3E   00111001
 *  0x0F   10110001   0x1F   01010011   0x2F   11001010   0x3F   00110011
 *
 *  ACK    00001111 (0x0F)
 *  ACK    11110000 (0xF0)
 *  res    11100001 (0xE1)
 *  res    11000011 (0xC3)
 *  res    10000111 (0x87)
 *  NACK   00111100 (0x3C)
 *
 *  See also: RCN-217 page 8
 *----------------------------------------------------------------------------------------------------------------------------------------------------------------
 */
#define ACK         0x40            // ACK      if ch == 0x0F (0b00001111) or ch == 0xF0 (0b11110000)
#define RES1        0x41            // reserved if ch == 0xE1 (0b11100001)
#define RES2        0x42            // reserved if ch == 0xC3 (0b11000011)
#define RES3        0x43            // reserved if ch == 0x85 (0b10000111)
#define NACK        0x44            // NACK     if ch == 0x3C (0b00111100)

static uint8_t hamming_map[256] =
{
//   0     1     2     3     4     5     6     7     8     9     A     B     C     D     E     F
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, ACK,      // 0
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x33, 0xFF, 0xFF, 0xFF, 0x34, 0xFF, 0x35, 0x36, 0xFF,     // 1
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x3A, 0xFF, 0xFF, 0xFF, 0x3B, 0xFF, 0x3C, 0x37, 0xFF,     // 2
    0xFF, 0xFF, 0xFF, 0x3F, 0xFF, 0x3D, 0x38, 0xFF, 0xFF, 0x3E, 0x39, 0xFF, NACK, 0xFF, 0xFF, 0xFF,     // 3
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x24, 0xFF, 0xFF, 0xFF, 0x23, 0xFF, 0x22, 0x21, 0xFF,     // 4
    0xFF, 0xFF, 0xFF, 0x1F, 0xFF, 0x1E, 0x20, 0xFF, 0xFF, 0x1D, 0x1C, 0xFF, 0x1B, 0xFF, 0xFF, 0xFF,     // 5
    0xFF, 0xFF, 0xFF, 0x19, 0xFF, 0x18, 0x1A, 0xFF, 0xFF, 0x17, 0x16, 0xFF, 0x15, 0xFF, 0xFF, 0xFF,     // 6
    0xFF, 0x25, 0x14, 0xFF, 0x13, 0xFF, 0xFF, 0xFF, 0x32, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,     // 7
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, RES3, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x0E, 0xFF, 0x0D, 0x0C, 0xFF,     // 8
    0xFF, 0xFF, 0xFF, 0x0A, 0xFF, 0x09, 0x0B, 0xFF, 0xFF, 0x08, 0x07, 0xFF, 0x06, 0xFF, 0xFF, 0xFF,     // 9
    0xFF, 0xFF, 0xFF, 0x04, 0xFF, 0x03, 0x05, 0xFF, 0xFF, 0x02, 0x01, 0xFF, 0x00, 0xFF, 0xFF, 0xFF,     // A
    0xFF, 0x0F, 0x10, 0xFF, 0x11, 0xFF, 0xFF, 0xFF, 0x12, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,     // B
    0xFF, 0xFF, 0xFF, RES2, 0xFF, 0x2B, 0x30, 0xFF, 0xFF, 0x2A, 0x2F, 0xFF, 0x31, 0xFF, 0xFF, 0xFF,     // C
    0xFF, 0x29, 0x2E, 0xFF, 0x2D, 0xFF, 0xFF, 0xFF, 0x2C, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,     // D
    0xFF, RES1, 0x28, 0xFF, 0x27, 0xFF, 0xFF, 0xFF, 0x26, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,     // E
    ACK,  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,     // F
};

#define ID_POM          0x00
#define ID_ADDR_HIGH    0x01
#define ID_ADDR_LOW     0x02
#define ID_DYN          0x07
#define ID_XPOM00       0x08
#define ID_XPOM01       0x09
#define ID_XPOM02       0x0A
#define ID_XPOM03       0x0B
#define ID_INVALID      0xFF

// volatile: rc_detector_read_rcx() called by TIM3_IRQHandler()

typedef struct
{
    volatile uint_fast8_t       addr_high;
    volatile uint_fast8_t       addr_low;
    volatile uint_fast8_t       addr_high_valid;
    volatile uint_fast8_t       addr_low_valid;
    volatile uint_fast16_t      addr;
    volatile uint32_t           timestamp;
} RC1_DATA;

static RC1_DATA                 rc1_data;

static volatile uint_fast8_t    rc2_addr_high_valid;
static volatile uint_fast8_t    rc2_addr_low_valid;
volatile uint_fast16_t          rc2_addr;
volatile uint_fast8_t           rc2_addr_valid;

volatile uint_fast8_t           rc2_cv_value;
volatile uint_fast8_t           rc2_cv_value_valid = 0;

#define MAX_XPOM_SEQUENCES      4
#define MAX_XPOM_CV_VALUES      4
volatile uint8_t                rc2_cv_values[MAX_XPOM_SEQUENCES][MAX_XPOM_CV_VALUES];
volatile uint8_t                rc2_cv_values_valid[MAX_XPOM_SEQUENCES] = { 0, 0, 0, 0 };

volatile uint_fast8_t           rc2_dyn_subidx;
volatile uint_fast8_t           rc2_dyn_value;
volatile uint_fast8_t           rc2_dyn_valid = 0;

#define MAX_RC2_INFOS           256

#define RC2_COUNTS_PER_SLOT     50                                      // max counts per slot, must be < 256!

typedef struct
{
    volatile uint32_t           cmd_millis;                             // milliseconds, when cmd has been sent
    volatile uint32_t           rc2_millis;                             // milliseconds, when cmd has been answered
    volatile uint8_t            cmd_cnt[2];                             // counters, how many commands have been sent
    volatile uint8_t            rc2_cnt[2];                             // counters, how many RC2 anwers arrived
    volatile uint8_t            active_slotidx;
} RC2INFO;

static RC2INFO                  rc2info[MAX_RC2_INFOS];
volatile uint_fast16_t          rc_detector_n_rc2infos;

static uint8_t                  rc_detector_locations[RC_DETECTOR_MAX_LOCOS];
static uint32_t                 rc_detector_location_millis[RC_DETECTOR_MAX_LOCOS];
uint8_t                         rc_detector_n_locations;

uint16_t                        rc_loco_addrs[RC_DETECTOR_MAX_LOCOS];
uint_fast16_t                   n_rc_loco_addrs = 0;

static void
set_rc2_millis_and_counter (uint_fast16_t idx)
{
    if (idx < MAX_RC2_INFOS)
    {
        if (idx >= rc_detector_n_rc2infos)
        {
            rc_detector_n_rc2infos = idx + 1;
        }

        rc2info[idx].rc2_millis = millis;
        rc2info[idx].rc2_cnt[rc2info[idx].active_slotidx]++;
    }
}

static void
set_rc2_millis_only (uint_fast16_t idx)
{
    if (idx < MAX_RC2_INFOS)
    {
        if (idx >= rc_detector_n_rc2infos)
        {
            rc_detector_n_rc2infos = idx + 1;
        }

        rc2info[idx].rc2_millis = millis;
    }
}

static void
inc_cmd_counter (uint_fast16_t idx)
{
    if (idx < MAX_RC2_INFOS)
    {
        uint_fast8_t    active_slotidx;

        if (idx >= rc_detector_n_rc2infos)
        {
            rc_detector_n_rc2infos = idx + 1;
        }

        rc2info[idx].cmd_millis = millis;                       // yes, update timestamp

        active_slotidx = rc2info[idx].active_slotidx;

        if (rc2info[idx].cmd_cnt[active_slotidx] == RC2_COUNTS_PER_SLOT)
        {
            if (active_slotidx == 0)
            {
                active_slotidx = 1;
            }
            else
            {
                active_slotidx = 0;
            }

            rc2info[idx].rc2_cnt[active_slotidx] = 0;
            rc2info[idx].cmd_cnt[active_slotidx] = 1;
            rc2info[idx].active_slotidx = active_slotidx;
        }
        else
        {
            rc2info[idx].cmd_cnt[active_slotidx]++;
        }
    }
}

uint32_t
rc_detector_get_rc2_millis (uint_fast16_t idx)
{
    uint32_t    rtc = 0;

    if (idx < rc_detector_n_rc2infos)
    {
        rtc = rc2info[idx].rc2_millis;
    }

    return rtc;
}

uint32_t
rc_detector_get_rc2_millis_diff (uint_fast16_t idx)
{
    uint32_t    rtc = 0;

    if (idx < rc_detector_n_rc2infos)
    {
        if (rc2info[idx].rc2_millis)
        {
            rtc = rc2info[idx].rc2_millis - rc2info[idx].cmd_millis;
        }
        else
        {
            rtc = 999999;
        }
    }

    return rtc;
}

uint_fast8_t
rc_detector_get_rc2_rate (uint_fast16_t loco_idx)
{
    uint_fast8_t    rtc = 0;

    if (loco_idx < rc_detector_n_rc2infos)
    {
        uint_fast8_t        slotidx;
        uint32_t            cmd_cnt;

        if (rc2info[loco_idx].active_slotidx == 0)
        {
            slotidx = 1;
        }
        else
        {
            slotidx = 0;
        }

        cmd_cnt = rc2info[loco_idx].cmd_cnt[slotidx];

        if (cmd_cnt > 0)
        {
            rtc = (100 * rc2info[loco_idx].rc2_cnt[slotidx]) / RC2_COUNTS_PER_SLOT;
        }
    }

    return rtc;
}

void
rc_detector_reset_rc2_millis (void)
{
    uint_fast16_t loco_idx;

    for (loco_idx = 0; loco_idx < rc_detector_n_rc2infos; loco_idx++)
    {
        rc2info[loco_idx].rc2_millis = 0;
        rc2info[loco_idx].rc2_cnt[0] = 0;
        rc2info[loco_idx].rc2_cnt[1] = 0;
    }
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * rc_detector_reset_rc1_data() - reset rc1 data
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
rc_detector_reset_rc1_data (void)
{
    rc1_data.addr_high_valid  = 0;
    rc1_data.addr_low_valid   = 0;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * rc_detector_reset_rc2_data() - reset channel 2 data
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
rc_detector_reset_rc2_data (void)
{
    // rc_detector_uart_input_flush ();
    rc2_addr_high_valid    = 0;
    rc2_addr_low_valid     = 0;
    rc2_addr_valid         = 0;
    rc2_cv_value_valid     = 0;
    rc2_dyn_valid          = 0;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * rc_detector_reset_rc_data() - reset channel 1+2 data
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
rc_detector_reset_rc_data (void)
{
    rc_detector_reset_rc1_data ();
    rc_detector_reset_rc2_data ();
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * rc_detector_enable() - enable decoder
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
rc_detector_enable (void)
{
    GPIO_SET_BIT(RC_DETECTOR_ENABLE_PORT, RC_DETECTOR_ENABLE_PIN);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * rc_detector_disable() - disable decoder
 *
 * fm TODO: if we disable, we get no data for RC1? See dcc.c
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
rc_detector_disable (void)
{
    GPIO_RESET_BIT(RC_DETECTOR_ENABLE_PORT, RC_DETECTOR_ENABLE_PIN);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * rc_detector_read_rc1() - read 2 bytes from rc1
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
rc_detector_read_rc1 (void)
{
    uint_fast8_t        rc1_byte1;
    uint_fast8_t        rc1_byte2;
    uint_fast8_t        ch1;
    uint_fast8_t        ch2;
    uint_fast16_t       rxsize;

    rxsize = rc_detector_uart_rxsize ();

    if (rxsize >= 2)
    {
        ch1 = rc_detector_uart_getc ();
        ch2 = rc_detector_uart_getc ();
        rc1_byte1 = hamming_map[ch1];                       // 0  0  ID ID ID ID A7 A6
        rc1_byte2 = hamming_map[ch2];                       // 0  0  A5 A4 A3 A2 A1 A0

        // if (debug >= 2) console_printf ("ch1: 0x%02x 0x%02x\r\n", rc1_byte1, rc1_byte2);

        if (rc1_byte1 != 0xFF && rc1_byte2 != 0xFF)
        {
            uint_fast8_t    id = rc1_byte1 >> 2;

            if (id == ID_ADDR_HIGH)
            {
                rc1_data.addr_high = (rc1_byte2 & 0x3F);    // ignore A7 A6 of uppper address: always set
                rc1_data.addr_high_valid = 1;
            }
            else if (id == ID_ADDR_LOW)
            {
                rc1_data.addr_low = ((rc1_byte1 & 0x03) << 6) | (rc1_byte2 & 0x3F);
                rc1_data.addr_low_valid = 1;
            }

            if (rc1_data.addr_high_valid && rc1_data.addr_low_valid)
            {
                rc1_data.addr = (rc1_data.addr_high << 8) | rc1_data.addr_low;
                rc1_data.timestamp          = millis;
                rc1_data.addr_high_valid    = 0;
                rc1_data.addr_low_valid     = 0;
                // if (debug >= 2) console_printf ("ch1_addr = %u\r\n", rc1_addr);
            }
        }
        else
        {
            // if (debug >= 2) console_puts ("ch1: invalid hamming code\r\n");       // stört Kommunikation mit ESP8266 bei "!GO"
        }
    }

    while (rc_detector_uart_rxsize ())
    {
        (void) rc_detector_uart_getc ();
    }
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * rc_detector_rc1_addr() - get address of loco sent in rc1
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
uint_fast16_t
rc_detector_rc1_addr (void)
{
    uint_fast16_t rtc = RC_DETECTOR_ADDRESS_INVALID;

    if (millis - rc1_data.timestamp < 1000)                            // last rc1 data less than one second
    {
        rtc = rc1_data.addr;
    }

    return rtc;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * rc_detector_rc2_dyn() - get dynamic data values
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
uint_fast16_t
rc_detector_rc2_dyn (void)
{
    uint_fast8_t rtc = RC_DETECTOR_DYN_INVALID;

    if (rc2_dyn_valid)
    {
        rtc = (rc2_dyn_valid << 8) | rc2_dyn_subidx;
        rc2_dyn_valid = 0;
    }

    return rtc;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * rc_detector_read_rc2() - read n bytes from channel 2
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
rc_detector_read_rc2 (uint_fast16_t loco_idx)
{
    static uint_fast8_t rc2_addr_high;
    static uint_fast8_t rc2_addr_low;
    uint8_t             rec_buf[6];                                                             // only for debugging
    uint8_t             buf[6];
    uint_fast16_t       rxsize;
    uint_fast8_t        ch;
    uint_fast16_t       idx;
    uint_fast8_t        hamming_error = 0;

    inc_cmd_counter (loco_idx);

    rxsize = rc_detector_uart_rxsize ();

    if (rxsize > 0)
    {
        if (rxsize > 6)
        {
            rxsize = 6;
        }

        for (idx = 0; idx < rxsize; idx++)
        {
            ch = rc_detector_uart_getc ();

            if (ch == 0xF8)                                                                     // ESU hack: ESU sends 0xF8 instead of 0xF0
            {
                ch = 0xF0;
            }

            if (debuglevel >= 2)
            {
                rec_buf[idx]   = ch;
            }

            buf[idx] = hamming_map[ch];

            if (buf[idx] == 0xFF)                                                               // invalid hamming code
            {
                hamming_error = 1;
            }
        }

        if (! hamming_error)                                                                    // all hamming codes valid
        {
            if (buf[0] == ACK || buf[0] == NACK || buf[0] == RES1 || buf[0] == RES2 || buf[0] == RES3)
            {
                set_rc2_millis_and_counter (loco_idx);
            }
            else if (rxsize >= 2 && buf[0] < 0x40 && buf[1] < 0x40)                             // valid data
            {
                uint_fast8_t    id      = buf[0] >> 2;

                switch (id)
                {
                    case ID_POM:
                    {
                        uint_fast8_t value = ((buf[0] & 0x03) << 6) | (buf[1] & 0x3F);          // 00IIIIVV 00VVVVVV
                        rc2_cv_value = value;
                        rc2_cv_value_valid = 1;
                        set_rc2_millis_and_counter (loco_idx);
                        break;
                    }

                    case ID_ADDR_HIGH:
                    {
                        uint_fast8_t value = ((buf[0] & 0x03) << 6) | (buf[1] & 0x3F);          // 00IIIIVV 00VVVVVV
                        rc2_addr_high = value & 0x3F;                                           // ignore A7 A6 of uppper address: always set
                        rc2_addr_high_valid = 1;
                        set_rc2_millis_and_counter (loco_idx);
                        break;
                    }

                    case ID_ADDR_LOW:
                    {
                        uint_fast8_t value = ((buf[0] & 0x03) << 6) | (buf[1] & 0x3F);          // 00IIIIVV 00VVVVVV
                        rc2_addr_low = value;
                        rc2_addr_low_valid = 1;
                        set_rc2_millis_and_counter (loco_idx);
                        break;
                    }

                    case ID_DYN:
                    {
                        if (rxsize >= 3)
                        {
                            uint_fast8_t value = ((buf[0] & 0x03) << 6) | (buf[1] & 0x3F);      // 00IIIIVV 00VVVVVV

                            rc2_dyn_subidx = buf[2];
                            rc2_dyn_value  = value;
                            rc2_dyn_valid  = 1;
                            set_rc2_millis_and_counter (loco_idx);

                            if (rxsize == 6)
                            {
                                uint_fast8_t    id2     = buf[3] >> 2;

                                if (id2 == ID_DYN)
                                {
                                    // uint_fast8_t    value2  = ((buf[3] & 0x03) << 6) | (buf[4] & 0x3F);
                                    // uint_fast8_t    subidx2 = buf[5];
                                }
                            }
                        }
                        break;
                    }

                    case ID_XPOM00:                                                             // XPOM sequence 00
                    case ID_XPOM01:                                                             // XPOM sequence 01
                    case ID_XPOM02:                                                             // XPOM sequence 02
                    case ID_XPOM03:                                                             // XPOM sequence 03
                    {                                                                           // [0]      [1]      [2]      [3]      [4]      [5]
                        if (rxsize == 6)                                                        // 00IISSAA 00AAAAAA 00BBBBBB 00BBCCCC 00CCCCDD 00DDDDDD
                        {
                            uint_fast8_t    seq = id & 0x03;

                            if (loco_idx == 0xFFFF && ! rc2_cv_values_valid[seq])
                            {
                                rc2_cv_values[seq][0] = ((buf[0] & 0x03) << 6) | (buf[1] & 0x3F);   // 00IISSAA 00AAAAAA
                                rc2_cv_values[seq][1] = (buf[2] << 2) | (buf[3] >> 4);              // 00BBBBBB 00BBxxxx
                                rc2_cv_values[seq][2] = ((buf[3] & 0x0F) << 4) | (buf[4] >> 2);     // 00xxCCCC 00CCCCxx
                                rc2_cv_values[seq][3] = ((buf[4] & 0x03) << 6) | (buf[5] & 0x3F);   // 00xxxxDD 00DDDDDD
                                rc2_cv_values_valid[seq] = 1;
                            }

                            // char b[128];
                            // sprintf (b, "x: %d, %d %d: %d %d %d %d", loco_idx, id, rxsize, rc2_cv_values[seq][0], rc2_cv_values[seq][1], rc2_cv_values[seq][2], rc2_cv_values[seq][3]);
                            // listener_send_debug_msg (b);

                            set_rc2_millis_and_counter (loco_idx);
                        }
                        break;
                    }

                    default:                                                                    // invalid id
                    {
#if 0
                        char        b[128];
                        uint8_t     v[4];
                        v[0] = ((buf[0] & 0x03) << 6) | (buf[1] & 0x3F);   // 00IISSAA 00AAAAAA
                        v[1] = (buf[2] << 2) | (buf[3] >> 4);              // 00BBBBBB 00BBxxxx
                        v[2] = ((buf[3] & 0x0F) << 4) | (buf[4] >> 2);     // 00xxCCCC 00CCCCxx
                        v[3] = ((buf[4] & 0x03) << 6) | (buf[5] & 0x3F);   // 00xxxxDD 00DDDDDD
                        sprintf (b, "x: %d, %d %d: %d %d %d %d", loco_idx, id, rxsize, v[0], v[1], v[2], v[3]);
                        listener_send_debug_msg (b);
#endif
                        set_rc2_millis_only (loco_idx);
                        break;
                    }
                }

                if (rc2_addr_high_valid && rc2_addr_low_valid)
                {
                    rc2_addr               = (rc2_addr_high << 8) | rc2_addr_low;
                    rc2_addr_valid         = 1;
                    rc2_addr_high_valid    = 0;
                    rc2_addr_low_valid     = 0;
                }
            }
            else                                                                                // invalid data
            {
                set_rc2_millis_only (loco_idx);
                rc_detector_reset_rc2_data ();
            }
        }
        else                                                                                    // invalid hamming code, see above
        {
            if (debuglevel >= 2)
            {
                char hex_buf[64];
                char * p = hex_buf;

                for (idx = 0; idx < rxsize; idx++)
                {
                    sprintf (p, "%02X ", rec_buf[idx]);
                    p += 3;
                }

                listener_queue_debug_msgf ("rc2: %d inv %s", loco_idx, hex_buf);
            }

            set_rc2_millis_only (loco_idx);
            rc_detector_reset_rc2_data ();
        }

    }
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * rc_detector_set_loco_location() - set location of loco
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
rc_detector_set_loco_location (uint_fast16_t addr, uint_fast8_t location)
{
    uint_fast16_t loco_idx;

    for (loco_idx = 0; loco_idx < n_rc_loco_addrs; loco_idx++)
    {
        if (rc_loco_addrs[loco_idx] == addr)
        {
            break;
        }
    }

    if (loco_idx < n_rc_loco_addrs)
    {
        if (loco_idx >= rc_detector_n_locations)
        {
            rc_detector_n_locations = loco_idx + 1;
        }

        rc_detector_locations[loco_idx]         = location;
        rc_detector_location_millis[loco_idx]   = millis;
    }
}

#define MAX_LOCATION_AGE    1000                             // 1000 msec as location age
/*-------------------------------------------------------------------------------------------------------------------------------------------
 * rc_detector_get_loco_location() - get location of loco
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
rc_detector_get_loco_location (uint_fast16_t loco_idx)
{
    uint_fast8_t    location;

    if (loco_idx < rc_detector_n_locations && millis - rc_detector_location_millis[loco_idx] < MAX_LOCATION_AGE)
    {
        location = rc_detector_locations[loco_idx];
    }
    else
    {
        location = 0xFF;
    }

    return location;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * rc_detector_reset_all_loco_locations() - reset location of loco
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
rc_detector_reset_all_loco_locations (void)
{
    uint_fast16_t   loco_idx;

    for (loco_idx = 0; loco_idx < RC_DETECTOR_MAX_LOCOS; loco_idx++)
    {
        rc_detector_locations[loco_idx] = 0xFF;
    }
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * rc_detector_init() - init UART4 and GPIO
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
rc_detector_init (void)
{
    GPIO_InitTypeDef gpio;

    rc_detector_uart_init (250000, USART_WordLength_8b, USART_Parity_No, USART_StopBits_1);

    GPIO_StructInit (&gpio);
    RC_DETECTOR_ENABLE_PERIPH_CLOCK_CMD (RC_DETECTOR_ENABLE_PERIPH, ENABLE);     // enable clock for LED Port
    GPIO_SET_MODE_OUT_PP(gpio, RC_DETECTOR_ENABLE_PIN, GPIO_Speed_2MHz);
    GPIO_Init(RC_DETECTOR_ENABLE_PORT, &gpio);
    rc_detector_disable ();
    rc_detector_reset_all_loco_locations ();
}


