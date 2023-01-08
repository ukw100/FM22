/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * rs485.h - declaration of RS485 routines for STM32F4XX or STM32F10X
 *
 * Copyright (c) 2015-2022 Frank Meyer - frank(at)uclock.de
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
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
#if defined (STM32F10X)
#  include "stm32f10x.h"
#  include "stm32f10x_gpio.h"
#  include "stm32f10x_usart.h"
#  include "stm32f10x_rcc.h"
#elif defined (STM32F4XX)
#  include "stm32f4xx.h"
#  include "stm32f4xx_gpio.h"
#  include "stm32f4xx_usart.h"
#  include "stm32f4xx_rcc.h"
#else
#error unknown STM32
#endif

#include "misc.h"

extern void             rs485_init (uint32_t baudrate, uint16_t wordlength, uint16_t parity, uint16_t stopbits);
extern void             rs485_putc (uint_fast8_t);
extern void             rs485_puts (char *);
extern uint_fast8_t     rs485_getc (void);
extern uint_fast8_t     rs485_poll (uint_fast8_t *);
extern void             rs485_flush ();
extern uint_fast16_t    rs485_read (char *, uint_fast16_t);
extern uint_fast16_t    rs485_write (char *, uint_fast16_t);
