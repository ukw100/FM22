/*------------------------------------------------------------------------------------------------------------------------
 * func.cc - loco functions
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
#include <string>

#include "func.h"

std::string     Functions::names[MAX_FUNCTION_NAMES];
uint_fast16_t   Functions::entries;

/*------------------------------------------------------------------------------------------------------------------------
 *  add () - add entry
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast16_t
Functions::add (std::string& name)
{
    uint_fast16_t rtc = 0xFFFF;

    if (Functions::entries + 1 < MAX_FUNCTION_NAMES)
    {
        rtc = Functions::entries;
        Functions::set (rtc, name);
        entries++;
    }

    return rtc;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  add () - add entry
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast16_t
Functions::add (const char * name)
{
    std::string sname = name;
    return add (sname);
}

/*------------------------------------------------------------------------------------------------------------------------
 *  set () - set name
 *------------------------------------------------------------------------------------------------------------------------
 */
void
Functions::set (uint_fast16_t fidx, std::string& name)
{
    if (fidx < MAX_FUNCTION_NAMES)
    {
        names[fidx] = name;
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 *  get () - get name
 *------------------------------------------------------------------------------------------------------------------------
 */
std::string&
Functions::get (uint_fast16_t fidx)
{
    static std::string rtc = "";

    if (fidx < MAX_FUNCTION_NAMES)
    {
        return names[fidx];
    }
    return rtc;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  get_n_entries () - get number of function name entries
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast16_t
Functions::get_n_entries (void)
{
    return entries;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  search () - search function name
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast16_t
Functions::search (const char * name)
{
    std::string sname = name;
    return Functions::search (sname);
}

/*------------------------------------------------------------------------------------------------------------------------
 *  search () - search function name
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast16_t
Functions::search (std::string& name)
{
    uint_fast16_t    idx;

    for (idx = 0; idx < entries; idx++)
    {
        if (! name.compare (names[idx]))
        {
            return idx;
        }
    }

    return 0xFFFF;
}
