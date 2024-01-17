/*------------------------------------------------------------------------------------------------------------------------
 * switch.cc - switch machine management functions
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
#include <stdlib.h>
#include <string.h>
#include "dcc.h"
#include "debug.h"
#include "railroad.h"
#include "loco.h"
#include "s88.h"
#include "rcl.h"
#include "switch.h"

#define SWITCH_SCHEDULE_DELAY   220                                         // schedule time for switches in msec

std::vector<Switch>             Switches::switches;                         // switches
uint_fast16_t                   Switches::n_switches  = 0;                  // number of switches
bool                            Switches::data_changed = false;

SWITCH_EVENTS                   Switches::events[SWITCH_EVENT_LEN];         // event ringbuffer
uint_fast16_t                   Switches::event_size      = 0;              // current event size
uint_fast16_t                   Switches::event_start     = 0;              // current event start index
uint_fast16_t                   Switches::event_stop      = 0;              // current event stop index

/*------------------------------------------------------------------------------------------------------------------------
 * Switch () - constructor
 *------------------------------------------------------------------------------------------------------------------------
 */
Switch::Switch ()
{
    this->name                  = "";
    this->addr                  = 0;
    this->current_state         = DCC_SWITCH_STATE_UNDEFINED;
    this->flags                 = 0;
}

/*------------------------------------------------------------------------------------------------------------------------
 * set_id () - set id
 *------------------------------------------------------------------------------------------------------------------------
 */
void
Switch::set_id (uint_fast16_t id)
{
    this->id = id;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  Switch::set_name () - set name
 *------------------------------------------------------------------------------------------------------------------------
 */
void
Switch::set_name (std::string name)
{
    this->name = name;
    Switches::data_changed = true;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  Switch::get_name () - get name
 *------------------------------------------------------------------------------------------------------------------------
 */
std::string
Switch::get_name ()
{
    return this->name;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  Switch::set_addr () - set address
 *------------------------------------------------------------------------------------------------------------------------
 */
void
Switch::set_addr (uint_fast16_t addr)
{
    this->addr = addr;
    Switches::data_changed = true;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  Switch::get_addr () - get address
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast16_t
Switch::get_addr ()
{
    return this->addr;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  Switch::set_state () - set state
 *------------------------------------------------------------------------------------------------------------------------
 */
void
Switch::set_state (uint_fast8_t f)
{
    if (this->current_state != f)
    {
        this->current_state = f;

        if (f == DCC_SWITCH_STATE_BRANCH ||
            (f == DCC_SWITCH_STATE_BRANCH2 && (this->flags & SWITCH_FLAG_3WAY)) ||
            f == DCC_SWITCH_STATE_STRAIGHT)
        {
            Switches::add_event (this->id, f | SWITCH_ACTIVE_MASK);
            Switches::add_event (this->id, f);
        }
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 *  Switch::get_state ()
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
Switch::get_state ()
{
    return this->current_state;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  Switch::set_flags () - set state
 *------------------------------------------------------------------------------------------------------------------------
 */
void
Switch::set_flags (uint_fast8_t f)
{
    this->flags = f;
    Switches::data_changed = true;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  Switch::get_flags ()
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
Switch::get_flags ()
{
    return this->flags;
}

/*------------------------------------------------------------------------------------------------------------------------
 * Switches::add_event() - add event
 *------------------------------------------------------------------------------------------------------------------------
 */
void
Switches::add_event (uint16_t switch_idx, uint_fast8_t mask)
{
    if (event_size < SWITCH_EVENT_LEN)                                      // buffer full?
    {
        Switches::events[event_stop].switch_idx   = switch_idx;
        Switches::events[event_stop].mask         = mask;

        event_stop++;

        if (event_stop >= SWITCH_EVENT_LEN)                                 // at end of ringbuffer?
        {                                                                   // yes
            event_stop = 0;                                                 // reset to beginning
        }

        event_size++;                                                       // increment used size
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 * Switches::schedule () - schedule events
 *
 * Return value:
 *  time in msec when next call should be done
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast16_t
Switches::schedule (void)
{
    uint_fast16_t   rtc = SWITCH_SCHEDULE_DELAY;

    if (event_size != 0)
    {
        uint_fast8_t    active  = Switches::events[event_start].mask & SWITCH_ACTIVE_MASK;
        uint_fast8_t    f       = Switches::events[event_start].mask & DCC_SWITCH_STATE_MASK;
        uint16_t        swidx   = Switches::events[event_start].switch_idx;
        uint_fast16_t   addr    = Switches::switches[swidx].get_addr();
        uint_fast16_t   flags   = Switches::switches[swidx].get_flags();

        Debug::printf (DEBUG_LEVEL_VERBOSE, "Switches::schedule: active=%u f=%u swidx=%u addr=%u\r\n", active, f, swidx, addr);

        if (active)         // activate switch
        {
            DCC::base_switch_set (addr, f);
        }
        else                // deactivate switch
        {
            // DCC::base_switch_reset (addr, f);                            // not necessary anymore, see DCC controller software

            if (flags & SWITCH_FLAG_3WAY)                                   // a 3-way switch powers on 2 switches
            {
                rtc = 2 * SWITCH_SCHEDULE_DELAY;
            }
        }

        event_start++;

        if (event_start == SWITCH_EVENT_LEN)                                // at end of ringbuffer?
        {                                                                   // yes
            event_start = 0;                                                // reset to beginning
        }
        event_size--;
    }

    return rtc;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  Switches::add () - add a switch
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast16_t
Switches::add (const Switch& new_switch)
{
    if (Switches::n_switches < MAX_SWITCHES)
    {
        switches.push_back(new_switch);
        Switches::switches[n_switches].set_id(n_switches);
        Switches::n_switches++;
        return switches.size() - 1;
    }
    return 0xFFFF;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  Switches::remove ()
 *------------------------------------------------------------------------------------------------------------------------
 */
bool
Switches::remove (uint_fast16_t swidx)
{
    uint_fast16_t   idx;
    uint_fast8_t    rtc = false;

    if (swidx < Switches::n_switches)
    {
        Switches::n_switches--;

        for (idx = swidx; idx < Switches::n_switches; idx++)
        {
            Switches::switches[idx] = Switches::switches[idx + 1];
        }

        Switches::data_changed = true;
        rtc = true;
    }

    return rtc;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  set_new_id () - set new id
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast16_t
Switches::set_new_id (uint_fast16_t swidx, uint_fast16_t new_swidx)
{
    uint_fast16_t   rtc = 0xFFFF;

    if (swidx < Switches::n_switches && new_swidx < Switches::n_switches)
    {
        if (swidx != new_swidx)
        {
            Switch          tmpswitch;
            uint_fast16_t   sidx;

            uint16_t *      map_new_switch_idx = (uint16_t *) calloc (Switches::n_switches, sizeof (uint16_t));

            for (sidx = 0; sidx < Switches::n_switches; sidx++)
            {
                map_new_switch_idx[sidx] = sidx;
            }

            tmpswitch = Switches::switches[swidx];

            if (new_swidx < swidx)
            {
                for (sidx = swidx; sidx > new_swidx; sidx--)
                {
                    Switches::switches[sidx] = Switches::switches[sidx - 1];
                    map_new_switch_idx[sidx - 1] = sidx;
                }
            }
            else // if (new_swidx > swidx)
            {
                for (sidx = swidx; sidx < new_swidx; sidx++)
                {
                    Switches::switches[sidx] = Switches::switches[sidx + 1];
                    map_new_switch_idx[sidx + 1] = sidx;
                }
            }

            Switches::switches[sidx] = tmpswitch;
            map_new_switch_idx[swidx] = sidx;

            for (sidx = 0; sidx < Switches::n_switches; sidx++)
            {
                Debug::printf (DEBUG_LEVEL_VERBOSE, "%2d -> %2d\n", sidx, map_new_switch_idx[sidx]);
            }

            RailroadGroups::set_new_switch_ids (map_new_switch_idx, Switches::n_switches);
            RCL::set_new_switch_ids (map_new_switch_idx, Switches::n_switches);
            S88::set_new_switch_ids (map_new_switch_idx, Switches::n_switches);

            free (map_new_switch_idx);
            Switches::data_changed = true;
            rtc = new_swidx;

            Switches::renumber ();
        }
        else
        {
            rtc = swidx;
        }
    }

    return rtc;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  Switches::get_n_switches () - get number of switches
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast16_t
Switches::get_n_switches (void)
{
    return Switches::n_switches;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  renumber () - renumber ids
 *------------------------------------------------------------------------------------------------------------------------
 */
void
Switches::renumber ()
{
    uint_fast16_t swidx;

    for (swidx = 0; swidx < Switches::n_switches; swidx++)
    {
        Switches::switches[swidx].set_id (swidx);
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 *  Switches::booster_on ()
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
Switches::booster_on (void)
{
    uint_fast16_t swidx;

    for (swidx = 0; swidx < Switches::n_switches; swidx++)
    {
        Switches::switches[swidx].set_state (DCC_SWITCH_STATE_UNDEFINED);
    }

    return 0;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  Switches::booster_off ()
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
Switches::booster_off (void)
{
    uint_fast16_t swidx;

    for (swidx = 0; swidx < Switches::n_switches; swidx++)
    {
        Switches::switches[swidx].set_state (DCC_SWITCH_STATE_UNDEFINED);
    }

    return 0;
}
