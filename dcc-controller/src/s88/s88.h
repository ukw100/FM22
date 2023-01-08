/*------------------------------------------------------------------------------------------------------------------------
 * s88.h   - s88 management functions
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
 *------------------------------------------------------------------------------------------------------------------------
 */
#include <stdint.h>

extern void                     s88_set_n_contacts (uint_fast16_t n);
extern uint_fast16_t            s88_get_n_contacts (void);
extern uint_fast8_t             s88_get_n_bytes (void);
extern uint_fast16_t            s88_get_status_byte (uint_fast8_t idx);
extern void                     s88_booster_on (void);
extern void                     s88_booster_off (void);
extern void                     s88_read (void);
extern void                     s88_init (void);
