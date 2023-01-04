/*------------------------------------------------------------------------------------------------------------------------
 * addon.cc - addon management functions
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
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include "func.h"
#include "dcc.h"
#include "millis.h"
#include "debug.h"
#include "event.h"
#include "railroad.h"
#include "loco.h"
#include "addon.h"
#include "rcl.h"
#include "s88.h"

#define MAX_PACKET_SEQUENCES    10

bool                    AddOn::data_changed = false;
ADDON                   AddOn::addons[MAX_ADDONS];                                  // addon
uint_fast16_t           AddOn::n_addons = 0;                                          // number of addons

/*------------------------------------------------------------------------------------------------------------------------
 * sendfunction() - send function
 *------------------------------------------------------------------------------------------------------------------------
 */
void
AddOn::sendfunction (uint_fast16_t addon_idx, uint_fast8_t range)
{
    if (addon_idx < AddOn::n_addons)
    {
        uint16_t    addr            = AddOn::addons[addon_idx].addr;
        uint32_t    functions       = AddOn::addons[addon_idx].functions;

        DCC::loco_function (0xFFFF, addr, functions, range);
        Debug::printf (DEBUG_LEVEL_VERBOSE, "AddOn::sendfunction: addon_idx=%d range %d\n", addon_idx, range);
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 *  sendcmd (idx, packet_no) - send addon command
 *
 *  Packet sequences:
 *      0       send direction/speed
 *      1       send F0-F4
 *      2       send direction/speed
 *      3       send F5-F8
 *      4       send direction/speed
 *      5       send F9-F12
 *      6       send direction/speed
 *      7       send F13-F20
 *      8       send direction/speed
 *      9       send F21-F28
 *------------------------------------------------------------------------------------------------------------------------
 */
void
AddOn::sendcmd (uint_fast16_t addon_idx, uint_fast8_t packet_no)
{
    if (addon_idx < AddOn::n_addons)
    {
        if (packet_no & 0x01)           // odd packet number: send functions
        {
            switch (packet_no)
            {
                case 1:
                {
                    AddOn::sendfunction (addon_idx, DCC_F00_F04_RANGE);
                    Debug::printf (DEBUG_LEVEL_VERBOSE, "sendfunction (%d, DCC_F00_F04_RANGE)\n", addon_idx);
                    break;
                }
                case 3:
                {
                    if (AddOn::addons[addon_idx].addonfunction.max >= 5)
                    {
                        AddOn::sendfunction (addon_idx, DCC_F05_F08_RANGE);
                        Debug::printf (DEBUG_LEVEL_VERBOSE, "sendfunction (%d, DCC_F05_F08_RANGE)\n", addon_idx);
                    }
                    break;
                }
                case 5:
                {
                    if (AddOn::addons[addon_idx].addonfunction.max >= 9)
                    {
                        AddOn::sendfunction (addon_idx, DCC_F09_F12_RANGE);
                        Debug::printf (DEBUG_LEVEL_VERBOSE, "sendfunction (%d, DCC_F09_F12_RANGE)\n", addon_idx);
                    }
                    break;
                }
                case 7:
                {
                    if (AddOn::addons[addon_idx].addonfunction.max >= 13)
                    {
                        AddOn::sendfunction (addon_idx, DCC_F13_F20_RANGE);
                        Debug::printf (DEBUG_LEVEL_VERBOSE, "sendfunction (%d, DCC_F13_F20_RANGE)\n", addon_idx);
                    }
                    break;
                }
                case 9:
                {
                    if (AddOn::addons[addon_idx].addonfunction.max >= 21)
                    {
                        AddOn::sendfunction (addon_idx, DCC_F21_F28_RANGE);
                        Debug::printf (DEBUG_LEVEL_VERBOSE, "sendfunction (%d, DCC_F21_F28_RANGE)\n", addon_idx);
                    }
                    break;
                }
            }
        }
        else // even packet number: send speed/direction
        {
            uint_fast16_t   loco_idx = AddOn::addons[addon_idx].loco_idx;

            if (loco_idx != 0xFFFF)
            {
                uint_fast16_t   addr    = AddOn::addons[addon_idx].addr;            // use addr of addon
                uint_fast8_t    fwd     = Loco::get_fwd (loco_idx);                 // use speed and dir of loco!
                uint_fast8_t    speed   = Loco::get_speed (loco_idx);               // use speed and dir of loco!

                DCC::loco (0xFFFF, addr, fwd, speed, 0, 0);
                Debug::printf (DEBUG_LEVEL_VERBOSE, "AddOn::schedule: addon_idx=%d loco_idx=%d addr=%d, fwd=%d, speed=%d\n", addon_idx, loco_idx, addr, fwd, speed);
            }
        }
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 *  add () - add a addon
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast16_t
AddOn::add (void)
{
    uint_fast16_t addon_idx = AddOn::n_addons;

    if (addon_idx < MAX_ADDONS)
    {
        uint_fast8_t    fidx;
        AddOn::addons[addon_idx].functions  = 0;
        AddOn::addons[addon_idx].loco_idx   = 0xFFFF;

        for (fidx = 0; fidx < MAX_LOCO_FUNCTIONS; fidx++)
        {
            AddOn::addons[addon_idx].addonfunction.name_idx[fidx] = 0xFFFF;
        }

        AddOn::n_addons++;
        AddOn::data_changed = true;
    }
    else
    {
        addon_idx = 0xFFFF;
    }

    return addon_idx;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  setid () - set new id
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast16_t
AddOn::setid (uint_fast16_t addon_idx, uint_fast16_t new_addon_idx)
{
    uint_fast16_t   rtc = 0xFFFF;

    if (addon_idx < AddOn::n_addons && new_addon_idx < AddOn::n_addons)
    {
        if (addon_idx != new_addon_idx)
        {
            ADDON            tmpaddon;
            uint_fast16_t   lidx;

            uint16_t *      map_new_addon_idx = (uint16_t *) calloc (AddOn::n_addons, sizeof (uint16_t));

            for (lidx = 0; lidx < AddOn::n_addons; lidx++)
            {
                map_new_addon_idx[lidx] = lidx;
            }

            memcpy (&tmpaddon, AddOn::addons + addon_idx, sizeof (ADDON));

            if (new_addon_idx < addon_idx)
            {
                // step 1: shift addon
                for (lidx = addon_idx; lidx > new_addon_idx; lidx--)
                {
                    memcpy (AddOn::addons + lidx, AddOn::addons + lidx - 1, sizeof (ADDON));
                    map_new_addon_idx[lidx - 1] = lidx;
                }
            }
            else // if (new_addon_idx > addon_idx)
            {
                // step 1: shift addon
                for (lidx = addon_idx; lidx < new_addon_idx; lidx++)
                {
                    memcpy (AddOn::addons + lidx, AddOn::addons + lidx + 1, sizeof (ADDON));
                    map_new_addon_idx[lidx + 1] = lidx;
                }
            }

            memcpy (AddOn::addons + lidx, &tmpaddon, sizeof (ADDON));
            map_new_addon_idx[addon_idx] = lidx;

            for (lidx = 0; lidx < AddOn::n_addons; lidx++)
            {
                Debug::printf (DEBUG_LEVEL_NORMAL, "%2d -> %2d\n", lidx, map_new_addon_idx[lidx]);
            }

            // step 2: correct loco_idx in LOCO, RAILROAD, RCL, S88
            Loco::set_new_addon_ids (map_new_addon_idx, AddOn::n_addons);
            // Railroad::set_new_addon_ids (map_new_addon_idx, AddOn::n_addons);    // not necessary
            RCL::set_new_addon_ids (map_new_addon_idx, AddOn::n_addons);
            S88::set_new_addon_ids (map_new_addon_idx, AddOn::n_addons);

            free (map_new_addon_idx);
            AddOn::data_changed = true;
            rtc = new_addon_idx;
        }
        else
        {
            rtc = addon_idx;
        }
    }

    return rtc;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  AddOn::set_new_loco_ids ()
 *------------------------------------------------------------------------------------------------------------------------
 */
void
AddOn::set_new_loco_ids (uint16_t * map_new_loco_idx, uint16_t n_locos)
{
    uint_fast8_t     addon_idx;

    for (addon_idx = 0; addon_idx < AddOn::n_addons; addon_idx++)
    {
        if (AddOn::addons[addon_idx].loco_idx < n_locos)
        {
            AddOn::addons[addon_idx].loco_idx = map_new_loco_idx[AddOn::addons[addon_idx].loco_idx];
        }
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 *  get_n_addons () - get number of addons
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast16_t
AddOn::get_n_addons (void)
{
    return AddOn::n_addons;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  set_loco () - set loco index
 *------------------------------------------------------------------------------------------------------------------------
 */
void
AddOn::set_loco (uint_fast16_t addon_idx, uint_fast16_t loco_idx)
{
    uint_fast16_t n_locos = Loco::get_n_locos ();

    if (addon_idx < AddOn::n_addons && loco_idx < n_locos)
    {
        uint_fast16_t   aidx;

        for (aidx = 0; aidx < AddOn::n_addons; aidx++)                              // first remove loco from other addons
        {
            if (AddOn::addons[aidx].loco_idx == loco_idx)
            {
                if (aidx != addon_idx)
                {
                    AddOn::addons[aidx].loco_idx = 0xFFFF;
                    AddOn::data_changed = true;
                }
            }
        }

        if (AddOn::addons[addon_idx].loco_idx != loco_idx)
        {
            AddOn::addons[addon_idx].loco_idx = loco_idx;
            AddOn::data_changed = true;
        }
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 *  reset_loco () - delete loco index
 *------------------------------------------------------------------------------------------------------------------------
 */
void
AddOn::reset_loco (uint_fast16_t addon_idx)
{
    if (addon_idx < AddOn::n_addons)
    {
        if (AddOn::addons[addon_idx].loco_idx != 0xFFFF)
        {
            AddOn::addons[addon_idx].loco_idx = 0xFFFF;
            AddOn::data_changed = true;
        }
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 *  get_loco () - get loco index
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast16_t
AddOn::get_loco (uint_fast16_t addon_idx)
{
    uint_fast16_t   rtc;

    if (addon_idx < AddOn::n_addons)
    {
        rtc = AddOn::addons[addon_idx].loco_idx;
    }
    else
    {
        rtc = 0xFFFF;
    }

    return rtc;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  set_name (idx) - set name
 *------------------------------------------------------------------------------------------------------------------------
 */
void
AddOn::set_name (uint_fast16_t addon_idx, const char * name)
{
    if (addon_idx < AddOn::n_addons)
    {
        int l = strlen (name);

        if (! AddOn::addons[addon_idx].name || strcmp (AddOn::addons[addon_idx].name, name) != 0)
        {
            if (AddOn::addons[addon_idx].name)
            {
                if ((int) strlen (AddOn::addons[addon_idx].name) < l)
                {
                    free (AddOn::addons[addon_idx].name);
                    AddOn::addons[addon_idx].name = (char *) NULL;
                }
            }

            if (! AddOn::addons[addon_idx].name)
            {
                AddOn::addons[addon_idx].name = (char *) malloc (l + 1);
            }

            strcpy (AddOn::addons[addon_idx].name, name);
            AddOn::data_changed = true;
        }
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 *  get_name (idx) - get name
 *------------------------------------------------------------------------------------------------------------------------
 */
char *
AddOn::get_name (uint_fast16_t addon_idx)
{
    if (addon_idx < AddOn::n_addons)
    {
        return AddOn::addons[addon_idx].name;
    }
    return (char *) NULL;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  set_addr (idx, address) - set address
 *------------------------------------------------------------------------------------------------------------------------
 */
void
AddOn::set_addr (uint_fast16_t addon_idx, uint_fast16_t addr)
{
    if (addon_idx < AddOn::n_addons)
    {
        if (AddOn::addons[addon_idx].addr != addr)
        {
            AddOn::addons[addon_idx].addr = addr;
            AddOn::data_changed = true;
        }
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 *  get_addr (idx) - get address
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast16_t
AddOn::get_addr (uint_fast16_t addon_idx)
{
    if (addon_idx < AddOn::n_addons)
    {
        return (AddOn::addons[addon_idx].addr);
    }

    return 0;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  set_function_type_pulse (idx, f, b) - set/reset function to type "pulse"
 *------------------------------------------------------------------------------------------------------------------------
 */
void
AddOn::set_function_type_pulse (uint_fast16_t addon_idx, uint_fast8_t f, bool b)
{
    if (addon_idx < AddOn::n_addons)
    {
        uint32_t    f_mask = 1 << f;

        if (b)
        {
            AddOn::addons[addon_idx].addonfunction.pulse_mask |= f_mask;
        }
        else
        {
            AddOn::addons[addon_idx].addonfunction.pulse_mask &= ~f_mask;
        }

        AddOn::data_changed = true;
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 *  set_function_type_sound (idx, f, b) - set/reset function to type "sound"
 *------------------------------------------------------------------------------------------------------------------------
 */
void
AddOn::set_function_type_sound (uint_fast16_t addon_idx, uint_fast8_t f, bool b)
{
    if (addon_idx < AddOn::n_addons)
    {
        uint32_t    f_mask = 1 << f;

        if (b)
        {
            AddOn::addons[addon_idx].addonfunction.sound_mask |= f_mask;
        }
        else
        {
            AddOn::addons[addon_idx].addonfunction.sound_mask &= ~f_mask;
        }

        AddOn::data_changed = true;
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 *  set_function_type (idx, fidx, name) - set function type
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
AddOn::set_function_type (uint_fast16_t addon_idx, uint_fast8_t fidx, uint_fast16_t function_name_idx, bool pulse, bool sound)
{
    if (addon_idx < AddOn::n_addons)
    {
        if (AddOn::addons[addon_idx].addonfunction.max < fidx)
        {
            AddOn::addons[addon_idx].addonfunction.max = fidx;
        }

        AddOn::addons[addon_idx].addonfunction.name_idx[fidx] = function_name_idx;
        AddOn::set_function_type_pulse (addon_idx, fidx, pulse);
        AddOn::set_function_type_sound (addon_idx, fidx, sound);
        return 1;
    }
    return 0;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  get_function_name () - get function name
 *------------------------------------------------------------------------------------------------------------------------
 */
std::string&
AddOn::get_function_name (uint_fast16_t addon_idx, uint_fast8_t fidx)
{
    if (addon_idx < AddOn::n_addons)
    {
        uint_fast16_t    function_name_idx = AddOn::addons[addon_idx].addonfunction.name_idx[fidx];
        return Functions::get (function_name_idx);
    }
    return Functions::get (0xFFFF);
}

/*------------------------------------------------------------------------------------------------------------------------
 *  get_function_name_idx (idx, f) - get function name index
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast16_t
AddOn::get_function_name_idx (uint_fast16_t addon_idx, uint_fast8_t fidx)
{
    if (addon_idx < AddOn::n_addons)
    {
        uint_fast16_t    function_name_idx = AddOn::addons[addon_idx].addonfunction.name_idx[fidx];
        return function_name_idx;
    }
    return 0;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  get_function_pulse (idx, f) - get function of type "pulse"
 *------------------------------------------------------------------------------------------------------------------------
 */
bool
AddOn::get_function_pulse (uint_fast16_t addon_idx, uint_fast8_t fidx)
{
    if (addon_idx < AddOn::n_addons)
    {
        uint32_t        f_mask = 1 << fidx;
        bool            b = false;

        if (AddOn::addons[addon_idx].addonfunction.pulse_mask & f_mask)
        {
            b = true;
        }
        return b;
    }
    return false;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  get_function_pulse_mask (idx, f) - get complete function of type pulse as mask
 *------------------------------------------------------------------------------------------------------------------------
 */
uint32_t
AddOn::get_function_pulse_mask (uint_fast16_t addon_idx)
{
    if (addon_idx < AddOn::n_addons)
    {
        return AddOn::addons[addon_idx].addonfunction.pulse_mask;
    }
    return 0;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  get_function_sound (idx, f) - get function of type "sound"
 *------------------------------------------------------------------------------------------------------------------------
 */
bool
AddOn::get_function_sound (uint_fast16_t addon_idx, uint_fast8_t fidx)
{
    if (addon_idx < AddOn::n_addons)
    {
        uint32_t        f_mask = 1 << fidx;
        bool            b = false;

        if (AddOn::addons[addon_idx].addonfunction.sound_mask & f_mask)
        {
            b = true;
        }
        return b;
    }
    return false;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  get_function_sound_mask (idx, f) - get complete function of type sound as mask
 *------------------------------------------------------------------------------------------------------------------------
 */
uint32_t
AddOn::get_function_sound_mask (uint_fast16_t addon_idx)
{
    if (addon_idx < AddOn::n_addons)
    {
        return AddOn::addons[addon_idx].addonfunction.sound_mask;
    }
    return 0;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  activate (idx) - activate a addon
 *------------------------------------------------------------------------------------------------------------------------
 */
void
AddOn::activate (uint_fast16_t addon_idx)
{
    if (addon_idx < AddOn::n_addons)
    {
        if (! AddOn::addons[addon_idx].active)
        {
            AddOn::addons[addon_idx].active = true;
        }
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 *  deactivate_addon (idx) - deactivate a addon
 *------------------------------------------------------------------------------------------------------------------------
 */
void
AddOn::deactivate (uint_fast16_t addon_idx)
{
    if (addon_idx < AddOn::n_addons)
    {
        if (AddOn::addons[addon_idx].active)
        {
            AddOn::addons[addon_idx].active = false;
        }
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 *  is_active (idx) - check if addon is active
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
AddOn::is_active (uint_fast16_t addon_idx)
{
    if (addon_idx < AddOn::n_addons && AddOn::addons[addon_idx].active)
    {
        return true;
    }
    return false;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  set_function (idx, f) - set a function
 *------------------------------------------------------------------------------------------------------------------------
 */
void
AddOn::set_function (uint_fast16_t addon_idx, uint_fast8_t f, bool b)
{
    if (addon_idx < AddOn::n_addons)
    {
        uint32_t        fmask = 1 << f;
        uint_fast8_t    range;

        if (b)
        {
            AddOn::addons[addon_idx].functions |= fmask;

            if (fmask & AddOn::addons[addon_idx].addonfunction.pulse_mask)      // function is of type pulse
            {
                Event::add_event_addon_function (2, addon_idx, f, false);       // reset function in 200 msec
            }
        }
        else
        {
            AddOn::addons[addon_idx].functions &= ~fmask;
        }

        if (f <= 4)
        {
            range = DCC_F00_F04_RANGE;
        }
        else if (f <= 8)
        {
            range = DCC_F05_F08_RANGE;
        }
        else if (f <= 12)
        {
            range = DCC_F09_F12_RANGE;
        }
        else if (f <= 20)
        {
            range = DCC_F13_F20_RANGE;
        }
        else if (f <= 28)
        {
            range = DCC_F21_F28_RANGE;
        }
        else
        {
            range = DCC_F29_F36_RANGE;
        }

        AddOn::sendfunction (addon_idx, range);
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 *  get_function (idx, f)
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
AddOn::get_function (uint_fast16_t addon_idx, uint_fast8_t f)
{
    if (addon_idx < AddOn::n_addons)
    {
        uint32_t mask = 1 << f;
        return (AddOn::addons[addon_idx].functions & mask) ? true : false;
    }
    return false;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  reset_functions (idx)
 *------------------------------------------------------------------------------------------------------------------------
 */
void
AddOn::reset_functions (uint_fast16_t addon_idx)
{
    if (addon_idx < AddOn::n_addons)
    {
        AddOn::addons[addon_idx].functions = 0;       // let scheduler turn off functions
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 *  get_functions (idx, f)
 *------------------------------------------------------------------------------------------------------------------------
 */
uint32_t
AddOn::get_functions (uint_fast16_t addon_idx)
{
    if (addon_idx < AddOn::n_addons)
    {
        return AddOn::addons[addon_idx].functions;
    }
    return 0;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  schedule () - schedule all addons
 *  called by loco.cc, not main.cc!
 *  send rate: 25%
 *------------------------------------------------------------------------------------------------------------------------
 */
bool
AddOn::schedule (void)
{
    static uint_fast16_t    addon_idx   = 0;
    static uint_fast8_t     cnt         = 0;
    bool                    rtc         = false;

    if (cnt == 0)
    {
        if (addon_idx < AddOn::n_addons)
        {
            if (AddOn::addons[addon_idx].active)
            {
                uint_fast8_t packet_sequence_idx = AddOn::addons[addon_idx].packet_sequence_idx;

                AddOn::sendcmd (addon_idx, packet_sequence_idx);
                packet_sequence_idx++;

                if (packet_sequence_idx >= MAX_PACKET_SEQUENCES)
                {
                    packet_sequence_idx = 0;
                }

                AddOn::addons[addon_idx].packet_sequence_idx = packet_sequence_idx;
            }

            addon_idx++;

            if (addon_idx >= AddOn::n_addons)
            {
                addon_idx = 0;
                rtc = true;
            }
        }
        else
        {
            rtc = true;
        }

        cnt++;
    }
    else
    {
        rtc = true;

        cnt++;

        if (cnt >= 4)
        {
            cnt = 0;
        }
    }

    return rtc;
}
