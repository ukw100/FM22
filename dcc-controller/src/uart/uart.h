/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * uart.h - declaration of UART driver routines for STM32F4XX or STM32F10X
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 * Copyright (c) 2014-2024 Frank Meyer - frank(at)uclock.de
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
#if ! defined(STM32F10X)
#if defined(STM32L1XX_MD) || defined(STM32L1XX_MDP) || defined(STM32L1XX_HD)
#define STM32L1XX
#elif defined(STM32F10X_LD) || defined(STM32F10X_LD_VL) \
   || defined(STM32F10X_MD) || defined(STM32F10X_MD_VL) \
   || defined(STM32F10X_HD) || defined(STM32F10X_HD_VL) \
   || defined(STM32F10X_XL) || defined(STM32F10X_CL)
#define STM32F10X
#endif
#endif

#if defined (STM32F10X)
#  include "stm32f10x.h"
#  include "stm32f10x_gpio.h"
#  include "stm32f10x_usart.h"
#  include "stm32f10x_rcc.h"
#  include "misc.h"
#elif defined (STM32F30X)
#  include "stm32f30x.h"
#  include "stm32f30x_gpio.h"
#  include "stm32f30x_usart.h"
#  include "stm32f30x_rcc.h"
#  include "stm32f30x_tim.h"
#  include "stm32f30x_misc.h"
#elif defined (STM32F4XX)
#  include "stm32f4xx.h"
#  include "stm32f4xx_gpio.h"
#  include "stm32f4xx_usart.h"
#  include "stm32f4xx_rcc.h"
#  include "misc.h"
#else
#error unknown STM32
#endif

#include <stdarg.h>

#define _UART_CONCAT(a,b)                a##b
#define UART_CONCAT(a,b)                 _UART_CONCAT(a,b)

extern void             UART_CONCAT(UART_PREFIX, _init)            (uint32_t, uint16_t, uint16_t, uint16_t);
extern void             UART_CONCAT(UART_PREFIX, _putc)            (uint_fast8_t);
extern void             UART_CONCAT(UART_PREFIX, _puts)            (const char *);
extern int              UART_CONCAT(UART_PREFIX, _vprintf)         (const char *, va_list);
extern int              UART_CONCAT(UART_PREFIX, _printf)          (const char *, ...);
extern uint_fast8_t     UART_CONCAT(UART_PREFIX, _getc)            (void);
extern uint_fast8_t     UART_CONCAT(UART_PREFIX, _poll)            (uint_fast8_t *);
extern void             UART_CONCAT(UART_PREFIX, _input_flush)     (void);
extern uint_fast8_t     UART_CONCAT(UART_PREFIX, _interrupted)     (void);
extern void             UART_CONCAT(UART_PREFIX, _rawmode)         (uint_fast8_t);
extern uint_fast16_t    UART_CONCAT(UART_PREFIX, _rxsize)          (void);
extern void             UART_CONCAT(UART_PREFIX, _flush)           (void);
extern uint_fast16_t    UART_CONCAT(UART_PREFIX, _read)            (char *, uint_fast16_t);
extern uint_fast16_t    UART_CONCAT(UART_PREFIX, _write)           (char *, uint_fast16_t);

