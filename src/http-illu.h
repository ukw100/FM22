/*------------------------------------------------------------------------------------------------------------------------
 * http-illu.h - HTTP illumination routines
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
#ifndef HTTP_ILLU_H
#define HTTP_ILLU_H

#include <stdint.h>

class HTTP_Illu
{
    public:
        static void     handle_illu (void);
        static void     handle_illu_macro_edit (void);
        static void     action_illus (void);
        static void     action_illu (void);
        static void     action_getf (void);
        static void     action_macro (void);
        static void     action_setspeed (void);
        static void     action_togglefunction (void);
    private:
};

#endif
