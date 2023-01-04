/*------------------------------------------------------------------------------------------------------------------------
 * http-pgm.h - HTTP PGM routines
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
#ifndef HTTP_PGM_H
#define HTTP_PGM_H

#include <stdint.h>

class HTTP_PGM
{
    public:
        static void     handle_pgminfo (void);
        static void     handle_pgmaddr (void);
        static void     handle_pgmcv (void);
    private:
};

#endif
