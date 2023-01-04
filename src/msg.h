/*------------------------------------------------------------------------------------------------------------------------
 * msg.h - message functions
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
#ifndef MSG_H
#define MSG_H

class MSG
{
    public:
        static void         flush_msg (void);
        static void         read_msg (void);
    private:
        static void         adc (uint8_t * bufp, uint_fast8_t len);
        static void         rc1 (uint8_t * bufp, uint_fast8_t len);
        static void         rc2 (uint8_t * bufp, uint_fast8_t len);
        static void         pom_cv (uint8_t * bufp, uint_fast8_t len);
        static void         pgm_cv (uint8_t * bufp, uint_fast8_t len);
        static void         loco_rc2_rate (uint8_t * bufp, uint_fast8_t len);
        static void         s88 (uint8_t * bufp, uint_fast8_t len);
        static void         rcl (uint8_t * bufp, uint_fast8_t len);
        static void         debug_message (uint8_t * bufp, uint_fast8_t len);
        static void         msg (uint8_t * buf, uint_fast8_t len);
};

#endif
