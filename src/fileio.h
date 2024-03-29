/*-------------------------------------------------------------------------------------------------------------------------------------------
 * fileio.h - file I/O
 *-------------------------------------------------------------------------------------------------------------------------------------------
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
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
#ifndef FILEIO_H
#define FILEIO_H

class FileIO
{
    public:
        static void     read_all_ini_files (void);
        static bool     write_all_ini_files (void);
    private:
        static bool     read_fm22_ini (void);
        static bool     read_func_ini (void);
        static bool     read_loco_ini (void);
        static bool     read_switch_ini (void);
        static bool     read_signal_ini (void);
        static bool     read_led_ini (void);
        static bool     read_s88_ini (void);
        static bool     read_rcl_ini (void);

        static bool     write_fm22_ini (void);
        static bool     write_loco_ini (void);
        static bool     write_switch_ini (void);
        static bool     write_signal_ini (void);
        static bool     write_led_ini (void);
        static bool     write_s88_ini (void);
        static bool     write_rcl_ini (void);
};

#endif
