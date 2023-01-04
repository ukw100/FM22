/*------------------------------------------------------------------------------------------------------------------------
 * dcc.c - send DCC commands to STM32F4
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
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "dcc.h"
#include "s88.h"
#include "serial.h"
#include "millis.h"
#include "debug.h"
#include "msg.h"

#define CMD_FRAME_START                 0xFF            // Start of Text
#define CMD_FRAME_END                   0xFE            // End of Text
#define CMD_FRAME_ESCAPE                0xFD            // Data Link Escape
#define CMD_FRAME_ESCAPE_OFFSET         0xF0

#define ACK                             0x06            // Acknowledge
#define NAK                             0x15            // Negative Acknowledge

#define CMD_BOOSTER_ON                  0x01
#define CMD_BOOSTER_OFF                 0x02
#define CMD_SET_MODE                    0x03

#define CMD_PGM_READ_CV                 0x11
#define CMD_PGM_WRITE_CV                0x12
#define CMD_PGM_WRITE_CV_BIT            0x13
#define CMD_PGM_WRITE_ADDRESS           0x14

#define CMD_POM_READ_CV                 0x21
#define CMD_POM_WRITE_CV                0x22
#define CMD_POM_WRITE_CV_BIT            0x23
#define CMD_POM_WRITE_ADDRESS           0x24

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

#define WAIT_FOR_POM_CV_MSEC            300
#define WAIT_FOR_PGM_CV_MSEC            3000

uint_fast8_t                            DCC::channel_stopped = 0;

uint16_t                                DCC::adc_value;
uint16_t                                DCC::rc1_value;
uint_fast8_t                            DCC::booster_is_on;
unsigned long                           DCC::booster_is_on_time;

uint16_t                                DCC::pgm_limit;
uint16_t                                DCC::pgm_min_lower_value;
uint16_t                                DCC::pgm_max_lower_value;
uint16_t                                DCC::pgm_min_upper_value;
uint16_t                                DCC::pgm_max_upper_value;
uint16_t                                DCC::pgm_min_cnt;
uint16_t                                DCC::pgm_max_cnt;

POM_CV                                  DCC::pom_cv;
PGM_CV                                  DCC::pgm_cv;

uint_fast8_t                            DCC::mode        = RAILCOM_MODE;

void
DCC::send_cmd (uint8_t * buf, uint_fast8_t len, bool set_flag_stopped)
{
    uint8_t     ch;
    uint8_t *   bufp = buf;

    if (DCC::channel_stopped)
    {
        uint32_t    idx;

        for (idx = 0; idx < 100 && DCC::channel_stopped; idx++)             // wait 100 msec for message "continue"
        {
            usleep (1000);
            MSG::read_msg ();
        } 

        if (idx == 500)
        {
            Debug::printf (DEBUG_LEVEL_NORMAL, "dcc channel stop timeout, let's continue\n");
            DCC::channel_stopped = 0;
        }
    }

    Serial::send (CMD_FRAME_START);
    Serial::send (len);

    while (len--)
    {
        ch = *bufp;

        if (ch == CMD_FRAME_START || ch == CMD_FRAME_END || ch == CMD_FRAME_ESCAPE)
        {
            Serial::send (CMD_FRAME_ESCAPE);
            ch -= CMD_FRAME_ESCAPE_OFFSET;
        }
        Serial::send (ch);
        bufp++;
    }

    Serial::send (CMD_FRAME_END);

    if (set_flag_stopped)
    {
        DCC::channel_stopped = 1;
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 * booster_off () - switch booster off
 *------------------------------------------------------------------------------------------------------------------------
 */
void
DCC::booster_off (void)
{
    uint8_t buf[1];

    buf[0] = CMD_BOOSTER_OFF;
    send_cmd (buf, 1, false);
    DCC::booster_is_on = 0;
}

/*------------------------------------------------------------------------------------------------------------------------
 * booster_on () - switch booster on
 *------------------------------------------------------------------------------------------------------------------------
 */
void
DCC::booster_on (void)
{
    uint8_t buf[1];

    buf[0] = CMD_BOOSTER_ON;
    send_cmd (buf, 1, false);
    DCC::booster_is_on = 1;
    DCC::booster_is_on_time = Millis::elapsed ();
}


/*------------------------------------------------------------------------------------------------------------------------
 * get_mode () - get DCC mode
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
DCC::get_mode (void)
{
    uint_fast8_t mode = DCC::mode;
    return mode;
}

/*------------------------------------------------------------------------------------------------------------------------
 * set_mode () - set DCC mode
 *
 * Parameters:
 *      RAILCOM_MODE        - set RailCom mode
 *      NO_RAILCOM_MODE     - set RailCom mode
 *      PROGRAMMING_MODE    - set Programming mode
 *------------------------------------------------------------------------------------------------------------------------
 */
void
DCC::set_mode (uint_fast8_t newmode)
{
    uint8_t buf[2];

    DCC::mode = newmode;
    buf[0] = CMD_SET_MODE;
    buf[1] = DCC::mode;
    send_cmd (buf, 2, false);
}

/*------------------------------------------------------------------------------------------------------------------------
 * pgm_read_cv () - read CV in programming mode
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
DCC::pgm_read_cv (uint_fast8_t * cv_valuep, uint_fast16_t cv)
{
    uint8_t         buf[3];

    buf[0] = CMD_PGM_READ_CV;
    buf[1] = cv >> 8;
    buf[2] = cv & 0xFF;

    send_cmd (buf, 3, true);

    pgm_cv.valid = 0;

    unsigned long next_millis = Millis::elapsed() + WAIT_FOR_PGM_CV_MSEC;

    while (Millis::elapsed() < next_millis)
    {
        MSG::read_msg ();

        if (pgm_cv.valid)
        {
            if (pgm_cv.cv == cv)
            {
                *cv_valuep = pgm_cv.cv_value;
                return 1;
            }
        }
        usleep (1000);  // sleep one millisecond        
    }

    return 0;
}

/*------------------------------------------------------------------------------------------------------------------------
 * pgm_write_cv () - write CV in programming mode
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
DCC::pgm_write_cv (uint_fast16_t cv, uint_fast8_t cv_value)
{
    uint8_t         buf[4];

    buf[0] = CMD_PGM_WRITE_CV;
    buf[1] = cv >> 8;
    buf[2] = cv & 0xFF;
    buf[3] = cv_value;

    send_cmd (buf, 4, true);

    return 1;           // todo: read and compare
}

/*------------------------------------------------------------------------------------------------------------------------
 * pgm_write_cv_bit () - write bit into CV in programming mode
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
DCC::pgm_write_cv_bit (uint_fast16_t cv, uint_fast8_t bitpos, uint_fast8_t bitvalue)
{
    uint8_t         buf[5];

    buf[0] = CMD_PGM_WRITE_CV_BIT;
    buf[1] = cv >> 8;
    buf[2] = cv & 0xFF;
    buf[3] = bitpos;
    buf[4] = bitvalue;

    send_cmd (buf, 5, true);
    return 1;           // todo: read and compare
}

/*------------------------------------------------------------------------------------------------------------------------
 * pgm_write_address () - write loco address in programming mode
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
DCC::pgm_write_address (uint_fast16_t addr)
{
    uint8_t         buf[3];

    buf[0] = CMD_PGM_WRITE_ADDRESS;
    buf[1] = addr >> 8;
    buf[2] = addr & 0xFF;

    send_cmd (buf, 3, true);
    return 1;           // todo: read and compare
}

/*------------------------------------------------------------------------------------------------------------------------
 * loco_28 () - send loco command, 28 speed steps
 *------------------------------------------------------------------------------------------------------------------------
 */
void
DCC::loco_28 (uint_fast16_t loco_idx, uint_fast16_t addr, uint_fast8_t direction, uint_fast8_t speed)
{
    uint8_t         buf[7];

    buf[0] = CMD_LOCO_28;
    buf[1] = loco_idx >> 8;
    buf[2] = loco_idx & 0xFF;
    buf[3] = addr >> 8;
    buf[4] = addr & 0xFF;
    buf[5] = direction;
    buf[6] = speed;

    send_cmd (buf, 7, true);
}

/*------------------------------------------------------------------------------------------------------------------------
 * loco_function () - send loco functions
 *------------------------------------------------------------------------------------------------------------------------
 */
void
DCC::loco_function (uint_fast16_t loco_idx, uint_fast16_t addr, uint32_t fmask, uint_fast8_t range)
{
    uint8_t         buf[10];
    uint8_t         mask;

    switch (range)
    {
        case DCC_F00_F04_RANGE:
            buf[0] = CMD_LOCO_FUNCTION_F00_F04;
            mask = fmask & 0x1F;
            break;
        case DCC_F05_F08_RANGE:
            buf[0] = CMD_LOCO_FUNCTION_F05_F08;
            mask = (fmask >> 5) & 0x0F;
            break;
        case DCC_F09_F12_RANGE:
            buf[0] = CMD_LOCO_FUNCTION_F09_F12;
            mask = (fmask >> 9) & 0x0F;
            break;
        case DCC_F13_F20_RANGE:
            buf[0] = CMD_LOCO_FUNCTION_F13_F20;
            mask = (fmask >> 13) & 0xFF;
            break;
        case DCC_F21_F28_RANGE:
            buf[0] = CMD_LOCO_FUNCTION_F21_F28;
            mask = (fmask >> 21) & 0xFF;
            break;
        default:
            Debug::printf (DEBUG_LEVEL_NORMAL, "DCC::loco_function(): invalid range\n");
            return;                 // invalid range
    }

    buf[1] = loco_idx >> 8;
    buf[2] = loco_idx & 0xFF;
    buf[3] = addr >> 8;
    buf[4] = addr & 0xFF;
    buf[5] = mask;

    send_cmd (buf, 6, true);
}

/*------------------------------------------------------------------------------------------------------------------------
 * loco () - send loco speed, direction and function bits (F0-F31)
 *------------------------------------------------------------------------------------------------------------------------
 */
void
DCC::loco (uint_fast16_t loco_idx, uint_fast16_t addr, uint_fast8_t direction, uint_fast8_t speed, uint32_t fmask, uint_fast8_t n)
{
    uint8_t         buf[12];
    uint_fast8_t    idx;

    if (n > 4)
    {
        n = 4;
    }

    buf[0]  = CMD_LOCO;
    buf[1]  = loco_idx >> 8;
    buf[2]  = loco_idx & 0xFF;
    buf[3]  = addr >> 8;
    buf[4]  = addr & 0xFF;
    buf[5]  = direction;
    buf[6]  = speed;

    for (idx = 0; idx < n; idx++)
    {
        buf[7 + idx] = fmask & 0xFF;
        fmask >>= 8;
    }

    send_cmd (buf, 7 + idx, true);
}

/*------------------------------------------------------------------------------------------------------------------------
 * loco_rc2_rate () - get RC2 rate of loco
 *------------------------------------------------------------------------------------------------------------------------
 */
void
DCC::loco_rc2_rate (uint_fast16_t loco_idx)
{
    uint8_t     buf[3];

    buf[0] = CMD_LOCO_RC2_RATE;
    buf[1] = loco_idx >> 8;
    buf[2] = loco_idx & 0xFF;

    send_cmd (buf, 3, false);
}

/*------------------------------------------------------------------------------------------------------------------------
 * reset () - send DCC reset packet - reset all locos and stop (broadcast)
 *------------------------------------------------------------------------------------------------------------------------
 */
void
DCC::reset (void)
{
    uint8_t         buf[1];

    buf[0]  = CMD_RESET;
    send_cmd (buf, 1, true);
}

/*------------------------------------------------------------------------------------------------------------------------
 * stop () - send DCC stop packet - stop all locos (broadcast)
 *------------------------------------------------------------------------------------------------------------------------
 */
void
DCC::stop (void)
{
    uint8_t         buf[1];

    buf[0]  = CMD_STOP;
    send_cmd (buf, 1, true);
}

/*------------------------------------------------------------------------------------------------------------------------
 * estop () - send DCC emergency stop packet - stop all locos (broadcast)
 *------------------------------------------------------------------------------------------------------------------------
 */
void
DCC::estop (void)
{
    uint8_t         buf[1];

    buf[0]  = CMD_ESTOP;
    send_cmd (buf, 1, true);
}

/*------------------------------------------------------------------------------------------------------------------------
 * reset_decoder () - send DCC reset packet - reset loco and stop
 *------------------------------------------------------------------------------------------------------------------------
 */
void
DCC::reset_decoder (uint_fast16_t addr)
{
    uint8_t         buf[3];

    buf[0]  = CMD_RESET_DECODER;
    buf[1]  = addr >> 8;
    buf[2]  = addr & 0xFF;
    send_cmd (buf, 3, true);
}

/*------------------------------------------------------------------------------------------------------------------------
 * hard_reset_decoder () - send DCC reset packet - hard reset of loco
 *------------------------------------------------------------------------------------------------------------------------
 */
void
DCC::hard_reset_decoder (uint_fast16_t addr)
{
    uint8_t         buf[3];

    buf[0]  = CMD_HARD_RESET_DECODER;
    buf[1]  = addr >> 8;
    buf[2]  = addr & 0xFF;
    send_cmd (buf, 3, true);
}

/*------------------------------------------------------------------------------------------------------------------------
 * pom_write_address () - write loco address in POM mode
 *------------------------------------------------------------------------------------------------------------------------
 */
void
DCC::pom_write_address (uint_fast16_t oldaddr, uint_fast16_t newaddr)
{
    uint8_t         buf[5];

    buf[0]  = CMD_POM_WRITE_ADDRESS;
    buf[1]  = oldaddr >> 8;
    buf[2]  = oldaddr & 0xFF;
    buf[3]  = newaddr >> 8;
    buf[4]  = newaddr & 0xFF;
    send_cmd (buf, 5, true);
}

/*------------------------------------------------------------------------------------------------------------------------
 * pom_read_cv () - read value of CV
 *------------------------------------------------------------------------------------------------------------------------
 */
bool
DCC::pom_read_cv (uint_fast8_t * valuep, uint_fast16_t addr, uint16_t cv)
{
    uint8_t         buf[5];

    buf[0] = CMD_POM_READ_CV;
    buf[1] = addr >> 8;
    buf[2] = addr & 0xFF;
    buf[3] = cv >> 8;
    buf[4] = cv & 0xFF;

    send_cmd (buf, 5, true);

    pom_cv.valid = 0;

    unsigned long next_millis = Millis::elapsed() + WAIT_FOR_POM_CV_MSEC;

    while (Millis::elapsed() < next_millis)
    {
        MSG::read_msg ();

        if (pom_cv.valid)
        {
            if (pom_cv.addr == addr && pom_cv.cv == cv)
            {
                *valuep = pom_cv.cv_value;
                return true;
            }
        }
        usleep (1000);  // sleep one millisecond        
    }

    return false;
}

/*------------------------------------------------------------------------------------------------------------------------
 * pom_write_cv () - write value of CV
 *------------------------------------------------------------------------------------------------------------------------
 */
void
DCC::pom_write_cv (uint_fast16_t addr, uint16_t cv, uint_fast8_t value)
{
    uint8_t         buf[6];

    buf[0] = CMD_POM_WRITE_CV;
    buf[1] = addr >> 8;
    buf[2] = addr & 0xFF;
    buf[3] = cv >> 8;
    buf[4] = cv & 0xFF;
    buf[5] = value;

    send_cmd (buf, 6, true);
}

/*------------------------------------------------------------------------------------------------------------------------
 * pom_write_cv_bit () - write bit of CV
 *------------------------------------------------------------------------------------------------------------------------
 */
void
DCC::pom_write_cv_bit (uint_fast16_t addr, uint16_t cv, uint_fast8_t bitpos, uint_fast8_t value)
{
    uint8_t         buf[7];

    buf[0] = CMD_POM_WRITE_CV_BIT;
    buf[1] = addr >> 8;
    buf[2] = addr & 0xFF;
    buf[3] = cv >> 8;
    buf[4] = cv & 0xFF;
    buf[5] = bitpos;
    buf[6] = value;

    send_cmd (buf, 7, true);
}

/*------------------------------------------------------------------------------------------------------------------------
 * get_ack () - get ACK from decoder
 *------------------------------------------------------------------------------------------------------------------------
 */
void
DCC::get_ack (uint_fast16_t addr)
{
    uint8_t         buf[3];

    buf[0] = CMD_GET_ACK;
    buf[1]  = addr >> 8;
    buf[2]  = addr & 0xFF;
    send_cmd (buf, 3, true);
}

/*------------------------------------------------------------------------------------------------------------------------
 * base_switch_set () - activate rail switch (base accessory decoder)
 *------------------------------------------------------------------------------------------------------------------------
 */
void
DCC::base_switch_set (uint_fast16_t addr, uint_fast8_t nswitch)
{
    uint8_t         buf[4];

    buf[0] = CMD_BASE_SWITCH_SET;
    buf[1] = addr >> 8;
    buf[2] = addr & 0xFF;
    buf[3] = nswitch;

    send_cmd (buf, 4, true);
}

/*------------------------------------------------------------------------------------------------------------------------
 * base_switch_reset () - deactivate rail switch (base accessory decoder)
 *------------------------------------------------------------------------------------------------------------------------
 */
void
DCC::base_switch_reset (uint_fast16_t addr, uint_fast8_t nswitch)
{
    uint8_t         buf[4];

    buf[0] = CMD_BASE_SWITCH_RESET;
    buf[1] = addr >> 8;
    buf[2] = addr & 0xFF;
    buf[3] = nswitch;

    send_cmd (buf, 4, true);
}

/*------------------------------------------------------------------------------------------------------------------------
 * set_s88_n_contacts () - set number of S88 contacts
 *------------------------------------------------------------------------------------------------------------------------
 */
void
DCC::set_s88_n_contacts (uint_fast16_t n_s88_contacts)
{
    uint8_t         buf[3];

    buf[0] = CMD_S88_SET_N_CONTACTS;
    buf[1] = n_s88_contacts >> 8;
    buf[2] = n_s88_contacts & 0xFF;

    send_cmd (buf, 3, false);
}

/*------------------------------------------------------------------------------------------------------------------------
 * init () - init
 *------------------------------------------------------------------------------------------------------------------------
 */
void
DCC::init (void)
{
}
