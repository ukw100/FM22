/*-------------------------------------------------------------------------------------------------------------------------------------------
 * board-led.c - LED routines
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
#include "board-led.h"
#include "io.h"

#define BOARD_LED_PERIPH_CLOCK_CMD   RCC_AHB1PeriphClockCmd

#if defined STM32F407VE

#define BOARD_LED_PERIPH            RCC_AHB1Periph_GPIOA
#define BOARD_LED_PORT              GPIOA

#define BOARD_LED2_PIN              GPIO_Pin_6
#define BOARD_LED2_ON               GPIO_RESET_BIT(BOARD_LED_PORT, BOARD_LED2_PIN)              // active low
#define BOARD_LED2_OFF              GPIO_SET_BIT(BOARD_LED_PORT, BOARD_LED2_PIN)

#define BOARD_LED3_PIN              GPIO_Pin_7
#define BOARD_LED3_ON               GPIO_RESET_BIT(BOARD_LED_PORT, BOARD_LED3_PIN)              // active low
#define BOARD_LED3_OFF              GPIO_SET_BIT(BOARD_LED_PORT, BOARD_LED3_PIN)

#elif defined STM32F401CC

#define BOARD_LED_PERIPH            RCC_AHB1Periph_GPIOC
#define BOARD_LED_PORT              GPIOC

#define BOARD_LED_PIN               GPIO_Pin_13
#define BOARD_LED_ON                GPIO_RESET_BIT(BOARD_LED_PORT, BOARD_LED_PIN)               // active low
#define BOARD_LED_OFF               GPIO_SET_BIT(BOARD_LED_PORT, BOARD_LED_PIN)

#else
#error unknown STM32
#endif


/*-------------------------------------------------------------------------------------------------------------------------------------------
 * initialize LED port
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
board_led_init (void)
{
    GPIO_InitTypeDef gpio;

    GPIO_StructInit (&gpio);
    BOARD_LED_PERIPH_CLOCK_CMD (BOARD_LED_PERIPH, ENABLE);     // enable clock for LED Port

#if defined STM32F407VE
    GPIO_SET_MODE_OUT_PP(gpio, BOARD_LED2_PIN | BOARD_LED3_PIN, GPIO_Speed_2MHz);
    GPIO_Init(BOARD_LED_PORT, &gpio);
    board_led2_off ();
    board_led3_off ();
#elif defined STM32F401CC
    GPIO_SET_MODE_OUT_PP(gpio, BOARD_LED_PIN, GPIO_Speed_2MHz);
    GPIO_Init(BOARD_LED_PORT, &gpio);
    board_led_off ();
#else
#error unknown STM32
#endif
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * LED on
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
#if defined STM32F401CC
void
board_led_on (void)
{
    BOARD_LED_ON;
}
#endif

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * LED off
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
#if defined STM32F401CC
void
board_led_off (void)
{
    BOARD_LED_OFF;
}
#endif

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * LED2 on
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
#if defined STM32F407VE
void
board_led2_on (void)
{
    BOARD_LED2_ON;
}
#endif

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * LED2 off
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
#if defined STM32F407VE
void
board_led2_off (void)
{
    BOARD_LED2_OFF;
}
#endif

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * LED3 on
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
#if defined STM32F407VE
void
board_led3_on (void)
{
    BOARD_LED3_ON;
}
#endif

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * LED3 off
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
#if defined STM32F407VE
void
board_led3_off (void)
{
    BOARD_LED3_OFF;
}
#endif
