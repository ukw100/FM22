/*------------------------------------------------------------------------------------------------------------------------
 * debug.h - debug functions
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
#ifndef DEBUG_H
#define DEBUG_H
#include <stdint.h>

#define DEBUG_LEVEL_NONE    0
#define DEBUG_LEVEL_NORMAL  1
#define DEBUG_LEVEL_VERBOSE 2

class Debug
{
    public:
        static void         set_level (uint_fast8_t level);
        static void         puts (uint_fast8_t level, const char * s);
        static int          printf (uint_fast8_t level, const char * fmt, ...);
    private:
        static uint_fast8_t debug_level;
};

#endif
