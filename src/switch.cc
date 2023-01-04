/*------------------------------------------------------------------------------------------------------------------------
 * switch.cc - switch machine management functions
 *------------------------------------------------------------------------------------------------------------------------
 * Copyright (c) 2022-2023 Frank Meyer - frank(at)uclock.de
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
#include "switch.h"

#define SWITCH_SCHEDULE_DELAY   250                                         // schedule time for switches in msec

SWITCH                          Switch::switches[MAX_SWITCHES];
uint_fast16_t                   Switch::n_switches  = 0;                            // number of switches
bool                            Switch::data_changed = false;

SWITCH_EVENTS                   Switch::events[SWITCH_EVENT_LEN];           // event ringbuffer
uint_fast16_t                   Switch::event_size      = 0;                // current event size
uint_fast16_t                   Switch::event_start     = 0;                // current event start index
uint_fast16_t                   Switch::event_stop      = 0;                // current event stop index

/*------------------------------------------------------------------------------------------------------------------------
 * Switch::add_event() - add event
 *------------------------------------------------------------------------------------------------------------------------
 */
void
Switch::add_event (uint16_t switch_idx, uint_fast8_t mask)
{
    if (event_size < SWITCH_EVENT_LEN)                                      // buffer full?
    {
        Switch::events[event_stop].switch_idx   = switch_idx;
        Switch::events[event_stop].mask         = mask;

        event_stop++;

        if (event_stop >= SWITCH_EVENT_LEN)                                 // at end of ringbuffer?
        {                                                                   // yes
            event_stop = 0;                                                 // reset to beginning
        }

        event_size++;                                                       // increment used size
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 * Switch::schedule () - schedule events
 *
 * Return value:
 *  time in msec when next call should be done
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast16_t
Switch::schedule (void)
{
    uint_fast16_t   rtc = SWITCH_SCHEDULE_DELAY;

    if (event_size != 0)
    {
        uint_fast8_t    active  = Switch::events[event_start].mask & SWITCH_ACTIVE_MASK;
        uint_fast8_t    f       = Switch::events[event_start].mask & DCC_SWITCH_STATE_MASK;
        uint16_t        swidx   = Switch::events[event_start].switch_idx;
        uint_fast16_t   addr    = Switch::switches[swidx].addr;
        uint_fast16_t   flags   = Switch::switches[swidx].flags;

        Debug::printf (DEBUG_LEVEL_VERBOSE, "Switch::schedule: active=%u f=%u swidx=%u addr=%u\r\n", active, f, swidx, addr);
        if (active)         // activate switch
        {
            DCC::base_switch_set (addr, f);
        }
        else                // deactivate switch
        {
            DCC::base_switch_reset (addr, f);

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
 *  Switch::add () - add a switch
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast16_t
Switch::add (void)
{
    uint_fast16_t swidx;

    if (Switch::n_switches < MAX_SWITCHES - 1)
    {
        swidx = Switch::n_switches;
        Switch::switches[swidx].current_state |= DCC_SWITCH_STATE_UNDEFINED;
        Switch::n_switches++;
        Switch::data_changed = true;
    }
    else
    {
        swidx = 0xFFFF;
    }

    return swidx;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  setid () - set new id
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast16_t
Switch::setid (uint_fast16_t swidx, uint_fast16_t new_swidx)
{
    uint_fast16_t   rtc = 0xFFFF;

    if (swidx < Switch::n_switches && new_swidx < Switch::n_switches)
    {
        if (swidx != new_swidx)
        {
            SWITCH          tmpswitch;
            uint_fast16_t   sidx;

            uint16_t *      map_new_switch_idx = (uint16_t *) calloc (Switch::n_switches, sizeof (uint16_t));

            for (sidx = 0; sidx < Switch::n_switches; sidx++)
            {
                map_new_switch_idx[sidx] = sidx;
            }

            memcpy (&tmpswitch, Switch::switches + swidx, sizeof (SWITCH));

            if (new_swidx < swidx)
            {
                for (sidx = swidx; sidx > new_swidx; sidx--)
                {
                    memcpy (Switch::switches + sidx, Switch::switches + sidx - 1, sizeof (SWITCH));
                    map_new_switch_idx[sidx - 1] = sidx;
                }
            }
            else // if (new_swidx > swidx)
            {
                for (sidx = swidx; sidx < new_swidx; sidx++)
                {
                    memcpy (Switch::switches + sidx, Switch::switches + sidx + 1, sizeof (SWITCH));
                    map_new_switch_idx[sidx + 1] = sidx;
                }
            }

            memcpy (Switch::switches + sidx, &tmpswitch, sizeof (SWITCH));
            map_new_switch_idx[swidx] = sidx;

            for (sidx = 0; sidx < Switch::n_switches; sidx++)
            {
                Debug::printf (DEBUG_LEVEL_NORMAL, "%2d -> %2d\n", sidx, map_new_switch_idx[sidx]);
            }

            Railroad::set_new_switch_ids (map_new_switch_idx, Switch::n_switches);

            free (map_new_switch_idx);
            Switch::data_changed = true;
            rtc = new_swidx;
        }
        else
        {
            rtc = swidx;
        }
    }

    return rtc;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  Switch::get_n_switches () - get number of switches
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast16_t
Switch::get_n_switches (void)
{
    return Switch::n_switches;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  Switch::set_name (swidx) - set name
 *------------------------------------------------------------------------------------------------------------------------
 */
void
Switch::set_name (uint_fast16_t swidx, const char * name)
{
    int l = strlen (name);

    if (Switch::switches[swidx].name)
    {
        if ((int) strlen (Switch::switches[swidx].name) < l)
        {
            free (Switch::switches[swidx].name);
            Switch::switches[swidx].name = (char *) NULL;
        }
    }

    if (! Switch::switches[swidx].name)
    {
        Switch::switches[swidx].name = (char *) malloc (l + 1);
    }

    strcpy (Switch::switches[swidx].name, name);
    Switch::data_changed = true;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  Switch::get_name (swidx) - get name
 *------------------------------------------------------------------------------------------------------------------------
 */
char *
Switch::get_name (uint_fast16_t swidx)
{
    return Switch::switches[swidx].name;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  Switch::set_addr (swidx, address) - set address
 *------------------------------------------------------------------------------------------------------------------------
 */
void
Switch::set_addr (uint_fast16_t swidx, uint_fast16_t addr)
{
    Switch::switches[swidx].addr = addr;
    Switch::data_changed = true;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  Switch::get_addr (swidx) - get address
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast16_t
Switch::get_addr (uint_fast16_t swidx)
{
    return (Switch::switches[swidx].addr);
}

/*------------------------------------------------------------------------------------------------------------------------
 *  Switch::set_state (swidx, f) - set state
 *------------------------------------------------------------------------------------------------------------------------
 */
void
Switch::set_state (uint_fast16_t swidx, uint_fast8_t f)
{
    if (swidx < Switch::n_switches)
    {
        if (Switch::switches[swidx].current_state != f)
        {
            Switch::switches[swidx].current_state = f;

            if (f == DCC_SWITCH_STATE_BRANCH ||
                (f == DCC_SWITCH_STATE_BRANCH2 && (Switch::switches[swidx].flags & SWITCH_FLAG_3WAY)) ||
                f == DCC_SWITCH_STATE_STRAIGHT)
            {
                Switch::add_event (swidx, f | SWITCH_ACTIVE_MASK);
                Switch::add_event (swidx, f);
            }
        }
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 *  Switch::get_state (idx, f)
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
Switch::get_state (uint_fast16_t swidx)
{
    if (swidx < Switch::n_switches)
    {
        return Switch::switches[swidx].current_state;
    }

    return 0;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  Switch::set_flags (swidx, f) - set state
 *------------------------------------------------------------------------------------------------------------------------
 */
void
Switch::set_flags (uint_fast16_t swidx, uint_fast8_t f)
{
    if (swidx < Switch::n_switches)
    {
        Switch::switches[swidx].flags = f;
        Switch::data_changed = true;
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 *  Switch::get_flags (idx, f)
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
Switch::get_flags (uint_fast16_t swidx)
{
    if (swidx < Switch::n_switches)
    {
        return Switch::switches[swidx].flags;
    }

    return 0;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  Switch::remove (idx)
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
Switch::remove (uint_fast16_t swidx)
{
    uint_fast16_t   idx;
    char *          savename;
    uint_fast8_t    rtc = 0;

    if (swidx < Switch::n_switches)
    {
        Switch::n_switches--;

        savename = Switch::switches[swidx].name;

        for (idx = swidx; idx < Switch::n_switches; idx++)
        {
            memcpy (switches + idx, switches + (idx + 1), sizeof (SWITCH));
        }

        if (swidx < Switch::n_switches)                     // it's not the last entry
        {
            Switch::switches[Switch::n_switches].name = savename;
        }

        Switch::data_changed = true;
        rtc = 1;
    }

    return rtc;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  Switch::booster_on ()
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
Switch::booster_on (void)
{
    uint_fast16_t swidx;

    for (swidx = 0; swidx < Switch::n_switches; swidx++)
    {
        Switch::switches[swidx].current_state |= DCC_SWITCH_STATE_UNDEFINED;
    }

    return 0;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  Switch::booster_on ()
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
Switch::booster_off (void)
{
    uint_fast16_t swidx;

    for (swidx = 0; swidx < Switch::n_switches; swidx++)
    {
        Switch::switches[swidx].current_state |= DCC_SWITCH_STATE_UNDEFINED;
    }
    return 0;
}
