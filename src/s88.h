/*------------------------------------------------------------------------------------------------------------------------
 * s88.h - s88 management functions
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
#ifndef S88_H
#define S88_H

#include <stdint.h>

#define S88_MAX_ACTION_PARAMETERS                   8
#define S88_STATE_FREE                              false
#define S88_STATE_OCCUPIED                          true

#define S88_MAX_CONTACTS                            128                                             // should be a multiple of 16
#define S88_MAX_CONTACT_BYTES                       (S88_MAX_CONTACTS / sizeof (uint8_t))

#define S88_MAX_NAME_SIZE                           32
#define S88_MAX_ACTIONS_PER_CONTACT                 8

#define S88_RCL_DETECTED_OFFSET                     65000U                      // offset for rcl-detected locos

#define S88_ACTION_NONE                             0
#define S88_ACTION_SET_LOCO_SPEED                   1                           // parameters: START, LOCO_IDX, SPEED, TENTHS
#define S88_ACTION_SET_LOCO_MIN_SPEED               2                           // parameters: START, LOCO_IDX, SPEED, TENTHS
#define S88_ACTION_SET_LOCO_MAX_SPEED               3                           // parameters: START, LOCO_IDX, SPEED, TENTHS
#define S88_ACTION_SET_LOCO_FORWARD_DIRECTION       4                           // parameters: START, LOCO_IDX, FWD
#define S88_ACTION_SET_LOCO_ALL_FUNCTIONS_OFF       5                           // parameters: START, LOCO_IDX
#define S88_ACTION_SET_LOCO_FUNCTION_OFF            6                           // parameters: START, LOCO_IDX, FUNC_IDX
#define S88_ACTION_SET_LOCO_FUNCTION_ON             7                           // parameters: START, LOCO_IDX, FUNC_IDX
#define S88_ACTION_EXECUTE_LOCO_MACRO               8                           // parameters: START, LOCO_IDX, MACRO_IDX
#define S88_ACTION_SET_ADDON_ALL_FUNCTIONS_OFF      9                           // parameters: START, LOCO_IDX
#define S88_ACTION_SET_ADDON_FUNCTION_OFF           10                           // parameters: START, LOCO_IDX, FUNC_IDX
#define S88_ACTION_SET_ADDON_FUNCTION_ON            11                          // parameters: START, LOCO_IDX, FUNC_IDX
#define S88_ACTION_SET_RAILROAD                     12                          // parameters: RRG_IDX, RR_IDX
#define S88_ACTION_SET_FREE_RAILROAD                13                          // parameters: RRG_IDX
#define S88_ACTIONS                                 14                          // number of possible actions

typedef struct
{
    uint8_t                             action;
    uint8_t                             n_parameters;
    uint16_t                            parameters[S88_MAX_ACTION_PARAMETERS];
} CONTACT_ACTION;

typedef struct
{
    char *                              name;
    uint8_t                             rrgidx;
    uint8_t                             rridx;
    uint8_t                             n_contact_actions_in;
    uint8_t                             n_contact_actions_out;
    CONTACT_ACTION                      contact_actions_in[S88_MAX_ACTIONS_PER_CONTACT];
    CONTACT_ACTION                      contact_actions_out[S88_MAX_ACTIONS_PER_CONTACT];
} CONTACT;

class S88
{
    public:
        static CONTACT                  contacts[S88_MAX_CONTACTS];
        static uint8_t                  current_bits[S88_MAX_CONTACT_BYTES];
        static bool                     data_changed;

        static uint_fast16_t            add_contact (void);
        static uint_fast16_t            setid (uint_fast16_t coidx, uint_fast16_t new_coidx);
        static uint_fast16_t            get_n_contacts (void);
        static bool                     get_n_contacts_changed (void);
        static void                     set_name (uint_fast16_t coidx, char * name);
        static char *                   get_name (uint_fast16_t coidx);
        static void                     set_link_railroad (uint_fast16_t coidx, uint_fast8_t rrgidx, uint_fast8_t rridx);
        static uint_fast16_t            get_link_railroad (uint_fast16_t coidx);
        static uint_fast8_t             add_contact_action (bool in, uint_fast16_t coidx);
        static void                     delete_contact_action (bool in, uint_fast16_t coidx, uint_fast8_t caidx);
        static uint_fast8_t             get_n_contact_actions (bool in, uint_fast16_t coidx);
        static uint_fast8_t             set_contact_action (bool in, uint_fast16_t coidx, uint_fast8_t caidx, CONTACT_ACTION * cap);
        static uint_fast8_t             get_contact_action (bool in, uint_fast16_t coidx, uint_fast8_t caidx, CONTACT_ACTION * cap);
        static uint_fast8_t             booster_on (void);
        static uint_fast8_t             booster_off (void);
        static void                     schedule (void);

        static bool                     get_state_bit (uint_fast16_t coidx);
        static uint_fast8_t             get_state_byte (uint_fast16_t byteidx);
        static void                     set_state_bit (uint_fast16_t coidx, bool value);
        static void                     set_state_byte (uint_fast8_t byte_idx, uint_fast8_t value);

        static uint_fast8_t             get_newstate_bit (uint_fast16_t coidx);
        static uint_fast8_t             get_newstate_byte (uint_fast16_t byteidx);
        static void                     set_newstate_bit (uint_fast16_t coidx, bool value);
        static void                     set_newstate_byte (uint_fast16_t byte_idx, uint_fast8_t value);

        static uint_fast16_t            get_link_loco (bool in, uint_fast8_t coidx, uint_fast8_t caidx, uint_fast8_t paidx);
        static uint_fast16_t            get_free_railroad (uint_fast8_t rrgidx);
        static void                     set_new_loco_ids (uint16_t * map_new_loco_idx, uint16_t n_locos);
        static void                     set_new_addon_ids (uint16_t * map_new_addon_idx, uint16_t n_addons);
        static void                     set_new_railroad_group_ids (uint8_t * map_new_rrgidx, uint8_t n_railroad_groups);
        static void                     set_new_railroad_ids (uint_fast8_t rrgidx, uint8_t * map_new_rridx, uint_fast8_t n_railroads);
        static void                     init (void);

    private:
        static uint_fast16_t            n_contacts;                                 // number of contacts
        static bool                     n_contacts_changed;
        static uint8_t                  new_bits[S88_MAX_CONTACT_BYTES];

        static uint_fast16_t            number_of_status_bytes (void);
        static void                     execute_contact_actions (bool in, uint_fast16_t coidx);
        static void                     set_new_loco_ids_for_contact (bool in, uint_fast16_t coidx, uint16_t * map_new_loco_idx, uint16_t n_locos);
        static void                     set_new_addon_ids_for_contact (bool in, uint_fast16_t coidx, uint16_t * map_new_addon_idx, uint16_t n_addons);
        static void                     set_new_railroad_group_ids_for_contact (bool in, uint_fast16_t coidx, uint8_t * map_new_rrgidx, uint_fast8_t n_railroad_groups);
        static void                     set_new_railroad_ids_for_contact (bool in, uint_fast16_t coidx, uint_fast8_t current_rrgidx, uint8_t * map_new_rridx, uint_fast8_t n_railroads);
};

#endif
