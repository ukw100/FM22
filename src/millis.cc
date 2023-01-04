/*------------------------------------------------------------------------------------------------------------------------
 * millis.cc - time measurement in milliseconds
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
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include "millis.h"

static unsigned long    start;

void
Millis::init (void)
{
    struct timeval  timestart;

    if (gettimeofday (&timestart, NULL) < 0)
    {
        perror ("gettimeofday timestart");
        exit (1);
    }
    start = timestart.tv_sec;
}

unsigned long
Millis::elapsed (void)
{
    struct timeval end;
    unsigned long mtime;

    if (gettimeofday (&end, NULL) < 0)
    {
        perror ("gettimeofday timestart");
        exit (1);
    }

    mtime = ((end.tv_sec - start) * 1000 + end.tv_usec/1000.0) + 0.5;

    return mtime;
}

