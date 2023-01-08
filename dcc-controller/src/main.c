/*-------------------------------------------------------------------------------------------------------------------------------------------
 * main.c - main module of dcc-central
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
 *
 * Peripherals STM32F401CC Black Pill:
 *
 *    +-------------------------+---------------------------+---------------------------------------------------+
 *    | Device                  | STM32F401CC Black Pill    | Remarks                                           |
 *    +-------------------------+---------------------------+---------------------------------------------------+
 *    | Bootloader              |                           |                                                   |
 *    | RasPi   connections     |                           |                                                   |
 *    | RasPi   RXD0    RX<-TX  | USART1_TX     PA9         | RasPi Pin 10: GPIO15 (UART RX)                    |
 *    | RasPi   TXD0    TX->RX  | USART1_RX     PA10        | RasPi Pin  8: GPIO14 (UART TX)                    |
 *    | RasPi   GPIO    RESET   | NRST          RST         | RasPi Pin 11: GPIO17                              |
 *    | RasPi   GPIO    BOOT0   | BOOT0         BOOT0       | RasPi Pin 13: GPIO27                              |
 *    | RasPi   GPIO    BOOT1   | BOOT1         PB2         | n.c. 10 K Pullup on board                         |
 *    | Bootloader mode         | BOOT0=1       BOOT1=0     |                                                   |
 *    | Runmode                 | BOOT0=0       BOOT1=X     |                                                   |
 *    | Listener LED            | ----          ----        |                                                   |
 *    +-------------------------+---------------------------+---------------------------------------------------+
 *    | RailCom Local           |                           |                                                   |
 *    | RailCom DE              | GPIO          PA1         | RS485 DE                                          |
 *    | RailCom TX              | USART2_TX     PA2         | RS485 DI not used                                 |
 *    | RailCom RX              | USART2_RX     PA3         | RS485 RO                                          |
 *    +-------------------------+---------------------------+---------------------------------------------------+
 *    | RailCom Global          |                           |                                                   |
 *    | RailCom TX              | USART6_TX     PA11        | TX not used                                       |
 *    | RailCom RX              | USART6_RX     PA12        | Global Railcom Decoder                            |
 *    | RailCom Enable          | GPIO OUT      PA15        |                                                   |
 *    +-------------------------+---------------------------+---------------------------------------------------+
 *    | WII                     |                           |                                                   |
 *    | WII             SCL     | I2C1 SCL      PB8         | WII 1 Gamepad                                     |
 *    | WII             SDA     | I2C1 SDA      PB9         | WII 1 Gamepad                                     |
 *    +-------------------------+---------------------------+---------------------------------------------------+
 *    | Booster                 |                           |                                                   |
 *    | Booster L_PWM           | GPIO          PA4         |                                                   |
 *    | Booster R_PWM           | GPIO          PA5         |                                                   |
 *    | Booster ENABLE          | GPIO          PA6         |                                                   |
 *    | Booster CT              | GPIO          PA7         | ADC                                               |
 *    | Booster LED             | GPIO          PC13        | LED 401 Black Pill                                |
 *    +-------------------------+---------------------------+---------------------------------------------------+
 *    | S88                     |                           |                                                   |
 *    | S88 PS                  | GPIO          PB3         | S88 PS output                                     |
 *    | S88 CLK                 | GPIO          PB4         | S88 CLK output                                    |
 *    | S88 RESET               | GPIO          PB5         | S88 RESET output                                  |
 *    | S88 DATA                | GPIO          PB6         | S88 DATA input                                    |
 *    +-------------------------+---------------------------+---------------------------------------------------+
 *    | AUX1                    | GPIO          PA8         | Reserved for later use                            |
 *    | AUX2                    | GPIO          PB15        |                                                   |
 *    | AUX3                    | GPIO          PB14        |                                                   |
 *    | AUX4                    | GPIO          PB13        |                                                   |
 *    | AUX5                    | GPIO          PB12        |                                                   |
 *    | AUX6                    | GPIO          PB0         |                                                   |
 *    | AUX7                    | GPIO          PB1         |                                                   |
 *    | AUX8                    | GPIO          PB2         |                                                   |
 *    | AUX9                    | GPIO          PB10        |                                                   |
 *    +-------------------------+---------------------------+---------------------------------------------------+
 *    | FREE                    | GPIO          PA0         |                                                   |
 *    | FREE                    | GPIO          PB7         |                                                   |
 *    | FREE                    | GPIO          PC14        |                                                   |
 *    | FREE                    | GPIO          PC15        |                                                   |
 *    +-------------------------+---------------------------+---------------------------------------------------+
 *
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "dcc.h"
#include "s88.h"
#include "booster.h"
#include "delay.h"
#include "listener.h"
#include "rc-detector.h"
#include "rc-local.h"
#include "board-led.h"

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * Definitions
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
#define ADC_AVERAGE_8                       8                           // for shortcut detection
#define ADC_AVERAGE_N                       128                         // for display
#define SHORTCUT_ADC_VALUE                  3000                        // indicate shortcut if adc value > 3000

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * Public global variables
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static uint_fast16_t        adc_average_8   = 0;
static uint_fast16_t        adc_average_n   = 0;

#define ADC_STATUS_PERIOD                   10                          // measure ADC status every 10 msec
#define ADC_STATUS_CNT                      10                          // but send ADC status every 10 * 10 msec

#define RC1_STATUS_PERIOD                   100                         // send RC1 status every 40 msec
#define RC2_STATUS_PERIOD                   100                         // send RC2 status every 100 msec
#define S88_STATUS_PERIOD                   90                          // send S88 status every 90 msec
#define RCL_STATUS_PERIOD                   222                         // send RC-Local status every 222 msec
#define RC2_RATE_PERIOD                     300                         // send RC2 rates every 300 msec

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * main () - main function
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
int
main ()
{
    uint16_t        adc_values_8[ADC_AVERAGE_8] = { 0 };
    uint16_t        adc_values_n[ADC_AVERAGE_N] = { 0 };
    uint_fast8_t    do_start_single_conversion  = 1;
    uint_fast8_t    adc_value_idx_8             = 0;
    uint_fast8_t    adc_value_idx_n             = 0;
    uint_fast32_t   adc_sum_8                   = 0;
    uint_fast32_t   adc_sum_n                   = 0;
    uint16_t        adc_value                   = 0;
    uint32_t        current_millis;
    uint32_t        next_adc;
    uint32_t        next_rc1_status;
    uint32_t        next_rc2_status;
    uint32_t        next_s88_status;
    uint32_t        next_rcl_status;
    uint32_t        next_rc2_rate;
    uint32_t        timeout;
    uint32_t        next_timeout;
    uint_fast16_t   rc2_rate_loco_idx = 0;

    SystemInit ();
    SystemCoreClockUpdate();

    delay_init (DELAY_RESOLUTION_1_US);                                     // initialize delay functions with granularity of 1 us

    board_led_init ();
    s88_init ();
    dcc_init ();
    booster_off ();
    listener_init (115200, USART_WordLength_9b, USART_Parity_Even, USART_StopBits_1);
    listener_msg_init ();
    rc_detector_init ();
    rc_local_init ();

    current_millis      = millis;
    next_adc            = current_millis + ADC_STATUS_PERIOD;
    next_rc1_status     = current_millis + RC1_STATUS_PERIOD;
    next_rc2_status     = current_millis + RC2_STATUS_PERIOD;
    next_s88_status     = current_millis + S88_STATUS_PERIOD;
    next_rcl_status     = current_millis + RCL_STATUS_PERIOD;
    next_rc2_rate       = current_millis + RC2_RATE_PERIOD;

    rc_detector_reset_rc2_millis ();

    next_timeout = millis + 1000;

    while (1)
    {
        current_millis = millis;

        listener_send_continue ();
        listener_read_rcl ();

        if (do_start_single_conversion && next_adc <= current_millis)
        {
            booster_adc_start_single_conversion ();
            do_start_single_conversion = 0;
            next_adc = current_millis + ADC_STATUS_PERIOD;
        }

        if (next_rc1_status <= current_millis)
        {
            listener_send_msg_rc1 ();
            next_rc1_status = current_millis + RC1_STATUS_PERIOD;
        }

        if (next_rcl_status <= current_millis)
        {
            listener_send_msg_rcl ();
            next_rcl_status = current_millis + RCL_STATUS_PERIOD;
        }

        if (next_rc2_status <= current_millis)
        {
            listener_send_msg_rc2 ();
            next_rc2_status = current_millis + RC2_STATUS_PERIOD;
        }

        if (next_rc2_rate <= current_millis)
        {
            uint_fast8_t rc2_rate = rc_detector_get_rc2_rate (rc2_rate_loco_idx);
            listener_send_msg_rc2_rate (rc2_rate_loco_idx, rc2_rate);
            next_rc2_rate = current_millis + RC2_RATE_PERIOD;
            rc2_rate_loco_idx++;

            if (rc2_rate_loco_idx >= rc_detector_n_rc2infos)
            {
                rc2_rate_loco_idx = 0;
            }
        }

        if (next_s88_status <= current_millis)
        {
            listener_send_msg_s88 ();
            next_s88_status = current_millis + S88_STATUS_PERIOD;
        }

        timeout = listener_read_cmd ();

        if (timeout)
        {
            next_timeout = millis + timeout;                                                    // don't use current_millis, some commands (PGM) are too slow
        }
        else if (current_millis > next_timeout)                                                 // last cmd received in the past > 1000msec?
        {
            if (booster_is_on)
            {
                listener_send_debug_msg ("connection timeout, booster switched off");
                booster_off ();
            }
        }

        if (! do_start_single_conversion && booster_adc_poll_conversion_value (&adc_value))
        {
            static uint32_t status_cnt = 0;
            status_cnt++;

            adc_sum_8 -= adc_values_8[adc_value_idx_8];
            adc_sum_8 += adc_value;

            adc_sum_n -= adc_values_n[adc_value_idx_n];
            adc_sum_n += adc_value;

            adc_values_8[adc_value_idx_8] = adc_value;
            adc_values_n[adc_value_idx_n] = adc_value;

            adc_value_idx_8++;

            if (adc_value_idx_8 == ADC_AVERAGE_8)
            {
                adc_value_idx_8 = 0;
            }

            adc_value_idx_n++;

            if (adc_value_idx_n == ADC_AVERAGE_N)
            {
                adc_value_idx_n = 0;
            }

            adc_average_8 = adc_sum_8 / ADC_AVERAGE_8;

            if (adc_average_8 > SHORTCUT_ADC_VALUE)
            {
                booster_off ();
                listener_send_msg_adc (adc_average_8);
                listener_send_debug_msgf ("shortcut - booster off: adc:%d", adc_average_8);
            }

            if (status_cnt >= ADC_STATUS_CNT)
            {
                adc_average_n = adc_sum_n / ADC_AVERAGE_N;
                listener_send_msg_adc (adc_average_n);
                status_cnt = 0;
            }

            do_start_single_conversion = 1;
        }

        listener_flush_debug_msg ();
    }

    return (0);
}
