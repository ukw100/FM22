/*-------------------------------------------------------------------------------------------------------------------------------------------
 * rc-local.c - Railcom(TM) receiver
 *-------------------------------------------------------------------------------------------------------------------------------------------
 * Copyright (c) 2022 Frank Meyer - frank(at)uclock.de
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
#include <string.h>
#include "rs485.h"
#include "rc-local.h"


/*-------------------------------------------------------------------------------------------------------------------------------------------
 * rc_local_init() - init RS485
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
rc_local_init (void)
{
    rs485_init (115200, USART_WordLength_8b, USART_Parity_No, USART_StopBits_1);
}


