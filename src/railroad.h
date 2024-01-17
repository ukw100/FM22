/*-------------------------------------------------------------------------------------------------------------------------------------------
 * railroad.h - railroad machine management functions
 *-------------------------------------------------------------------------------------------------------------------------------------------
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
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
#ifndef RAILROAD_H
#define RAILROAD_H

#include <stdint.h>
#include <string>
#include <vector>
#include <memory>

#define MAX_RAILROAD_GROUPS                     254
#define MAX_RAILROADS_PER_RAILROAD_GROUP        32
#define MAX_SWITCHES_PER_RAILROAD               32

class Railroad
{
    public:
                                            Railroad ();
        void                                set_name (std::string name);
        std::string                         get_name ();

        void                                set_link_loco (uint_fast16_t loco_idx);
        uint_fast16_t                       get_link_loco ();

        uint_fast16_t                       get_active_loco ();
        void                                set_active_loco (uint_fast16_t loco_idx);

        uint_fast16_t                       get_located_loco ();
        void                                set_located_loco (uint_fast16_t loco_idx);

        uint_fast8_t                        add_switch ();
        uint_fast8_t                        get_n_switches ();

        void                                set_switch_idx (uint_fast8_t sub_idx, uint_fast16_t sw_idx);
        uint_fast16_t                       get_switch_idx (uint_fast8_t sub_idx);

        void                                set_switch_state (uint_fast8_t sub_idx, uint_fast8_t state);
        uint_fast8_t                        get_switch_state (uint_fast8_t sub_idx);

        uint_fast8_t                        del_switch (uint_fast8_t subidx);

    private:
        std::string                         name;                                           // configuration: name of railroad
        uint8_t                             n_switches;                                     // configuration: number of switches
        uint16_t                            linked_loco_idx;                                // configuration: linked loco
        uint16_t                            active_loco_idx;                                // runtime: set if RCL detector activates railroad by detected loco_idx
        uint16_t                            located_loco_idx;                               // runtime: set if S88 contact gets occupied by active_loco_idx
        uint16_t                            switches[MAX_SWITCHES_PER_RAILROAD];            // configuration: switch definitions
        uint8_t                             states[MAX_SWITCHES_PER_RAILROAD];              // runtime: switch states
};

class RailroadGroup
{
    public:
        std::vector<Railroad>               railroads;                                      // configuration: railroads, see above

                                            RailroadGroup ();
        void                                set_id (uint_fast8_t id);
        uint_fast8_t                        add (const Railroad& railroad);
        uint_fast8_t                        get_n_railroads ();
        void                                set_name (std::string name);
        std::string                         get_name ();

        uint_fast8_t                        get_link_railroad (uint_fast16_t loco_idx);

        void                                set_active_railroad (uint_fast8_t rridx, uint_fast16_t loco_idx);
        void                                set_active_railroad (uint_fast8_t rridx);
        uint_fast8_t                        get_active_railroad ();

        uint_fast8_t                        set_new_id (uint_fast8_t rridx, uint_fast8_t new_rridx);
        void                                del (uint_fast8_t rridx);

    private:
        uint16_t                            id;
        std::string                         name;                                           // configuration: name of railroad group
        uint_fast8_t                        n_railroads;                                    // configuration: number of railroads
        uint_fast8_t                        active_railroad_idx;                            // runtime: active railroad
};

class RailroadGroups
{
    public:
        static bool                         data_changed;
        static std::vector<RailroadGroup>   railroad_groups;

        static void                         set_id (uint_fast8_t id);
        static uint_fast8_t                 add (const RailroadGroup& railroad_group);
        static void                         del (uint_fast8_t rrgidx);
        static uint_fast8_t                 get_n_railroad_groups (void);
        static uint_fast8_t                 set_new_id (uint_fast8_t rrgidx, uint_fast8_t new_rrgidx);
        static void                         set_new_switch_ids (uint16_t * map_new_switch_idx, uint_fast16_t n_switches);
        static void                         set_new_loco_ids (uint16_t * map_new_loco_idx, uint_fast16_t n_locos);
        static void                         booster_on (void);
        static void                         booster_off (void);

    private:
        static uint8_t                      n_railroad_groups;
        static void                         renumber ();
};

#endif
