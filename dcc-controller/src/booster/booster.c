/*------------------------------------------------------------------------------------------------------------------------
 * booster.c - handle booster
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
 * L6201 booster
 *
 * L6201 pins:
 *    +-----------------+---------------------------+
 *    | L6201 pins      | Remarks                   |
 *    +-----------------+---------------------------+
 *    | B+              | Battery + (not used)      |
 *    | GND             | GND                       |
 *    | EN              | Enable                    |
 *    | RPWM            | Right PWM                 |
 *    | LPWM            | Left PWM                  |
 *    | CT              | Current Sense             |
 *    | VT              | Voltage Sense (not used)  |
 *    +-------------------------+-------------------+
 *
 *    +-----------------+---------------------------+
 *    | Device          | STM32F401CC Black Pill    |
 *    +-----------------+---------------------------+
 *    | Booster         |                           |
 *    | Booster L_PWM   | GPIO          PA4         |
 *    | Booster R_PWM   | GPIO          PA5         |
 *    | Booster ENABLE  | GPIO          PA6         |
 *    | Booster CT      | GPIO          PA7         |
 *    | Booster LED     | GPIO          PC13        |
 *    +-----------------+---------------------------+
 *------------------------------------------------------------------------------------------------------------------------
 */
#include "board-led.h"
#include "rc-detector.h"
#include "booster.h"
#include "io.h"

#define BOOSTER_PERIPH_CLOCK_CMD    RCC_AHB1PeriphClockCmd

#if defined STM32F407VE

#define BOOSTER_PERIPH              RCC_AHB1Periph_GPIOC
#define BOOSTER_PORT                GPIOC

#define BOOSTER_L_PIN               GPIO_Pin_0
#define BOOSTER_R_PIN               GPIO_Pin_1
#define BOOSTER_E_PIN               GPIO_Pin_2

#elif defined STM32F401CC

#define BOOSTER_PERIPH              RCC_AHB1Periph_GPIOA
#define BOOSTER_PORT                GPIOA

#define BOOSTER_L_PIN               GPIO_Pin_4
#define BOOSTER_R_PIN               GPIO_Pin_5
#define BOOSTER_E_PIN               GPIO_Pin_6

#else
#error unknown STM32
#endif

#define BOOSTER_L_LOW               GPIO_RESET_BIT(BOOSTER_PORT, BOOSTER_L_PIN)
#define BOOSTER_L_HIGH              GPIO_SET_BIT(BOOSTER_PORT, BOOSTER_L_PIN)
#define BOOSTER_R_LOW               GPIO_RESET_BIT(BOOSTER_PORT, BOOSTER_R_PIN)
#define BOOSTER_R_HIGH              GPIO_SET_BIT(BOOSTER_PORT, BOOSTER_R_PIN)
#define BOOSTER_E_LOW               GPIO_RESET_BIT(BOOSTER_PORT, BOOSTER_E_PIN)
#define BOOSTER_E_HIGH              GPIO_SET_BIT(BOOSTER_PORT, BOOSTER_E_PIN)

volatile uint_fast8_t               booster_is_on = 0;      // volatile: changed in ISR
uint_fast8_t                        booster_is_positive;
uint_fast8_t                        booster_is_negative;

/*------------------------------------------------------------------------------------------------------------------------
 * booster_negative () - set voltage to -12V
 *------------------------------------------------------------------------------------------------------------------------
 */
void
booster_negative (void)
{
    BOOSTER_L_HIGH;
    BOOSTER_R_LOW;
    booster_is_positive = 0;
    booster_is_negative = 1;
}

/*------------------------------------------------------------------------------------------------------------------------
 * booster_positive () - set voltage to +12V
 *------------------------------------------------------------------------------------------------------------------------
 */
void
booster_positive (void)
{
    BOOSTER_L_LOW;
    BOOSTER_R_HIGH;
    booster_is_positive = 1;
    booster_is_negative = 0;
}

/*------------------------------------------------------------------------------------------------------------------------
 * booster_on () - enable both L6201
 *------------------------------------------------------------------------------------------------------------------------
 */
void
booster_on (void)
{
    BOOSTER_E_HIGH;
    booster_is_on = 1;

#if defined STM32F407VE
    board_led2_on ();
#elif defined STM32F401CC
    board_led_on ();
#else
#error unknown STM32
#endif

}

/*------------------------------------------------------------------------------------------------------------------------
 * booster_off () - disable both L6201
 *------------------------------------------------------------------------------------------------------------------------
 */
void
booster_off (void)
{
    BOOSTER_E_LOW;
    booster_is_on = 0;

#if defined STM32F407VE
    board_led2_off ();
#elif defined STM32F401CC
    board_led_off ();
#else
#error unknown STM32
#endif

    booster_is_positive = 0;
    booster_is_negative = 0;
    rc_detector_reset_rc2_millis ();
}

/*------------------------------------------------------------------------------------------------------------------------
 * booster_cutout () - cutout both L6201
 *------------------------------------------------------------------------------------------------------------------------
 */
void
booster_cutout (void)
{
    BOOSTER_L_LOW;
    BOOSTER_R_LOW;
}

/*------------------------------------------------------------------------------------------------------------------------
 * booster_init () - init
 *------------------------------------------------------------------------------------------------------------------------
 */
void
booster_init (void)
{
    GPIO_InitTypeDef gpio;
    GPIO_StructInit (&gpio);
    BOOSTER_PERIPH_CLOCK_CMD (BOOSTER_PERIPH, ENABLE);
    GPIO_SET_MODE_OUT_PP(gpio, BOOSTER_L_PIN | BOOSTER_R_PIN | BOOSTER_E_PIN, GPIO_Speed_2MHz);
    GPIO_Init(BOOSTER_PORT, &gpio);
    booster_off ();
}
