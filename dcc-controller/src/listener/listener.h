/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * listener.h - listener routines
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 * Copyright (c) 2016-2022 Frank Meyer - frank(at)uclock.de
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
#define UART_PREFIX             listener
#include "uart.h"

extern void                     listener_set_stop (void);
extern void                     listener_set_continue (void);
extern void                     listener_send_continue (void);

extern void                     listener_send_msg_adc (uint_fast16_t adc);
extern void                     listener_send_msg_rc1 (void);
extern void                     listener_send_msg_rc2 (void);
extern void                     listener_send_msg_rcl (void);
extern void                     listener_send_msg_cv (uint_fast16_t addr, uint_fast16_t cv, uint_fast8_t cv_value);
extern void                     listener_send_msg_xpom_cv (uint_fast16_t addr, uint_fast8_t cv31, uint_fast8_t cv32, uint_fast8_t cv_range, uint8_t * cv_values, uint_fast8_t n);
extern void                     listener_send_msg_s88 (void);
extern void                     listener_send_msg_rc2_rate (uint_fast16_t loco_idx, uint_fast8_t rc2_rate);
extern void                     listener_send_debug_msg (char * msg);
extern void                     listener_send_debug_msgf (const char * fmt, ...);
extern void                     listener_queue_debug_msg (char * msg);
extern void                     listener_queue_debug_msgf (const char * fmt, ...);
extern void                     listener_flush_debug_msg (void);

extern uint_fast8_t             listener_read_cmd (void);
extern uint_fast8_t             listener_read_rcl (void);
extern void                     listener_msg_init (void);

