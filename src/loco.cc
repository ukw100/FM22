/*------------------------------------------------------------------------------------------------------------------------
 * loco.cc - loco management functions
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

bool                    Loco::data_changed = false;
LOCO                    Loco::locos[MAX_LOCOS];                                     // loco
uint_fast16_t           Loco::n_locos = 0;                                          // number of locos

/*------------------------------------------------------------------------------------------------------------------------
 * sendspeed() - send speed
 *------------------------------------------------------------------------------------------------------------------------
 */
void
Loco::sendspeed (uint_fast16_t loco_idx)
{
    if (loco_idx < Loco::n_locos)
    {
        uint16_t        addr            = Loco::locos[loco_idx].addr;
        uint_fast8_t    speed_steps     = Loco::locos[loco_idx].speed_steps;
        uint_fast8_t    fwd             = Loco::locos[loco_idx].fwd;
        uint_fast8_t    speed           = Loco::locos[loco_idx].speed;

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

            DCC::loco_28 (loco_idx, addr, fwd, speed);
            Debug::printf (DEBUG_LEVEL_VERBOSE, "Loco::sendspeed: loco_idx=%d send speed_28: %d\n", loco_idx, speed);
        }
        else // if (speed_steps == 128)
        {
            DCC::loco (loco_idx, addr, fwd, speed, 0, 0);
            Debug::printf (DEBUG_LEVEL_VERBOSE, "Loco::sendspeed: loco_idx=%d send speed_126: %d\n", loco_idx, speed);
        }
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 * sendfunction() - send function
 *------------------------------------------------------------------------------------------------------------------------
 */
void
Loco::sendfunction (uint_fast16_t loco_idx, uint_fast8_t range)
{
    if (loco_idx < Loco::n_locos)
    {
        uint16_t    addr            = Loco::locos[loco_idx].addr;
        uint32_t    functions       = Loco::locos[loco_idx].functions;

        DCC::loco_function (loco_idx, addr, functions, range);
        Debug::printf (DEBUG_LEVEL_VERBOSE, "Loco::sendfunction: loco_idx=%d range %d\n", loco_idx, range);
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 *  sendcmd (idx, packet_no) - send loco command
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
Loco::sendcmd (uint_fast16_t loco_idx, uint_fast8_t packet_no)
{
    if (loco_idx < Loco::n_locos)
    {
        uint32_t        target_next_millis  = Loco::locos[loco_idx].target_next_millis;

        if (target_next_millis > 0 && Millis::elapsed() >= target_next_millis)
        {
            uint32_t    millis = Millis::elapsed();

            uint_fast8_t    speed           = Loco::locos[loco_idx].speed;
            uint_fast8_t    tspeed          = Loco::locos[loco_idx].target_speed;

            if (tspeed == speed)
            {
                Loco::locos[loco_idx].target_next_millis = 0;
            }
            else
            {
                if (tspeed > speed)
                {
                    while (Loco::locos[loco_idx].target_next_millis < millis)
                    {
                        Loco::locos[loco_idx].target_next_millis += Loco::locos[loco_idx].target_millis_step;

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

                    Loco::locos[loco_idx].speed = speed;
                }
                else
                {
                    while (Loco::locos[loco_idx].target_next_millis < millis)
                    {
                        Loco::locos[loco_idx].target_next_millis += Loco::locos[loco_idx].target_millis_step;

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

                    Loco::locos[loco_idx].speed = speed;
                }
            }
        }

        if (packet_no & 0x01)           // odd packet number: send functions
        {
            switch (packet_no)
            {
                case 1:
                {
                    Loco::sendfunction (loco_idx, DCC_F00_F04_RANGE);
                    Debug::printf (DEBUG_LEVEL_VERBOSE, "sendfunction (%d, DCC_F00_F04_RANGE)\n", loco_idx);
                    break;
                }
                case 3:
                {
                    if (Loco::locos[loco_idx].locofunction.max >= 5)
                    {
                        Loco::sendfunction (loco_idx, DCC_F05_F08_RANGE);
                        Debug::printf (DEBUG_LEVEL_VERBOSE, "sendfunction (%d, DCC_F05_F08_RANGE)\n", loco_idx);
                    }
                    break;
                }
                case 5:
                {
                    if (Loco::locos[loco_idx].locofunction.max >= 9)
                    {
                        Loco::sendfunction (loco_idx, DCC_F09_F12_RANGE);
                        Debug::printf (DEBUG_LEVEL_VERBOSE, "sendfunction (%d, DCC_F09_F12_RANGE)\n", loco_idx);
                    }
                    break;
                }
                case 7:
                {
                    if (Loco::locos[loco_idx].locofunction.max >= 13)
                    {
                        Loco::sendfunction (loco_idx, DCC_F13_F20_RANGE);
                        Debug::printf (DEBUG_LEVEL_VERBOSE, "sendfunction (%d, DCC_F13_F20_RANGE)\n", loco_idx);
                    }
                    break;
                }
                case 9:
                {
                    if (Loco::locos[loco_idx].locofunction.max >= 21)
                    {
                        Loco::sendfunction (loco_idx, DCC_F21_F28_RANGE);
                        Debug::printf (DEBUG_LEVEL_VERBOSE, "sendfunction (%d, DCC_F21_F28_RANGE)\n", loco_idx);
                    }
                    break;
                }
            }
        }
        else                        // even packet number: send speed
        {
            Loco::sendspeed (loco_idx);
        }
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 *  add () - add a loco
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast16_t
Loco::add (void)
{
    uint_fast16_t loco_idx = Loco::n_locos;

    if (loco_idx < MAX_LOCOS)
    {
        uint_fast8_t    fidx;

        Loco::locos[loco_idx].speed                 = 0;
        Loco::locos[loco_idx].target_next_millis    = 0;
        Loco::locos[loco_idx].fwd                   = 1;
        Loco::locos[loco_idx].functions             = 0;
        Loco::locos[loco_idx].locofunction.max      = 0;
        Loco::locos[loco_idx].rcl_location          = 0xFF;
        Loco::locos[loco_idx].rr_location           = 0xFFFF;
        Loco::locos[loco_idx].addon_idx             = 0xFFFF;

        for (fidx = 0; fidx < MAX_LOCO_FUNCTIONS; fidx++)
        {
            Loco::locos[loco_idx].locofunction.name_idx[fidx] = 0xFFFF;
        }

        Loco::n_locos++;
        Loco::data_changed = true;
    }
    else
    {
        loco_idx = 0xFFFF;
    }

    return loco_idx;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  setid () - set new id
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast16_t
Loco::setid (uint_fast16_t loco_idx, uint_fast16_t new_loco_idx)
{
    uint_fast16_t   rtc = 0xFFFF;

    if (loco_idx < Loco::n_locos && new_loco_idx < Loco::n_locos)
    {
        if (loco_idx != new_loco_idx)
        {
            LOCO            tmploco;
            uint_fast16_t   lidx;

            uint16_t *      map_new_loco_idx = (uint16_t *) calloc (Loco::n_locos, sizeof (uint16_t));

            for (lidx = 0; lidx < Loco::n_locos; lidx++)
            {
                map_new_loco_idx[lidx] = lidx;
            }

            memcpy (&tmploco, Loco::locos + loco_idx, sizeof (LOCO));

            if (new_loco_idx < loco_idx)
            {
                // step 1: shift loco
                for (lidx = loco_idx; lidx > new_loco_idx; lidx--)
                {
                    memcpy (Loco::locos + lidx, Loco::locos + lidx - 1, sizeof (LOCO));
                    map_new_loco_idx[lidx - 1] = lidx;
                }
            }
            else // if (new_loco_idx > loco_idx)
            {
                // step 1: shift loco
                for (lidx = loco_idx; lidx < new_loco_idx; lidx++)
                {
                    memcpy (Loco::locos + lidx, Loco::locos + lidx + 1, sizeof (LOCO));
                    map_new_loco_idx[lidx + 1] = lidx;
                }
            }

            memcpy (Loco::locos + lidx, &tmploco, sizeof (LOCO));
            map_new_loco_idx[loco_idx] = lidx;

            for (lidx = 0; lidx < Loco::n_locos; lidx++)
            {
                Debug::printf (DEBUG_LEVEL_NORMAL, "%2d -> %2d\n", lidx, map_new_loco_idx[lidx]);
            }

            // step 2: correct loco_idx in ADDON, RAILROAD, RCL, S88
            AddOn::set_new_loco_ids (map_new_loco_idx, Loco::n_locos);
            Railroad::set_new_loco_ids (map_new_loco_idx, Loco::n_locos);
            RCL::set_new_loco_ids (map_new_loco_idx, Loco::n_locos);
            S88::set_new_loco_ids (map_new_loco_idx, Loco::n_locos);

            free (map_new_loco_idx);
            Loco::data_changed = true;
            rtc = new_loco_idx;
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
Loco::set_new_addon_ids (uint16_t * map_new_addon_idx, uint16_t n_addons)
{
    uint_fast8_t     loco_idx;

    for (loco_idx = 0; loco_idx < Loco::n_locos; loco_idx++)
    {
        if (Loco::locos[loco_idx].addon_idx < n_addons)
        {
            Loco::locos[loco_idx].addon_idx = map_new_addon_idx[Loco::locos[loco_idx].addon_idx];
        }
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 *  get_n_locos () - get number of locos
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast16_t
Loco::get_n_locos (void)
{
    return Loco::n_locos;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  unlock (idx) - unlock a loco entry
 *------------------------------------------------------------------------------------------------------------------------
 */
void
Loco::unlock (uint_fast16_t loco_idx)
{
    if (loco_idx < Loco::n_locos)
    {
        if (Loco::locos[loco_idx].flags & LOCO_FLAG_LOCKED)
        {
            Loco::locos[loco_idx].flags &= ~LOCO_FLAG_LOCKED;
        }
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 *  lock (idx) - lock a loco entry
 *------------------------------------------------------------------------------------------------------------------------
 */
void
Loco::lock (uint_fast16_t loco_idx)
{
    if (loco_idx < Loco::n_locos)
    {
        if (! (Loco::locos[loco_idx].flags & LOCO_FLAG_LOCKED))
        {
            Loco::locos[loco_idx].flags |= LOCO_FLAG_LOCKED;
        }
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 *  islocked (idx) - check if loco is locked
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
Loco::islocked(uint_fast16_t loco_idx)
{
    if (loco_idx < Loco::n_locos && (Loco::locos[loco_idx].flags & LOCO_FLAG_LOCKED))
    {
        return true;
    }
    return false;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  set_addon (idx) - set add-on index
 *------------------------------------------------------------------------------------------------------------------------
 */
void
Loco::set_addon (uint_fast16_t loco_idx, uint_fast16_t addon_idx)
{
    if (loco_idx < Loco::n_locos && addon_idx < Loco::n_locos)
    {
        uint_fast16_t   lidx;
        uint_fast8_t    fidx;

        for (lidx = 0; lidx < Loco::n_locos; lidx++)                            // first remove addon from other locos
        {
            if (Loco::locos[lidx].addon_idx == addon_idx)
            {
                if (lidx != loco_idx)
                {
                    Loco::locos[lidx].addon_idx = 0xFFFF;

                    for (fidx = 0; fidx < MAX_LOCO_FUNCTIONS; fidx++)
                    {
                        Loco::locos[lidx].coupled_functions[fidx] = 0xFF;
                    }

                    Loco::data_changed = true;
                }
            }
        }

        if (Loco::locos[loco_idx].addon_idx != addon_idx)
        {
            Loco::locos[loco_idx].addon_idx = addon_idx;

            for (fidx = 0; fidx < MAX_LOCO_FUNCTIONS; fidx++)
            {
                Loco::locos[loco_idx].coupled_functions[fidx] = 0xFF;
            }

            Loco::data_changed = true;
        }
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 *  reset_addon (idx) - delete add-on index
 *------------------------------------------------------------------------------------------------------------------------
 */
void
Loco::reset_addon (uint_fast16_t loco_idx)
{
    if (loco_idx < Loco::n_locos)
    {
        if (Loco::locos[loco_idx].addon_idx != 0xFFFF)
        {
            Loco::locos[loco_idx].addon_idx = 0xFFFF;
            Loco::data_changed = true;
        }
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 *  get_addon (idx) - get add-on index
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast16_t
Loco::get_addon (uint_fast16_t loco_idx)
{
    uint_fast16_t   rtc;

    if (loco_idx < Loco::n_locos)
    {
        rtc = Loco::locos[loco_idx].addon_idx;
    }
    else
    {
        rtc = 0xFFFF;
    }
    return rtc;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  set_coupled_function () - set coupled function
 *------------------------------------------------------------------------------------------------------------------------
 */
void
Loco::set_coupled_function (uint_fast16_t loco_idx, uint_fast8_t lfidx, uint_fast8_t afidx)
{
    if (loco_idx < Loco::n_locos)
    {
        uint_fast16_t addon_idx = Loco::locos[loco_idx].addon_idx;

        if (addon_idx != 0xFFFF)
        {
            Loco::locos[loco_idx].coupled_functions[lfidx] = afidx;
            Loco::data_changed = true;
        }
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 *  get_coupled_function () - get coupled function
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
Loco::get_coupled_function (uint_fast16_t loco_idx, uint_fast8_t lfidx)
{
    uint_fast8_t    afidx = 0xFF;

    if (loco_idx < Loco::n_locos)
    {
        uint_fast16_t addon_idx = Loco::locos[loco_idx].addon_idx;

        if (addon_idx != 0xFFFF)
        {
            afidx = Loco::locos[loco_idx].coupled_functions[lfidx];
        }
    }

    return afidx;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  set_name (idx) - set name
 *------------------------------------------------------------------------------------------------------------------------
 */
void
Loco::set_name (uint_fast16_t loco_idx, const char * name)
{
    if (loco_idx < Loco::n_locos)
    {
        int l = strlen (name);

        if (! Loco::locos[loco_idx].name || strcmp (Loco::locos[loco_idx].name, name) != 0)
        {
            if (Loco::locos[loco_idx].name)
            {
                if ((int) strlen (Loco::locos[loco_idx].name) < l)
                {
                    free (Loco::locos[loco_idx].name);
                    Loco::locos[loco_idx].name = (char *) NULL;
                }
            }

            if (! Loco::locos[loco_idx].name)
            {
                Loco::locos[loco_idx].name = (char *) malloc (l + 1);
            }

            strcpy (Loco::locos[loco_idx].name, name);
            Loco::data_changed = true;
        }
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 *  get_name (idx) - get name
 *------------------------------------------------------------------------------------------------------------------------
 */
char *
Loco::get_name (uint_fast16_t loco_idx)
{
    if (loco_idx < Loco::n_locos)
    {
        return Loco::locos[loco_idx].name;
    }
    return (char *) NULL;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  set_addr (idx, address) - set address
 *------------------------------------------------------------------------------------------------------------------------
 */
void
Loco::set_addr (uint_fast16_t loco_idx, uint_fast16_t addr)
{
    if (loco_idx < Loco::n_locos)
    {
        if (Loco::locos[loco_idx].addr != addr)
        {
            Loco::locos[loco_idx].addr = addr;
            Loco::data_changed = true;
        }
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 *  get_addr (idx) - get address
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast16_t
Loco::get_addr (uint_fast16_t loco_idx)
{
    if (loco_idx < Loco::n_locos)
    {
        return (Loco::locos[loco_idx].addr);
    }

    return 0;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  set_speed_steps (idx, speed_steps) - set speed steps, only 14, 28 and 128 allowed
 *------------------------------------------------------------------------------------------------------------------------
 */
void
Loco::set_speed_steps (uint_fast16_t loco_idx, uint_fast8_t speed_steps)
{
    if (loco_idx < Loco::n_locos)
    {
        if (Loco::locos[loco_idx].speed_steps != speed_steps)
        {
            Loco::locos[loco_idx].speed_steps = speed_steps;
            Loco::data_changed = true;
        }
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 *  get_speed_steps (idx) - get speed steps
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
Loco::get_speed_steps (uint_fast16_t loco_idx)
{
    if (loco_idx < Loco::n_locos)
    {
        return (Loco::locos[loco_idx].speed_steps);
    }
    return 0;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  set_flags (idx) - set flags
 *------------------------------------------------------------------------------------------------------------------------
 */
void
Loco::set_flags (uint_fast16_t loco_idx, uint32_t flags)
{
    if (loco_idx < Loco::n_locos)
    {
        Loco::locos[loco_idx].flags = flags;
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 *  get_flags (idx) - get flags
 *------------------------------------------------------------------------------------------------------------------------
 */
uint32_t
Loco::get_flags (uint_fast16_t loco_idx)
{
    if (loco_idx < Loco::n_locos)
    {
        return Loco::locos[loco_idx].flags;
    }
    return 0;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  set_flag_halt (idx) - set flag HALT
 *------------------------------------------------------------------------------------------------------------------------
 */
void
Loco::set_flag_halt (uint_fast16_t loco_idx)
{
    if (loco_idx < Loco::n_locos)
    {
        Loco::locos[loco_idx].flags |= LOCO_FLAG_HALT;
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 *  reset_flag_halt (idx) - reset flag HALT
 *------------------------------------------------------------------------------------------------------------------------
 */
void
Loco::reset_flag_halt (uint_fast16_t loco_idx)
{
    if (loco_idx < Loco::n_locos)
    {
        Loco::locos[loco_idx].flags &= ~LOCO_FLAG_HALT;
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 *  get_flag_halt (idx) - get flag HALT
 *------------------------------------------------------------------------------------------------------------------------
 */
bool
Loco::get_flag_halt (uint_fast16_t loco_idx)
{
    bool rtc = false;

    if (loco_idx < Loco::n_locos)
    {
        if (Loco::locos[loco_idx].flags & LOCO_FLAG_HALT)
        {
            rtc = true;
        }
    }

    return rtc;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  Loco::add_macro_action () - add macro action
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
Loco::add_macro_action (uint_fast16_t loco_idx, uint_fast8_t macroidx)
{
    uint_fast8_t actionidx;

    if (loco_idx < Loco::n_locos && Loco::locos[loco_idx].macros[macroidx].n_actions < LOCO_MAX_ACTIONS_PER_MACRO - 1)
    {
        actionidx = Loco::locos[loco_idx].macros[macroidx].n_actions;
        Loco::locos[loco_idx].macros[macroidx].actions[actionidx].action        = LOCO_ACTION_NONE;
        Loco::locos[loco_idx].macros[macroidx].actions[actionidx].n_parameters  = 0;
        Loco::locos[loco_idx].macros[macroidx].n_actions++;
        Loco::data_changed = true;
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
Loco::delete_macro_action (uint_fast16_t loco_idx, uint_fast8_t macroidx, uint_fast8_t actionidx)
{
    uint_fast8_t    n_parameters;
    uint_fast8_t    pidx;

    if (actionidx < Loco::locos[loco_idx].macros[macroidx].n_actions)
    {
        while (actionidx < Loco::locos[loco_idx].macros[macroidx].n_actions - 1)
        {
            n_parameters = Loco::locos[loco_idx].macros[macroidx].actions[actionidx + 1].n_parameters;

            Loco::locos[loco_idx].macros[macroidx].actions[actionidx].action        = Loco::locos[loco_idx].macros[macroidx].actions[actionidx + 1].action;
            Loco::locos[loco_idx].macros[macroidx].actions[actionidx].n_parameters  = n_parameters;

            for (pidx = 0; pidx < n_parameters; pidx++)
            {
                Loco::locos[loco_idx].macros[macroidx].actions[actionidx].parameters[pidx]  = Loco::locos[loco_idx].macros[macroidx].actions[actionidx + 1].parameters[pidx];
            }

            actionidx++;
        }

        Loco::locos[loco_idx].macros[macroidx].n_actions--;
        Loco::data_changed = true;
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 *  Loco::get_n_macro_actions () - get number of macro actions
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
Loco::get_n_macro_actions (uint_fast16_t loco_idx, uint_fast8_t macroidx)
{
    uint_fast8_t    rtc;
    rtc = Loco::locos[loco_idx].macros[macroidx].n_actions;
    return rtc;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  Loco::set_macro_action () - set macro action
 *------------------------------------------------------------------------------------------------------------------------
 */
bool
Loco::set_macro_action (uint_fast16_t loco_idx, uint_fast8_t macroidx, uint_fast8_t actionidx, LOCOACTION * lap)
{
    uint_fast8_t rtc = false;

    if (actionidx < Loco::locos[loco_idx].macros[macroidx].n_actions)
    {
        uint_fast8_t pidx;

        Loco::locos[loco_idx].macros[macroidx].actions[actionidx].action        = lap->action;
        Loco::locos[loco_idx].macros[macroidx].actions[actionidx].n_parameters  = lap->n_parameters;

        for (pidx = 0; pidx < lap->n_parameters; pidx++)
        {
            Loco::locos[loco_idx].macros[macroidx].actions[actionidx].parameters[pidx] = lap->parameters[pidx];
        }

        Loco::data_changed = true;
        rtc = true;
    }

    return rtc;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  Loco::get_macro_action () - get macro action
 *------------------------------------------------------------------------------------------------------------------------
 */
bool
Loco::get_macro_action (uint_fast16_t loco_idx, uint_fast8_t macroidx, uint_fast8_t actionidx, LOCOACTION * lap)
{
    uint_fast8_t rtc = false;

    if (actionidx < Loco::locos[loco_idx].macros[macroidx].n_actions)
    {
        uint_fast8_t pidx;

        lap->action         = Loco::locos[loco_idx].macros[macroidx].actions[actionidx].action;
        lap->n_parameters   = Loco::locos[loco_idx].macros[macroidx].actions[actionidx].n_parameters;

        for (pidx = 0; pidx < lap->n_parameters; pidx++)
        {
            lap->parameters[pidx] = Loco::locos[loco_idx].macros[macroidx].actions[actionidx].parameters[pidx];
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
Loco::execute_macro (uint_fast16_t loco_idx, uint_fast8_t macroidx)
{
    LOCOACTION *    lap;
    uint_fast8_t    n_actions;
    uint_fast8_t    actionidx;

    n_actions = Loco::locos[loco_idx].macros[macroidx].n_actions;
    lap = Loco::locos[loco_idx].macros[macroidx].actions;

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
                    Loco::set_speed (loco_idx, speed, tenths);
                }
                else
                {
                    Event::add_event_loco_speed (start, loco_idx, EVENT_SET_LOCO_SPEED, speed, tenths);
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
                    uint_fast8_t current_speed = Loco::get_speed (loco_idx);

                    if (current_speed < speed)
                    {
                        Loco::set_speed (loco_idx, speed, tenths);
                    }
                }
                else
                {
                    Event::add_event_loco_speed (start, loco_idx, EVENT_SET_LOCO_MIN_SPEED, speed, tenths);
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
                    uint_fast8_t current_speed = Loco::get_speed (loco_idx);

                    if (current_speed > speed)
                    {
                        Loco::set_speed (loco_idx, speed, tenths);
                    }
                }
                else
                {
                    Event::add_event_loco_speed (start, loco_idx, EVENT_SET_LOCO_MAX_SPEED, speed, tenths);
                }
                break;
            }

            case LOCO_ACTION_SET_LOCO_FORWARD_DIRECTION:                                // parameters: START, FWD
            {
                uint_fast32_t   start       = lap[actionidx].parameters[0];
                uint_fast8_t    fwd         = lap[actionidx].parameters[1];

                if (start == 0)
                {
                    Loco::set_fwd (loco_idx, fwd);
                }
                else
                {
                    Event::add_event_loco_dir (start, loco_idx, fwd);
                }
                break;
            }

            case LOCO_ACTION_SET_LOCO_ALL_FUNCTIONS_OFF:                                // parameters: START
            {
                uint_fast32_t   start       = lap[actionidx].parameters[0];

                if (start == 0)
                {
                    Loco::reset_functions (loco_idx);
                }
                else
                {
                    Event::add_event_loco_function (start, loco_idx, 0xFF, false);
                }
                break;
            }

            case LOCO_ACTION_SET_LOCO_FUNCTION_OFF:                                     // parameters: START, FUNC_IDX
            {
                uint_fast32_t   start       = lap[actionidx].parameters[0];
                uint_fast8_t    f           = lap[actionidx].parameters[1];

                if (start == 0)
                {
                    Loco::set_function (loco_idx, f, false);
                }
                else
                {
                    Event::add_event_loco_function (start, loco_idx, f, false);
                }
                break;
            }

            case LOCO_ACTION_SET_LOCO_FUNCTION_ON:                                      // parameters: START, FUNC_IDX
            {
                uint_fast32_t   start       = lap[actionidx].parameters[0];
                uint_fast8_t    f           = lap[actionidx].parameters[1];

                if (start == 0)
                {
                    Loco::set_function (loco_idx, f, true);
                }
                else
                {
                    Event::add_event_loco_function (start, loco_idx, f, true);
                }
                break;
            }

            case LOCO_ACTION_SET_ADDON_ALL_FUNCTIONS_OFF:                               // parameters: START
            {
                uint_fast32_t   start       = lap[actionidx].parameters[0];
                uint_fast16_t   addon_idx   = locos[loco_idx].addon_idx;

                if (addon_idx != 0xFFFF)
                {
                    if (start == 0)
                    {
                        AddOn::reset_functions (addon_idx);
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
                uint_fast16_t   addon_idx   = locos[loco_idx].addon_idx;

                if (addon_idx != 0xFFFF)
                {
                    if (start == 0)
                    {
                        AddOn::set_function (addon_idx, f, false);
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
                uint_fast16_t   addon_idx   = locos[loco_idx].addon_idx;

                if (addon_idx != 0xFFFF)
                {
                    if (start == 0)
                    {
                        AddOn::set_function (addon_idx, f, true);
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
 *  set_function_type_pulse (idx, f, b) - set/reset function to type "pulse"
 *------------------------------------------------------------------------------------------------------------------------
 */
void
Loco::set_function_type_pulse (uint_fast16_t loco_idx, uint_fast8_t f, bool b)
{
    if (loco_idx < Loco::n_locos)
    {
        uint32_t    f_mask = 1 << f;

        if (b)
        {
            Loco::locos[loco_idx].locofunction.pulse_mask |= f_mask;
        }
        else
        {
            Loco::locos[loco_idx].locofunction.pulse_mask &= ~f_mask;
        }

        Loco::data_changed = true;
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 *  set_function_type_sound (idx, f, b) - set/reset function to type "sound"
 *------------------------------------------------------------------------------------------------------------------------
 */
void
Loco::set_function_type_sound (uint_fast16_t loco_idx, uint_fast8_t f, bool b)
{
    if (loco_idx < Loco::n_locos)
    {
        uint32_t    f_mask = 1 << f;

        if (b)
        {
            Loco::locos[loco_idx].locofunction.sound_mask |= f_mask;
        }
        else
        {
            Loco::locos[loco_idx].locofunction.sound_mask &= ~f_mask;
        }

        Loco::data_changed = true;
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 *  set_function_type (idx, fidx, name) - set function type
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
Loco::set_function_type (uint_fast16_t loco_idx, uint_fast8_t fidx, uint_fast16_t function_name_idx, bool pulse, bool sound)
{
    if (loco_idx < Loco::n_locos)
    {
        if (Loco::locos[loco_idx].locofunction.max < fidx)
        {
            Loco::locos[loco_idx].locofunction.max = fidx;
        }

        Loco::locos[loco_idx].locofunction.name_idx[fidx] = function_name_idx;
        Loco::set_function_type_pulse (loco_idx, fidx, pulse);
        Loco::set_function_type_sound (loco_idx, fidx, sound);
        return 1;
    }
    return 0;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  get_function_name () - get function name
 *------------------------------------------------------------------------------------------------------------------------
 */
std::string&
Loco::get_function_name (uint_fast16_t loco_idx, uint_fast8_t fidx)
{
    if (loco_idx < Loco::n_locos)
    {
        uint_fast16_t    function_name_idx = Loco::locos[loco_idx].locofunction.name_idx[fidx];
        return Functions::get (function_name_idx);
    }
    return Functions::get (0xFFFF);
}

/*------------------------------------------------------------------------------------------------------------------------
 *  get_function_name_idx (idx, f) - get function name index
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast16_t
Loco::get_function_name_idx (uint_fast16_t loco_idx, uint_fast8_t fidx)
{
    if (loco_idx < Loco::n_locos)
    {
        uint_fast16_t    function_name_idx = Loco::locos[loco_idx].locofunction.name_idx[fidx];
        return function_name_idx;
    }
    return 0;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  get_function_pulse (idx, f) - get function of type "pulse"
 *------------------------------------------------------------------------------------------------------------------------
 */
bool
Loco::get_function_pulse (uint_fast16_t loco_idx, uint_fast8_t fidx)
{
    if (loco_idx < Loco::n_locos)
    {
        uint32_t        f_mask = 1 << fidx;
        bool            b = false;

        if (Loco::locos[loco_idx].locofunction.pulse_mask & f_mask)
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
Loco::get_function_pulse_mask (uint_fast16_t loco_idx)
{
    if (loco_idx < Loco::n_locos)
    {
        return Loco::locos[loco_idx].locofunction.pulse_mask;
    }
    return 0;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  get_function_sound (idx, f) - get function of type "sound"
 *------------------------------------------------------------------------------------------------------------------------
 */
bool
Loco::get_function_sound (uint_fast16_t loco_idx, uint_fast8_t fidx)
{
    if (loco_idx < Loco::n_locos)
    {
        uint32_t        f_mask = 1 << fidx;
        bool            b = false;

        if (Loco::locos[loco_idx].locofunction.sound_mask & f_mask)
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
Loco::get_function_sound_mask (uint_fast16_t loco_idx)
{
    if (loco_idx < Loco::n_locos)
    {
        return Loco::locos[loco_idx].locofunction.sound_mask;
    }
    return 0;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  activate (idx) - activate a loco
 *------------------------------------------------------------------------------------------------------------------------
 */
void
Loco::activate (uint_fast16_t loco_idx)
{
    if (loco_idx < Loco::n_locos)
    {
        if (! Loco::locos[loco_idx].active)
        {
            Loco::locos[loco_idx].active = true;
        }
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 *  deactivate_loco (idx) - deactivate a loco
 *------------------------------------------------------------------------------------------------------------------------
 */
void
Loco::deactivate (uint_fast16_t loco_idx)
{
    if (loco_idx < Loco::n_locos)
    {
        if (Loco::locos[loco_idx].active)
        {
            Loco::locos[loco_idx].active = false;
        }
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 *  is_active (idx) - check if loco is active
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
Loco::is_active (uint_fast16_t loco_idx)
{
    if (loco_idx < Loco::n_locos && Loco::locos[loco_idx].active)
    {
        return true;
    }
    return false;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  set_online (idx) - set timestamp if loco has been detected by global RAILCOM detector
 *------------------------------------------------------------------------------------------------------------------------
 */
#define MAX_OFFLINE_COUNTER_VALUE       4
void
Loco::set_online (uint_fast16_t loco_idx, bool value)
{
    if (loco_idx < Loco::n_locos)
    {
        if (value)
        {
            Loco::locos[loco_idx].offline_cnt = 0;
            Loco::locos[loco_idx].flags |= LOCO_FLAG_ONLINE;
        }
        else
        {
            if (Loco::locos[loco_idx].offline_cnt < MAX_OFFLINE_COUNTER_VALUE)
            {
                Loco::locos[loco_idx].offline_cnt++;
            }

            if (Loco::locos[loco_idx].offline_cnt == MAX_OFFLINE_COUNTER_VALUE)
            {
                Loco::locos[loco_idx].flags &= ~LOCO_FLAG_ONLINE;
            }
        }
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 *  is_online (idx) - check if loco is online
 *------------------------------------------------------------------------------------------------------------------------
 */
bool
Loco::is_online (uint_fast16_t loco_idx)
{
    if (loco_idx < Loco::n_locos && Loco::locos[loco_idx].flags & LOCO_FLAG_ONLINE)
    {
        return true;
    }
    return false;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  set_speed (idx, speed) - set speed
 *------------------------------------------------------------------------------------------------------------------------
 */
void
Loco::set_speed (uint_fast16_t loco_idx, uint_fast8_t speed)
{
    if (loco_idx < Loco::n_locos)
    {
        if (! (Loco::locos[loco_idx].flags & LOCO_FLAG_HALT))
        {
            Loco::locos[loco_idx].speed                 = speed;
            Loco::locos[loco_idx].target_next_millis    = 0;
            Loco::sendspeed (loco_idx);
        }
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 *  set_speed (idx, tspeed, tenths) - set speed within tenths of a second
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
Loco::set_speed (uint_fast16_t loco_idx, uint_fast8_t tspeed, uint_fast16_t tenths)
{
    // printf ("loco_idx=%u tspeed=%u, tenths=%u\n", loco_idx, tspeed, tenths);

    if (loco_idx < Loco::n_locos)
    {
        if (! (Loco::locos[loco_idx].flags & LOCO_FLAG_HALT))
        {
            if (tenths > 0)
            {
                uint_fast8_t    cspeed = Loco::locos[loco_idx].speed;       // current speed

                if (tspeed > cspeed)
                {
                    Loco::locos[loco_idx].target_speed          = tspeed;
                    Loco::locos[loco_idx].target_millis_step    = (100 * tenths) / (tspeed - cspeed);
                    Loco::locos[loco_idx].target_next_millis    = Millis::elapsed() + Loco::locos[loco_idx].target_millis_step;
                }
                else if (tspeed < cspeed)
                {
                    Loco::locos[loco_idx].target_speed          = tspeed;
                    Loco::locos[loco_idx].target_millis_step    = (100 * tenths) / (cspeed - tspeed);
                    Loco::locos[loco_idx].target_next_millis    = Millis::elapsed() + Loco::locos[loco_idx].target_millis_step;
                }
                // if speeds are identical, do nothing
            }
            else // tenths are 0, set speed immediately
            {
                Loco::set_speed (loco_idx, tspeed);
            }
        }
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 *  get_speed (idx) - get speed
 *------------------------------------------------------------------------------------------------------------------------
 */
uint16_t
Loco::get_speed (uint_fast16_t loco_idx)
{
    if (loco_idx < Loco::n_locos)
    {
        return (Loco::locos[loco_idx].speed);
    }
    return 0;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  set_fwd (idx, fwd) - set forward direction
 *------------------------------------------------------------------------------------------------------------------------
 */
void
Loco::set_fwd (uint_fast16_t loco_idx, uint_fast8_t fwd)
{
    if (loco_idx < Loco::n_locos)
    {
        if (Loco::locos[loco_idx].fwd != fwd)
        {
            uint_fast16_t addon_idx;

            Loco::locos[loco_idx].speed = 0;
            Loco::locos[loco_idx].fwd = fwd;
            Loco::sendspeed (loco_idx);

            addon_idx = Loco::locos[loco_idx].addon_idx;

            if (addon_idx != 0xFFFF)
            {
                Loco::locos[addon_idx].fwd = fwd;
                Loco::sendspeed (addon_idx);
                Debug::printf (DEBUG_LEVEL_VERBOSE, "Loco::set_fwd %u to addon %u\n", fwd, addon_idx);
            }
        }
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 *  set_speed_fwd (idx, speed) - set speed and direction
 *------------------------------------------------------------------------------------------------------------------------
 */
void
Loco::set_speed_fwd (uint_fast16_t loco_idx, uint_fast8_t speed, uint_fast8_t fwd)
{
    if (loco_idx < Loco::n_locos)
    {
        Loco::locos[loco_idx].speed = speed;
        Loco::locos[loco_idx].fwd = fwd;
        Loco::sendspeed (loco_idx);
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 *  get_fwd (idx) - get direction
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
Loco::get_fwd (uint_fast16_t loco_idx)
{
    if (loco_idx < Loco::n_locos)
    {
        return Loco::locos[loco_idx].fwd;
    }

    return 0;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  set_function (idx, f) - set a function
 *------------------------------------------------------------------------------------------------------------------------
 */
void
Loco::set_function (uint_fast16_t loco_idx, uint_fast8_t f, bool b)
{
    if (loco_idx < Loco::n_locos)
    {
        uint32_t        fmask = 1 << f;
        uint_fast16_t   addon_idx = Loco::locos[loco_idx].addon_idx;
        uint_fast8_t    range;

        if (b)
        {
            Loco::locos[loco_idx].functions |= fmask;

            if (fmask & Loco::locos[loco_idx].locofunction.pulse_mask)          // function is of type pulse
            {
                Event::add_event_loco_function (2, loco_idx, f, false);         // reset function in 200 msec
            }
        }
        else
        {
            Loco::locos[loco_idx].functions &= ~fmask;
        }

        if (addon_idx != 0xFFFF)
        {
            uint_fast8_t cf = Loco::locos[loco_idx].coupled_functions[f];

            if (cf != 0xFF)
            {
                AddOn::set_function (addon_idx, cf, b);
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

        Loco::sendfunction (loco_idx, range);
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 *  get_function (idx, f)
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
Loco::get_function (uint_fast16_t loco_idx, uint_fast8_t f)
{
    if (loco_idx < Loco::n_locos)
    {
        uint32_t mask = 1 << f;
        return (Loco::locos[loco_idx].functions & mask) ? true : false;
    }
    return false;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  reset_functions (idx)
 *------------------------------------------------------------------------------------------------------------------------
 */
void
Loco::reset_functions (uint_fast16_t loco_idx)
{
    if (loco_idx < Loco::n_locos)
    {
        Loco::locos[loco_idx].functions = 0;       // let scheduler turn off functions

        uint_fast16_t addon_idx = Loco::locos[loco_idx].addon_idx;

        if (addon_idx != 0xFFFF)
        {
            Loco::locos[addon_idx].functions = 0;
        }
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 *  get_functions (idx, f)
 *------------------------------------------------------------------------------------------------------------------------
 */
uint32_t
Loco::get_functions (uint_fast16_t loco_idx)
{
    if (loco_idx < Loco::n_locos)
    {
        return Loco::locos[loco_idx].functions;
    }
    return 0;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  set_rcllocation (uint_fast16_t loco_idx, uint_fast8_t location)
 *------------------------------------------------------------------------------------------------------------------------
 */
void
Loco::set_rcllocation (uint_fast16_t loco_idx, uint_fast8_t location)
{
    Debug::printf (DEBUG_LEVEL_NORMAL, "Loco::set_rcllocation: loco_idx=%u location=%u\n", loco_idx, location);

    if (loco_idx < Loco::n_locos)
    {
        Loco::locos[loco_idx].rcl_location = location;
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 *  get_rcllocation (uint_fast16_t loco_idx)
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
Loco::get_rcllocation (uint_fast16_t loco_idx)
{
    uint_fast8_t    rtc = 0xFF;

    if (loco_idx < Loco::n_locos)
    {
        rtc = Loco::locos[loco_idx].rcl_location;
    }

    return rtc;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  set_rrlocation (uint_fast16_t loco_idx, uint_fast8_t rrgidx, uint_fast8_t rridx)
 *------------------------------------------------------------------------------------------------------------------------
 */
void
Loco::set_rrlocation (uint_fast16_t loco_idx, uint_fast16_t rrgrridx)
{
    Debug::printf (DEBUG_LEVEL_NORMAL, "Loco::set_rrlocation: loco_idx=%u rrgidx=%u rridx=%u\n", loco_idx, rrgrridx >> 8, rrgrridx & 0xFF);

    if (loco_idx < Loco::n_locos)
    {
        Loco::locos[loco_idx].rr_location = rrgrridx;                   // maybe 0xFFFF
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 *  get_rrlocation (uint_fast16_t loco_idx)
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast16_t
Loco::get_rrlocation (uint_fast16_t loco_idx)
{
    uint_fast16_t   rtc = 0xFFFF;

    if (loco_idx < Loco::n_locos)
    {
        rtc = Loco::locos[loco_idx].rr_location;
    }

    return rtc;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  estop (loco_idx)
 *------------------------------------------------------------------------------------------------------------------------
 */
void
Loco::estop (uint_fast16_t loco_idx)
{
    if (loco_idx < Loco::n_locos)
    {
        Loco::set_speed (loco_idx, 1);
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 *  estop (void)
 *------------------------------------------------------------------------------------------------------------------------
 */
void
Loco::estop (void)
{
    uint_fast16_t loco_idx;

    DCC::estop ();

    for (loco_idx = 0; loco_idx < Loco::n_locos; loco_idx++)
    {
        Loco::locos[loco_idx].speed = 1;
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 *  get_ack (void)
 *------------------------------------------------------------------------------------------------------------------------
 */
void
Loco::get_ack (uint_fast16_t loco_idx)
{
    if (loco_idx < Loco::n_locos)
    {
        DCC::get_ack (Loco::locos[loco_idx].addr);
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 *  booster_off (void)
 *------------------------------------------------------------------------------------------------------------------------
 */
void
Loco::booster_off (bool do_send_booster_cmd)
{
    uint_fast16_t loco_idx;

    if (do_send_booster_cmd)
    {
        DCC::booster_off ();
    }

    for (loco_idx = 0; loco_idx < Loco::n_locos; loco_idx++)
    {
        Loco::locos[loco_idx].speed         = 0;
        Loco::locos[loco_idx].rcl_location  = 0xFF;
        Loco::locos[loco_idx].rr_location   = 0xFFFF;
        Loco::locos[loco_idx].rc2_rate      = 0;
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 *  booster_on (void)
 *------------------------------------------------------------------------------------------------------------------------
 */
void
Loco::booster_on (bool do_send_booster_cmd)
{
    uint_fast16_t loco_idx;

    for (loco_idx = 0; loco_idx < Loco::n_locos; loco_idx++)
    {
        Loco::locos[loco_idx].speed         = 0;
        Loco::locos[loco_idx].rcl_location  = 0xFF;
        Loco::locos[loco_idx].rr_location   = 0xFFFF;
    }

    if (do_send_booster_cmd)
    {
        DCC::booster_on ();
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 *  set_rc2_rate () - set RC2 rate (0% - 100%)
 *------------------------------------------------------------------------------------------------------------------------
 */
void
Loco::set_rc2_rate (uint_fast16_t loco_idx, uint_fast8_t rate)
{
    if (loco_idx < Loco::n_locos)
    {
        Debug::printf (DEBUG_LEVEL_VERBOSE, "Loco::set_rc2_rate: loco_idx=%u rate=%u\n", (uint16_t) loco_idx, (uint16_t) rate);
        Loco::locos[loco_idx].rc2_rate = rate;
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 *  get_rc2_rate () - get RC2 rate (0% - 100%)
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
Loco::get_rc2_rate (uint_fast16_t loco_idx)
{
    if (loco_idx < Loco::n_locos)
    {
        Debug::printf (DEBUG_LEVEL_VERBOSE, "Loco::get_rc2_rate: loco_idx=%u rate=%u\n", (uint16_t) loco_idx, (uint16_t) Loco::locos[loco_idx].rc2_rate);
        return Loco::locos[loco_idx].rc2_rate;
    }
    return 0;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  schedule () - schedule all locos
 *------------------------------------------------------------------------------------------------------------------------
 */
bool
Loco::schedule (void)
{
    static bool             handle_locos = true;
    static uint_fast16_t    loco_idx = 0;
    bool                    rtc = false;

    if (handle_locos)
    {
        if (loco_idx < Loco::n_locos)
        {
            if (Loco::locos[loco_idx].active && Loco::locos[loco_idx].addr != 0)
            {
                uint_fast8_t packet_sequence_idx = Loco::locos[loco_idx].packet_sequence_idx;

                Loco::sendcmd (loco_idx, packet_sequence_idx);
                packet_sequence_idx++;

                if (packet_sequence_idx >= MAX_PACKET_SEQUENCES)
                {
                    packet_sequence_idx = 0;
                }

                Loco::locos[loco_idx].packet_sequence_idx = packet_sequence_idx;
            }

            loco_idx++;

            if (loco_idx >= Loco::n_locos)
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
        rtc = AddOn::schedule ();

        if (rtc)
        {
            handle_locos = true;
        }
    }

    return rtc;
}
