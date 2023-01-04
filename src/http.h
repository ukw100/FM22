/*------------------------------------------------------------------------------------------------------------------------
 * http.h - http server
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
#ifndef HTTP_H
#define HTTP_H

#include <string>

typedef std::string                         String;

class HTTP
{
    public:
        static String           response;

        static const char *     parameter (const char * name);
        static const char *     parameter (String sname);
        static int              parameter_number (const char * name);
        static int              parameter_number (String sname);
        static void             set_alert (const char * msg);
        static void             init (void);
        static void             deinit (void);
        static void             send (const char * str);
        static void             flush (void);
        static bool             server (bool);
};

#endif
