/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * rc-detector.h - Railcom(TM) decoder
 *---------------------------------------------------------------------------------------------------------------------------------------------------
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
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
#include <stdarg.h>

#undef UART_PREFIX
#define UART_PREFIX         rc_detector_uart
#include "uart.h"

#define RC_DETECTOR_MAX_LOCOS       256

#define RC_DETECTOR_ADDRESS_INVALID 0xFFFF
#define RC_DETECTOR_DYN_INVALID     0xFF

#define MAX_XPOM_SEQUENCES          4
#define MAX_XPOM_CV_VALUES          4

extern volatile uint_fast16_t       rc2_addr;
extern volatile uint_fast8_t        rc2_addr_valid;

extern volatile uint_fast8_t        rc2_cv_value;
extern volatile uint_fast8_t        rc2_cv_value_valid;

extern volatile uint8_t             rc2_cv_values[MAX_XPOM_SEQUENCES][MAX_XPOM_CV_VALUES];
extern volatile uint8_t             rc2_cv_values_valid[MAX_XPOM_SEQUENCES];

extern volatile uint_fast16_t       rc_detector_n_rc2infos;
extern uint8_t                      rc_detector_n_locations;

extern uint16_t                     rc_loco_addrs[RC_DETECTOR_MAX_LOCOS];
extern uint_fast16_t                n_rc_loco_addrs;


extern uint32_t                     rc_detector_get_rc2_millis (uint_fast16_t idx);
extern uint32_t                     rc_detector_get_rc2_millis_diff (uint_fast16_t idx);
extern void                         rc_detector_reset_rc2_millis (void);
extern uint_fast8_t                 rc_detector_get_rc2_rate (uint_fast16_t idx);

extern void                         rc_detector_reset_rc2_data (void);
extern void                         rc_detector_reset_rc_data (void);
extern void                         rc_detector_enable (void);
extern void                         rc_detector_disable (void);
extern void                         rc_detector_read_rc1 (void);
extern uint_fast16_t                rc_detector_rc1_addr (void);
extern uint_fast16_t                rc_detector_rc2_dyn (void);
extern void                         rc_detector_read_rc2 (uint_fast16_t);

extern void                         rc_detector_set_loco_location (uint_fast16_t addr, uint_fast8_t location);
extern uint_fast8_t                 rc_detector_get_loco_location (uint_fast16_t loco_idx);

extern void                         rc_detector_reset_all_loco_locations (void);

extern void                         rc_detector_init (void);
