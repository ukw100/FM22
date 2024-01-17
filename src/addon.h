/*------------------------------------------------------------------------------------------------------------------------
 * addon.h - addon management functions
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
#ifndef ADDON_H
#define ADDON_H

#include <stdint.h>
#include <string>
#include <vector>
#include <memory>

#define MAX_ADDONS      1024

typedef struct
{
    uint32_t            pulse_mask;
    uint32_t            sound_mask;
    uint16_t            name_idx[MAX_LOCO_FUNCTIONS];
    uint8_t             max;                                // highest used function index
} ADDONFUNCTION;

class AddOn
{
    public:
                                        AddOn ();
        void                            sched();
        void                            set_id (uint_fast16_t id);
        void                            set_loco (uint_fast16_t loco_idx);
        void                            reset_loco ();
        uint_fast16_t                   get_loco ();

        void                            set_name (std::string name);
        std::string                     get_name ();

        void                            set_addr (uint_fast16_t addr);
        uint_fast16_t                   get_addr ();

        uint_fast8_t                    set_function_type (uint_fast8_t fidx, uint_fast16_t function_name_idx, bool pulse, bool sound);

        std::string&                    get_function_name (uint_fast8_t fidx);
        uint_fast16_t                   get_function_name_idx (uint_fast8_t fidx);

        bool                            get_function_pulse (uint_fast8_t fidx);
        uint32_t                        get_function_pulse_mask ();

        bool                            get_function_sound (uint_fast8_t fidx);
        uint32_t                        get_function_sound_mask ();

        void                            activate ();
        void                            deactivate ();
        uint_fast8_t                    is_active ();

        void                            set_function (uint_fast8_t f, bool b);
        uint_fast8_t                    get_function (uint_fast8_t f);
        void                            reset_functions ();
        uint32_t                        get_functions ();

    private:
        uint16_t                        id;
        std::string                     name;
        uint16_t                        addr;
        ADDONFUNCTION                   addonfunction;
        uint32_t                        functions;
        uint16_t                        loco_idx;
        uint8_t                         packet_sequence_idx;
        uint8_t                         active;

        void                            set_function_type_pulse (uint_fast8_t f, bool b);
        void                            set_function_type_sound (uint_fast8_t f, bool b);
        void                            sendfunction (uint_fast8_t range);
        void                            sendcmd (uint_fast8_t packet_no);
};

class AddOns
{
    public:
        static std::vector<AddOn>       addons;
        static bool                     data_changed;
        static uint_fast16_t            add (const AddOn& addon);
        static uint_fast16_t            get_n_addons (void);
        static uint_fast16_t            set_new_id (uint_fast16_t addon_idx, uint_fast16_t new_addon_idx);
        static void                     set_new_loco_ids (uint16_t * map_new_loco_idx, uint16_t n_locos);
        static bool                     schedule (void);
    private:
        static uint_fast16_t            n_addons;                                   // number of addons
        static void                     renumber();
};

#endif
