/*------------------------------------------------------------------------------------------------------------------------
 * base.cc - base functions
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
#include <stdint.h>
#include <string.h>

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * asciitolower (int ch) - convert A-Z to lowercase
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
int
asciitolower (int ch)
{
    if (ch <= 'Z' && ch >= 'A')
    {
        return ch - ('Z' - 'z');
    }
    return ch;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * trim () - trim string
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
trim (char * bufp)
{
    char *  p;
    int     l;
    int     ch;

    l = strlen (bufp);
    p = bufp + l - 1;

    while (l > 0)
    {
        ch = *p;

        if (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n')
        {
            *p = '\0';
            p--;
            l--;
        }
        else
        {
            break;
        }
    }
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * htoi () - hex to integer
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
uint16_t
htoi (char * buf, uint8_t max_digits)
{
    uint8_t     i;
    uint8_t     x;
    uint16_t    sum = 0;

    for (i = 0; i < max_digits && *buf; i++)
    {
        x = buf[i];

        if (x >= '0' && x <= '9')
        {
            x -= '0';
        }
        else if (x >= 'A' && x <= 'F')
        {
            x -= 'A' - 10;
        }
        else if (x >= 'a' && x <= 'f')
        {
            x -= 'a' - 10;
        }
        else
        {
            x = 0;
        }
        sum <<= 4;
        sum += x;
    }

    return (sum);
}
