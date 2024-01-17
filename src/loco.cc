/*------------------------------------------------------------------------------------------------------------------------
 * loco.cc - loco management functions
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

#include <cstdint>
#include <cstring>
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

bool                    Locos::data_changed = false;                                // flag: data changed, public
std::vector<Loco>       Locos::locos;                                               // locos array, public
uint_fast16_t           Locos::n_locos = 0;                                         // number of locos, private

/*------------------------------------------------------------------------------------------------------------------------
 * sendspeed() - send speed
 *------------------------------------------------------------------------------------------------------------------------
 */
void
Loco::sendspeed ()
{
    uint16_t        addr            = this->addr;
    uint_fast8_t    speed_steps     = this->speed_steps;
    uint_fast8_t    fwd             = this->fwd;
    uint_fast8_t    speed           = this->speed;

    // TODO: speed_steps == 14
    if (speed_steps == 28)
    {
        speed /= 4;

        if (speed == 1 || speed == 2)
        {
            speed = 3;
        }
        else if (speed > 31)
        {
            speed = 31;
        }

        DCC::loco_28 (this->id, addr, fwd, speed);
        Debug::printf (DEBUG_LEVEL_VERBOSE, "Loco::sendspeed: loco_idx=%d send speed_28: %d\n", this->id, speed);
    }
    else // if (speed_steps == 128)
    {
        DCC::loco (this->id, addr, fwd, speed, 0, 0);
        Debug::printf (DEBUG_LEVEL_VERBOSE, "Loco::sendspeed: loco_idx=%d send speed_128: %d\n", this->id, speed);
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 * sendfunction() - send function
 *------------------------------------------------------------------------------------------------------------------------
 */
void
Loco::sendfunction (uint_fast8_t range)
{
    uint16_t    addr            = this->addr;
    uint32_t    functions       = this->functions;

    DCC::loco_function (this->id, addr, functions, range);
    Debug::printf (DEBUG_LEVEL_VERBOSE, "Loco::sendfunction: loco_idx=%d range %d\n", this->id, range);
}

/*------------------------------------------------------------------------------------------------------------------------
 * sendcmd () - send loco command
 *
 *  Packet sequences:
 *      0       send speed
 *      1       send F0-F4
 *      2       send speed
 *      3       send F5-F8
 *      4       send speed
 *      5       send F9-F12
 *      6       send speed
 *      7       send F13-F20
 *      8       send speed
 *      9       send F21-F28
 *------------------------------------------------------------------------------------------------------------------------
 */
void
Loco::sendcmd (uint_fast8_t packet_no)
{
    uint32_t        target_next_millis  = this->target_next_millis;

    if (target_next_millis > 0 && Millis::elapsed() >= target_next_millis)
    {
        uint32_t    millis = Millis::elapsed();

        uint_fast8_t    speed           = this->speed;
        uint_fast8_t    tspeed          = this->target_speed;

        if (tspeed == speed)
        {
            this->target_next_millis = 0;
        }
        else
        {
            if (tspeed > speed)
            {
                while (this->target_next_millis < millis)
                {
                    this->target_next_millis += this->target_millis_step;

                    speed++;

                    if (speed == tspeed)
                    {
                        break;
                    }

                    if (speed == 1)
                    {
                        speed++;

                        if (speed == tspeed)
                        {
                            break;
                        }
                    }
                }

                this->speed = speed;
            }
            else
            {
                while (this->target_next_millis < millis)
                {
                    this->target_next_millis += this->target_millis_step;

                    speed--;

                    if (speed == tspeed)
                    {
                        break;
                    }

                    if (speed == 1)
                    {
                        speed--;

                        if (speed == tspeed)
                        {
                            break;
                        }
                    }

                }

                this->speed = speed;
            }
        }
    }

#if 0

#define LOCO_SLEEP_CNT_MAX      256

    if (this->target_next_millis == 0 && this->speed == 0)
    {
        if (this->packet_sequence_idx == 0)
        {
            if (this->sleep_cnt < LOCO_SLEEP_CNT_MAX)
            {
                this->sleep_cnt++;
            }
            else
            {
                this->sleep_cnt = 0;
            }
        }
    }
    else
    {
        this->sleep_cnt = 0;
    }

#endif

    if (this->sleep_cnt < 4 || (this->sleep_cnt % 4) == 0)
    {
        if (packet_no & 0x01)           // odd packet number: send functions
        {
            switch (packet_no)
            {
                case 1:
                {
                    this->sendfunction (DCC_F00_F04_RANGE);
                    Debug::printf (DEBUG_LEVEL_VERBOSE, "sendfunction (%d, DCC_F00_F04_RANGE)\n", this->id);
                    break;
                }
                case 3:
                {
                    if (this->locofunction.max >= 5)
                    {
                        this->sendfunction (DCC_F05_F08_RANGE);
                        Debug::printf (DEBUG_LEVEL_VERBOSE, "sendfunction (%d, DCC_F05_F08_RANGE)\n", this->id);
                    }
                    break;
                }
                case 5:
                {
                    if (this->locofunction.max >= 9)
                    {
                        this->sendfunction (DCC_F09_F12_RANGE);
                        Debug::printf (DEBUG_LEVEL_VERBOSE, "sendfunction (%d, DCC_F09_F12_RANGE)\n", this->id);
                    }
                    break;
                }
                case 7:
                {
                    if (this->locofunction.max >= 13)
                    {
                        this->sendfunction (DCC_F13_F20_RANGE);
                        Debug::printf (DEBUG_LEVEL_VERBOSE, "sendfunction (%d, DCC_F13_F20_RANGE)\n", this->id);
                    }
                    break;
                }
                case 9:
                {
                    if (this->locofunction.max >= 21)
                    {
                        this->sendfunction (DCC_F21_F28_RANGE);
                        Debug::printf (DEBUG_LEVEL_VERBOSE, "sendfunction (%d, DCC_F21_F28_RANGE)\n", this->id);
                    }
                    break;
                }
            }
        }
        else                        // even packet number: send speed
        {
            this->sendspeed ();
        }
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 *  Loco () - constructor
 *------------------------------------------------------------------------------------------------------------------------
 */
Loco::Loco ()
{
    uint_fast8_t    fidx;
    uint_fast8_t    midx;

    this->name                      = "";
    this->addr                      = 0;
    this->addon_idx                 = 0xFFFF;
    this->target_next_millis        = 0;
    this->speed_steps               = 0;
    this->fwd                       = 1;
    this->speed                     = 0;
    this->target_speed              = 0;
    this->target_millis_step        = 0;
    this->target_next_millis        = 0;
    this->functions                 = 0;
    this->packet_sequence_idx       = 0;
    this->locofunction.pulse_mask   = 0;
    this->locofunction.sound_mask   = 0;
    this->locofunction.max          = 0;
    this->rc_millis                 = 0;
    this->rc2_rate                  = 0;
    this->offline_cnt               = 0;
    this->rcl_location              = 0xFF;
    this->rr_location               = 0xFFFF;
    this->sleep_cnt                 = 0;
    this->destination               = 0xFF;
    this->flags                     = 0;
    this->active                    = 0;

    for (fidx = 0; fidx < MAX_LOCO_FUNCTIONS; fidx++)
    {
        this->locofunction.name_idx[fidx] = 0xFFFF;
    }

    for (fidx = 0; fidx < MAX_LOCO_FUNCTIONS; fidx++)
    {
        this->coupled_functions[fidx] = 0xFF;
    }

    for (midx = 0; midx < MAX_LOCO_MACROS_PER_LOCO; midx++)
    {
        this->macros[midx].n_actions = 0;
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 * set_id () - set id
 *------------------------------------------------------------------------------------------------------------------------
 */
void
Loco::set_id (uint_fast16_t id)
{
    this->id = id;
}

/*------------------------------------------------------------------------------------------------------------------------
 * set_addon () - set add-on index
 *------------------------------------------------------------------------------------------------------------------------
 */
void
Loco::set_addon (uint_fast16_t addon_idx)
{
    uint_fast16_t   n_locos;
    uint_fast16_t   lidx;
    uint_fast8_t    fidx;

    n_locos = Locos::get_n_locos();

    for (lidx = 0; lidx < n_locos; lidx++)                            // first remove addon from other locos
    {
        if (Locos::locos[lidx].get_addon() == addon_idx)
        {
            if (lidx != this->id)
            {
                Locos::locos[lidx].reset_addon();

                for (fidx = 0; fidx < MAX_LOCO_FUNCTIONS; fidx++)
                {
                    Locos::locos[lidx].coupled_functions[fidx] = 0xFF;
                }

                Locos::data_changed = true;
            }
        }
    }

    if (this->addon_idx != addon_idx)
    {
        this->addon_idx = addon_idx;

        for (fidx = 0; fidx < MAX_LOCO_FUNCTIONS; fidx++)
        {
            this->coupled_functions[fidx] = 0xFF;
        }

        Locos::data_changed = true;
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 * get_addon () - get add-on index
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast16_t
Loco::get_addon ()
{
    uint_fast16_t   rtc;
    rtc = this->addon_idx;
    return rtc;
}

/*------------------------------------------------------------------------------------------------------------------------
 * reset_addon (idx) - delete add-on index
 *------------------------------------------------------------------------------------------------------------------------
 */
void
Loco::reset_addon ()
{
    if (this->addon_idx != 0xFFFF)
    {
        this->addon_idx = 0xFFFF;
        Locos::data_changed = true;
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 * set_coupled_function () - set coupled function
 *------------------------------------------------------------------------------------------------------------------------
 */
void
Loco::set_coupled_function (uint_fast8_t lfidx, uint_fast8_t afidx)
{
    uint_fast16_t addon_idx = this->addon_idx;

    if (addon_idx != 0xFFFF)
    {
        this->coupled_functions[lfidx] = afidx;
        Locos::data_changed = true;
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 *  get_coupled_function () - get coupled function
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
Loco::get_coupled_function (uint_fast8_t lfidx)
{
    uint_fast8_t    afidx = 0xFF;

    uint_fast16_t addon_idx = this->addon_idx;

    if (addon_idx != 0xFFFF)
    {
        afidx = this->coupled_functions[lfidx];
    }

    return afidx;
}

/*------------------------------------------------------------------------------------------------------------------------
 * set_name () - set name
 *------------------------------------------------------------------------------------------------------------------------
 */
void
Loco::set_name (std::string name)
{
    this->name = name;
    Locos::data_changed = true;
}

/*------------------------------------------------------------------------------------------------------------------------
 * get_name () - get name
 *------------------------------------------------------------------------------------------------------------------------
 */
std::string
Loco::get_name ()
{
    return this->name;
}

/*------------------------------------------------------------------------------------------------------------------------
 * set_addr () - set address
 *------------------------------------------------------------------------------------------------------------------------
 */
void
Loco::set_addr (uint_fast16_t addr)
{
    if (this->addr != addr)
    {
        this->addr = addr;
        Locos::data_changed = true;
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 * get_addr () - get address
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast16_t
Loco::get_addr ()
{
    return (this->addr);
}

/*------------------------------------------------------------------------------------------------------------------------
 * set_speed_steps () - set speed steps, only 14, 28 and 128 allowed
 *------------------------------------------------------------------------------------------------------------------------
 */
void
Loco::set_speed_steps (uint_fast8_t speed_steps)
{
    if (this->speed_steps != speed_steps)
    {
        this->speed_steps = speed_steps;
        Locos::data_changed = true;
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 * get_speed_steps () - get speed steps
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
Loco::get_speed_steps ()
{
    return (this->speed_steps);
}

/*------------------------------------------------------------------------------------------------------------------------
 * set_flags () - set flags
 *------------------------------------------------------------------------------------------------------------------------
 */
void
Loco::set_flags (uint32_t flags)
{
    this->flags = flags;
}

/*------------------------------------------------------------------------------------------------------------------------
 * get_flags () - get flags
 *------------------------------------------------------------------------------------------------------------------------
 */
uint32_t
Loco::get_flags ()
{
    return this->flags;
}

/*------------------------------------------------------------------------------------------------------------------------
 * set_flag_halt () - set flag HALT
 *------------------------------------------------------------------------------------------------------------------------
 */
void
Loco::set_flag_halt ()
{
    this->flags |= LOCO_FLAG_HALT;
}

/*------------------------------------------------------------------------------------------------------------------------
 * reset_flag_halt () - reset flag HALT
 *------------------------------------------------------------------------------------------------------------------------
 */
void
Loco::reset_flag_halt ()
{
    this->flags &= ~LOCO_FLAG_HALT;
}

/*------------------------------------------------------------------------------------------------------------------------
 * get_flag_halt () - get flag HALT
 *------------------------------------------------------------------------------------------------------------------------
 */
bool
Loco::get_flag_halt ()
{
    bool rtc = false;

    if (this->flags & LOCO_FLAG_HALT)
    {
        rtc = true;
    }

    return rtc;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  Loco::add_macro_action () - add macro action
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
Loco::add_macro_action (uint_fast8_t macroidx)
{
    uint_fast8_t actionidx;

    if (this->macros[macroidx].n_actions < LOCO_MAX_ACTIONS_PER_MACRO)
    {
        actionidx = this->macros[macroidx].n_actions;
        this->macros[macroidx].actions[actionidx].action        = LOCO_ACTION_NONE;
        this->macros[macroidx].actions[actionidx].n_parameters  = 0;
        this->macros[macroidx].n_actions++;
        Locos::data_changed = true;
    }
    else
    {
        actionidx = 0xFF;
    }

    return actionidx;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  Loco::delete_macro_action () - delete macro action
 *------------------------------------------------------------------------------------------------------------------------
 */
void
Loco::delete_macro_action (uint_fast8_t macroidx, uint_fast8_t actionidx)
{
    uint_fast8_t    n_parameters;
    uint_fast8_t    pidx;

    if (actionidx < this->macros[macroidx].n_actions)
    {
        while (actionidx < this->macros[macroidx].n_actions - 1)
        {
            n_parameters = this->macros[macroidx].actions[actionidx + 1].n_parameters;

            this->macros[macroidx].actions[actionidx].action        = this->macros[macroidx].actions[actionidx + 1].action;
            this->macros[macroidx].actions[actionidx].n_parameters  = n_parameters;

            for (pidx = 0; pidx < n_parameters; pidx++)
            {
                this->macros[macroidx].actions[actionidx].parameters[pidx]  = this->macros[macroidx].actions[actionidx + 1].parameters[pidx];
            }

            actionidx++;
        }

        this->macros[macroidx].n_actions--;
        Locos::data_changed = true;
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 *  Loco::get_n_macro_actions () - get number of macro actions
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
Loco::get_n_macro_actions (uint_fast8_t macroidx)
{
    uint_fast8_t    rtc;
    rtc = this->macros[macroidx].n_actions;
    return rtc;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  Loco::set_macro_action () - set macro action
 *------------------------------------------------------------------------------------------------------------------------
 */
bool
Loco::set_macro_action (uint_fast8_t macroidx, uint_fast8_t actionidx, LOCOACTION * lap)
{
    uint_fast8_t rtc = false;

    if (actionidx < this->macros[macroidx].n_actions)
    {
        uint_fast8_t pidx;

        this->macros[macroidx].actions[actionidx].action        = lap->action;
        this->macros[macroidx].actions[actionidx].n_parameters  = lap->n_parameters;

        for (pidx = 0; pidx < lap->n_parameters; pidx++)
        {
            this->macros[macroidx].actions[actionidx].parameters[pidx] = lap->parameters[pidx];
        }

        Locos::data_changed = true;
        rtc = true;
    }

    return rtc;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  Loco::get_macro_action () - get macro action
 *------------------------------------------------------------------------------------------------------------------------
 */
bool
Loco::get_macro_action (uint_fast8_t macroidx, uint_fast8_t actionidx, LOCOACTION * lap)
{
    uint_fast8_t rtc = false;

    if (actionidx < this->macros[macroidx].n_actions)
    {
        uint_fast8_t pidx;

        lap->action         = this->macros[macroidx].actions[actionidx].action;
        lap->n_parameters   = this->macros[macroidx].actions[actionidx].n_parameters;

        for (pidx = 0; pidx < lap->n_parameters; pidx++)
        {
            lap->parameters[pidx] = this->macros[macroidx].actions[actionidx].parameters[pidx];
        }
        rtc = true;
    }

    return rtc;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  Loco::execute_macro () - execute macro
 *------------------------------------------------------------------------------------------------------------------------
 */
void
Loco::execute_macro (uint_fast8_t macroidx)
{
    LOCOACTION *    lap;
    uint_fast8_t    n_actions;
    uint_fast8_t    actionidx;

    n_actions = this->macros[macroidx].n_actions;
    lap = this->macros[macroidx].actions;

    for (actionidx = 0; actionidx < n_actions; actionidx++)
    {
        switch (lap[actionidx].action)
        {
            case LOCO_ACTION_SET_LOCO_SPEED:                                            // parameters: START, SPEED, TENTHS
            {
                uint_fast32_t   start       = lap[actionidx].parameters[0];
                uint_fast8_t    speed       = lap[actionidx].parameters[1];
                uint_fast16_t   tenths      = lap[actionidx].parameters[2];

                if (start == 0)
                {
                    this->set_speed (speed, tenths);
                }
                else
                {
                    Event::add_event_loco_speed (start, this->id, EVENT_SET_LOCO_SPEED, speed, tenths);
                }
                break;
            }

            case LOCO_ACTION_SET_LOCO_MIN_SPEED:                                        // parameters: START, SPEED, TENTHS
            {
                uint_fast32_t   start       = lap[actionidx].parameters[0];
                uint_fast8_t    speed       = lap[actionidx].parameters[1];
                uint_fast16_t   tenths      = lap[actionidx].parameters[2];

                if (start == 0)
                {
                    uint_fast8_t current_speed = this->get_speed ();

                    if (current_speed < speed)
                    {
                        this->set_speed (speed, tenths);
                    }
                }
                else
                {
                    Event::add_event_loco_speed (start, this->id, EVENT_SET_LOCO_MIN_SPEED, speed, tenths);
                }
                break;
            }

            case LOCO_ACTION_SET_LOCO_MAX_SPEED:                                        // parameters: START, SPEED, TENTHS
            {
                uint_fast32_t   start       = lap[actionidx].parameters[0];
                uint_fast8_t    speed       = lap[actionidx].parameters[1];
                uint_fast16_t   tenths      = lap[actionidx].parameters[2];

                if (start == 0)
                {
                    uint_fast8_t current_speed = this->get_speed ();

                    if (current_speed > speed)
                    {
                        this->set_speed (speed, tenths);
                    }
                }
                else
                {
                    Event::add_event_loco_speed (start, this->id, EVENT_SET_LOCO_MAX_SPEED, speed, tenths);
                }
                break;
            }

            case LOCO_ACTION_SET_LOCO_FORWARD_DIRECTION:                                // parameters: START, FWD
            {
                uint_fast32_t   start       = lap[actionidx].parameters[0];
                uint_fast8_t    fwd         = lap[actionidx].parameters[1];

                if (start == 0)
                {
                    this->set_fwd (fwd);
                }
                else
                {
                    Event::add_event_loco_dir (start, this->id, fwd);
                }
                break;
            }

            case LOCO_ACTION_SET_LOCO_ALL_FUNCTIONS_OFF:                                // parameters: START
            {
                uint_fast32_t   start       = lap[actionidx].parameters[0];

                if (start == 0)
                {
                    this->reset_functions ();
                }
                else
                {
                    Event::add_event_loco_function (start, this->id, 0xFF, false);
                }
                break;
            }

            case LOCO_ACTION_SET_LOCO_FUNCTION_OFF:                                     // parameters: START, FUNC_IDX
            {
                uint_fast32_t   start       = lap[actionidx].parameters[0];
                uint_fast8_t    f           = lap[actionidx].parameters[1];

                if (start == 0)
                {
                    this->set_function (f, false);
                }
                else
                {
                    Event::add_event_loco_function (start, this->id, f, false);
                }
                break;
            }

            case LOCO_ACTION_SET_LOCO_FUNCTION_ON:                                      // parameters: START, FUNC_IDX
            {
                uint_fast32_t   start       = lap[actionidx].parameters[0];
                uint_fast8_t    f           = lap[actionidx].parameters[1];

                if (start == 0)
                {
                    this->set_function (f, true);
                }
                else
                {
                    Event::add_event_loco_function (start, this->id, f, true);
                }
                break;
            }

            case LOCO_ACTION_SET_ADDON_ALL_FUNCTIONS_OFF:                               // parameters: START
            {
                uint_fast32_t   start       = lap[actionidx].parameters[0];
                uint_fast16_t   addon_idx   = this->addon_idx;

                if (addon_idx != 0xFFFF)
                {
                    if (start == 0)
                    {
                        AddOns::addons[addon_idx].reset_functions ();
                    }
                    else
                    {
                        Event::add_event_addon_function (start, addon_idx, 0xFF, false);
                    }
                }
                break;
            }

            case LOCO_ACTION_SET_ADDON_FUNCTION_OFF:                                        // parameters: START, FUNC_IDX
            {
                uint_fast32_t   start       = lap[actionidx].parameters[0];
                uint_fast8_t    f           = lap[actionidx].parameters[1];
                uint_fast16_t   addon_idx   = this->addon_idx;

                if (addon_idx != 0xFFFF)
                {
                    if (start == 0)
                    {
                        AddOns::addons[addon_idx].set_function (f, false);
                    }
                    else
                    {
                        Event::add_event_addon_function (start, addon_idx, f, false);
                    }
                }
                break;
            }

            case LOCO_ACTION_SET_ADDON_FUNCTION_ON:                                         // parameters: START, FUNC_IDX
            {
                uint_fast32_t   start       = lap[actionidx].parameters[0];
                uint_fast8_t    f           = lap[actionidx].parameters[1];
                uint_fast16_t   addon_idx   = this->addon_idx;

                if (addon_idx != 0xFFFF)
                {
                    if (start == 0)
                    {
                        AddOns::addons[addon_idx].set_function (f, true);
                    }
                    else
                    {
                        Event::add_event_addon_function (start, addon_idx, f, true);
                    }
                }
                break;
            }
        }
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 * set_function_type_pulse () - set/reset function to type "pulse"
 *------------------------------------------------------------------------------------------------------------------------
 */
void
Loco::set_function_type_pulse (uint_fast8_t f, bool b)
{
    uint32_t    f_mask = 1 << f;

    if (b)
    {
        this->locofunction.pulse_mask |= f_mask;
    }
    else
    {
        this->locofunction.pulse_mask &= ~f_mask;
    }

    Locos::data_changed = true;
}

/*------------------------------------------------------------------------------------------------------------------------
 * set_function_type_sound () - set/reset function to type "sound"
 *------------------------------------------------------------------------------------------------------------------------
 */
void
Loco::set_function_type_sound (uint_fast8_t f, bool b)
{
    uint32_t    f_mask = 1 << f;

    if (b)
    {
        this->locofunction.sound_mask |= f_mask;
    }
    else
    {
        this->locofunction.sound_mask &= ~f_mask;
    }

    Locos::data_changed = true;
}

/*------------------------------------------------------------------------------------------------------------------------
 * set_function_type () - set function type
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
Loco::set_function_type (uint_fast8_t fidx, uint_fast16_t function_name_idx, bool pulse, bool sound)
{
    if (this->locofunction.max < fidx)
    {
        this->locofunction.max = fidx;
    }

    this->locofunction.name_idx[fidx] = function_name_idx;
    this->set_function_type_pulse (fidx, pulse);
    this->set_function_type_sound (fidx, sound);
    return 1;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  get_function_name () - get function name
 *------------------------------------------------------------------------------------------------------------------------
 */
std::string&
Loco::get_function_name (uint_fast8_t fidx)
{
    uint_fast16_t    function_name_idx = this->locofunction.name_idx[fidx];
    return Functions::get (function_name_idx);
}

/*------------------------------------------------------------------------------------------------------------------------
 * get_function_name_idx () - get function name index
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast16_t
Loco::get_function_name_idx (uint_fast8_t fidx)
{
    uint_fast16_t    function_name_idx = this->locofunction.name_idx[fidx];
    return function_name_idx;
}

/*------------------------------------------------------------------------------------------------------------------------
 * get_function_pulse () - get function of type "pulse"
 *------------------------------------------------------------------------------------------------------------------------
 */
bool
Loco::get_function_pulse (uint_fast8_t fidx)
{
    uint32_t        f_mask = 1 << fidx;
    bool            b = false;

    if (this->locofunction.pulse_mask & f_mask)
    {
        b = true;
    }
    return b;
}

/*------------------------------------------------------------------------------------------------------------------------
 * get_function_pulse_mask () - get complete function of type pulse as mask
 *------------------------------------------------------------------------------------------------------------------------
 */
uint32_t
Loco::get_function_pulse_mask ()
{
    return this->locofunction.pulse_mask;
}

/*------------------------------------------------------------------------------------------------------------------------
 * get_function_sound () - get function of type "sound"
 *------------------------------------------------------------------------------------------------------------------------
 */
bool
Loco::get_function_sound (uint_fast8_t fidx)
{
    uint32_t        f_mask = 1 << fidx;
    bool            b = false;

    if (this->locofunction.sound_mask & f_mask)
    {
        b = true;
    }
    return b;
}

/*------------------------------------------------------------------------------------------------------------------------
 * get_function_sound_mask () - get complete function of type sound as mask
 *------------------------------------------------------------------------------------------------------------------------
 */
uint32_t
Loco::get_function_sound_mask ()
{
    return this->locofunction.sound_mask;
}

/*------------------------------------------------------------------------------------------------------------------------
 * activate () - activate a loco
 *------------------------------------------------------------------------------------------------------------------------
 */
void
Loco::activate ()
{
    if (! this->active)
    {
        this->active = true;
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 * deactivate_loco () - deactivate a loco
 *------------------------------------------------------------------------------------------------------------------------
 */
void
Loco::deactivate ()
{
    if (this->active)
    {
        this->active = false;
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 * is_active () - check if loco is active
 *------------------------------------------------------------------------------------------------------------------------
 */
bool
Loco::is_active ()
{
    if (this->active)
    {
        return true;
    }
    return false;
}

/*------------------------------------------------------------------------------------------------------------------------
 * set_online () - set timestamp if loco has been detected by global RAILCOM detector
 *------------------------------------------------------------------------------------------------------------------------
 */
#define MAX_OFFLINE_COUNTER_VALUE       4
void
Loco::set_online (bool value)
{
    if (value)
    {
        this->offline_cnt = 0;
        this->flags |= LOCO_FLAG_ONLINE;
    }
    else
    {
        if (this->offline_cnt < MAX_OFFLINE_COUNTER_VALUE)
        {
            this->offline_cnt++;
        }

        if (this->offline_cnt == MAX_OFFLINE_COUNTER_VALUE)
        {
            this->flags &= ~LOCO_FLAG_ONLINE;
        }
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 * is_online () - check if loco is online
 *------------------------------------------------------------------------------------------------------------------------
 */
bool
Loco::is_online ()
{
    if (this->flags & LOCO_FLAG_ONLINE)
    {
        return true;
    }
    return false;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  set_speed_value (speed) - set only speed variable, nothing else
 *------------------------------------------------------------------------------------------------------------------------
 */
void
Loco::set_speed_value (uint_fast8_t tspeed)
{
    if (! (this->flags & LOCO_FLAG_HALT))
    {
        this->speed = tspeed;
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 * set_speed () - set speed
 *------------------------------------------------------------------------------------------------------------------------
 */
void
Loco::set_speed (uint_fast8_t speed)
{
    if (! (this->flags & LOCO_FLAG_HALT))
    {
        this->speed                 = speed;
        this->target_next_millis    = 0;
        this->sendspeed ();
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 * set_speed () - set speed within tenths of a second
 *
 *  example:
 *      current speed:  20
 *      target_speed:   30
 *      tenths:         50  (5 seconds)
 *  -------------------------------------------------------
 *  --> millis_step     500 (increment speed every 500msec)
 *
 * formula: milis_step  = (100 * tenths) / (target_speed - current_speed)
 *                      = (100 * 50) / (30 - 20)
 *                      = 5000 / 10
 *                      = 500
 *------------------------------------------------------------------------------------------------------------------------
 */
void
Loco::set_speed (uint_fast8_t tspeed, uint_fast16_t tenths)
{
    // printf ("loco_idx=%u tspeed=%u, tenths=%u\n", loco_idx, tspeed, tenths);

    if (! (this->flags & LOCO_FLAG_HALT))
    {
        if (tenths > 0)
        {
            uint_fast8_t    cspeed = this->speed;       // current speed

            if (tspeed > cspeed)
            {
                this->target_speed          = tspeed;
                this->target_millis_step    = (100 * tenths) / (tspeed - cspeed);
                this->target_next_millis    = Millis::elapsed() + this->target_millis_step;
            }
            else if (tspeed < cspeed)
            {
                this->target_speed          = tspeed;
                this->target_millis_step    = (100 * tenths) / (cspeed - tspeed);
                this->target_next_millis    = Millis::elapsed() + this->target_millis_step;
            }
            // if speeds are identical, do nothing
        }
        else // tenths are 0, set speed immediately
        {
            this->set_speed (tspeed);
        }
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 * get_speed () - get speed
 *------------------------------------------------------------------------------------------------------------------------
 */
uint16_t
Loco::get_speed ()
{
    return (this->speed);
}

/*------------------------------------------------------------------------------------------------------------------------
 * set_fwd () - set forward direction
 *------------------------------------------------------------------------------------------------------------------------
 */
void
Loco::set_fwd (uint_fast8_t fwd)
{
    if (this->fwd != fwd)
    {
        this->speed = 0;
        this->fwd = fwd;
        this->sendspeed ();
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 * set_speed_fwd () - set speed and direction
 *------------------------------------------------------------------------------------------------------------------------
 */
void
Loco::set_speed_fwd (uint_fast8_t speed, uint_fast8_t fwd)
{
    this->speed = speed;
    this->fwd = fwd;
    this->sendspeed ();
}

/*------------------------------------------------------------------------------------------------------------------------
 * get_fwd () - get direction
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
Loco::get_fwd ()
{
    return this->fwd;
}

/*------------------------------------------------------------------------------------------------------------------------
 * set_function () - set a function
 *------------------------------------------------------------------------------------------------------------------------
 */
void
Loco::set_function (uint_fast8_t f, bool b)
{
    uint32_t        fmask = 1 << f;
    uint_fast16_t   addon_idx = this->addon_idx;
    uint_fast8_t    range;

    if (b)
    {
        this->functions |= fmask;

        if (fmask & this->locofunction.pulse_mask)          // function is of type pulse
        {
            Event::add_event_loco_function (2, this->id, f, false);         // reset function in 200 msec
        }
    }
    else
    {
        this->functions &= ~fmask;
    }

    if (addon_idx != 0xFFFF)
    {
        uint_fast8_t cf = this->coupled_functions[f];

        if (cf != 0xFF)
        {
            AddOns::addons[addon_idx].set_function (cf, b);
        }
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

    this->sendfunction (range);
}

/*------------------------------------------------------------------------------------------------------------------------
 * get_function ()
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
Loco::get_function (uint_fast8_t f)
{
    uint32_t mask = 1 << f;
    return (this->functions & mask) ? true : false;
}

/*------------------------------------------------------------------------------------------------------------------------
 * reset_functions ()
 *------------------------------------------------------------------------------------------------------------------------
 */
void
Loco::reset_functions ()
{
    this->functions = 0;       // let scheduler turn off functions

    uint_fast16_t addon_idx = this->addon_idx;

    if (addon_idx != 0xFFFF)
    {
        AddOns::addons[addon_idx].reset_functions ();
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 * get_functions ()
 *------------------------------------------------------------------------------------------------------------------------
 */
uint32_t
Loco::get_functions ()
{
    return this->functions;
}

/*------------------------------------------------------------------------------------------------------------------------
 * set_destination (uint_fast8_t rrg)
 *------------------------------------------------------------------------------------------------------------------------
 */
void
Loco::set_destination (uint_fast8_t rrg)
{
    this->destination = rrg;
}

/*------------------------------------------------------------------------------------------------------------------------
 * get_destination ()
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
Loco::get_destination (void)
{
    return this->destination;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  set_rcllocation (uint_fast16_t loco_idx, uint_fast8_t location)
 *------------------------------------------------------------------------------------------------------------------------
 */
void
Loco::set_rcllocation (uint_fast8_t location)
{
    if (location == 0xFF || location < RCL::get_n_tracks ())
    {
        Debug::printf (DEBUG_LEVEL_NORMAL, "Loco::set_rcllocation: loco_idx=%u location=%u\n", this->id, location);
        this->rcl_location = location;
    }
    else
    {
        this->rcl_location = 0xFF;                              // set to unknown
        Debug::printf (DEBUG_LEVEL_NONE, "Loco::set_rcllocation: loco_idx=%u location=%u: invalid location\n", this->id, location);
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 *  get_rcllocation (uint_fast16_t loco_idx)
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
Loco::get_rcllocation ()
{
    uint_fast8_t    rtc;
    rtc = this->rcl_location;
    return rtc;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  set_rrlocation (uint_fast16_t loco_idx, uint_fast8_t rrgidx, uint_fast8_t rridx)
 *------------------------------------------------------------------------------------------------------------------------
 */
void
Loco::set_rrlocation (uint_fast16_t rrgrridx)
{
    Debug::printf (DEBUG_LEVEL_NORMAL, "Loco::set_rrlocation: loco_idx=%u rrgidx=%u rridx=%u\n", this->id, rrgrridx >> 8, rrgrridx & 0xFF);
    this->rr_location = rrgrridx;                   // maybe 0xFFFF
}

/*------------------------------------------------------------------------------------------------------------------------
 *  get_rrlocation (uint_fast16_t loco_idx)
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast16_t
Loco::get_rrlocation ()
{
    uint_fast16_t rtc;
    rtc = this->rr_location;
    return rtc;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  estop (loco_idx)
 *------------------------------------------------------------------------------------------------------------------------
 */
void
Loco::estop ()
{
    this->set_speed (1);
}

/*------------------------------------------------------------------------------------------------------------------------
 *  get_ack (void)
 *------------------------------------------------------------------------------------------------------------------------
 */
void
Loco::get_ack ()
{
    DCC::get_ack (this->addr);
}

/*------------------------------------------------------------------------------------------------------------------------
 *  set_rc2_rate () - set RC2 rate (0% - 100%)
 *------------------------------------------------------------------------------------------------------------------------
 */
void
Loco::set_rc2_rate (uint_fast8_t rate)
{
    Debug::printf (DEBUG_LEVEL_VERBOSE, "Loco::set_rc2_rate: loco_idx=%u rate=%u\n", (uint16_t) this->id, (uint16_t) rate);
    this->rc2_rate = rate;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  get_rc2_rate () - get RC2 rate (0% - 100%)
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
Loco::get_rc2_rate ()
{
    Debug::printf (DEBUG_LEVEL_VERBOSE, "Loco::get_rc2_rate: loco_idx=%u rate=%u\n", (uint16_t) this->id, (uint16_t) this->rc2_rate);
    return this->rc2_rate;
}

void
Loco::sched ()
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

uint_fast16_t
Locos::add (const Loco& loco)
{
    uint_fast16_t rtc = 0xFFFF;

    if (Locos::n_locos < MAX_LOCOS)
    {
        Locos::locos.push_back(loco);
        Locos::locos[n_locos].set_id(n_locos);
        Locos::n_locos++;
        Locos::data_changed = true;
        rtc = locos.size() - 1;
    }
    return rtc;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  get_n_locos () - get number of locos
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast16_t
Locos::get_n_locos (void)
{
    return Locos::n_locos;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  set_new_id () - set new id
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast16_t
Locos::set_new_id (uint_fast16_t loco_idx, uint_fast16_t new_loco_idx)
{
    uint_fast16_t   rtc = 0xFFFF;

    if (loco_idx < Locos::n_locos && new_loco_idx < Locos::n_locos)
    {
        if (loco_idx != new_loco_idx)
        {
            Loco            tmploco;
            uint_fast16_t   lidx;

            uint16_t *      map_new_loco_idx = (uint16_t *) calloc (Locos::n_locos, sizeof (uint16_t));

            for (lidx = 0; lidx < Locos::n_locos; lidx++)
            {
                map_new_loco_idx[lidx] = lidx;
            }

            tmploco = Locos::locos[loco_idx];

            if (new_loco_idx < loco_idx)
            {
                // step 1: shift loco
                for (lidx = loco_idx; lidx > new_loco_idx; lidx--)
                {
                    Locos::locos[lidx] = Locos::locos[lidx - 1];
                    map_new_loco_idx[lidx - 1] = lidx;
                }
            }
            else // if (new_loco_idx > loco_idx)
            {
                // step 1: shift loco
                for (lidx = loco_idx; lidx < new_loco_idx; lidx++)
                {
                    Locos::locos[lidx] = Locos::locos[lidx + 1];
                    map_new_loco_idx[lidx + 1] = lidx;
                }
            }

            Locos::locos[lidx] = tmploco;
            map_new_loco_idx[loco_idx] = lidx;

            for (lidx = 0; lidx < Locos::n_locos; lidx++)
            {
                Debug::printf (DEBUG_LEVEL_VERBOSE, "%2d -> %2d\n", lidx, map_new_loco_idx[lidx]);
            }

            // step 2: correct loco_idx in ADDON, RAILROAD, RCL, S88
            AddOns::set_new_loco_ids (map_new_loco_idx, Locos::n_locos);
            RailroadGroups::set_new_loco_ids (map_new_loco_idx, Locos::n_locos);
            RCL::set_new_loco_ids (map_new_loco_idx, Locos::n_locos);
            S88::set_new_loco_ids (map_new_loco_idx, Locos::n_locos);

            free (map_new_loco_idx);
            Locos::data_changed = true;
            rtc = new_loco_idx;

            Locos::renumber ();
        }
        else
        {
            rtc = loco_idx;
        }
    }

    return rtc;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  Loco::set_new_addon_ids ()
 *------------------------------------------------------------------------------------------------------------------------
 */
void
Locos::set_new_addon_ids (uint16_t * map_new_addon_idx, uint16_t n_addons)
{
    uint_fast16_t     loco_idx;

    for (loco_idx = 0; loco_idx < Locos::n_locos; loco_idx++)
    {
        uint_fast16_t     addon_idx = Locos::locos[loco_idx].get_addon();

        if (addon_idx < n_addons)
        {
            Locos::locos[loco_idx].set_addon (map_new_addon_idx[addon_idx]);
        }
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 *  schedule () - schedule all locos
 *------------------------------------------------------------------------------------------------------------------------
 */
bool
Locos::schedule (void)
{
    static bool             handle_locos = true;
    static uint_fast16_t    loco_idx = 0;
    bool                    rtc = false;

    if (handle_locos)
    {
        if (loco_idx < Locos::n_locos)
        {
            Locos::locos[loco_idx].sched();
            loco_idx++;

            if (loco_idx >= Locos::n_locos)
            {
                loco_idx = 0;
                handle_locos = false;
            }
        }
        else
        {
            loco_idx = 0;
            handle_locos = false;
        }
    }
    else // addons
    {
        rtc = AddOns::schedule ();

        if (rtc)
        {
            handle_locos = true;
        }
    }

    return rtc;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  renumber () - renumber ids
 *------------------------------------------------------------------------------------------------------------------------
 */
void
Locos::renumber ()
{
    uint_fast16_t loco_idx;

    for (loco_idx = 0; loco_idx < Locos::n_locos; loco_idx++)
    {
        Locos::locos[loco_idx].set_id (loco_idx);
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 *  estop (void)
 *------------------------------------------------------------------------------------------------------------------------
 */
void
Locos::estop (void)
{
    uint_fast16_t loco_idx;
    uint_fast16_t n_locos = Locos::get_n_locos();
    DCC::estop ();

    for (loco_idx = 0; loco_idx < n_locos; loco_idx++)
    {
        Locos::locos[loco_idx].set_speed_value (1);
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 *  booster_off (void)
 *------------------------------------------------------------------------------------------------------------------------
 */
void
Locos::booster_off (bool do_send_booster_cmd)
{
    uint_fast16_t loco_idx;

    if (do_send_booster_cmd)
    {
        DCC::booster_off ();
    }

    for (loco_idx = 0; loco_idx < Locos::n_locos; loco_idx++)
    {
        Locos::locos[loco_idx].set_speed_value(0);              // only set variable, nothing else
        Locos::locos[loco_idx].set_rcllocation(0xFF);
        Locos::locos[loco_idx].set_rrlocation (0xFFFF);
        Locos::locos[loco_idx].set_rc2_rate (0);
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 *  booster_on (void)
 *------------------------------------------------------------------------------------------------------------------------
 */
void
Locos::booster_on (bool do_send_booster_cmd)
{
    uint_fast16_t loco_idx;

    for (loco_idx = 0; loco_idx < Locos::n_locos; loco_idx++)
    {
        Locos::locos[loco_idx].set_speed_value(0);              // only set variable, nothing else
        Locos::locos[loco_idx].set_rcllocation(0xFF);
        Locos::locos[loco_idx].set_rrlocation (0xFFFF);
        Locos::locos[loco_idx].set_rc2_rate (0);
    }

    if (do_send_booster_cmd)
    {
        DCC::booster_on ();
    }
}
