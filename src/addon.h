/*------------------------------------------------------------------------------------------------------------------------
 * addon.h - addon management functions
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
#ifndef ADDON_H
#define ADDON_H

#include <stdint.h>
#include <string>

#define MAX_ADDONS      64

typedef struct
{
    uint32_t            pulse_mask;
    uint32_t            sound_mask;
    uint16_t            name_idx[MAX_LOCO_FUNCTIONS];
    uint8_t             max;                                // highest used function index
} ADDONFUNCTION;

typedef struct
{
    char *              name;
    uint16_t            addr;
    ADDONFUNCTION       addonfunction;
    uint32_t            functions;
    uint16_t            loco_idx;
    uint8_t             packet_sequence_idx;
    uint32_t            rc_millis;
    uint8_t             rc2_rate;
    uint8_t             offline_cnt;
    uint8_t             active;
} ADDON;

class AddOn
{
    public:
        static bool                     data_changed;

        static uint_fast16_t            add (void);
        static uint_fast16_t            setid (uint_fast16_t addon_idx, uint_fast16_t new_addon_idx);
        static void                     set_new_loco_ids (uint16_t * map_new_loco_idx, uint16_t n_locos);
        static uint_fast16_t            get_n_addons (void);

        static void                     set_loco (uint_fast16_t addon_idx, uint_fast16_t loco_idx);
        static void                     reset_loco (uint_fast16_t addon_idx);
        static uint_fast16_t            get_loco (uint_fast16_t addon_idx);

        static void                     set_name (uint_fast16_t addon_idx, const char * name);
        static char *                   get_name (uint_fast16_t addon_idx);

        static void                     set_addr (uint_fast16_t addon_idx, uint_fast16_t addr);
        static uint_fast16_t            get_addr (uint_fast16_t addon_idx);

        static void                     set_speed_steps (uint_fast16_t addon_idx, uint_fast8_t speed_steps);
        static uint_fast8_t             get_speed_steps (uint_fast16_t addon_idx);

        static uint_fast8_t             set_function_type (uint_fast16_t addon_idx, uint_fast8_t fidx, uint_fast16_t function_name_idx, bool pulse, bool sound);

        static std::string&             get_function_name (uint_fast16_t addon_idx, uint_fast8_t fidx);
        static uint_fast16_t            get_function_name_idx (uint_fast16_t addon_idx, uint_fast8_t fidx);

        static bool                     get_function_pulse (uint_fast16_t addon_idx, uint_fast8_t fidx);
        static uint32_t                 get_function_pulse_mask (uint_fast16_t addon_idx);

        static bool                     get_function_sound (uint_fast16_t addon_idx, uint_fast8_t fidx);
        static uint32_t                 get_function_sound_mask (uint_fast16_t addon_idx);

        static void                     activate (uint_fast16_t addon_idx);
        static void                     deactivate (uint_fast16_t addon_idx);
        static uint_fast8_t             is_active (uint_fast16_t addon_idx);

        static void                     set_speed (uint_fast16_t addon_idx, uint_fast8_t speed);
        static void                     set_speed (uint_fast16_t addon_idx, uint_fast8_t tspeed, uint_fast16_t tenths);
        static uint16_t                 get_speed (uint_fast16_t addon_idx);

        static void                     set_function (uint_fast16_t addon_idx, uint_fast8_t f, bool b);
        static uint_fast8_t             get_function (uint_fast16_t addon_idx, uint_fast8_t f);
        static void                     reset_functions (uint_fast16_t addon_idx);
        static uint32_t                 get_functions (uint_fast16_t addon_idx);

        static bool                     schedule (void);
    private:
        static ADDON                    addons[];                                   // addons
        static uint_fast16_t            n_addons;                                   // number of addons

        static void                     sendcmd (uint_fast16_t addon_idx, uint_fast8_t packet_no);
        static void                     set_function_type_pulse (uint_fast16_t addon_idx, uint_fast8_t f, bool b);
        static void                     set_function_type_sound (uint_fast16_t addon_idx, uint_fast8_t f, bool b);
        static void                     sendfunction (uint_fast16_t addon_idx, uint_fast8_t range);
};

#endif
