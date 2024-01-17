/*------------------------------------------------------------------------------------------------------------------------
 * pom.cc - POM CV routines
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
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "dcc.h"
#include "pom.h"

#define POM_READ_MAX_TRIES      10
#define XPOM_READ_MAX_TRIES     20

uint32_t                        POM::sum_retries;
uint32_t                        POM::sum_reads;

/*------------------------------------------------------------------------------------------------------------------------
 * pom_read_cv () - read value of CV
 *------------------------------------------------------------------------------------------------------------------------
 */
bool
POM::pom_read_cv (uint_fast8_t * valuep, uint_fast16_t addr, uint16_t cv)
{
    uint_fast8_t    tries;
    bool            rtc = false;

    for (tries = 0; tries < POM_READ_MAX_TRIES; tries++)
    {
        rtc = DCC::pom_read_cv (valuep, addr, cv);
        // printf ("pom_read_cv: addr=%d cv=%d rtc=%d\n", addr, cv, rtc);

        if (rtc)
        {
            break;
        }
    }

    sum_retries += tries;
    sum_reads++;

    return rtc;
}

/*------------------------------------------------------------------------------------------------------------------------
 * pom_read_cv () - read values of n * 4 CVs
 *
 * cv begins with 0 and must be a multiple of 4.
 * n is 1 .. 4 for reading 4, 8, 12, or 16 CVs
 *------------------------------------------------------------------------------------------------------------------------
 */
bool
POM::xpom_read_cv (uint8_t * valuep, uint_fast8_t n, uint_fast16_t addr, uint_fast8_t cv31, uint_fast8_t cv32, uint_fast8_t cv)
{
    uint_fast8_t    tries;
    bool            rtc = false;

    for (tries = 0; tries < XPOM_READ_MAX_TRIES; tries++)
    {
        if (DCC::xpom_read_cv (valuep, n, addr, cv31, cv32, cv))
        {
            rtc = true;
            break;
        }
    }
    sum_retries += tries;
    sum_reads += 4 * n;

    return rtc;
}

/*------------------------------------------------------------------------------------------------------------------------
 * pom_get_read_tries () - get number of read tries
 *------------------------------------------------------------------------------------------------------------------------
 */
uint32_t
POM::pom_get_read_retries (void)
{
    return (sum_retries);
}

/*------------------------------------------------------------------------------------------------------------------------
 * pom_get_read_tries () - get number of read tries
 *------------------------------------------------------------------------------------------------------------------------
 */
uint32_t
POM::pom_get_num_reads (void)
{
    return (sum_reads);
}

/*------------------------------------------------------------------------------------------------------------------------
 * pom_reset_read_tries () - reset number of read tries
 *------------------------------------------------------------------------------------------------------------------------
 */
void
POM::pom_reset_num_reads (void)
{
    sum_retries = 0;
    sum_reads = 0;
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
                // printf ("pom_write_cv: nothing to do: addr=%d cv=%d value=%d val=%d\n", addr, cv, value, val);
            }
        }
    }

    if (! rtc)
    {
        // printf ("pom_write_cv: addr=%d cv=%d value=%d\n", addr, cv, value);
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
            // printf ("pom_read_cv: val=%d\n", val);
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
