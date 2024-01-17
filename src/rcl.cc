/*------------------------------------------------------------------------------------------------------------------------
 * rcl.cc - RailCom local management functions
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
#include "http.h"
#include "event.h"
#include "dcc.h"
#include "switch.h"
#include "sig.h"
#include "loco.h"
#include "addon.h"
#include "led.h"
#include "railroad.h"
#include "s88.h"
#include "debug.h"
#include "rcl.h"

uint8_t                             RCL::locations[MAX_LOCOS];
std::vector<RCL_Track>              RCL::tracks;
uint_fast8_t                        RCL::n_tracks = 0;                        // number of tracks
bool                                RCL::data_changed = false;

RCL_Track::RCL_Track ()
{
    this->name                  = "";
    this->loco_idx              = 0xFFFF;
    this->last_loco_idx         = 0xFFFF;
    this->n_track_actions_in    = 0;
    this->n_track_actions_out   = 0;
    this->flags                 = RCL_TRACK_FLAG_NONE;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  RCL_Track::set_name (name) - set name of track
 *------------------------------------------------------------------------------------------------------------------------
 */
void
RCL_Track::set_name (std::string name)
{
    this->name = name;
    RCL::data_changed = true;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  RCL_Track::get_name () - get name of track
 *------------------------------------------------------------------------------------------------------------------------
 */
std::string
RCL_Track::get_name ()
{
    return this->name;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  RCL_Track::set_flags () - set flags of track
 *------------------------------------------------------------------------------------------------------------------------
 */
void
RCL_Track::set_flags (uint32_t flags)
{
    this->flags |= flags;
    RCL::data_changed = true;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  RCL_Track::reset_flags () - set flags of track
 *------------------------------------------------------------------------------------------------------------------------
 */
void
RCL_Track::reset_flags (uint32_t flags)
{
    this->flags &= ~flags;
    RCL::data_changed = true;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  RCL_Track::get_flags () - get flags of track
 *------------------------------------------------------------------------------------------------------------------------
 */
uint32_t
RCL_Track::get_flags ()
{
    return this->flags;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  RCL_Track::add_track_action () - add a track action
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
RCL_Track::add_track_action (bool in)
{
    uint_fast8_t track_action_idx;

    if (in)
    {
        if (this->n_track_actions_in < RCL_MAX_ACTIONS_PER_RCL_TRACK)
        {
            track_action_idx = this->n_track_actions_in;
            this->track_actions_in[track_action_idx].condition              = RCL_CONDITION_NEVER;
            this->track_actions_in[track_action_idx].condition_destination  = 0xFF;
            this->track_actions_in[track_action_idx].action                 = RCL_ACTION_NONE;
            this->track_actions_in[track_action_idx].n_parameters           = 0;
            this->n_track_actions_in++;
            RCL::data_changed = true;
        }
        else
        {
            track_action_idx = 0xFF;
        }
    }   
    else
    {
        if (this->n_track_actions_out < RCL_MAX_ACTIONS_PER_RCL_TRACK)
        {
            track_action_idx = this->n_track_actions_out;
            this->track_actions_out[track_action_idx].condition             = RCL_CONDITION_NEVER;
            this->track_actions_out[track_action_idx].condition_destination = 0xFF;
            this->track_actions_out[track_action_idx].action                = RCL_ACTION_NONE;
            this->track_actions_out[track_action_idx].n_parameters          = 0;
            this->n_track_actions_out++;
            RCL::data_changed = true;
        }
        else
        {
            track_action_idx = 0xFF;
        }
    }   


    return track_action_idx;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  RCL_Track::delete_track_action () - delete a track action
 *------------------------------------------------------------------------------------------------------------------------
 */
void
RCL_Track::delete_track_action (bool in, uint_fast8_t track_action_idx)
{
    uint_fast8_t    n_parameters;
    uint_fast8_t    pidx;

    if (in)
    {
        if (track_action_idx < this->n_track_actions_in)
        {
            while (track_action_idx < this->n_track_actions_in - 1)
            {
                n_parameters = this->track_actions_in[track_action_idx + 1].n_parameters;

                this->track_actions_in[track_action_idx].condition              = this->track_actions_in[track_action_idx + 1].condition;
                this->track_actions_in[track_action_idx].condition_destination  = this->track_actions_in[track_action_idx + 1].condition_destination;
                this->track_actions_in[track_action_idx].action                 = this->track_actions_in[track_action_idx + 1].action;
                this->track_actions_in[track_action_idx].n_parameters           = n_parameters;

                for (pidx = 0; pidx < n_parameters; pidx++)
                {
                    this->track_actions_in[track_action_idx].parameters[pidx]   = this->track_actions_in[track_action_idx + 1].parameters[pidx];
                }

                track_action_idx++;
            }

            this->n_track_actions_in--;
            RCL::data_changed = true;
        }
    }
    else
    {
        if (track_action_idx < this->n_track_actions_out)
        {
            while (track_action_idx < this->n_track_actions_out - 1)
            {
                n_parameters = this->track_actions_out[track_action_idx + 1].n_parameters;

                this->track_actions_out[track_action_idx].condition             = this->track_actions_out[track_action_idx + 1].condition;
                this->track_actions_out[track_action_idx].condition_destination = this->track_actions_out[track_action_idx + 1].condition_destination;
                this->track_actions_out[track_action_idx].action                = this->track_actions_out[track_action_idx + 1].action;
                this->track_actions_out[track_action_idx].n_parameters          = n_parameters;

                for (pidx = 0; pidx < n_parameters; pidx++)
                {
                    this->track_actions_out[track_action_idx].parameters[pidx]   = this->track_actions_out[track_action_idx + 1].parameters[pidx];
                }

                track_action_idx++;
            }

            this->n_track_actions_out--;
            RCL::data_changed = true;
        }
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 *  RCL_Track::get_n_track_actions () - get number of track actions
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
RCL_Track::get_n_track_actions (bool in)
{
    if (in)
    {
        return this->n_track_actions_in;
    }
    return this->n_track_actions_out;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  RCL_Track::set_track_action () - set track action
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
RCL_Track::set_track_action (bool in, uint_fast8_t track_action_idx, RCL_TRACK_ACTION * trap)
{
    uint_fast8_t rtc = 0;

    if (in)
    {
        if (track_action_idx < this->n_track_actions_in)
        {
            uint_fast8_t paidx;

            this->track_actions_in[track_action_idx].condition              = trap->condition;
            this->track_actions_in[track_action_idx].condition_destination  = trap->condition_destination;
            this->track_actions_in[track_action_idx].action                 = trap->action;
            this->track_actions_in[track_action_idx].n_parameters           = trap->n_parameters;

            for (paidx = 0; paidx < trap->n_parameters; paidx++)
            {
                this->track_actions_in[track_action_idx].parameters[paidx] = trap->parameters[paidx];
            }

            RCL::data_changed = true;
            rtc = 1;
        }
    }
    else
    {
        if (track_action_idx < this->n_track_actions_out)
        {
            uint_fast8_t paidx;

            this->track_actions_out[track_action_idx].condition             = trap->condition;
            this->track_actions_out[track_action_idx].condition_destination = trap->condition_destination;
            this->track_actions_out[track_action_idx].action                = trap->action;
            this->track_actions_out[track_action_idx].n_parameters          = trap->n_parameters;

            for (paidx = 0; paidx < trap->n_parameters; paidx++)
            {
                this->track_actions_out[track_action_idx].parameters[paidx] = trap->parameters[paidx];
            }

            RCL::data_changed = true;
            rtc = 1;
        }
    }

    return rtc;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  RCL_Track::get_track_action () - get track action
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
RCL_Track::get_track_action (bool in, uint_fast8_t track_action_idx, RCL_TRACK_ACTION * trap)
{
    uint_fast8_t rtc = 0;

    if (in)
    {
        if (track_action_idx < this->n_track_actions_in)
        {
            uint_fast8_t paidx;

            trap->condition             = this->track_actions_in[track_action_idx].condition;
            trap->condition_destination = this->track_actions_in[track_action_idx].condition_destination;
            trap->action                = this->track_actions_in[track_action_idx].action;
            trap->n_parameters          = this->track_actions_in[track_action_idx].n_parameters;

            for (paidx = 0; paidx < trap->n_parameters; paidx++)
            {
                trap->parameters[paidx] = this->track_actions_in[track_action_idx].parameters[paidx];
            }
            rtc = 1;
        }
    }
    else
    {
        if (track_action_idx < this->n_track_actions_out)
        {
            uint_fast8_t paidx;

            trap->condition             = this->track_actions_out[track_action_idx].condition;
            trap->condition_destination = this->track_actions_out[track_action_idx].condition_destination;
            trap->action                = this->track_actions_out[track_action_idx].action;
            trap->n_parameters          = this->track_actions_out[track_action_idx].n_parameters;

            for (paidx = 0; paidx < trap->n_parameters; paidx++)
            {
                trap->parameters[paidx] = this->track_actions_out[track_action_idx].parameters[paidx];
            }
            rtc = 1;
        }
    }

    return rtc;
}

void
RCL::reset_all_locations (void)
{
    uint_fast16_t   loco_idx;
    uint_fast8_t    track_idx;

    for (loco_idx = 0; loco_idx < MAX_LOCOS; loco_idx++)
    {
        RCL::locations[loco_idx] = 0xFF;
    }

    for (track_idx = 0; track_idx < n_tracks; track_idx++)
    {
        RCL::tracks[track_idx].loco_idx         = 0xFFFF;
        RCL::tracks[track_idx].last_loco_idx    = 0xFFFF;
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 *  RCL::booster_on ()
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
RCL::booster_on (void)
{
    reset_all_locations ();
    return 0;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  RCL::booster_off ()
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
RCL::booster_off (void)
{
    reset_all_locations ();
    return 0;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  RCL::schedule ()
 *------------------------------------------------------------------------------------------------------------------------
 */
void
RCL::schedule (void)
{
    uint_fast16_t   n_locos = Locos::get_n_locos ();
    uint_fast16_t   loco_idx;

    if (DCC::booster_is_on)
    {
        for (loco_idx = 0; loco_idx < n_locos; loco_idx++)
        {
            uint_fast8_t    location        = Locos::locos[loco_idx].get_rcllocation ();
            uint_fast8_t    old_location    = locations[loco_idx];

            if (old_location != location)
            {
                if (location != 0xFF)           // enter rcl track
                {
                    if (location < n_tracks)
                    {
                        if ((tracks[location].flags & RCL_TRACK_FLAG_BLOCK_PROTECTION) && tracks[location].loco_idx != 0xFFFF && tracks[location].loco_idx != loco_idx)
                        {
                            static char buf[80];

                            Locos::locos[loco_idx].set_speed (0);
                            Debug::printf (DEBUG_LEVEL_NONE, "rcl in: loco_idx=%u, but already loco_idx=%u: STOP\n", loco_idx, tracks[location].loco_idx);
                            sprintf (buf, "Blocksicherung: Stop Lok %u", (unsigned int) loco_idx);
                            HTTP::set_alert (buf);
                        }

                        Debug::printf (DEBUG_LEVEL_NORMAL, "executing actions 'in': loco_idx=%u, location=%u\n", loco_idx, location);
                        tracks[location].loco_idx = loco_idx;
                        tracks[location].last_loco_idx = loco_idx;
                        RCL::execute_track_actions (true, loco_idx, location);
                    }
                    else
                    {
                        Debug::printf (DEBUG_LEVEL_VERBOSE, "RCL::schedule in: got location = %u, but n_tracks = %u\n", location, n_tracks);
                    }
                }
                else                            // leave
                {
                    if (old_location < n_tracks)
                    {
                        Debug::printf (DEBUG_LEVEL_NORMAL, "executing actions 'out': loco_idx=%u, location=%u\n", loco_idx, old_location);
                        tracks[old_location].loco_idx = 0xFFFF;
                        RCL::execute_track_actions (false, loco_idx, old_location);
                    }
                    else
                    {
                        Debug::printf (DEBUG_LEVEL_VERBOSE, "RCL::schedule out: got location = %u, but n_tracks = %u\n", locations[loco_idx], n_tracks);
                    }
                }

                locations[loco_idx] = location;
            }
        }
    }
}

void
RCL::set_new_loco_ids_for_track (bool in, uint_fast8_t trackidx, uint16_t * map_new_loco_idx, uint16_t n_locos)
{
    uint_fast8_t    track_action_idx;

    if (trackidx < n_tracks)
    {
        uint_fast8_t    n_track_actions;

        if (in)
        {
            n_track_actions = tracks[trackidx].n_track_actions_in;
        }
        else
        {
            n_track_actions = tracks[trackidx].n_track_actions_out;
        }

        for (track_action_idx = 0; track_action_idx < n_track_actions; track_action_idx++)
        {
            uint_fast8_t    action;
            uint16_t *      param;

            if (in)
            {
                action                  = tracks[trackidx].track_actions_in[track_action_idx].action;
                param                   = tracks[trackidx].track_actions_in[track_action_idx].parameters;
            }
            else
            {
                action                  = tracks[trackidx].track_actions_out[track_action_idx].action;
                param                   = tracks[trackidx].track_actions_out[track_action_idx].parameters;
            }

            switch (action)
            {
                case RCL_ACTION_SET_LOCO_SPEED:                                             // parameters: START, LOCO, SPEED, TENTHS
                case RCL_ACTION_SET_LOCO_MIN_SPEED:                                         // parameters: START, LOCO, SPEED, TENTHS
                case RCL_ACTION_SET_LOCO_MAX_SPEED:                                         // parameters: START, LOCO, SPEED, TENTHS
                case RCL_ACTION_SET_LOCO_FORWARD_DIRECTION:                                 // parameters: START, LOCO, FWD
                case RCL_ACTION_SET_LOCO_ALL_FUNCTIONS_OFF:                                 // parameters: START, LOCO
                case RCL_ACTION_SET_LOCO_FUNCTION_OFF:                                      // parameters: START, LOCO, FUNC_IDX
                case RCL_ACTION_SET_LOCO_FUNCTION_ON:                                       // parameters: START, LOCO, FUNC_IDX
                case RCL_ACTION_EXECUTE_LOCO_MACRO:                                         // parameters: START, LOCO, MACRO_IDX
                case RCL_ACTION_WAIT_FOR_FREE_S88_CONTACT:                                  // parameters: START, LOCO, COIDX, TENTHS
                {
                    uint_fast16_t   loco_idx    = param[1];

                    if (loco_idx < n_locos && loco_idx != map_new_loco_idx[loco_idx])
                    {
                        if (in)
                        {
                            tracks[trackidx].track_actions_in[track_action_idx].parameters[1] = map_new_loco_idx[loco_idx];
                        }
                        else
                        {
                            tracks[trackidx].track_actions_out[track_action_idx].parameters[1] = map_new_loco_idx[loco_idx];
                        }

                        RCL::data_changed = true;
                    }

                    break;
                }
            }
        }
    }
}

void
RCL::set_new_addon_ids_for_track (bool in, uint_fast8_t trackidx, uint16_t * map_new_addon_idx, uint16_t n_addons)
{
    uint_fast8_t    track_action_idx;

    if (trackidx < n_tracks)
    {
        uint_fast8_t    n_track_actions;

        if (in)
        {
            n_track_actions = tracks[trackidx].n_track_actions_in;
        }
        else
        {
            n_track_actions = tracks[trackidx].n_track_actions_out;
        }

        for (track_action_idx = 0; track_action_idx < n_track_actions; track_action_idx++)
        {
            uint_fast8_t    action;
            uint16_t *      param;

            if (in)
            {
                action  = tracks[trackidx].track_actions_in[track_action_idx].action;
                param   = tracks[trackidx].track_actions_in[track_action_idx].parameters;
            }
            else
            {
                action  = tracks[trackidx].track_actions_out[track_action_idx].action;
                param   = tracks[trackidx].track_actions_out[track_action_idx].parameters;
            }

            switch (action)
            {
                case RCL_ACTION_SET_ADDON_ALL_FUNCTIONS_OFF:                                // parameters: START, ADDON
                case RCL_ACTION_SET_ADDON_FUNCTION_OFF:                                     // parameters: START, ADDON, FUNC_IDX
                case RCL_ACTION_SET_ADDON_FUNCTION_ON:                                      // parameters: START, ADDON, FUNC_IDX
                {
                    uint_fast16_t   addon_idx    = param[1];

                    if (addon_idx < n_addons && addon_idx != map_new_addon_idx[addon_idx])
                    {
                        if (in)
                        {
                            tracks[trackidx].track_actions_in[track_action_idx].parameters[1] = map_new_addon_idx[addon_idx];
                        }
                        else
                        {
                            tracks[trackidx].track_actions_out[track_action_idx].parameters[1] = map_new_addon_idx[addon_idx];
                        }
                        RCL::data_changed = true;
                    }

                    break;
                }
            }
        }
    }
}

void
RCL::set_new_switch_ids_for_track (bool in, uint_fast8_t trackidx, uint16_t * map_new_switch_idx, uint16_t n_switches)
{
    uint_fast8_t    track_action_idx;

    if (trackidx < n_tracks)
    {
        uint_fast8_t    n_track_actions;

        if (in)
        {
            n_track_actions = tracks[trackidx].n_track_actions_in;
        }
        else
        {
            n_track_actions = tracks[trackidx].n_track_actions_out;
        }

        for (track_action_idx = 0; track_action_idx < n_track_actions; track_action_idx++)
        {
            uint_fast8_t    action;
            uint16_t *      param;

            if (in)
            {
                action                  = tracks[trackidx].track_actions_in[track_action_idx].action;
                param                   = tracks[trackidx].track_actions_in[track_action_idx].parameters;
            }
            else
            {
                action                  = tracks[trackidx].track_actions_out[track_action_idx].action;
                param                   = tracks[trackidx].track_actions_out[track_action_idx].parameters;
            }

            switch (action)
            {
                case RCL_ACTION_SET_SWITCH:                                                 // parameters: SW_IDX, SW_STATE
                {
                    uint_fast16_t   switch_idx    = param[0];

                    if (switch_idx < n_switches && switch_idx != map_new_switch_idx[switch_idx])
                    {
                        if (in)
                        {
                            tracks[trackidx].track_actions_in[track_action_idx].parameters[0] = map_new_switch_idx[switch_idx];
                        }
                        else
                        {
                            tracks[trackidx].track_actions_out[track_action_idx].parameters[0] = map_new_switch_idx[switch_idx];
                        }

                        RCL::data_changed = true;
                    }

                    break;
                }
            }
        }
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 *  RCL::set_new_loco_ids ()
 *------------------------------------------------------------------------------------------------------------------------
 */
void
RCL::set_new_loco_ids (uint16_t * map_new_loco_idx, uint16_t n_locos)
{
    uint_fast8_t     trackidx;

    for (trackidx = 0; trackidx < RCL::n_tracks; trackidx++)
    {
        if (RCL::tracks[trackidx].loco_idx < n_locos)
        {
            RCL::tracks[trackidx].loco_idx = map_new_loco_idx[RCL::tracks[trackidx].loco_idx];
        }

        if (RCL::tracks[trackidx].last_loco_idx < n_locos)
        {
            RCL::tracks[trackidx].last_loco_idx = map_new_loco_idx[RCL::tracks[trackidx].last_loco_idx];
        }

        RCL::set_new_loco_ids_for_track (true, trackidx, map_new_loco_idx, n_locos);
        RCL::set_new_loco_ids_for_track (false, trackidx, map_new_loco_idx, n_locos);
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 *  RCL::set_new_addon_ids ()
 *------------------------------------------------------------------------------------------------------------------------
 */
void
RCL::set_new_addon_ids (uint16_t * map_new_addon_idx, uint16_t n_addons)
{
    uint_fast8_t     trackidx;

    for (trackidx = 0; trackidx < RCL::n_tracks; trackidx++)
    {
        RCL::set_new_addon_ids_for_track (true, trackidx, map_new_addon_idx, n_addons);
        RCL::set_new_addon_ids_for_track (false, trackidx, map_new_addon_idx, n_addons);
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 *  RCL::set_new_switch_ids ()
 *------------------------------------------------------------------------------------------------------------------------
 */
void
RCL::set_new_switch_ids (uint16_t * map_new_switch_idx, uint16_t n_switches)
{
    uint_fast8_t     trackidx;

    for (trackidx = 0; trackidx < RCL::n_tracks; trackidx++)
    {
        RCL::set_new_switch_ids_for_track (true, trackidx, map_new_switch_idx, n_switches);
        RCL::set_new_switch_ids_for_track (false, trackidx, map_new_switch_idx, n_switches);
    }
}

void
RCL::set_new_railroad_group_ids_for_track (bool in, uint_fast8_t trackidx, uint8_t * map_new_rrgidx, uint_fast8_t n_railroad_groups)
{
    if (trackidx < RCL::n_tracks)
    {
        uint_fast8_t    n_track_actions;
        uint_fast8_t    track_action_idx;

        if (in)
        {
            n_track_actions = tracks[trackidx].n_track_actions_in;
        }
        else
        {
            n_track_actions = tracks[trackidx].n_track_actions_out;
        }

        for (track_action_idx = 0; track_action_idx < n_track_actions; track_action_idx++)
        {
            uint_fast8_t    action;
            uint16_t *      param;

            if (in)
            {
                action  = tracks[trackidx].track_actions_in[track_action_idx].action;
                param   = tracks[trackidx].track_actions_in[track_action_idx].parameters;
            }
            else
            {
                action  = tracks[trackidx].track_actions_out[track_action_idx].action;
                param   = tracks[trackidx].track_actions_out[track_action_idx].parameters;
            }

            switch (action)
            {
                case RCL_ACTION_SET_LINKED_RAILROAD:                                        // parameters: RRG_IDX
                case RCL_ACTION_SET_FREE_RAILROAD:                                          // parameters: RRG_IDX
                {
                    uint_fast8_t    rrgidx  = param[0];

                    if (rrgidx < n_railroad_groups && rrgidx != map_new_rrgidx[rrgidx])
                    {
                        param[0] = map_new_rrgidx[rrgidx];
                        RCL::data_changed = true;
                    }

                    break;
                }

                case RCL_ACTION_SET_LOCO_DESTINATION:                                       // parameters: LOCO_IDX, RRG_IDX
                {
                    uint_fast8_t    rrgidx  = param[1];

                    if (rrgidx < n_railroad_groups && rrgidx != map_new_rrgidx[rrgidx])
                    {
                        param[1] = map_new_rrgidx[rrgidx];
                        RCL::data_changed = true;
                    }

                    break;
                }
            }
        }
    }
}

void
RCL::set_new_railroad_ids_for_track (bool in, uint_fast8_t trackidx, uint_fast8_t current_rrgidx, uint8_t * map_new_rridx, uint_fast8_t n_railroads)
{
    if (trackidx < RCL::n_tracks)
    {
        uint_fast8_t    n_track_actions;
        uint_fast8_t    track_action_idx;

        if (in)
        {
            n_track_actions = tracks[trackidx].n_track_actions_in;
        }
        else
        {
            n_track_actions = tracks[trackidx].n_track_actions_out;
        }

        for (track_action_idx = 0; track_action_idx < n_track_actions; track_action_idx++)
        {
            uint_fast8_t    action;
            uint16_t *      param;

            if (in)
            {
                action  = tracks[trackidx].track_actions_in[track_action_idx].action;
                param   = tracks[trackidx].track_actions_in[track_action_idx].parameters;
            }
            else
            {
                action  = tracks[trackidx].track_actions_out[track_action_idx].action;
                param   = tracks[trackidx].track_actions_out[track_action_idx].parameters;
            }

            switch (action)
            {
                case RCL_ACTION_SET_RAILROAD:                                               // parameters: RRG_IDX, RR_IDX
                {
                    uint_fast8_t    rrgidx  = param[0];
                    uint_fast8_t    rridx   = param[1];

                    if (current_rrgidx == rrgidx && rridx < n_railroads && rridx != map_new_rridx[rridx])
                    {
                        param[1] = map_new_rridx[rridx];
                        RCL::data_changed = true;
                    }

                    break;
                }
            }
        }
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 *  RCL::set_new_railroad_group_ids ()
 *------------------------------------------------------------------------------------------------------------------------
 */
void
RCL::set_new_railroad_group_ids (uint8_t * map_new_rrgidx, uint_fast8_t n_railroad_groups)
{
    uint_fast8_t     trackidx;

    for (trackidx = 0; trackidx < RCL::n_tracks; trackidx++)
    {
        RCL::set_new_railroad_group_ids_for_track (true, trackidx, map_new_rrgidx, n_railroad_groups);
        RCL::set_new_railroad_group_ids_for_track (false, trackidx, map_new_rrgidx, n_railroad_groups);
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 *  RCL::set_new_railroad_ids ()
 *------------------------------------------------------------------------------------------------------------------------
 */
void
RCL::set_new_railroad_ids (uint_fast8_t rrgidx, uint8_t * map_new_rridx, uint_fast8_t n_railroads)
{
    uint_fast8_t     trackidx;

    for (trackidx = 0; trackidx < RCL::n_tracks; trackidx++)
    {
        RCL::set_new_railroad_ids_for_track (true, trackidx, rrgidx, map_new_rridx, n_railroads);
        RCL::set_new_railroad_ids_for_track (false, trackidx, rrgidx, map_new_rridx, n_railroads);
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 *  RCL::get_parameter_loco () get detected loco
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast16_t
RCL::get_parameter_loco (uint_fast16_t loco_idx, uint_fast16_t detected_loco_idx)
{
    if (loco_idx == 0xFFFF)
    {
        loco_idx = detected_loco_idx;
    }
    else if (loco_idx >= S88_RCL_DETECTED_OFFSET)
    {
        uint_fast8_t    track_idx = loco_idx - S88_RCL_DETECTED_OFFSET;
        uint_fast8_t    n_tracks = RCL::get_n_tracks ();

        if (track_idx < n_tracks)
        {
            loco_idx = RCL::tracks[track_idx].last_loco_idx;
        }
        else
        {
            loco_idx = 0xFFFF;
        }
    }

    return loco_idx;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  RCL::execute_track_actions () - execute track actions
 *------------------------------------------------------------------------------------------------------------------------
 */
void
RCL::execute_track_actions (bool in, uint_fast16_t detected_loco_idx, uint_fast8_t trackidx)
{
    uint_fast8_t    track_action_idx;

    if (trackidx < n_tracks)
    {
        uint_fast8_t    n_track_actions;

        if (in)
        {
            n_track_actions = tracks[trackidx].n_track_actions_in;
        }
        else
        {
            n_track_actions = tracks[trackidx].n_track_actions_out;
        }

        for (track_action_idx = 0; track_action_idx < n_track_actions; track_action_idx++)
        {
            uint_fast8_t    destination;
            uint_fast8_t    condition;
            uint_fast8_t    condition_destination;
            uint_fast8_t    action;
            uint16_t *      param;

            if (in)
            {
                condition               = tracks[trackidx].track_actions_in[track_action_idx].condition;
                condition_destination   = tracks[trackidx].track_actions_in[track_action_idx].condition_destination;
                action                  = tracks[trackidx].track_actions_in[track_action_idx].action;
                param                   = tracks[trackidx].track_actions_in[track_action_idx].parameters;
            }
            else
            {
                condition               = tracks[trackidx].track_actions_out[track_action_idx].condition;
                condition_destination   = tracks[trackidx].track_actions_out[track_action_idx].condition_destination;
                action                  = tracks[trackidx].track_actions_out[track_action_idx].action;
                param                   = tracks[trackidx].track_actions_out[track_action_idx].parameters;
            }

            destination = Locos::locos[detected_loco_idx].get_destination();

            if ((condition == RCL_CONDITION_ALWAYS) ||
                (condition == RCL_CONDITION_IF_DESTINATION && condition_destination == destination) ||
                (condition == RCL_CONDITION_IF_NOT_DESTINATION && condition_destination != destination))
            {
                switch (action)
                {
                    case RCL_ACTION_SET_LOCO_SPEED:                                             // parameters: START, LOCO, SPEED, TENTHS
                    {
                        uint_fast16_t   start       = param[0];
                        uint_fast16_t   loco_idx    = RCL::get_parameter_loco (param[1], detected_loco_idx);
                        uint_fast8_t    speed       = param[2];
                        uint_fast16_t   tenths      = param[3];

                        if (loco_idx != 0xFFFF)
                        {
                            if (start == 0)
                            {
                                Locos::locos[loco_idx].set_speed (speed, tenths);
                            }
                            else
                            {
                                Event::add_event_loco_speed (start, loco_idx, EVENT_SET_LOCO_SPEED, speed, tenths);
                            }
                        }
                        break;
                    }

                    case RCL_ACTION_SET_LOCO_MIN_SPEED:                                         // parameters: START, LOCO, SPEED, TENTHS
                    {
                        uint_fast16_t   start       = param[0];
                        uint_fast16_t   loco_idx    = RCL::get_parameter_loco (param[1], detected_loco_idx);
                        uint_fast8_t    speed       = param[2];
                        uint_fast16_t   tenths      = param[3];

                        if (loco_idx != 0xFFFF)
                        {
                            if (start == 0)
                            {
                                uint_fast8_t current_speed = Locos::locos[loco_idx].get_speed ();

                                if (current_speed < speed)
                                {
                                    Locos::locos[loco_idx].set_speed (speed, tenths);
                                }
                            }
                            else
                            {
                                Event::add_event_loco_speed (start, loco_idx, EVENT_SET_LOCO_MIN_SPEED, speed, tenths);
                            }
                        }
                        break;
                    }

                    case RCL_ACTION_SET_LOCO_MAX_SPEED:                                         // parameters: START, LOCO, SPEED, TENTHS
                    {
                        uint_fast16_t   start       = param[0];
                        uint_fast16_t   loco_idx    = RCL::get_parameter_loco (param[1], detected_loco_idx);
                        uint_fast8_t    speed       = param[2];
                        uint_fast16_t   tenths      = param[3];

                        if (loco_idx != 0xFFFF)
                        {
                            if (start == 0)
                            {
                                uint_fast8_t current_speed = Locos::locos[loco_idx].get_speed ();

                                if (current_speed > speed)
                                {
                                    Locos::locos[loco_idx].set_speed (speed, tenths);
                                }
                            }
                            else
                            {
                                Event::add_event_loco_speed (start, loco_idx, EVENT_SET_LOCO_MAX_SPEED, speed, tenths);
                            }
                        }
                        break;
                    }

                    case RCL_ACTION_SET_LOCO_FORWARD_DIRECTION:                                 // parameters: START, LOCO, FWD
                    {
                        uint_fast16_t   start       = param[0];
                        uint_fast16_t   loco_idx    = RCL::get_parameter_loco (param[1], detected_loco_idx);
                        uint_fast8_t    fwd         = param[2];

                        if (loco_idx != 0xFFFF)
                        {
                            if (start == 0)
                            {
                                Locos::locos[loco_idx].set_fwd (fwd);
                            }
                            else
                            {
                                Event::add_event_loco_dir (start, loco_idx, fwd);
                            }
                        }
                        break;
                    }

                    case RCL_ACTION_SET_LOCO_ALL_FUNCTIONS_OFF:                                      // parameters: START, LOCO
                    {
                        uint_fast16_t   start       = param[0];
                        uint_fast16_t   loco_idx    = RCL::get_parameter_loco (param[1], detected_loco_idx);

                        if (loco_idx != 0xFFFF)
                        {
                            if (start == 0)
                            {
                                Locos::locos[loco_idx].reset_functions ();
                            }
                            else
                            {
                                Event::add_event_loco_function (start, loco_idx, 0xFF, false);
                            }
                        }
                        break;
                    }

                    case RCL_ACTION_SET_LOCO_FUNCTION_OFF:                                           // parameters: START, LOCO, FUNC_IDX
                    {
                        uint_fast16_t   start       = param[0];
                        uint_fast16_t   loco_idx    = RCL::get_parameter_loco (param[1], detected_loco_idx);
                        uint_fast8_t    f           = param[2];

                        if (loco_idx != 0xFFFF)
                        {
                            if (start == 0)
                            {
                                Locos::locos[loco_idx].set_function (f, false);
                            }
                            else
                            {
                                Event::add_event_loco_function (start, loco_idx, f, false);
                            }
                        }
                        break;
                    }

                    case RCL_ACTION_SET_LOCO_FUNCTION_ON:                                            // parameters: START, LOCO, FUNC_IDX
                    {
                        uint_fast16_t   start       = param[0];
                        uint_fast16_t   loco_idx    = RCL::get_parameter_loco (param[1], detected_loco_idx);
                        uint_fast8_t    f           = param[2];

                        if (loco_idx != 0xFFFF)
                        {
                            if (start == 0)
                            {
                                Locos::locos[loco_idx].set_function (f, true);
                            }
                            else
                            {
                                Event::add_event_loco_function (start, loco_idx, f, true);
                            }
                        }
                        break;
                    }

                    case RCL_ACTION_EXECUTE_LOCO_MACRO:                                                 // parameters: START, LOCO, MACRO_IDX
                    {
                        uint_fast16_t   start       = param[0];
                        uint_fast16_t   loco_idx    = RCL::get_parameter_loco (param[1], detected_loco_idx);
                        uint_fast8_t    macro_idx   = param[2];

                        if (loco_idx != 0xFFFF)
                        {
                            if (start == 0)
                            {
                                Locos::locos[loco_idx].execute_macro (macro_idx);
                            }
                            else
                            {
                                Event::add_event_execute_loco_macro (start, loco_idx, macro_idx);
                            }
                        }
                        break;
                    }

                    case RCL_ACTION_SET_ADDON_ALL_FUNCTIONS_OFF:                                      // parameters: START, ADDON
                    {
                        uint_fast16_t   start       = param[0];
                        uint_fast16_t   addon_idx   = param[1];

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

                    case RCL_ACTION_SET_ADDON_FUNCTION_OFF:                                           // parameters: START, ADDON, FUNC_IDX
                    {
                        uint_fast16_t   start       = param[0];
                        uint_fast16_t   addon_idx   = param[1];
                        uint_fast8_t    f           = param[2];

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

                    case RCL_ACTION_SET_ADDON_FUNCTION_ON:                                            // parameters: START, ADDON, FUNC_IDX
                    {
                        uint_fast16_t   start       = param[0];
                        uint_fast16_t   addon_idx   = param[1];
                        uint_fast8_t    f           = param[2];

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

                    case RCL_ACTION_SET_RAILROAD:                                               // parameters: RRG_IDX, RR_IDX
                    {
                        uint_fast8_t    rrgidx  = param[0];
                        uint_fast8_t    rridx   = param[1];
                        RailroadGroup * rrg     = &RailroadGroups::railroad_groups[rrgidx];

                        rrg->set_active_railroad (rridx, detected_loco_idx);
                        break;
                    }

                    case RCL_ACTION_SET_FREE_RAILROAD:                                          // parameters: RRG_IDX
                    {
                        uint_fast8_t    rrgidx  = param[0];
                        RailroadGroup * rrg     = &RailroadGroups::railroad_groups[rrgidx];
                        uint_fast8_t    rridx;

                        rridx = S88::get_free_railroad (rrgidx);

                        if (rridx != 0xFF)
                        {
                            rrg->set_active_railroad (rridx, detected_loco_idx);
                        }
                        else
                        {
                            Debug::printf (DEBUG_LEVEL_VERBOSE, "Error: RCL::execute_track_actions: cannot find free railroad\n");

                            if (detected_loco_idx != 0xFFFF)
                            {
                                Locos::locos[detected_loco_idx].set_speed (0);
                            }
                        }
                        break;
                    }

                    case RCL_ACTION_SET_LINKED_RAILROAD:                                        // parameters: RRG_IDX
                    {
                        uint_fast8_t    rrgidx  = param[0];
                        RailroadGroup * rrg     = &RailroadGroups::railroad_groups[rrgidx];
                        uint_fast8_t    rridx;

                        rridx = rrg->get_link_railroad (detected_loco_idx);

                        if (rridx != 0xFF)
                        {
                            rrg->set_active_railroad (rridx, detected_loco_idx);
                        }
                        else
                        {
                            Debug::printf (DEBUG_LEVEL_VERBOSE, "Warning: RCL::execute_track_actions: cannot find linked railroad, searching for free railroad\n");

                            rridx = S88::get_free_railroad (rrgidx);

                            if (rridx != 0xFF)
                            {
                                Debug::printf (DEBUG_LEVEL_VERBOSE, "Info: free railroad found: rridx=%u\n", rridx);
                                rrg->set_active_railroad (rridx, detected_loco_idx);
                            }
                            else
                            {
                                Debug::printf (DEBUG_LEVEL_VERBOSE, "Error: RCL::execute_track_actions: cannot find free railroad, stopping loco\n");

                                if (detected_loco_idx != 0xFFFF)
                                {
                                    Locos::locos[detected_loco_idx].set_speed (0);
                                }
                            }
                        }
                        break;
                    }

                    case RCL_ACTION_WAIT_FOR_FREE_S88_CONTACT:                                  // parameters: START, LOCO, COIDX, TENTHS
                    {
                        uint_fast16_t   start       = param[0];
                        uint_fast16_t   loco_idx    = RCL::get_parameter_loco (param[1], detected_loco_idx);
                        uint_fast16_t   coidx       = param[2];
                        uint_fast16_t   tenths      = param[3];

                        if (S88::get_state_bit (coidx) == S88_STATE_OCCUPIED)
                        {
                            if (loco_idx != 0xFFFF)
                            {
                                uint_fast8_t    current_speed = Locos::locos[loco_idx].get_speed ();

                                if (start == 0)
                                {
                                    Locos::locos[loco_idx].set_speed (0, tenths);
                                }
                                else
                                {
                                    Event::add_event_loco_speed (start, loco_idx, EVENT_SET_LOCO_SPEED, 0, tenths);
                                }

                                Event::add_event_wait_s88 (start, coidx, loco_idx, current_speed, tenths);
                            }
                        }
                        break;
                    }

                    case RCL_ACTION_SET_LOCO_DESTINATION:                                       // parameters: LOCO_IDX, RRG_IDX
                    {
                        uint_fast16_t   loco_idx    = RCL::get_parameter_loco (param[0], detected_loco_idx);
                        uint_fast8_t    rrgidx      = param[1];

                        if (loco_idx != 0xFFFF)
                        {
                            Locos::locos[loco_idx].set_destination (rrgidx);
                        }
                        break;
                    }

                    case RCL_ACTION_SET_LED:                                                    // parameters: START, LG_IDX, LED_MASK, LED_ON
                    {
                        uint_fast16_t   start           = param[0];
                        uint_fast16_t   led_group_idx   = param[1];
                        uint_fast8_t    led_mask        = param[2];
                        uint_fast8_t    led_on          = param[3];

                        if (led_group_idx != 0xFFFF)
                        {
                            if (start == 0)
                            {
                                Leds::led_groups[led_group_idx].set_state (led_mask, led_on);
                            }
                            else
                            {
                                Event::add_event_led_set_state (start, led_group_idx, led_mask, led_on);
                            }
                        }
                        break;
                    }

                    case RCL_ACTION_SET_SWITCH:                                                 // parameters: START, SW_IDX, SW_STATE
                    {
                        uint_fast16_t   start       = param[0];
                        uint_fast16_t   swidx       = param[1];
                        uint_fast8_t    swstate     = param[2];

                        if (swidx != 0xFFFF)
                        {
                            if (start == 0)
                            {
                                Switches::switches[swidx].set_state (swstate);
                            }
                            else
                            {
                                Event::add_event_switch_set_state (start, swidx, swstate);
                            }
                        }
                        break;
                    }

                    case RCL_ACTION_SET_SIGNAL:                                                 // parameters: START, SIG_IDX, SIG_STATE
                    {
                        uint_fast16_t   start       = param[0];
                        uint_fast16_t   sigidx      = param[1];
                        uint_fast8_t    sigstate    = param[2];

                        if (sigidx != 0xFFFF)
                        {
                            if (start == 0)
                            {
                                Signals::signals[sigidx].set_state (sigstate);
                            }
                            else
                            {
                                Event::add_event_signal_set_state (start, sigidx, sigstate);
                            }
                        }
                        break;
                    }
                }
            }
        }
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 *  set_new_id () - set new id
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
RCL::set_new_id (uint_fast8_t track_idx, uint_fast8_t new_track_idx)
{
    uint_fast8_t   rtc = 0xFF;

    if (track_idx < RCL::n_tracks && new_track_idx < RCL::n_tracks)
    {
        if (track_idx != new_track_idx)
        {
            RCL_Track       tmptrack;
            uint_fast8_t    tridx;

            uint8_t *      map_new_track_idx = (uint8_t *) calloc (RCL::n_tracks, sizeof (uint8_t));

            for (tridx = 0; tridx < RCL::n_tracks; tridx++)
            {
                map_new_track_idx[tridx] = tridx;
            }

            tmptrack = RCL::tracks[track_idx];

            if (new_track_idx < track_idx)
            {
                for (tridx = track_idx; tridx > new_track_idx; tridx--)
                {
                    RCL::tracks[tridx] = RCL::tracks[tridx - 1];
                    map_new_track_idx[tridx - 1] = tridx;
                }
            }
            else // if (new_track_idx > track_idx)
            {
                for (tridx = track_idx; tridx < new_track_idx; tridx++)
                {
                    RCL::tracks[tridx], RCL::tracks[tridx + 1];
                    map_new_track_idx[tridx + 1] = tridx;
                }
            }

            RCL::tracks[tridx] = tmptrack;
            map_new_track_idx[track_idx] = tridx;

            for (tridx = 0; tridx < RCL::n_tracks; tridx++)
            {
                Debug::printf (DEBUG_LEVEL_VERBOSE, "%2d -> %2d\n", tridx, map_new_track_idx[tridx]);
            }

            free (map_new_track_idx);
            RCL::data_changed = true;
            rtc = new_track_idx;
        }
        else
        {
            rtc = track_idx;
        }
    }

    return rtc;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  RCL::add_track () - add a track
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
RCL::add (const RCL_Track& track)
{
    if (RCL::n_tracks < RCL_MAX_RCL_TRACKS)
    {
        tracks.push_back(track);
        RCL::n_tracks++;
        return tracks.size() - 1;
    }
    return 0xFF;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  RCL::get_n_tracks () - get number of tracks
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
RCL::get_n_tracks (void)
{
    return n_tracks;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  RCL::init ()
 *------------------------------------------------------------------------------------------------------------------------
 */
void
RCL::init (void)
{
    reset_all_locations ();
}
