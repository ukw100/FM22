/*------------------------------------------------------------------------------------------------------------------------
 * gpio.h - gpio functions
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
#ifndef GPIO_H
#define GPIO_H

class GPIO
{
    public:
        static void             deactivate_stm32_nrst (void);
        static void             activate_stm32_nrst (void);
        static void             deactivate_stm32_boot0 (void);
        static void             activate_stm32_boot0 (void);
        static bool             init (void);
};

#endif
