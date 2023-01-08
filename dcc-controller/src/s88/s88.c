/*------------------------------------------------------------------------------------------------------------------------
 * s88.c - s88 management functions
 *------------------------------------------------------------------------------------------------------------------------
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
 *------------------------------------------------------------------------------------------------------------------------
 *
 * S88
 *
 * Pin Name        Pin RJ45  Farbe **)
 * 1   DATA        2         gn
 * 2   GND         3         ws/or
 * 2   GND         5         ws/bl
 * 3   CLOCK       4         bl
 * 4   PS (LOAD)   6         or
 * 5   RESET       7         ws/br
 * 6   +5V/+12V    1         ws/gn
 * -   RAILDATA *) 8         br
 * -   SHIELD      -         -
 *------------------------------------------------------------------------------------------------------------------------
 */
#include <stdio.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dcc.h"
#include "booster.h"
#include "s88.h"
#include "io.h"

#define S88_PERIPH_CLOCK_CMD    RCC_AHB1PeriphClockCmd

#if defined STM32F407VE

#define S88_PS_PERIPH           RCC_AHB1Periph_GPIOE
#define S88_PS_PORT             GPIOE
#define S88_PS_PIN              GPIO_Pin_0

#define S88_CLK_PERIPH          RCC_AHB1Periph_GPIOE
#define S88_CLK_PORT            GPIOE
#define S88_CLK_PIN             GPIO_Pin_1

#define S88_RST_PERIPH          RCC_AHB1Periph_GPIOE
#define S88_RST_PORT            GPIOE
#define S88_RST_PIN             GPIO_Pin_2


#define S88_DATA_PERIPH         RCC_AHB1Periph_GPIOE
#define S88_DATA_PORT           GPIOE
#define S88_DATA_PIN            GPIO_Pin_5

#elif defined STM32F401CC

#define S88_PS_PERIPH           RCC_AHB1Periph_GPIOB
#define S88_PS_PORT             GPIOB
#define S88_PS_PIN              GPIO_Pin_3

#define S88_CLK_PERIPH          RCC_AHB1Periph_GPIOB
#define S88_CLK_PORT            GPIOB
#define S88_CLK_PIN             GPIO_Pin_4

#define S88_RST_PERIPH          RCC_AHB1Periph_GPIOB
#define S88_RST_PORT            GPIOB
#define S88_RST_PIN             GPIO_Pin_5

#define S88_DATA_PERIPH         RCC_AHB1Periph_GPIOB
#define S88_DATA_PORT           GPIOB
#define S88_DATA_PIN            GPIO_Pin_6

#else
#error unknown STM32
#endif

#define S88_PS_HIGH()           GPIO_SET_BIT(S88_PS_PORT, S88_PS_PIN)
#define S88_PS_LOW()            GPIO_RESET_BIT(S88_PS_PORT, S88_PS_PIN)
#define S88_CLK_HIGH()          GPIO_SET_BIT(S88_CLK_PORT, S88_CLK_PIN)
#define S88_CLK_LOW()           GPIO_RESET_BIT(S88_CLK_PORT, S88_CLK_PIN)
#define S88_RST_HIGH()          GPIO_SET_BIT(S88_RST_PORT, S88_RST_PIN)
#define S88_RST_LOW()           GPIO_RESET_BIT(S88_RST_PORT, S88_RST_PIN)
#define S88_DATA_READ()         GPIO_GET_BIT(S88_DATA_PORT, S88_DATA_PIN)

#define S88_MAX_CONTACTS        128                                         // should be a multiple of 8, better 16
#define S88_MAX_CONTACT_BYTES   (S88_MAX_CONTACTS / sizeof (uint8_t))

#define S88_STATE_CLK_HIGH      0
#define S88_STATE_READ_DATA     1
#define S88_STATE_CLK_LOW       2

#define S88_STATE_FREE          0
#define S88_STATE_OCCUPIED      1

static volatile uint16_t        s88_count[S88_MAX_CONTACTS];                // filled by s88_read() called by ISR
static volatile uint8_t         s88_new_bits[S88_MAX_CONTACT_BYTES];        // filled by s88_read() called by ISR
static volatile uint_fast16_t   s88_n_contacts = 0;                         // number of S88 contacts, also used by ISR
static volatile uint_fast8_t    s88_n_bytes = 0;

/*------------------------------------------------------------------------------------------------------------------------
 *  s88_set_n_contacts () - set number of S88 contacts
 *------------------------------------------------------------------------------------------------------------------------
 */
void
s88_set_n_contacts (uint_fast16_t n)
{
    s88_n_contacts  = n;
    s88_n_bytes     = n / 8 + ((n % 8) ? 1 : 0);
}

/*------------------------------------------------------------------------------------------------------------------------
 *  s88_get_n_contacts () - get number of S88 contacts
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast16_t
s88_get_n_contacts (void)
{
    uint_fast16_t n = s88_n_contacts;
    return n;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  s88_get_n_bytes () - get number of S88 bytes
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
s88_get_n_bytes (void)
{
    uint_fast8_t n = s88_n_bytes;
    return n;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  s88_get_status_byte () - get status byte
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast16_t
s88_get_status_byte (uint_fast8_t idx)
{
    return s88_new_bits[idx];
}

/*------------------------------------------------------------------------------------------------------------------------
 *  s88_booster_on ()
 *------------------------------------------------------------------------------------------------------------------------
 */
void
s88_booster_on (void)
{
    uint_fast16_t idx;
    uint_fast16_t nbytes = s88_n_bytes;

    for (idx = 0; idx < S88_MAX_CONTACTS; idx++)
    {
        s88_count[idx] = 0;
    }

    for (idx = 0; idx < nbytes; idx++)
    {
        s88_new_bits[idx] = 0;
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 *  s88_booster_off ()
 *------------------------------------------------------------------------------------------------------------------------
 */
void
s88_booster_off (void)
{
    uint_fast16_t idx;
    uint_fast16_t nbytes = s88_n_bytes;

    for (idx = 0; idx < S88_MAX_CONTACTS; idx++)
    {
        s88_count[idx] = 0;
    }

    for (idx = 0; idx < nbytes; idx++)
    {
        s88_new_bits[idx] = 0;
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 *  s88_set_bit () - set bit, reset debounce counter
 *------------------------------------------------------------------------------------------------------------------------
 */
static void
s88_set_bit (uint_fast8_t byte_idx, uint_fast8_t bit)
{
    uint_fast16_t count_idx = (byte_idx << 3) | bit;

    s88_new_bits[byte_idx] |= 0x01 << bit;
    s88_count[count_idx] = 0;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  s88_reset_bit () - reset bit, if debounce counter >= S88_DEBOUNCE_LIMIT
 *------------------------------------------------------------------------------------------------------------------------
 */
#define S88_DEBOUNCE_LIMIT  500

static void
s88_reset_bit (uint_fast8_t byte_idx, uint_fast8_t bit)
{
    uint_fast16_t count_idx = (byte_idx << 3) | bit;

    if (s88_count[count_idx] < 0xFFFF - 1)
    {
        s88_count[count_idx]++;
    }

    if (s88_count[count_idx] >= S88_DEBOUNCE_LIMIT)
    {
        s88_new_bits[byte_idx] &= ~(0x01 << bit);
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 *  s88_read () - read S88 bus - called by ISR (see dcc.c)
 *------------------------------------------------------------------------------------------------------------------------
 */
void
s88_read (void)
{
    static uint_fast8_t     s88_cnt         = 0;
    static uint_fast8_t     s88_state       = 0;
    static uint_fast8_t     s88_byte_idx    = 0;
    static uint_fast8_t     s88_bit         = 0;

    if (s88_n_contacts > 0)
    {
        switch (s88_cnt)
        {
            case 0:
            {
                S88_PS_HIGH();
                s88_cnt = 1;
                break;
            }
            case 1:
            {
                S88_CLK_HIGH();
                s88_cnt = 2;
                break;
            }
            case 2:
            {
                if (S88_DATA_READ())
                {
                    s88_set_bit (0, 0);
                }
                else
                {
                    s88_reset_bit (0, 0);
                }

                S88_CLK_LOW();
                s88_state = S88_STATE_CLK_HIGH;
                s88_cnt = 3;
                s88_byte_idx = 0;
                s88_bit = 1;
                break;
            }
            case 3:
            {
                S88_RST_HIGH();
                s88_cnt = 4;
                break;
            }
            case 4:
            {
                S88_RST_LOW();
                s88_cnt = 5;
                break;
            }
            case 5:
            {
                S88_PS_LOW();
                s88_cnt = 6;
                s88_state = S88_STATE_CLK_HIGH;
                break;
            }
            default:
            {
                switch (s88_state)
                {
                    case S88_STATE_CLK_HIGH:
                    {
                        S88_CLK_HIGH();
                        s88_state = S88_STATE_READ_DATA;
                        break;
                    }
                    case S88_STATE_READ_DATA:
                    {
                        if (S88_DATA_READ ())
                        {
                            s88_set_bit (s88_byte_idx, s88_bit);
                        }
                        else
                        {
                            s88_reset_bit (s88_byte_idx, s88_bit);
                        }
                        s88_state = S88_STATE_CLK_LOW;
                        break;
                    }
                    case S88_STATE_CLK_LOW:
                    {
                        S88_CLK_LOW();
                        s88_state = S88_STATE_CLK_HIGH;
                        s88_bit++;

                        if (s88_bit == 8)
                        {
                            s88_bit = 0;
                            s88_byte_idx++;

                            if (s88_byte_idx >= s88_n_bytes)
                            {
                                s88_cnt = 0;
                            }
                        }
                        break;
                    }
                }
                break;
            }
        }
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 *  s88_init ()
 *------------------------------------------------------------------------------------------------------------------------
 */
void
s88_init (void)
{
    GPIO_InitTypeDef gpio;

    GPIO_StructInit (&gpio);
    S88_PERIPH_CLOCK_CMD (S88_PS_PERIPH, ENABLE);
    GPIO_SET_MODE_OUT_PP(gpio, S88_PS_PIN, GPIO_Speed_2MHz);
    GPIO_Init(S88_PS_PORT, &gpio);
    S88_PS_LOW();

    GPIO_StructInit (&gpio);
    S88_PERIPH_CLOCK_CMD (S88_CLK_PERIPH, ENABLE);
    GPIO_SET_MODE_OUT_PP(gpio, S88_CLK_PIN, GPIO_Speed_2MHz);
    GPIO_Init(S88_CLK_PORT, &gpio);
    S88_CLK_LOW();

    GPIO_StructInit (&gpio);
    S88_PERIPH_CLOCK_CMD (S88_RST_PERIPH, ENABLE);
    GPIO_SET_MODE_OUT_PP(gpio, S88_RST_PIN, GPIO_Speed_2MHz);
    GPIO_Init(S88_RST_PORT, &gpio);
    S88_RST_LOW();

    GPIO_StructInit (&gpio);
    S88_PERIPH_CLOCK_CMD (S88_DATA_PERIPH, ENABLE);
    GPIO_SET_MODE_IN_UP(gpio, S88_DATA_PIN, GPIO_Speed_2MHz);
    // GPIO_SET_MODE_IN_DOWN(gpio, S88_DATA_PIN, GPIO_Speed_2MHz);
    GPIO_Init(S88_DATA_PORT, &gpio);
}
