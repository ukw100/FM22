/*-------------------------------------------------------------------------------------------------------------------------------------------
 * main.c - main module of dcc controler
 *-------------------------------------------------------------------------------------------------------------------------------------------
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
 *    | PGM                     |                           |                                                   |
 *    | ACK_PGM                 | GPIO          PB10        | INPUT: PGM ACK                                    |
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
 *    | Booster VT              | GPIO          PB0         | ADC                                               |
 *    | Booster LED             | GPIO          PC13        | LED 401 Black Pill                                |
 *    +-------------------------+---------------------------+---------------------------------------------------+
 *    | S88                     |                           |                                                   |
 *    | S88 PS                  | GPIO          PB3         | S88 PS output                                     |
 *    | S88 CLK                 | GPIO          PB4         | S88 CLK output                                    |
 *    | S88 RESET               | GPIO          PB5         | S88 RESET output                                  |
 *    | S88 DATA                | GPIO          PB6         | S88 DATA input                                    |
 *    +-------------------------+---------------------------+---------------------------------------------------+
 *    | FREE                    | GPIO          PA0         |                                                   |
 *    | FREE                    | GPIO          PA8         |                                                   |
 *    | FREE                    | GPIO          PB1         |                                                   |
 *    | FREE                    | GPIO          PB2         |                                                   |
 *    | FREE                    | GPIO          PB7         |                                                   |
 *    | FREE                    | GPIO          PB12        |                                                   |
 *    | FREE                    | GPIO          PB13        |                                                   |
 *    | FREE                    | GPIO          PB14        |                                                   |
 *    | FREE                    | GPIO          PB15        |                                                   |
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
#include "adc1-dma.h"
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
#define ADC_AVERAGE_N                       256                         // average of N adc values, should be a power of 2

#define ADC_STATUS_PERIOD                   1                           // measure ADC status every 1 msec
#define ADC_STATUS_CNT                      200                         // but send ADC status every 200 * 1 msec = 200 msec

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
    static uint16_t adc_values[ADC_AVERAGE_N];
    uint_fast16_t   adc_average                 = 0;
    uint_fast8_t    adc_value_idx               = 0;
    uint_fast32_t   adc_sum                     = 0;
    uint16_t        adc_value                   = 0;
    uint32_t        next_adc;
    uint8_t *       rcl_msg;
    uint32_t        current_millis;
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
    adc1_dma_init ();

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
        dcc_reset_last_active_switch ();
        listener_send_continue ();

        rcl_msg = listener_read_rcl ();

        if (rcl_msg)
        {
            switch (rcl_msg[0])
            {
                case MSG_RCL_RC2:
                {
                    uint_fast8_t    channel_idx = rcl_msg[1];
                    uint_fast16_t   addr        = (rcl_msg[2] << 8) | rcl_msg[3];
                    rc_detector_set_loco_location (addr, channel_idx);
                    break;
                }
#if 0 // later
                case MSG_RCL_ACK:
                {
                    uint_fast16_t   addr        = (rcl_msg[1] << 8) | rcl_msg[2];
                    rc_detector_set_loco_location (addr, channel_idx);
                    break;
                }
#endif
            }
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

        if (next_adc <= current_millis)
        {
            static uint32_t status_cnt = 0;

            next_adc = current_millis + ADC_STATUS_PERIOD;
            status_cnt++;

            adc_value = adc1_dma_buffer[0];
            adc_sum -= adc_values[adc_value_idx];
            adc_sum += adc_value;

            adc_values[adc_value_idx] = adc_value;

            adc_value_idx++;

            if (adc_value_idx == ADC_AVERAGE_N)
            {
                adc_value_idx = 0;
            }

#if 0
            static uint16_t max_adc_value;

            if (adc_value > max_adc_value)
            {
                listener_send_debug_msgf ("max:%d\n", max_adc_value);
                max_adc_value = adc_value;
            }
#endif

            adc_average = adc_sum / ADC_AVERAGE_N;

            if (adc_average > dcc_shortcut_value)
            {
                char b[64];
                booster_off ();
                // listener_send_msg_adc (adc_average);
                sprintf (b, "Kurzschluss! ADC: %d", adc_average);
                listener_send_msg_alert (b);
            }

            if (status_cnt >= ADC_STATUS_CNT)
            {
                // listener_send_debug_msgf ("adc:%d", adc_average_8);
                listener_send_msg_adc (adc_average);
                status_cnt = 0;
            }
        }

        listener_flush_debug_msg ();
    }

    return (0);
}
