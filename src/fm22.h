/*------------------------------------------------------------------------------------------------------------------------
 * fm22.h - fm22 management functions
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
#ifndef FM22_H
#define FM22_H

#include <stdint.h>
#include <string>

#define FM22_SHORTCUT_DEFAULT           1000

class FM22
{
    public:
        static uint_fast16_t            shortcut_value;
        static bool                     data_changed;
        static void                     set_shortcut_value (uint_fast16_t value);
        static uint_fast16_t            get_shortcut_value ();

    private:
};

#endif
