/*------------------------------------------------------------------------------------------------------------------------
 * fm22.cc - fm22mination management functions
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cstdint>
#include <cstring>
#include "dcc.h"
#include "millis.h"
#include "debug.h"
#include "fm22.h"

uint_fast16_t                   FM22::shortcut_value = FM22_SHORTCUT_DEFAULT;               // shortcut value, public
bool                            FM22::data_changed = false;                                 // flag: data changed, public

/*------------------------------------------------------------------------------------------------------------------------
 * set_shortcut_value() - set shortcut value
 *------------------------------------------------------------------------------------------------------------------------
 */
void
FM22::set_shortcut_value (uint_fast16_t value)
{
    FM22::shortcut_value    = value;
    FM22::data_changed      = true;
}

/*------------------------------------------------------------------------------------------------------------------------
 * get_shortcut_value() - get shortcut value
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast16_t
FM22::get_shortcut_value (void)
{
    return FM22::shortcut_value;
}
