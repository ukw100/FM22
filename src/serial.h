/*------------------------------------------------------------------------------------------------------------------------
 * serial.h - serial functions
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
#ifndef SERIAL_H
#define SERIAL_H

class Serial
{
    public:
        static uint_fast8_t     poll (uint_fast8_t * chp);
        static int              send (uint_fast8_t ch);
        static void             send (uint8_t * bufp, uint32_t len);
        static uint_fast8_t     init (void);
};

#endif
