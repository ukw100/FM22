/*------------------------------------------------------------------------------------------------------------------------
 * rcl.cc - RailCom local management functions
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
#include "http.h"
#include "event.h"
#include "dcc.h"
#include "switch.h"
#include "loco.h"
#include "addon.h"
#include "railroad.h"
#include "s88.h"
#include "debug.h"
#include "rcl.h"

uint8_t                             RCL::locations[MAX_LOCOS];
RCL_TRACK                           RCL::tracks[RCL_MAX_RCL_TRACKS];
uint_fast8_t                        RCL::n_tracks = 0;                        // number of tracks
bool                                RCL::data_changed = false;

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
                            Loco::set_speed (loco_idx, speed, tenths);
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
                            Loco::set_fwd (loco_idx, fwd);
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
                            Loco::reset_functions (loco_idx);
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
                            Loco::set_function (loco_idx, f, false);
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
                            Loco::set_function (loco_idx, f, true);
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
                            Loco::execute_macro (loco_idx, macro_idx);
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
                            AddOn::reset_functions (addon_idx);
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
                            AddOn::set_function (addon_idx, f, false);
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
                            AddOn::set_function (addon_idx, f, true);
                        }
                        else
                        {
                            Event::add_event_addon_function (start, addon_idx, f, true);
                        }
                    }
                    break;
                }

                case RCL_ACTION_SET_LINKED_RAILROAD:                                        // parameters: RRG_IDX
                {
                    uint_fast8_t    rrgidx  = param[0];
                    uint_fast8_t    rridx;

                    rridx = Railroad::get_link_railroad (rrgidx, detected_loco_idx);

                    if (rridx != 0xFF)
                    {
                        Railroad::set (rrgidx, rridx, detected_loco_idx);
                    }
                    else
                    {
                        Debug::printf (DEBUG_LEVEL_VERBOSE, "Warning: RCL::execute_track_actions: cannot find linked railroad, searching for free railroad\n");

                        rridx = S88::get_free_railroad (rrgidx);

                        if (rridx != 0xFF)
                        {
                            Debug::printf (DEBUG_LEVEL_VERBOSE, "Info: free railroad found: rridx=%u\n", rridx);
                            Railroad::set (rrgidx, rridx, detected_loco_idx);
                        }
                        else
                        {
                            Debug::printf (DEBUG_LEVEL_VERBOSE, "Error: RCL::execute_track_actions: cannot find free railroad, stopping loco\n");

                            if (detected_loco_idx != 0xFFFF)
                            {
                                Loco::set_speed (detected_loco_idx, 0);
                            }
                        }
                    }
                    break;
                }

                case RCL_ACTION_SET_FREE_RAILROAD:                                          // parameters: RRG_IDX
                {
                    uint_fast8_t    rrgidx  = param[0];
                    uint_fast8_t    rridx;

                    rridx = S88::get_free_railroad (rrgidx);

                    if (rridx != 0xFF)
                    {
                        Railroad::set (rrgidx, rridx, detected_loco_idx);
                    }
                    else
                    {
                        Debug::printf (DEBUG_LEVEL_VERBOSE, "Error: RCL::execute_track_actions: cannot find free railroad\n");

                        if (detected_loco_idx != 0xFFFF)
                        {
                            Loco::set_speed (detected_loco_idx, 0);
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
                            uint_fast8_t    current_speed = Loco::get_speed (loco_idx);

                            if (start == 0)
                            {
                                Loco::set_speed (loco_idx, 0, tenths);
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
            }
        }
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 *  setid () - set new id
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
RCL::setid (uint_fast8_t track_idx, uint_fast8_t new_track_idx)
{
    uint_fast8_t   rtc = 0xFF;

    if (track_idx < RCL::n_tracks && new_track_idx < RCL::n_tracks)
    {
        if (track_idx != new_track_idx)
        {
            CONTACT         tmptrack;
            uint_fast8_t    tridx;

            uint8_t *      map_new_track_idx = (uint8_t *) calloc (RCL::n_tracks, sizeof (uint8_t));

            for (tridx = 0; tridx < RCL::n_tracks; tridx++)
            {
                map_new_track_idx[tridx] = tridx;
            }

            memcpy (&tmptrack, RCL::tracks + track_idx, sizeof (CONTACT));

            if (new_track_idx < track_idx)
            {
                for (tridx = track_idx; tridx > new_track_idx; tridx--)
                {
                    memcpy (RCL::tracks + tridx, RCL::tracks + tridx - 1, sizeof (CONTACT));
                    map_new_track_idx[tridx - 1] = tridx;
                }
            }
            else // if (new_track_idx > track_idx)
            {
                for (tridx = track_idx; tridx < new_track_idx; tridx++)
                {
                    memcpy (RCL::tracks + tridx, RCL::tracks + tridx + 1, sizeof (CONTACT));
                    map_new_track_idx[tridx + 1] = tridx;
                }
            }

            memcpy (RCL::tracks + tridx, &tmptrack, sizeof (CONTACT));
            map_new_track_idx[track_idx] = tridx;

            for (tridx = 0; tridx < RCL::n_tracks; tridx++)
            {
                Debug::printf (DEBUG_LEVEL_NORMAL, "%2d -> %2d\n", tridx, map_new_track_idx[tridx]);
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
RCL::add_track (void)
{
    uint_fast8_t    trackidx;

    if (n_tracks < RCL_MAX_RCL_TRACKS - 1)
    {
        trackidx = n_tracks;
        tracks[trackidx].loco_idx               = 0xFFFF;
        tracks[trackidx].last_loco_idx          = 0xFFFF;
        tracks[trackidx].n_track_actions_in     = 0;
        tracks[trackidx].n_track_actions_out    = 0;
        tracks[trackidx].flags                  = RCL_TRACK_FLAG_NONE;
        n_tracks++;
        RCL::data_changed = true;
    }
    else
    {
        trackidx = 0xFF;
    }

    return trackidx;
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
 *  RCL::set_name (trackidx, name) - set name of track
 *------------------------------------------------------------------------------------------------------------------------
 */
void
RCL::set_name (uint_fast8_t trackidx, const char * name)
{
    int l = strlen (name);

    if (tracks[trackidx].name)
    {
        if ((int) strlen (tracks[trackidx].name) < l)
        {
            free (tracks[trackidx].name);
            tracks[trackidx].name = (char *) NULL;
        }
    }

    if (! tracks[trackidx].name)
    {
        tracks[trackidx].name = (char *) malloc (l + 1);
    }

    strcpy (tracks[trackidx].name, name);
    RCL::data_changed = true;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  RCL::get_name (trackidx) - get name of track
 *------------------------------------------------------------------------------------------------------------------------
 */
char  *
RCL::get_name (uint_fast8_t trackidx)
{
    if (trackidx < n_tracks)
    {
        return tracks[trackidx].name;
    }

    return (char *) NULL;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  RCL::set_flags (trackidx, flags) - set flags of track
 *------------------------------------------------------------------------------------------------------------------------
 */
void
RCL::set_flags (uint_fast8_t trackidx, uint32_t flags)
{
    tracks[trackidx].flags |= flags;
    RCL::data_changed = true;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  RCL::reset_flags (trackidx, flags) - set flags of track
 *------------------------------------------------------------------------------------------------------------------------
 */
void
RCL::reset_flags (uint_fast8_t trackidx, uint32_t flags)
{
    tracks[trackidx].flags &= ~flags;
    RCL::data_changed = true;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  RCL::get_flags (flags) - get flags of track
 *------------------------------------------------------------------------------------------------------------------------
 */
uint32_t
RCL::get_flags (uint_fast8_t trackidx)
{
    return tracks[trackidx].flags;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  RCL::add_track_action () - add a track action
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
RCL::add_track_action (bool in, uint_fast8_t trackidx)
{
    uint_fast8_t track_action_idx;

    if (in)
    {
        if (tracks[trackidx].n_track_actions_in < RCL_MAX_ACTIONS_PER_RCL_TRACK - 1)
        {
            track_action_idx = tracks[trackidx].n_track_actions_in;
            tracks[trackidx].track_actions_in[track_action_idx].action = RCL_ACTION_NONE;
            tracks[trackidx].track_actions_in[track_action_idx].n_parameters = 0;
            tracks[trackidx].n_track_actions_in++;
            RCL::data_changed = true;
        }
        else
        {
            track_action_idx = 0xFF;
        }
    }   
    else
    {
        if (tracks[trackidx].n_track_actions_out < RCL_MAX_ACTIONS_PER_RCL_TRACK - 1)
        {
            track_action_idx = tracks[trackidx].n_track_actions_out;
            tracks[trackidx].track_actions_out[track_action_idx].action = RCL_ACTION_NONE;
            tracks[trackidx].track_actions_out[track_action_idx].n_parameters = 0;
            tracks[trackidx].n_track_actions_out++;
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
 *  RCL::delete_track_action () - delete a track action
 *------------------------------------------------------------------------------------------------------------------------
 */
void
RCL::delete_track_action (bool in, uint_fast8_t trackidx, uint_fast8_t track_action_idx)
{
    uint_fast8_t    n_parameters;
    uint_fast8_t    pidx;

    if (in)
    {
        if (track_action_idx < tracks[trackidx].n_track_actions_in)
        {
            while (track_action_idx < tracks[trackidx].n_track_actions_in - 1)
            {
                n_parameters = tracks[trackidx].track_actions_in[track_action_idx + 1].n_parameters;

                tracks[trackidx].track_actions_in[track_action_idx].action      = tracks[trackidx].track_actions_in[track_action_idx + 1].action;
                tracks[trackidx].track_actions_in[track_action_idx].n_parameters   = n_parameters;

                for (pidx = 0; pidx < n_parameters; pidx++)
                {
                    tracks[trackidx].track_actions_in[track_action_idx].parameters[pidx]   = tracks[trackidx].track_actions_in[track_action_idx + 1].parameters[pidx];
                }

                track_action_idx++;
            }

            tracks[trackidx].n_track_actions_in--;
            RCL::data_changed = true;
        }
    }
    else
    {
        if (track_action_idx < tracks[trackidx].n_track_actions_out)
        {
            while (track_action_idx < tracks[trackidx].n_track_actions_out - 1)
            {
                n_parameters = tracks[trackidx].track_actions_out[track_action_idx + 1].n_parameters;

                tracks[trackidx].track_actions_out[track_action_idx].action     = tracks[trackidx].track_actions_out[track_action_idx + 1].action;
                tracks[trackidx].track_actions_out[track_action_idx].n_parameters   = n_parameters;

                for (pidx = 0; pidx < n_parameters; pidx++)
                {
                    tracks[trackidx].track_actions_out[track_action_idx].parameters[pidx]   = tracks[trackidx].track_actions_out[track_action_idx + 1].parameters[pidx];
                }

                track_action_idx++;
            }

            tracks[trackidx].n_track_actions_out--;
            RCL::data_changed = true;
        }
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 *  RCL::get_n_track_actions () - get number of track actions
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
RCL::get_n_track_actions (bool in, uint_fast8_t trackidx)
{
    if (in)
    {
        return tracks[trackidx].n_track_actions_in;
    }
    return tracks[trackidx].n_track_actions_out;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  RCL::set_track_action () - set track action
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
RCL::set_track_action (bool in, uint_fast8_t trackidx, uint_fast8_t track_action_idx, RCL_TRACK_ACTION * trap)
{
    uint_fast8_t rtc = 0;

    if (in)
    {
        if (track_action_idx < tracks[trackidx].n_track_actions_in)
        {
            uint_fast8_t paidx;

            tracks[trackidx].track_actions_in[track_action_idx].action = trap->action;
            tracks[trackidx].track_actions_in[track_action_idx].n_parameters = trap->n_parameters;

            for (paidx = 0; paidx < trap->n_parameters; paidx++)
            {
                tracks[trackidx].track_actions_in[track_action_idx].parameters[paidx] = trap->parameters[paidx];
            }

            RCL::data_changed = true;
            rtc = 1;
        }
    }
    else
    {
        if (track_action_idx < tracks[trackidx].n_track_actions_out)
        {
            uint_fast8_t paidx;

            tracks[trackidx].track_actions_out[track_action_idx].action = trap->action;
            tracks[trackidx].track_actions_out[track_action_idx].n_parameters = trap->n_parameters;

            for (paidx = 0; paidx < trap->n_parameters; paidx++)
            {
                tracks[trackidx].track_actions_out[track_action_idx].parameters[paidx] = trap->parameters[paidx];
            }

            RCL::data_changed = true;
            rtc = 1;
        }
    }

    return rtc;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  RCL::get_track_action () - get track action
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
RCL::get_track_action (bool in, uint_fast8_t trackidx, uint_fast8_t track_action_idx, RCL_TRACK_ACTION * trap)
{
    uint_fast8_t rtc = 0;

    if (in)
    {
        if (track_action_idx < tracks[trackidx].n_track_actions_in)
        {
            uint_fast8_t paidx;

            trap->action         = tracks[trackidx].track_actions_in[track_action_idx].action;
            trap->n_parameters   = tracks[trackidx].track_actions_in[track_action_idx].n_parameters;

            for (paidx = 0; paidx < trap->n_parameters; paidx++)
            {
                trap->parameters[paidx] = tracks[trackidx].track_actions_in[track_action_idx].parameters[paidx];
            }
            rtc = 1;
        }
    }
    else
    {
        if (track_action_idx < tracks[trackidx].n_track_actions_out)
        {
            uint_fast8_t paidx;

            trap->action         = tracks[trackidx].track_actions_out[track_action_idx].action;
            trap->n_parameters   = tracks[trackidx].track_actions_out[track_action_idx].n_parameters;

            for (paidx = 0; paidx < trap->n_parameters; paidx++)
            {
                trap->parameters[paidx] = tracks[trackidx].track_actions_out[track_action_idx].parameters[paidx];
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
    uint_fast16_t   n_locos = Loco::get_n_locos ();
    uint_fast16_t   loco_idx;

    if (DCC::booster_is_on)
    {
        for (loco_idx = 0; loco_idx < n_locos; loco_idx++)
        {
            uint_fast8_t    location        = Loco::get_rcllocation (loco_idx);
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

                            Loco::set_speed (loco_idx, 0);
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
 *  RCL::init ()
 *------------------------------------------------------------------------------------------------------------------------
 */
void
RCL::init (void)
{
    reset_all_locations ();
}
