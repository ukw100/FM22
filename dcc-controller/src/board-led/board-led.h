/*-------------------------------------------------------------------------------------------------------------------------------------------
 * board-led.h - declarations of LED routines
 *-------------------------------------------------------------------------------------------------------------------------------------------
 * Copyright (c) 2014-2022 Frank Meyer - frank(at)uclock.de
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
#ifndef BOARD_LED_H
#define BOARD_LED_H

#include "stm32f4xx.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_rcc.h"

extern void         board_led_init (void);

#if defined STM32F407VE
extern void         board_led2_on (void);
extern void         board_led2_off (void);
extern void         board_led3_on (void);
extern void         board_led3_off (void);
#elif defined STM32F401CC
extern void         board_led_on (void);
extern void         board_led_off (void);
#else
#error unknown STM32
#endif

#endif
