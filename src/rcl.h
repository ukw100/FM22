/*------------------------------------------------------------------------------------------------------------------------
 * rcl.h - RailCom rc local management functions
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
#ifndef RCL_H
#define RCL_H

#include <stdint.h>
#include <string>
#include <vector>
#include <memory>

#define RCL_MAX_ACTION_PARAMETERS                   8
#define RCL_STATE_FREE                              0
#define RCL_STATE_OCCUPIED                          1

#define RCL_MAX_RCL_TRACKS                          128

#define RCL_MAX_NAME_SIZE                           32
#define RCL_MAX_ACTIONS_PER_RCL_TRACK               8

#define RCL_TRACK_FLAG_NONE                         0x00
#define RCL_TRACK_FLAG_BLOCK_PROTECTION             0x01

#define RCL_CONDITION_NEVER                         0
#define RCL_CONDITION_ALWAYS                        1
#define RCL_CONDITION_IF_DESTINATION                2
#define RCL_CONDITION_IF_NOT_DESTINATION            3

#define RCL_ACTION_NONE                             0
#define RCL_ACTION_SET_LOCO_SPEED                   1                           // parameters: START, LOCO_IDX, SPEED, TENTHS
#define RCL_ACTION_SET_LOCO_MIN_SPEED               2                           // parameters: START, LOCO_IDX, SPEED, TENTHS
#define RCL_ACTION_SET_LOCO_MAX_SPEED               3                           // parameters: START, LOCO_IDX, SPEED, TENTHS
#define RCL_ACTION_SET_LOCO_FORWARD_DIRECTION       4                           // parameters: START, LOCO_IDX, FWD
#define RCL_ACTION_SET_LOCO_ALL_FUNCTIONS_OFF       5                           // parameters: START, LOCO_IDX
#define RCL_ACTION_SET_LOCO_FUNCTION_OFF            6                           // parameters: START, LOCO_IDX, FUNC_IDX
#define RCL_ACTION_SET_LOCO_FUNCTION_ON             7                           // parameters: START, LOCO_IDX, FUNC_IDX
#define RCL_ACTION_EXECUTE_LOCO_MACRO               8                           // parameters: START, LOCO_IDX, MACRO_IDX
#define RCL_ACTION_SET_ADDON_ALL_FUNCTIONS_OFF      9                           // parameters: START, ADDON_IDX
#define RCL_ACTION_SET_ADDON_FUNCTION_OFF           10                          // parameters: START, ADDON_IDX, FUNC_IDX
#define RCL_ACTION_SET_ADDON_FUNCTION_ON            11                          // parameters: START, ADDON_IDX, FUNC_IDX
#define RCL_ACTION_SET_RAILROAD                     12                          // parameters: RRG_IDX, RR_IDX
#define RCL_ACTION_SET_FREE_RAILROAD                13                          // parameters: RRG_IDX
#define RCL_ACTION_SET_LINKED_RAILROAD              14                          // parameters: RRG_IDX
#define RCL_ACTION_WAIT_FOR_FREE_S88_CONTACT        15                          // parameters: START, LOCO_IDX, CO_IDX, TENTHS
#define RCL_ACTION_SET_LOCO_DESTINATION             16                          // parameters: LOCO_IDX, RRG_IDX
#define RCL_ACTION_SET_LED                          17                          // parameters: START, LG_IDX, LED_MASK, LED_ON
#define RCL_ACTION_SET_SWITCH                       18                          // parameters: START, SW_IDX, SW_STATE
#define RCL_ACTION_SET_SIGNAL                       19                          // parameters: START, SIG_IDX, SIG_STATE

#define RCL_ACTIONS                                 20                          // number of possible actions

typedef struct
{
    uint8_t                             condition;
    uint8_t                             condition_destination;
    uint8_t                             action;
    uint8_t                             n_parameters;
    uint16_t                            parameters[RCL_MAX_ACTION_PARAMETERS];
} RCL_TRACK_ACTION;

class RCL_Track
{
    public:
        uint16_t                        loco_idx;
        uint16_t                        last_loco_idx;
        uint32_t                        flags;
        uint8_t                         n_track_actions_in;
        uint8_t                         n_track_actions_out;
        RCL_TRACK_ACTION                track_actions_in[RCL_MAX_ACTIONS_PER_RCL_TRACK];
        RCL_TRACK_ACTION                track_actions_out[RCL_MAX_ACTIONS_PER_RCL_TRACK];

                                        RCL_Track();
        void                            set_name (std::string name);
        std::string                     get_name ();
        void                            set_flags (uint32_t flags);
        void                            reset_flags (uint32_t flags);
        uint32_t                        get_flags ();
        uint_fast8_t                    add_track_action (bool in);
        void                            delete_track_action (bool in, uint_fast8_t track_action_idx);
        uint_fast8_t                    get_n_track_actions (bool in);
        uint_fast8_t                    set_track_action (bool in, uint_fast8_t track_action_idx, RCL_TRACK_ACTION * trap);
        uint_fast8_t                    get_track_action (bool in, uint_fast8_t track_action_idx, RCL_TRACK_ACTION * trap);

    private:
        std::string                     name;
};

class RCL
{
    public:
        static std::vector<RCL_Track>   tracks;
        static bool                     data_changed;

        static uint_fast8_t             add (const RCL_Track& track);
        static uint_fast8_t             get_n_tracks (void);
        static uint_fast8_t             set_new_id (uint_fast8_t rcl_track_idx, uint_fast8_t new_rcl_track_idx);
        static void                     set_new_loco_ids (uint16_t * map_new_loco_idx, uint16_t n_locos);
        static void                     set_new_addon_ids (uint16_t * map_new_addon_idx, uint16_t n_addons);
        static void                     set_new_switch_ids (uint16_t * map_new_switch_idx, uint16_t n_switches);
        static void                     set_new_railroad_group_ids (uint8_t * map_new_rrgidx, uint_fast8_t n_railroad_groups);
        static void                     set_new_railroad_ids (uint_fast8_t rrgidx, uint8_t * map_new_rridx, uint_fast8_t n_railroads);
        static uint_fast8_t             booster_on (void);
        static uint_fast8_t             booster_off (void);
        static void                     schedule (void);
        static void                     init (void);

    private:
        static uint_fast8_t             n_tracks;                                   // number of tracks
        static uint8_t                  locations[MAX_LOCOS];
        static uint_fast16_t            get_parameter_loco (uint_fast16_t loco_idx, uint_fast16_t detected_loco_idx);
        static void                     execute_track_actions (bool in, uint_fast16_t detected_loco_idx, uint_fast8_t trackidx);
        static void                     reset_all_locations (void);
        static void                     set_new_loco_ids_for_track (bool in, uint_fast8_t trackidx, uint16_t * map_new_loco_idx, uint16_t n_locos);
        static void                     set_new_addon_ids_for_track (bool in, uint_fast8_t trackidx, uint16_t * map_new_addon_idx, uint16_t n_addons);
        static void                     set_new_switch_ids_for_track (bool in, uint_fast8_t trackidx, uint16_t * map_new_switch_idx, uint16_t n_switches);
        static void                     set_new_railroad_group_ids_for_track (bool in, uint_fast8_t trackidx, uint8_t * map_new_rrgidx, uint_fast8_t n_railroad_groups);
        static void                     set_new_railroad_ids_for_track (bool in, uint_fast8_t trackidx, uint_fast8_t current_rrgidx, uint8_t * map_new_rridx, uint_fast8_t n_railroads);
};

#endif
