/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * rc-detector-uart.c - definitions of uart driver routines
 *---------------------------------------------------------------------------------------------------------------------------------------------------
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
 *
 * Possible UARTs of STM32F10x:
 *           ALTERNATE=0    ALTERNATE=1    ALTERNATE=2
 *  +--------------------------------------------------+
 *  | UART | TX   | RX   || TX   | RX   || TX   | RX   |
 *  |======|======|======||======|======||======|======|
 *  | 1    | PA9  | PA10 || PB6  | PB7  ||      |      |
 *  | 2    | PA2  | PA3  || PD5  | PD6  ||      |      |
 *  | 3    | PB10 | PB11 || PC10 | PC11 || PD8  | PD9  |
 *  +--------------------------------------------------+
 *
 * Possible UARTs of STM32F401 / STM32F411:
 *           ALTERNATE=0    ALTERNATE=1    ALTERNATE=2
 *  +--------------------------------------------------+
 *  | UART | TX   | RX   || TX   | RX   || TX   | RX   |
 *  |======|======|======||======|======||======|======|
 *  | 1    | PA9  | PA10 || PB6  | PB7  ||      |      |
 *  | 2    | PA2  | PA3  || PD5  | PD6  ||      |      |
 *  | 6    | PC6  | PC7  || PA11 | PA12 ||      |      |
 *  +--------------------------------------------------+
 *
 * Possible UARTs of STM32F407:
 *           ALTERNATE=0    ALTERNATE=1    ALTERNATE=2
 *  +--------------------------------------------------+
 *  | UART | TX   | RX   || TX   | RX   || TX   | RX   |
 *  |======|======|======||======|======||======|======|
 *  | 1    | PA9  | PA10 || PB6  | PB7  ||      |      |
 *  | 2    | PA2  | PA3  || PD5  | PD6  ||      |      |
 *  | 3    | PB10 | PB11 || PC10 | PC11 || PD8  | PD9  |
 *  | 4    | PA0  | PA1  || PC10 | PC11 ||      |      |
 *  | 5    | PC12 | PD2  ||      |      ||      |      |
 *  | 6    | PC6  | PC7  || PG14 | PG9  ||      |      |
 *  +--------------------------------------------------+
 *
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */

#define UART_PREFIX             rc_detector_uart         // see also uart-driver.h

#if defined STM32F407VE
#define UART_NUMBER             4                       // UART number on STM32F407 (1-6 for UART)
#define UART_ALTERNATE          0                       // ALTERNATE number
#elif defined STM32F401CC
#define UART_NUMBER             6                       // UART number on STM32F401 (1-6 for UART)
#define UART_ALTERNATE          1                       // ALTERNATE number
#else
#error unknown STM32
#endif

#define UART_TXBUFLEN           16                      // ringbuffer size for UART TX, yet not needed
#define UART_RXBUFLEN           64                      // ringbuffer size for UART RX

#include "uart-driver.h"
