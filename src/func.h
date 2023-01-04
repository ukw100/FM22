/*------------------------------------------------------------------------------------------------------------------------
 * func.h - handle loco functions
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
#ifndef FUNC_H
#define FUNC_H

#include <string>

#define MAX_FUNCTION_NAMES      256

class Functions
{
    public:
        static uint_fast16_t    add (const char * name);
        static uint_fast16_t    add (std::string& name);
        static void             set (uint_fast16_t fidx, std::string& name);
        static std::string&     get (uint_fast16_t fidx);
        static uint_fast16_t    get_n_entries (void);
        static uint_fast16_t    search (const char * name);
        static uint_fast16_t    search (std::string& name);
    private:
        static std::string      names[MAX_FUNCTION_NAMES];
        static uint_fast16_t    entries;
};

#endif
