/*------------------------------------------------------------------------------------------------------------------------
 * dcc.h - DCC encoder
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
#ifndef DCC_H
#define DCC_H

#include <stdint.h>

#define RAILCOM_MODE                0
#define NO_RAILCOM_MODE             1
#define PROGRAMMING_MODE            2

#define DCC_DIRECTION_BWD           0
#define DCC_DIRECTION_FWD           1

#define DCC_F00_F04_RANGE           1
#define DCC_F05_F08_RANGE           2
#define DCC_F09_F12_RANGE           3
#define DCC_F13_F20_RANGE           4
#define DCC_F21_F28_RANGE           5
#define DCC_F29_F36_RANGE           6       // yet not supported
#define DCC_F37_F44_RANGE           7       // yet not supported
#define DCC_F45_F52_RANGE           8       // yet not supported
#define DCC_F53_F60_RANGE           9       // yet not supported
#define DCC_F61_F68_RANGE           10      // yet not supported

#define DCC_SWITCH_STATE_BRANCH     0x00    // switch: branch
#define DCC_SWITCH_STATE_STRAIGHT   0x01    // switch: straight
#define DCC_SWITCH_STATE_BRANCH2    0x02    // 3-way switch: branch2
#define DCC_SWITCH_STATE_UNDEFINED  0x03    // switch state undefined
#define DCC_SWITCH_STATE_MASK       0x03    // mask: 2 bits

typedef struct
{
    uint16_t    addr;
    uint16_t    cv;
    uint8_t     cv_value;   
    uint8_t     valid;
} POM_CV;

typedef struct
{
    uint16_t    cv;
    uint8_t     cv_value;   
    uint8_t     valid;
} PGM_CV;

class DCC
{
    public:
        static POM_CV           pom_cv;
        static PGM_CV           pgm_cv;
        static uint_fast8_t     channel_stopped;
        static uint16_t         adc_value;
        static uint16_t         rc1_value;
        static uint_fast8_t     booster_is_on;
        static unsigned long    booster_is_on_time;

        static uint16_t         pgm_limit;
        static uint16_t         pgm_min_lower_value;
        static uint16_t         pgm_max_lower_value;
        static uint16_t         pgm_min_upper_value;
        static uint16_t         pgm_max_upper_value;
        static uint16_t         pgm_min_cnt;
        static uint16_t         pgm_max_cnt;

        static void             booster_off (void);
        static void             booster_on (void);
        static uint_fast8_t     get_mode (void);
        static void             set_mode (uint_fast8_t newmode);
        static uint_fast8_t     pgm_read_cv (uint_fast8_t * cv_valuep, uint_fast16_t cv);
        static uint_fast8_t     pgm_write_cv (uint_fast16_t cv, uint_fast8_t cv_value);
        static uint_fast8_t     pgm_write_cv_bit (uint_fast16_t cv, uint_fast8_t bitpos, uint_fast8_t bitvalue);
        static uint_fast8_t     pgm_write_address (uint_fast16_t addr);
        static void             loco_28 (uint_fast16_t loco_idx, uint_fast16_t addr, uint_fast8_t direction, uint_fast8_t speed);
        static void             loco_function (uint_fast16_t loco_idx, uint_fast16_t addr, uint32_t fmask, uint_fast8_t range);
        static void             loco (uint_fast16_t loco_idx, uint_fast16_t addr, uint_fast8_t direction, uint_fast8_t speed, uint32_t fmask, uint_fast8_t n);
        static void             loco_rc2_rate (uint_fast16_t loco_idx);
        static void             reset (void);
        static void             stop (void);
        static void             estop (void);
        static void             reset_decoder (uint_fast16_t addr);
        static void             hard_reset_decoder (uint_fast16_t addr);
        static void             pom_write_address (uint_fast16_t oldaddr, uint_fast16_t newaddr);
        static bool             pom_read_cv (uint_fast8_t * valuep, uint_fast16_t addr, uint16_t cv);
        static void             pom_write_cv (uint_fast16_t addr, uint16_t cv, uint_fast8_t value);
        static void             pom_write_cv_bit (uint_fast16_t addr, uint16_t cv, uint_fast8_t bitpos, uint_fast8_t value);
        static void             get_ack (uint_fast16_t addr);
        static void             base_switch_set (uint_fast16_t addr, uint_fast8_t nswitch);
        static void             base_switch_reset (uint_fast16_t addr, uint_fast8_t nswitch);
        static void             set_s88_n_contacts (uint_fast16_t n_s88_contacts);
        static void             init (void);

    private:
        static uint_fast8_t     mode;
        static void             send_cmd (uint8_t * buf, uint_fast8_t len, bool set_flag_stopped);
        static bool             pom_send_read_cv (uint_fast8_t * valuep, uint_fast16_t addr, uint16_t cv);
};

#endif
