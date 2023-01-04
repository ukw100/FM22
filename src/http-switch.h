/*------------------------------------------------------------------------------------------------------------------------
 * http-switch.h - HTTP switch routines
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
#ifndef HTTP_SWITCH_H
#define HTTP_SWITCH_H

#include <stdint.h>

class HTTP_Switch
{
    public:
        static void     handle_switch (void);
        static void     handle_switch_test (void);
        static void     action_setsw (void);
        static void     action_switch (void);
    private:
};

#endif
