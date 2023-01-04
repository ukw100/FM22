/*-------------------------------------------------------------------------------------------------------------------------------------------
 * railroad.h - railroad machine management functions
 *-------------------------------------------------------------------------------------------------------------------------------------------
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
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
#ifndef RAILROAD_H
#define RAILROAD_H

#include <stdint.h>

#define MAX_RAILROAD_GROUPS                     8
#define MAX_RAILROADS_PER_RAILROAD_GROUP        16

#define MAX_SWITCHES_PER_RAILROAD               16

typedef struct
{
    char *                      name;                                           // configuration: name of railroad
    uint8_t                     n_switches;                                     // configuration: number of switches
    uint16_t                    linked_loco_idx;                                // configuration: linked loco
    uint16_t                    active_loco_idx;                                // runtime: set if RCL detector activates railroad by detected loco_idx
    uint16_t                    located_loco_idx;                               // runtime: set if S88 contact gets occupied by active_loco_idx
    uint16_t                    switches[MAX_SWITCHES_PER_RAILROAD];            // configuration: switch definitions
    uint8_t                     states[MAX_SWITCHES_PER_RAILROAD];              // runtime: switch states
} RAILROAD;

typedef struct
{
    char *                      name;                                           // configuration: name of railroad group
    uint8_t                     n_railroads;                                    // configuration: number of railroads
    uint8_t                     active_railroad_idx;                            // runtime: active railroad
    RAILROAD                    railroads[MAX_RAILROADS_PER_RAILROAD_GROUP];    // configuration: railroads, see above
} RAILROAD_GROUP;

class Railroad
{
    public:
        static bool             data_changed;

        static uint_fast8_t     group_add (void);
        static uint_fast8_t     group_setid (uint_fast8_t rrgidx, uint_fast8_t new_rrgidx);
        static uint_fast8_t     get_n_railroad_groups (void);

        static void             group_setname (uint_fast8_t rrgidx, const char * name);
        static char *           group_getname (uint_fast8_t rrgidx);

        static uint_fast8_t     add (uint_fast8_t rrgidx);
        static uint_fast8_t     setid (uint_fast8_t rrgidx, uint_fast8_t rridx, uint_fast8_t new_rridx);
        static uint_fast8_t     get_n_railroads (uint_fast8_t rrgidx);

        static void             set_name (uint_fast8_t rrgidx, uint_fast8_t rridx, const char * name);
        static char *           get_name (uint_fast8_t rrgidx, uint_fast8_t rridx);

        static void             set_link_loco (uint_fast8_t rrgidx, uint_fast8_t rridx, uint_fast16_t loco_idx);
        static uint_fast16_t    get_link_loco (uint_fast8_t rrgidx, uint_fast8_t rridx);
        static uint_fast8_t     get_link_railroad (uint_fast8_t rrgidx, uint_fast16_t loco_idx);

        static uint_fast16_t    get_active_loco (uint_fast8_t rrgidx, uint_fast8_t rridx);
        static void             set_active_loco (uint_fast8_t rrgidx, uint_fast8_t rridx, uint_fast16_t loco_idx);

        static uint_fast16_t    get_located_loco (uint_fast8_t rrgidx, uint_fast8_t rridx);
        static void             set_located_loco (uint_fast8_t rrgidx, uint_fast8_t rridx, uint_fast16_t loco_idx);

        static uint_fast8_t     add_switch (uint_fast8_t rrgidx, uint_fast8_t rridx);
        static uint_fast8_t     get_n_switches (uint_fast8_t rrgidx, uint_fast8_t rridx);

        static void             set_switch_idx (uint_fast8_t rrgidx, uint_fast8_t rridx, uint_fast8_t sub_idx, uint_fast16_t sw_idx);
        static uint_fast16_t    get_switch_idx (uint_fast8_t rrgidx, uint_fast8_t rridx, uint_fast8_t sub_idx);

        static void             set_switch_state (uint_fast8_t rrgidx, uint_fast8_t rridx, uint_fast8_t sub_idx, uint_fast8_t state);
        static uint_fast8_t     get_switch_state (uint_fast8_t rrgidx, uint_fast8_t rridx, uint_fast8_t sub_idx);
        static void             set_new_switch_ids (uint16_t * map_new_switch_idx, uint_fast16_t n_switches);

        static uint_fast8_t     del_switch (uint_fast8_t rrgidx, uint_fast8_t rridx, uint_fast8_t subidx);
        static uint_fast8_t     del (uint_fast8_t rrgidx, uint_fast8_t rridx);
        static uint_fast8_t     group_del (uint_fast8_t rrgidx);

        static void             set (uint_fast8_t rrgidx, uint_fast8_t rridx, uint_fast16_t loco_idx);
        static void             set (uint_fast8_t rrgidx, uint_fast8_t rridx);
        static uint_fast8_t     get (uint_fast8_t rrgidx);

        static void             set_new_loco_ids (uint16_t * map_new_loco_idx, uint_fast16_t n_locos);

        static void             booster_on (void);
        static void             booster_off (void);

    private:
        static RAILROAD_GROUP   railroad_groups[MAX_RAILROAD_GROUPS];
        static uint_fast8_t     n_railroad_groups;
};

#endif
