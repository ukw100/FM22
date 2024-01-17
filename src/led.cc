/*------------------------------------------------------------------------------------------------------------------------
 * led.cc - led management functions
 *------------------------------------------------------------------------------------------------------------------------
 * Copyright (c) 2023-2024 Frank Meyer - frank(at)uclock.de
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
#include <stdlib.h>
#include <string.h>
#include "dcc.h"
#include "debug.h"
#include "led.h"

std::vector<LedGroup>           Leds::led_groups;                       // leds
uint_fast16_t                   Leds::n_led_groups  = 0;                // number of leds
bool                            Leds::data_changed = false;

/*------------------------------------------------------------------------------------------------------------------------
 * LedGroup () - constructor
 *------------------------------------------------------------------------------------------------------------------------
 */
LedGroup::LedGroup ()
{
    this->name                  = "";
    this->addr                  = 0;
    this->current_state_mask    = 0x00;
}

/*------------------------------------------------------------------------------------------------------------------------
 * set_id () - set id
 *------------------------------------------------------------------------------------------------------------------------
 */
void
LedGroup::set_id (uint_fast16_t id)
{
    this->id = id;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  LedGroup::set_name () - set name
 *------------------------------------------------------------------------------------------------------------------------
 */
void
LedGroup::set_name (std::string name)
{
    this->name = name;
    Leds::data_changed = true;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  LedGroup::get_name () - get name
 *------------------------------------------------------------------------------------------------------------------------
 */
std::string
LedGroup::get_name ()
{
    return this->name;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  LedGroup::set_addr () - set address
 *------------------------------------------------------------------------------------------------------------------------
 */
void
LedGroup::set_addr (uint_fast16_t addr)
{
    this->addr = addr;
    Leds::data_changed = true;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  LedGroup::get_addr () - get address
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast16_t
LedGroup::get_addr ()
{
    return this->addr;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  LedGroup::set_state () - set state
 *------------------------------------------------------------------------------------------------------------------------
 */
void
LedGroup::set_state (uint_fast8_t led_mask)
{
    if (this->current_state_mask != led_mask)
    {
        this->current_state_mask = led_mask;

        DCC::ext_accessory_set (this->addr, led_mask);
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 *  LedGroup::set_state () - set state
 *------------------------------------------------------------------------------------------------------------------------
 */
void
LedGroup::set_state (uint_fast8_t led_mask, uint_fast8_t on)
{
    uint_fast8_t new_mask;

    if (on)
    {
        new_mask = this->current_state_mask | led_mask;
    }
    else
    {
        new_mask = this->current_state_mask & ~led_mask;
    }

    this->set_state (new_mask);
}

/*------------------------------------------------------------------------------------------------------------------------
 *  LedGroup::get_state ()
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
LedGroup::get_state ()
{
    return this->current_state_mask;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  Leds::add () - add a led
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast16_t
Leds::add (const LedGroup& new_led_group)
{
    if (Leds::n_led_groups < MAX_LED_GROUPS)
    {
        led_groups.push_back(new_led_group);
        Leds::led_groups[n_led_groups].set_id(n_led_groups);
        Leds::n_led_groups++;
        return led_groups.size() - 1;
    }

    return 0xFFFF;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  Leds::remove ()
 *------------------------------------------------------------------------------------------------------------------------
 */
bool
Leds::remove (uint_fast16_t led_group_idx)
{
    uint_fast16_t   idx;
    uint_fast8_t    rtc = false;

    if (led_group_idx < Leds::n_led_groups)
    {
        Leds::n_led_groups--;

        for (idx = led_group_idx; idx < Leds::n_led_groups; idx++)
        {
            Leds::led_groups[idx] = Leds::led_groups[idx + 1];
        }

        Leds::data_changed = true;
        rtc = true;
    }

    return rtc;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  set_new_id () - set new id
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast16_t
Leds::set_new_id (uint_fast16_t led_group_idx, uint_fast16_t new_led_group_idx)
{
    uint_fast16_t   rtc = 0xFFFF;

    if (led_group_idx < Leds::n_led_groups && new_led_group_idx < Leds::n_led_groups)
    {
        if (led_group_idx != new_led_group_idx)
        {
            LedGroup        tmpled;
            uint_fast16_t   sidx;

            uint16_t *      map_new_led_idx = (uint16_t *) calloc (Leds::n_led_groups, sizeof (uint16_t));

            for (sidx = 0; sidx < Leds::n_led_groups; sidx++)
            {
                map_new_led_idx[sidx] = sidx;
            }

            tmpled = Leds::led_groups[led_group_idx];

            if (new_led_group_idx < led_group_idx)
            {
                for (sidx = led_group_idx; sidx > new_led_group_idx; sidx--)
                {
                    Leds::led_groups[sidx] = Leds::led_groups[sidx - 1];
                    map_new_led_idx[sidx - 1] = sidx;
                }
            }
            else // if (new_led_group_idx > led_group_idx)
            {
                for (sidx = led_group_idx; sidx < new_led_group_idx; sidx++)
                {
                    Leds::led_groups[sidx] = Leds::led_groups[sidx + 1];
                    map_new_led_idx[sidx + 1] = sidx;
                }
            }

            Leds::led_groups[sidx] = tmpled;
            map_new_led_idx[led_group_idx] = sidx;

            for (sidx = 0; sidx < Leds::n_led_groups; sidx++)
            {
                Debug::printf (DEBUG_LEVEL_VERBOSE, "%2d -> %2d\n", sidx, map_new_led_idx[sidx]);
            }

            free (map_new_led_idx);
            Leds::data_changed = true;
            rtc = new_led_group_idx;

            Leds::renumber ();
        }
        else
        {
            rtc = led_group_idx;
        }
    }

    return rtc;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  Leds::get_n_led_groups () - get number of leds
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast16_t
Leds::get_n_led_groups (void)
{
    return Leds::n_led_groups;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  renumber () - renumber ids
 *------------------------------------------------------------------------------------------------------------------------
 */
void
Leds::renumber ()
{
    uint_fast16_t led_group_idx;

    for (led_group_idx = 0; led_group_idx < Leds::n_led_groups; led_group_idx++)
    {
        Leds::led_groups[led_group_idx].set_id (led_group_idx);
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 *  Leds::booster_on ()
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
Leds::booster_on (void)
{
    return 0;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  Leds::booster_off ()
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
Leds::booster_off (void)
{
    return 0;
}
