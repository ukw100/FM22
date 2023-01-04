/*------------------------------------------------------------------------------------------------------------------------
 * pom.cc - POM CV routines
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
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "dcc.h"
#include "pom.h"

#define POM_READ_MAX_TRIES                          10

/*------------------------------------------------------------------------------------------------------------------------
 * pom_read_cv () - read value of CV
 *------------------------------------------------------------------------------------------------------------------------
 */
bool
POM::pom_read_cv (uint_fast8_t * valuep, uint_fast16_t addr, uint16_t cv)
{
    uint_fast8_t    tries;
    bool            rtc = true;

    for (tries = 0; tries < POM_READ_MAX_TRIES; tries++)
    {
        // printf ("pom_read_cv: addr=%u cv=%u\n", addr, cv);

        if (DCC::pom_read_cv (valuep, addr, cv))
        {
            break;
        }
    }

    if (tries == POM_READ_MAX_TRIES)
    {
        rtc = false;
    }

    return rtc;
}

/*------------------------------------------------------------------------------------------------------------------------
 * pom_write_cv () - write value of CV
 *------------------------------------------------------------------------------------------------------------------------
 */
bool
POM::pom_write_cv (uint_fast16_t addr, uint16_t cv, uint_fast8_t value, uint_fast8_t compare)
{
    uint_fast8_t    val;
    bool            rtc = false;

    if (compare & POM_WRITE_COMPARE_BEFORE_WRITE)
    {
        if (POM::pom_read_cv (&val, addr, cv))
        {
            if (val == value)
            {
                rtc = true;
            }
        }
    }

    if (! rtc)
    {
        DCC::pom_write_cv (addr, cv, value);

        if (compare & POM_WRITE_COMPARE_AFTER_WRITE)
        {
            if (POM::pom_read_cv (&val, addr, cv))
            {
                if (val == value)
                {
                    rtc = true;
                }
            }
        }
        else
        {
            rtc = true;
        }
    }
    
    return rtc;
}

/*------------------------------------------------------------------------------------------------------------------------
 * pom_write_cv_index () - write CV index (CV31, CV32)
 *------------------------------------------------------------------------------------------------------------------------
 */
bool
POM::pom_write_cv_index (uint_fast16_t addr, uint_fast8_t cv31, uint_fast8_t cv32)
{
    bool rtc;

    rtc = pom_write_cv (addr, 31, cv31, POM_WRITE_COMPARE_BEFORE_AND_AFTER_WRITE);

    if (rtc)
    {
        rtc = pom_write_cv (addr, 32, cv32, POM_WRITE_COMPARE_BEFORE_AND_AFTER_WRITE);
    }
    
    return rtc;
}
