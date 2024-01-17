/*------------------------------------------------------------------------------------------------------------------------
 * dcc.h - DCC encoder
 *------------------------------------------------------------------------------------------------------------------------
 * Copyright (c) 2022-2024 Frank Meyer - frank(at)uclock.de
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

#define DCC_SWITCH_STATE_BRANCH     0       // switch: branch
#define DCC_SWITCH_STATE_STRAIGHT   1       // switch: straight
#define DCC_SWITCH_STATE_BRANCH2    2       // 3-way switch: branch2
#define DCC_SWITCH_STATE_MASK       0x03

/*------------------------------------------------------------------------------------------------------------------------
 * Variables:
 *------------------------------------------------------------------------------------------------------------------------
 */
extern volatile uint32_t            millis;
extern volatile uint_fast8_t        debuglevel;

extern uint_fast16_t                dcc_shortcut_value;

/*------------------------------------------------------------------------------------------------------------------------
 * Commands:
 *------------------------------------------------------------------------------------------------------------------------
 */
extern uint_fast8_t     dcc_get_mode (void);
extern void             dcc_set_mode (uint_fast8_t newmode);
extern uint_fast8_t     dcc_pgm_read_cv (uint_fast8_t * cv_valuep, uint_fast16_t cv);
extern uint_fast8_t     dcc_pgm_write_cv (uint_fast16_t cv, uint_fast8_t cv_value);
extern uint_fast8_t     dcc_pgm_write_cv_bit (uint_fast16_t cv, uint_fast8_t bitpos, uint_fast8_t bitvalue);
extern uint_fast8_t     dcc_pgm_write_address (uint_fast16_t addr);
extern void             dcc_loco_28 (uint_fast16_t loco_idx, uint_fast16_t addr, uint_fast8_t direction, uint_fast8_t speed);
extern void             dcc_loco_function (uint_fast16_t loco_idx, uint_fast16_t addr, uint32_t fmask, uint_fast8_t range);
extern void             dcc_loco (uint_fast16_t loco_idx, uint_fast16_t addr, uint_fast8_t direction, uint_fast8_t speed, uint32_t fmask, uint_fast8_t n);
extern void             dcc_idle (void);
extern void             dcc_reset (void);
extern void             dcc_stop (void);
extern void             dcc_estop (void);
extern void             dcc_reset_decoder (uint_fast16_t addr);
extern void             dcc_hard_reset_decoder (uint_fast16_t addr);
extern void             dcc_pom_write_address (uint_fast16_t oldaddr, uint_fast16_t newaddr);
extern uint_fast8_t     dcc_pom_read_cv (uint_fast8_t * valuep, uint_fast16_t addr, uint16_t cv);
extern void             dcc_pom_write_cv (uint_fast16_t addr, uint16_t cv, uint_fast8_t value);
extern void             dcc_pom_write_cv_bit (uint_fast16_t addr, uint16_t cv, uint_fast8_t bitpos, uint_fast8_t value);
extern uint_fast8_t     dcc_xpom_read_cv (uint8_t * valuep, uint_fast8_t n, uint_fast16_t addr, uint_fast8_t cv31, uint_fast8_t cv32, uint_fast8_t cv);
extern void             dcc_xpom_write_cv (uint_fast16_t addr, uint_fast8_t cv31, uint_fast8_t cv32, uint_fast8_t cv, uint8_t * ptr, uint_fast8_t len);
extern void             dcc_track_search (void);
extern void             dcc_get_ack (uint_fast16_t addr);
extern void             dcc_reset_last_active_switch (void);
extern void             dcc_base_switch_set (uint_fast16_t addr, uint_fast8_t nswitch);
extern void             dcc_base_switch_reset (uint_fast16_t addr, uint_fast8_t nswitch);
extern void             dcc_ext_accessory_set (uint_fast16_t addr, uint_fast8_t value);
extern void             dcc_booster_on (void);
extern void             dcc_booster_off (void);
extern void             dcc_set_shortcut_value (uint_fast16_t value);
extern void             dcc_init (void);
