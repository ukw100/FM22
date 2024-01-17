/*------------------------------------------------------------------------------------------------------------------------
 * gpio.cc - gpio functions
 *------------------------------------------------------------------------------------------------------------------------
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
 *------------------------------------------------------------------------------------------------------------------------
 */
#include <stdio.h>
#include <stdint.h>
#include "gpio.h"
#include "debug.h"

#ifdef __ARMEL__                                                    // Raspberry PI

#include <bcm2835.h>

#define PIN_NRST        RPI_V2_GPIO_P1_11                           // GPIO17
#define PIN_BOOT0       RPI_V2_GPIO_P1_13                           // GPIO27

void
GPIO::deactivate_stm32_nrst (void)
{
    bcm2835_gpio_write (PIN_NRST, HIGH);                            // deactivate NRST
    bcm2835_gpio_fsel (PIN_NRST, BCM2835_GPIO_FSEL_INPT);           // set NRST to input, has pullup
}


void
GPIO::activate_stm32_nrst (void)
{
    bcm2835_gpio_fsel (PIN_NRST, BCM2835_GPIO_FSEL_OUTP);           // set NRST to output
    bcm2835_gpio_write (PIN_NRST, LOW);                             // activate NRST
}


void
GPIO::deactivate_stm32_boot0 (void)
{
    bcm2835_gpio_write (PIN_BOOT0, LOW);                            // deactivate BOOT0
    bcm2835_gpio_fsel (PIN_BOOT0, BCM2835_GPIO_FSEL_INPT);          // set BOOT0 to input, has pulldown
}

void
GPIO::activate_stm32_boot0 (void)
{
    bcm2835_gpio_fsel (PIN_BOOT0, BCM2835_GPIO_FSEL_OUTP);          // set BOOT0 to output
    bcm2835_gpio_write (PIN_BOOT0, HIGH);                           // activate BOOT0
}


bool
GPIO::init (void)
{
    bool rtc = bcm2835_init ();

    if (rtc)
    {
        bcm2835_gpio_fsel (PIN_NRST, BCM2835_GPIO_FSEL_INPT);           // set NRST pin to input, has pullup
        bcm2835_gpio_fsel (PIN_BOOT0, BCM2835_GPIO_FSEL_INPT);          // set BOOT0 pin to input, has pulldown
    }
    else
    {
        Debug::printf (DEBUG_LEVEL_NONE, "bcm2835_init() failed\n");
    }

    return rtc;
}

#else

void
GPIO::deactivate_stm32_nrst (void)
{
}


void
GPIO::activate_stm32_nrst (void)
{
}


void
GPIO::deactivate_stm32_boot0 (void)
{
}

void
GPIO::activate_stm32_boot0 (void)
{
}


bool
GPIO::init (void)
{
    return false;
}

#endif
