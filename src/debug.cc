/*------------------------------------------------------------------------------------------------------------------------
 * debug.cc - debug functions
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
#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <time.h>
#include "debug.h"

uint_fast8_t    Debug::debug_level  = 0;

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * set_level () - set debug level
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
Debug::set_level (uint_fast8_t level)
{
    Debug::debug_level = level;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * puts () - print a message
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
Debug::puts (uint_fast8_t level, const char * s)
{
    if (debug_level >= level)
    {
        time_t  now = time ((time_t *) NULL);
        struct  tm * tmp = localtime (&now);

        ::printf ("%04d-%02d-%02d %02d:%02d:%02d: ", tmp->tm_year + 1900, tmp->tm_mon + 1, tmp->tm_mday, tmp->tm_hour, tmp->tm_min, tmp->tm_sec);
        ::fputs (s, stdout);
    }
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * printf () - print a formatted message (varargs)
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
int
Debug::printf (uint_fast8_t level, const char * fmt, ...)
{
    int len = 0;

    if (debug_level >= level)
    {
        time_t  now = time ((time_t *) NULL);
        struct  tm * tmp = localtime (&now);
        va_list ap;

        ::printf ("%04d-%02d-%02d %02d:%02d:%02d: ", tmp->tm_year + 1900, tmp->tm_mon + 1, tmp->tm_mday, tmp->tm_hour, tmp->tm_min, tmp->tm_sec);

        va_start (ap, fmt);
        len = vprintf (fmt, ap);
        va_end (ap);
    }
    return len;
}
