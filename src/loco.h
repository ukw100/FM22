/*------------------------------------------------------------------------------------------------------------------------
 * loco.h - loco management functions
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
#ifndef LOCO_H
#define LOCO_H

#include <stdint.h>
#include <string>
#include <vector>
#include <memory>

#define MAX_LOCOS               1024

#define LOCO_FLAG_FREE          0x01
#define LOCO_FLAG_ONLINE        0x02
#define LOCO_FLAG_HALT          0x04

#define MAX_LOCO_FUNCTIONS      32
#define LOCO_SPEED_STEPS_14     14
#define LOCO_SPEED_STEPS_18     28
#define LOCO_SPEED_STEPS_128    128

#define LOCO_F0                 0
#define LOCO_F1                 1
#define LOCO_F2                 2
#define LOCO_F3                 3
#define LOCO_F4                 4
#define LOCO_F5                 5
#define LOCO_F6                 6
#define LOCO_F7                 7
#define LOCO_F8                 8
#define LOCO_F9                 9
#define LOCO_F10                10
#define LOCO_F11                11
#define LOCO_F12                12
#define LOCO_F13                13
#define LOCO_F14                14
#define LOCO_F15                15
#define LOCO_F16                16
#define LOCO_F17                17
#define LOCO_F18                18
#define LOCO_F19                19
#define LOCO_F20                20
#define LOCO_F21                21
#define LOCO_F22                22
#define LOCO_F23                23
#define LOCO_F24                24
#define LOCO_F25                25
#define LOCO_F26                26
#define LOCO_F27                27
#define LOCO_F28                28
#define LOCO_F29                29
#define LOCO_F30                30
#define LOCO_F31                31

typedef struct
{
    uint32_t            pulse_mask;
    uint32_t            sound_mask;
    uint16_t            name_idx[MAX_LOCO_FUNCTIONS];
    uint8_t             max;                                // highest used function index
} LOCOFUNCTION;

#define LOCO_MAX_ACTION_PARAMETERS                  8

#define LOCO_ACTION_NONE                            0
#define LOCO_ACTION_SET_LOCO_SPEED                  1                           // parameters: START, SPEED, TENTHS
#define LOCO_ACTION_SET_LOCO_MIN_SPEED              2                           // parameters: START, SPEED, TENTHS
#define LOCO_ACTION_SET_LOCO_MAX_SPEED              3                           // parameters: START, SPEED, TENTHS
#define LOCO_ACTION_SET_LOCO_FORWARD_DIRECTION      4                           // parameters: START, FWD
#define LOCO_ACTION_SET_LOCO_ALL_FUNCTIONS_OFF      5                           // parameters: START
#define LOCO_ACTION_SET_LOCO_FUNCTION_OFF           6                           // parameters: START, FUNC_IDX
#define LOCO_ACTION_SET_LOCO_FUNCTION_ON            7                           // parameters: START, FUNC_IDX
#define LOCO_ACTION_SET_ADDON_ALL_FUNCTIONS_OFF     8                           // parameters: START
#define LOCO_ACTION_SET_ADDON_FUNCTION_OFF          9                           // parameters: START, FUNC_IDX
#define LOCO_ACTION_SET_ADDON_FUNCTION_ON           10                          // parameters: START, FUNC_IDX
#define LOCO_ACTIONS                                11                          // number of possible actions

#define LOCO_MAX_ACTIONS_PER_MACRO                  16
#define MAX_LOCO_MACROS_PER_LOCO                    8

typedef struct
{
    uint8_t                             action;
    uint8_t                             n_parameters;
    uint16_t                            parameters[LOCO_MAX_ACTION_PARAMETERS];
} LOCOACTION;

typedef struct
{
    LOCOACTION                          actions[LOCO_MAX_ACTIONS_PER_MACRO];
    uint8_t                             n_actions;
} LOCOMACRO;

class Loco
{
    public:
                                        Loco ();
        void                            set_id (uint_fast16_t id);

        void                            set_addon (uint_fast16_t addon_idx);
        uint_fast16_t                   get_addon (void);
        void                            reset_addon (void);

        void                            set_coupled_function (uint_fast8_t lfidx, uint_fast8_t afidx);
        uint_fast8_t                    get_coupled_function (uint_fast8_t lfidx);

        void                            set_name (std::string name);
        std::string                     get_name (void);

        void                            set_addr (uint_fast16_t addr);
        uint_fast16_t                   get_addr (void);

        void                            set_speed_steps (uint_fast8_t speed_steps);
        uint_fast8_t                    get_speed_steps (void);

        void                            set_flags (uint32_t flags);
        uint32_t                        get_flags (void);
        void                            set_flag_halt (void);
        void                            reset_flag_halt (void);
        bool                            get_flag_halt (void);

        uint_fast8_t                    add_macro_action (uint_fast8_t macroidx);
        void                            delete_macro_action (uint_fast8_t macroidx, uint_fast8_t actionidx);
        uint_fast8_t                    get_n_macro_actions (uint_fast8_t macroidx);
        bool                            set_macro_action (uint_fast8_t macroidx, uint_fast8_t actionidx, LOCOACTION * lap);
        bool                            get_macro_action (uint_fast8_t macroidx, uint_fast8_t actionidx, LOCOACTION * lap);
        void                            execute_macro (uint_fast8_t macroidx);

        uint_fast8_t                    set_function_type (uint_fast8_t fidx, uint_fast16_t function_name_idx, bool pulse, bool sound);

        std::string&                    get_function_name (uint_fast8_t fidx);
        uint_fast16_t                   get_function_name_idx (uint_fast8_t fidx);

        bool                            get_function_pulse (uint_fast8_t fidx);
        uint32_t                        get_function_pulse_mask (void);

        bool                            get_function_sound (uint_fast8_t fidx);
        uint32_t                        get_function_sound_mask (void);

        void                            activate (void);
        void                            deactivate (void);
        bool                            is_active (void);

        void                            set_online (bool value);
        bool                            is_online (void);

        void                            set_speed_value (uint_fast8_t tspeed);
        void                            set_speed (uint_fast8_t speed);
        void                            set_speed (uint_fast8_t tspeed, uint_fast16_t tenths);
        uint16_t                        get_speed (void);

        void                            set_fwd (uint_fast8_t fwd);
        void                            set_speed_fwd (uint_fast8_t speed, uint_fast8_t fwd);
        uint_fast8_t                    get_fwd (void);

        void                            set_function (uint_fast8_t f, bool b);
        uint_fast8_t                    get_function (uint_fast8_t f);
        void                            reset_functions (void);
        uint32_t                        get_functions (void);

        void                            set_destination (uint_fast8_t rrg);
        uint_fast8_t                    get_destination (void);

        void                            set_rcllocation (uint_fast8_t location);
        uint_fast8_t                    get_rcllocation (void);

        void                            set_rrlocation (uint_fast16_t rrgrridx);
        uint_fast16_t                   get_rrlocation (void);

        void                            get_ack (void);

        void                            set_rc2_rate (uint_fast8_t rate);
        uint_fast8_t                    get_rc2_rate (void);

        void                            estop (void);
        void                            sched (void);
    private:
        uint16_t                        id;
        std::string                     name;
        uint16_t                        addr;
        uint16_t                        addon_idx;                                              // idx of add-on
        uint8_t                         coupled_functions[MAX_LOCO_FUNCTIONS];                  // coupled functions
        LOCOFUNCTION                    locofunction;
        LOCOMACRO                       macros[MAX_LOCO_MACROS_PER_LOCO];
        uint8_t                         speed_steps;                                            // 14, 28, or 128
        uint8_t                         fwd;
        uint8_t                         speed;
        uint8_t                         target_speed;                                           // target speed
        uint32_t                        target_millis_step;                                     // increment speed every millis_step
        uint32_t                        target_next_millis;                                     // next millis to step down/up
        uint32_t                        functions;
        uint8_t                         packet_sequence_idx;
        uint32_t                        rc_millis;
        uint8_t                         rc2_rate;
        uint8_t                         offline_cnt;
        uint8_t                         rcl_location;                                           // a loco can be in rcl & rr_location at the same time!
        uint16_t                        rr_location;    
        uint16_t                        sleep_cnt;
        uint32_t                        flags;
        uint8_t                         destination;                                            // id of railroad, FF = no destination
        uint8_t                         active;

        void                            set_function_type_pulse (uint_fast8_t f, bool b);
        void                            set_function_type_sound (uint_fast8_t f, bool b);
        void                            sendspeed (void);
        void                            sendfunction (uint_fast8_t range);
        void                            sendcmd (uint_fast8_t packet_no);

};

class Locos
{
    public:
        static std::vector<Loco>        locos;
        static bool                     data_changed;

        static uint_fast16_t            add (const Loco& loco);
        static uint_fast16_t            get_n_locos (void);
        static uint_fast16_t            set_new_id (uint_fast16_t loco_idx, uint_fast16_t new_loco_idx);
        static void                     set_new_addon_ids (uint16_t * map_new_addon_idx, uint16_t n_addons);
        static bool                     schedule (void);
        static void                     estop (void);
        static void                     booster_off (bool do_send_booster_cmd);
        static void                     booster_on (bool do_send_booster_cmd);
    private:
        static uint_fast16_t            n_locos;                                        // number of locos
        static void                     renumber(void);
};

#endif
