/*------------------------------------------------------------------------------------------------------------------------
 * addon.cc - addon management functions
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

std::vector<AddOn>      AddOns::addons;                                         // addons
bool                    AddOns::data_changed = false;                           // flag
uint_fast16_t           AddOns::n_addons = 0;                                   // number of addons

/*------------------------------------------------------------------------------------------------------------------------
 * sendfunction() - send function
 *------------------------------------------------------------------------------------------------------------------------
 */
void
AddOn::sendfunction (uint_fast8_t range)
{
    uint16_t    addr            = this->addr;
    uint32_t    functions       = this->functions;

    DCC::loco_function (0xFFFF, addr, functions, range);
    Debug::printf (DEBUG_LEVEL_VERBOSE, "AddOn::sendfunction: addon_idx=%d range %d\n", this->id, range);
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
AddOn::sendcmd (uint_fast8_t packet_no)
{
    if (packet_no & 0x01)           // odd packet number: send functions
    {
        switch (packet_no)
        {
            case 1:
            {
                AddOn::sendfunction (DCC_F00_F04_RANGE);
                Debug::printf (DEBUG_LEVEL_VERBOSE, "sendfunction (%d, DCC_F00_F04_RANGE)\n", this->id);
                break;
            }
            case 3:
            {
                if (this->addonfunction.max >= 5)
                {
                    AddOn::sendfunction (DCC_F05_F08_RANGE);
                    Debug::printf (DEBUG_LEVEL_VERBOSE, "sendfunction (%d, DCC_F05_F08_RANGE)\n", this->id);
                }
                break;
            }
            case 5:
            {
                if (this->addonfunction.max >= 9)
                {
                    AddOn::sendfunction (DCC_F09_F12_RANGE);
                    Debug::printf (DEBUG_LEVEL_VERBOSE, "sendfunction (%d, DCC_F09_F12_RANGE)\n", this->id);
                }
                break;
            }
            case 7:
            {
                if (this->addonfunction.max >= 13)
                {
                    AddOn::sendfunction (DCC_F13_F20_RANGE);
                    Debug::printf (DEBUG_LEVEL_VERBOSE, "sendfunction (%d, DCC_F13_F20_RANGE)\n", this->id);
                }
                break;
            }
            case 9:
            {
                if (this->addonfunction.max >= 21)
                {
                    AddOn::sendfunction (DCC_F21_F28_RANGE);
                    Debug::printf (DEBUG_LEVEL_VERBOSE, "sendfunction (%d, DCC_F21_F28_RANGE)\n", this->id);
                }
                break;
            }
        }
    }
    else // even packet number: send speed/direction
    {
        uint_fast16_t   loco_idx = this->loco_idx;

        if (loco_idx != 0xFFFF)
        {
            uint_fast16_t   addr    = this->addr;            // use addr of addon
            uint_fast8_t    fwd     = Locos::locos[loco_idx].get_fwd ();        // use speed and dir of loco!
            uint_fast8_t    speed   = Locos::locos[loco_idx].get_speed ();      // use speed and dir of loco!

            DCC::loco (0xFFFF, addr, fwd, speed, 0, 0);
            Debug::printf (DEBUG_LEVEL_VERBOSE, "AddOn::sendcmd: addon_idx=%d loco_idx=%d addr=%d, fwd=%d, speed=%d\n", this->id, loco_idx, addr, fwd, speed);
        }
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 *  AddOn () - constructor
 *------------------------------------------------------------------------------------------------------------------------
 */
AddOn::AddOn ()
{
    uint_fast8_t    fidx;

    this->name                  = "";
    this->addr                  = 0;
    this->loco_idx              = 0xFFFF;
    this->functions             = 0;
    this->packet_sequence_idx   = 0;
    this->active                = 0;

    for (fidx = 0; fidx < MAX_LOCO_FUNCTIONS; fidx++)
    {
        this->addonfunction.name_idx[fidx] = 0xFFFF;
    }

    AddOns::data_changed = true;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  set_id () - set id
 *------------------------------------------------------------------------------------------------------------------------
 */
void
AddOn::set_id (uint_fast16_t id)
{
    this->id = id;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  set_loco () - set loco index
 *------------------------------------------------------------------------------------------------------------------------
 */
void
AddOn::set_loco (uint_fast16_t loco_idx)
{
    uint_fast16_t n_locos = Locos::get_n_locos ();

    if (loco_idx < n_locos)
    {
        uint_fast16_t   aidx;

        for (aidx = 0; aidx < AddOns::get_n_addons(); aidx++)                              // first remove loco from other addons
        {
            if (AddOns::addons[aidx].loco_idx == loco_idx)
            {
                if (aidx != this->id)
                {
                    AddOns::addons[aidx].loco_idx = 0xFFFF;
                    AddOns::data_changed = true;
                }
            }
        }

        if (this->loco_idx != loco_idx)
        {
            this->loco_idx = loco_idx;
            AddOns::data_changed = true;
        }
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 *  reset_loco () - delete loco index
 *------------------------------------------------------------------------------------------------------------------------
 */
void
AddOn::reset_loco ()
{
    if (this->loco_idx != 0xFFFF)
    {
        this->loco_idx = 0xFFFF;
        AddOns::data_changed = true;
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 *  get_loco () - get loco index
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast16_t
AddOn::get_loco ()
{
    uint_fast16_t   rtc;
    rtc = this->loco_idx;
    return rtc;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  set_name (idx) - set name
 *------------------------------------------------------------------------------------------------------------------------
 */
void
AddOn::set_name (std::string name)
{
    this->name = name;
    AddOns::data_changed = true;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  get_name (idx) - get name
 *------------------------------------------------------------------------------------------------------------------------
 */
std::string
AddOn::get_name ()
{
    return this->name;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  set_addr (idx, address) - set address
 *------------------------------------------------------------------------------------------------------------------------
 */
void
AddOn::set_addr (uint_fast16_t addr)
{
    if (this->addr != addr)
    {
        this->addr = addr;
        AddOns::data_changed = true;
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 *  get_addr (idx) - get address
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast16_t
AddOn::get_addr ()
{
    return (this->addr);
}

/*------------------------------------------------------------------------------------------------------------------------
 *  set_function_type_pulse (idx, f, b) - set/reset function to type "pulse"
 *------------------------------------------------------------------------------------------------------------------------
 */
void
AddOn::set_function_type_pulse (uint_fast8_t f, bool b)
{
    uint32_t    f_mask = 1 << f;

    if (b)
    {
        this->addonfunction.pulse_mask |= f_mask;
    }
    else
    {
        this->addonfunction.pulse_mask &= ~f_mask;
    }

    AddOns::data_changed = true;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  set_function_type_sound (idx, f, b) - set/reset function to type "sound"
 *------------------------------------------------------------------------------------------------------------------------
 */
void
AddOn::set_function_type_sound (uint_fast8_t f, bool b)
{
    uint32_t    f_mask = 1 << f;

    if (b)
    {
        this->addonfunction.sound_mask |= f_mask;
    }
    else
    {
        this->addonfunction.sound_mask &= ~f_mask;
    }

    AddOns::data_changed = true;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  set_function_type (idx, fidx, name) - set function type
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
AddOn::set_function_type (uint_fast8_t fidx, uint_fast16_t function_name_idx, bool pulse, bool sound)
{
    if (this->addonfunction.max < fidx)
    {
        this->addonfunction.max = fidx;
    }

    this->addonfunction.name_idx[fidx] = function_name_idx;
    AddOn::set_function_type_pulse (fidx, pulse);
    AddOn::set_function_type_sound (fidx, sound);
    return 1;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  get_function_name () - get function name
 *------------------------------------------------------------------------------------------------------------------------
 */
std::string&
AddOn::get_function_name (uint_fast8_t fidx)
{
    uint_fast16_t    function_name_idx = this->addonfunction.name_idx[fidx];
    return Functions::get (function_name_idx);
}

/*------------------------------------------------------------------------------------------------------------------------
 *  get_function_name_idx (idx, f) - get function name index
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast16_t
AddOn::get_function_name_idx (uint_fast8_t fidx)
{
    uint_fast16_t    function_name_idx = this->addonfunction.name_idx[fidx];
    return function_name_idx;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  get_function_pulse (idx, f) - get function of type "pulse"
 *------------------------------------------------------------------------------------------------------------------------
 */
bool
AddOn::get_function_pulse (uint_fast8_t fidx)
{
    uint32_t        f_mask = 1 << fidx;
    bool            b = false;

    if (this->addonfunction.pulse_mask & f_mask)
    {
        b = true;
    }
    return b;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  get_function_pulse_mask (idx, f) - get complete function of type pulse as mask
 *------------------------------------------------------------------------------------------------------------------------
 */
uint32_t
AddOn::get_function_pulse_mask ()
{
    return this->addonfunction.pulse_mask;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  get_function_sound (idx, f) - get function of type "sound"
 *------------------------------------------------------------------------------------------------------------------------
 */
bool
AddOn::get_function_sound (uint_fast8_t fidx)
{
    uint32_t        f_mask = 1 << fidx;
    bool            b = false;

    if (this->addonfunction.sound_mask & f_mask)
    {
        b = true;
    }
    return b;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  get_function_sound_mask (idx, f) - get complete function of type sound as mask
 *------------------------------------------------------------------------------------------------------------------------
 */
uint32_t
AddOn::get_function_sound_mask ()
{
    return this->addonfunction.sound_mask;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  activate (idx) - activate a addon
 *------------------------------------------------------------------------------------------------------------------------
 */
void
AddOn::activate ()
{
    if (! this->active)
    {
        this->active = true;
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 *  deactivate_addon (idx) - deactivate a addon
 *------------------------------------------------------------------------------------------------------------------------
 */
void
AddOn::deactivate ()
{
    if (this->active)
    {
        this->active = false;
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 *  is_active (idx) - check if addon is active
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
AddOn::is_active ()
{
    if (this->active)
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
AddOn::set_function (uint_fast8_t f, bool b)
{
    uint32_t        fmask = 1 << f;
    uint_fast8_t    range;

    if (b)
    {
        this->functions |= fmask;

        if (fmask & this->addonfunction.pulse_mask)      // function is of type pulse
        {
            Event::add_event_addon_function (2, this->id, f, false);       // reset function in 200 msec
        }
    }
    else
    {
        this->functions &= ~fmask;
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

    AddOn::sendfunction (range);
}

/*------------------------------------------------------------------------------------------------------------------------
 *  get_function (idx, f)
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
AddOn::get_function (uint_fast8_t f)
{
    uint32_t mask = 1 << f;
    return (this->functions & mask) ? true : false;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  reset_functions (idx)
 *------------------------------------------------------------------------------------------------------------------------
 */
void
AddOn::reset_functions ()
{
    this->functions = 0;       // let scheduler turn off functions
}

/*------------------------------------------------------------------------------------------------------------------------
 *  get_functions (idx, f)
 *------------------------------------------------------------------------------------------------------------------------
 */
uint32_t
AddOn::get_functions ()
{
    return this->functions;
}

void
AddOn::sched ()
{
    if (this->active && this->addr != 0)
    {
        uint_fast8_t packet_sequence_idx = this->packet_sequence_idx;

        this->sendcmd (packet_sequence_idx);
        packet_sequence_idx++;

        if (packet_sequence_idx >= MAX_PACKET_SEQUENCES)
        {
            packet_sequence_idx = 0;
        }

        this->packet_sequence_idx = packet_sequence_idx;
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 *  add () - add addon
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast16_t
AddOns::add (const AddOn& addon)
{
    if (AddOns::n_addons < MAX_ADDONS)
    {
        addons.push_back(addon);
        AddOns::addons[n_addons].set_id(n_addons);
        AddOns::n_addons++;
        return addons.size() - 1;
    }
    return 0xFFFF;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  get_n_addons () - get number of addons
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast16_t
AddOns::get_n_addons (void)
{
    return AddOns::n_addons;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  set_new_id () - set new id
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast16_t
AddOns::set_new_id (uint_fast16_t addon_idx, uint_fast16_t new_addon_idx)
{
    uint_fast16_t   rtc = 0xFFFF;

    if (addon_idx < AddOns::n_addons && new_addon_idx < AddOns::n_addons)
    {
        if (addon_idx != new_addon_idx)
        {
            AddOn           tmpaddon;
            uint_fast16_t   lidx;

            uint16_t *      map_new_addon_idx = (uint16_t *) calloc (AddOns::n_addons, sizeof (uint16_t));

            for (lidx = 0; lidx < AddOns::n_addons; lidx++)
            {
                map_new_addon_idx[lidx] = lidx;
            }

            tmpaddon = AddOns::addons[addon_idx];

            if (new_addon_idx < addon_idx)
            {
                // step 1: shift addon
                for (lidx = addon_idx; lidx > new_addon_idx; lidx--)
                {
                    AddOns::addons[lidx] = AddOns::addons[lidx - 1];
                    map_new_addon_idx[lidx - 1] = lidx;
                }
            }
            else // if (new_addon_idx > addon_idx)
            {
                // step 1: shift addon
                for (lidx = addon_idx; lidx < new_addon_idx; lidx++)
                {
                    AddOns::addons[lidx] = AddOns::addons[lidx + 1];
                    map_new_addon_idx[lidx + 1] = lidx;
                }
            }

            AddOns::addons[lidx] = tmpaddon;
            map_new_addon_idx[addon_idx] = lidx;

            for (lidx = 0; lidx < AddOns::n_addons; lidx++)
            {
                Debug::printf (DEBUG_LEVEL_VERBOSE, "%2d -> %2d\n", lidx, map_new_addon_idx[lidx]);
            }

            // step 2: correct loco_idx in LOCO, RAILROAD, RCL, S88
            Locos::set_new_addon_ids (map_new_addon_idx, AddOns::n_addons);
            // Railroad::set_new_addon_ids (map_new_addon_idx, AddOns::n_addons);    // not necessary
            RCL::set_new_addon_ids (map_new_addon_idx, AddOns::n_addons);
            S88::set_new_addon_ids (map_new_addon_idx, AddOns::n_addons);

            free (map_new_addon_idx);
            AddOns::data_changed = true;
            rtc = new_addon_idx;

            AddOns::renumber ();
        }
        else
        {
            rtc = addon_idx;
        }
    }

    return rtc;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  AddOns::set_new_loco_ids ()
 *------------------------------------------------------------------------------------------------------------------------
 */
void
AddOns::set_new_loco_ids (uint16_t * map_new_loco_idx, uint16_t n_locos)
{
    uint_fast16_t     addon_idx;

    for (addon_idx = 0; addon_idx < AddOns::n_addons; addon_idx++)
    {
        uint_fast16_t   loco_idx = AddOns::addons[addon_idx].get_loco(); 

        if (loco_idx < n_locos)
        {
            AddOns::addons[addon_idx].set_loco(map_new_loco_idx[loco_idx]);
        }
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 *  schedule () - schedule all addons
 *  called by loco.cc, not main.cc!
 *  send rate: 25%
 *------------------------------------------------------------------------------------------------------------------------
 */
bool
AddOns::schedule (void)
{
    static uint_fast16_t    addon_idx   = 0;
    static uint_fast8_t     cnt         = 0;
    bool                    rtc         = false;

    if (cnt == 0)
    {
        if (addon_idx < AddOns::n_addons)
        {
            AddOns::addons[addon_idx].sched();
            addon_idx++;

            if (addon_idx >= AddOns::n_addons)
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

/*------------------------------------------------------------------------------------------------------------------------
 *  renumber () - renumber ids
 *------------------------------------------------------------------------------------------------------------------------
 */
void
AddOns::renumber ()
{
    uint_fast16_t addon_idx;

    for (addon_idx = 0; addon_idx < AddOns::n_addons; addon_idx++)
    {
        AddOns::addons[addon_idx].set_id (addon_idx);
    }
}
