/*------------------------------------------------------------------------------------------------------------------------
 * booster.h - handle booster
 *------------------------------------------------------------------------------------------------------------------------
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

extern volatile uint_fast8_t        booster_is_on;

extern void                         booster_adc_start_single_conversion (void);
extern uint_fast8_t                 booster_adc_poll_conversion_value (uint16_t * value_p);

extern void                         booster_negative (void);
extern void                         booster_positive (void);
extern void                         booster_on (void);
extern void                         booster_off (void);
extern void                         booster_cutout (void);
extern void                         booster_init (void);
