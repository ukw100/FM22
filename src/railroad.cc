/*------------------------------------------------------------------------------------------------------------------------
 * railroad.cc - railroad management functions
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
#include <stdlib.h>
#include <string.h>
#include "switch.h"
#include "loco.h"
#include "s88.h"
#include "rcl.h"
#include "railroad.h"
#include "debug.h"

bool                                RailroadGroups::data_changed = false;
std::vector<RailroadGroup>          RailroadGroups::railroad_groups;
uint_fast8_t                        RailroadGroups::n_railroad_groups = 0;

/*------------------------------------------------------------------------------------------------------------------------
 *  Railroad::Railroad () - constructor
 *------------------------------------------------------------------------------------------------------------------------
 */
Railroad::Railroad ()
{
    this->name              = "";
    this->n_switches        = 0;
    this->linked_loco_idx   = 0xFFFF;
    this->active_loco_idx   = 0xFFFF;
    this->located_loco_idx  = 0xFFFF;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  Railroad::set_name () - set name of railroad
 *------------------------------------------------------------------------------------------------------------------------
 */
void
Railroad::set_name (std::string name)
{
    this->name = name;
    RailroadGroups::data_changed = true;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  Railroad::get_name () - get name
 *------------------------------------------------------------------------------------------------------------------------
 */
std::string
Railroad::get_name ()
{
    return this->name;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  Railroad::set_link_loco () - link railroad with loco
 *------------------------------------------------------------------------------------------------------------------------
 */
void
Railroad::set_link_loco (uint_fast16_t loco_idx)
{
    if (this->linked_loco_idx != loco_idx)
    {
        this->linked_loco_idx = loco_idx;
        RailroadGroups::data_changed = true;
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 *  Railroad::get_link_loco () - get linked loco
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast16_t
Railroad::get_link_loco ()
{
    return this->linked_loco_idx;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  Railroad::get_active_loco () - get active loco
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast16_t
Railroad::get_active_loco ()
{
    return this->active_loco_idx;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  Railroad::set_active_loco () - set active loco
 *------------------------------------------------------------------------------------------------------------------------
 */
void
Railroad::set_active_loco (uint_fast16_t loco_idx)
{
    this->active_loco_idx = loco_idx;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  Railroad::get_located_loco () - get located loco
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast16_t
Railroad::get_located_loco ()
{
    return this->located_loco_idx;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  Railroad::set_located_loco () - set located loco
 *------------------------------------------------------------------------------------------------------------------------
 */
void
Railroad::set_located_loco (uint_fast16_t loco_idx)
{
    this->located_loco_idx = loco_idx;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  Railroad::add_switch () - add a switch to railroad
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
Railroad::add_switch ()
{
    uint_fast8_t subidx;

    subidx = this->n_switches;

    if (subidx < MAX_SWITCHES_PER_RAILROAD)
    {
        this->n_switches++;
        RailroadGroups::data_changed = true;
    }
    else
    {
        subidx = 0xFF;
    }

    return subidx;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  Railroad::get_n_switches  () - get number of switches of railroad
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
Railroad::get_n_switches ()
{
    return this->n_switches;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  Railroad::set_switch_idx () - set switch index
 *------------------------------------------------------------------------------------------------------------------------
 */
void
Railroad::set_switch_idx (uint_fast8_t subidx, uint_fast16_t swidx)
{
    if (this->switches[subidx] != swidx)
    {
        this->switches[subidx] = swidx;
        RailroadGroups::data_changed = true;
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 *  Railroad::get_switch_idx () - get switch index
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast16_t
Railroad::get_switch_idx (uint_fast8_t subidx)
{
    return this->switches[subidx];
}

/*------------------------------------------------------------------------------------------------------------------------
 *  Railroad::set_switch_state () - set switch state
 *------------------------------------------------------------------------------------------------------------------------
 */
void
Railroad::set_switch_state (uint_fast8_t subidx, uint_fast8_t state)
{
    this->states[subidx] = state;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  Railroad::get_switch_state () - get switch state
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
Railroad::get_switch_state (uint_fast8_t subidx)
{
    return this->states[subidx];
}

/*------------------------------------------------------------------------------------------------------------------------
 *  Railroad::del_switch () - remove switch from railroad
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
Railroad::del_switch (uint_fast8_t subidx)
{
    uint_fast8_t    n_rr_switches   = this->n_switches;
    uint16_t *      rrsp            = this->switches;
    uint_fast8_t    idx;
    uint_fast8_t    rtc = 0;

    if (subidx < n_rr_switches)
    {
        n_rr_switches--;

        for (idx = subidx; idx < n_rr_switches; idx++)
        {
            memcpy (rrsp + idx, rrsp + (idx + 1), sizeof (uint16_t));
        }

        this->n_switches = n_rr_switches;
        RailroadGroups::data_changed = true;
        rtc = 1;
    }

    return rtc;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  RailroadGroup::RailroadGroup () - constructor
 *------------------------------------------------------------------------------------------------------------------------
 */
RailroadGroup::RailroadGroup ()
{
    this->id                    = 0;
    this->name                  = "";
    this->n_railroads           = 0;
    this->active_railroad_idx   = 0xFF;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  RailroadGroup::set_id () - set of railroad group
 *------------------------------------------------------------------------------------------------------------------------
 */
void
RailroadGroup::set_id (uint_fast8_t id)
{
    this->id = id;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  set_active_railroad () - set active railroad
 *------------------------------------------------------------------------------------------------------------------------
 */
void
RailroadGroup::set_active_railroad (uint_fast8_t rridx, uint_fast16_t loco_idx)
{
    uint_fast8_t subidx;

    if (rridx < this->n_railroads)
    {
        Railroad *      rr          = &(this->railroads[rridx]);
        uint_fast8_t    n_switches  = rr->get_n_switches();

        for (subidx = 0; subidx < n_switches; subidx++)
        {
            uint_fast16_t   swidx = rr->get_switch_idx(subidx);
            uint_fast8_t    state = rr->get_switch_state(subidx);
            Switches::switches[swidx].set_state (state);
            Debug::printf (DEBUG_LEVEL_VERBOSE, "RailroadGroup::set: switch: %d set_state: %d\r\n", swidx, state);
        }
    }

    this->active_railroad_idx = rridx;        // may be 0xFF (undefined)

    if (rridx != 0xFF)
    {
        this->railroads[rridx].set_active_loco (loco_idx);
    }
}


/*------------------------------------------------------------------------------------------------------------------------
 *  RailroadGroup::set_active_railroad () - set railroad
 *------------------------------------------------------------------------------------------------------------------------
 */
void
RailroadGroup::set_active_railroad (uint_fast8_t rridx)
{
    RailroadGroup::set_active_railroad (rridx, 0xFFFF);
}

/*------------------------------------------------------------------------------------------------------------------------
 *  RailroadGroup::get_active_railroad () - get active railroad
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
RailroadGroup::get_active_railroad ()
{
    return this->active_railroad_idx;   // may be 0xFF (undefined)
}

/*------------------------------------------------------------------------------------------------------------------------
 *  set_name () - set name
 *------------------------------------------------------------------------------------------------------------------------
 */
void
RailroadGroup::set_name (std::string name)
{
    this->name = name;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  RailroadGroup::get_name () - get name
 *------------------------------------------------------------------------------------------------------------------------
 */
std::string
RailroadGroup::get_name ()
{
    return this->name;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  RailroadGroup::add () - add a railroad
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
RailroadGroup::add (const Railroad& railroad)
{
    uint_fast8_t rtc = 0xFF;

    if (this->n_railroads < MAX_RAILROAD_GROUPS)
    {
        if (this->n_railroads < MAX_RAILROADS_PER_RAILROAD_GROUP)
        {
            this->railroads.push_back(railroad);
            rtc = this->n_railroads;
            this->n_railroads++;
            RailroadGroups::data_changed = true;
        }
    }

    return rtc;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  set_new_id () - set new id of railroad
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
RailroadGroup::set_new_id (uint_fast8_t rridx, uint_fast8_t new_rridx)
{
    uint_fast8_t    n_railroads = this->n_railroads;
    uint_fast8_t    rtc = 0xFF;

    if (rridx < n_railroads && new_rridx < n_railroads)
    {
        if (rridx != new_rridx)
        {
            Railroad        tmprailroad;
            uint_fast8_t    ridx;

            uint8_t *      map_new_rridx = (uint8_t *) calloc (n_railroads, sizeof (uint8_t));

            for (ridx = 0; ridx < n_railroads; ridx++)
            {
                map_new_rridx[ridx] = ridx;
            }

            tmprailroad = this->railroads[rridx];

            if (new_rridx < rridx)
            {
                for (ridx = rridx; ridx > new_rridx; ridx--)
                {
                    this->railroads[ridx] = this->railroads[ridx - 1];
                    map_new_rridx[ridx - 1] = ridx;
                }
            }
            else // if (new_rridx > rridx)
            {
                for (ridx = rridx; ridx < new_rridx; ridx++)
                {
                    this->railroads[ridx] = this->railroads[ridx + 1];
                    map_new_rridx[ridx + 1] = ridx;
                }
            }

            this->railroads[ridx] = tmprailroad;
            map_new_rridx[rridx] = ridx;

            for (ridx = 0; ridx < n_railroads; ridx++)
            {
                Debug::printf (DEBUG_LEVEL_VERBOSE, "%2d -> %2d\n", ridx, map_new_rridx[ridx]);
            }

            S88::set_new_railroad_ids (this->id, map_new_rridx, n_railroads);
            RCL::set_new_railroad_ids (this->id, map_new_rridx, n_railroads);

            free (map_new_rridx);
            RailroadGroups::data_changed = true;
            rtc = new_rridx;
        }
        else
        {
            rtc = rridx;
        }
    }

    return rtc;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  RailroadGroup::get_n_railroads () - get number of railroads
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
RailroadGroup::get_n_railroads ()
{
    return this->n_railroads;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  RailroadGroup::get_link_railroad () - get linked railroad
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
RailroadGroup::get_link_railroad (uint_fast16_t loco_idx)
{
    uint_fast8_t rridx;
    uint_fast8_t rtc = 0xFF;

    for (rridx = 0; rridx < this->n_railroads; rridx++)
    {
        if (this->railroads[rridx].get_link_loco() == loco_idx)
        {
            rtc = rridx;
            break;
        }
    }

    return rtc;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  RailroadGroup::del () - remove railroad
 *------------------------------------------------------------------------------------------------------------------------
 */
void
RailroadGroup::del (uint_fast8_t rridx)
{
    this->railroads.erase(this->railroads.begin() + rridx);
}

/*------------------------------------------------------------------------------------------------------------------------
 *  renumber () - renumber ids
 *------------------------------------------------------------------------------------------------------------------------
 */
void
RailroadGroups::renumber ()
{
    uint_fast16_t rrgidx;

    for (rrgidx = 0; rrgidx < RailroadGroups::n_railroad_groups; rrgidx++)
    {
        RailroadGroups::railroad_groups[rrgidx].set_id (rrgidx);
    }
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * RailroadGroups::set_new_loco_ids () - set new loco ids
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
RailroadGroups::set_new_loco_ids (uint16_t * map_new_loco_idx, uint_fast16_t n_locos)
{
    uint_fast8_t    n_railroad_groups   = RailroadGroups::n_railroad_groups;
    uint_fast8_t    n_railroads;
    uint_fast8_t    rrgidx;
    uint_fast8_t    rridx;

    for (rrgidx = 0; rrgidx < n_railroad_groups; rrgidx++)
    {
        RailroadGroup * rrg = &RailroadGroups::railroad_groups[rrgidx];
        n_railroads         = rrg->get_n_railroads();

        for (rridx = 0; rridx < n_railroads; rridx++)
        {
            Railroad * rr                   = &rrg->railroads[rridx];
            uint_fast16_t linked_loco_idx   = rr->get_link_loco();
            uint_fast16_t active_loco_idx   = rr->get_active_loco();
            uint_fast16_t located_loco_idx  = rr->get_located_loco();

            if (linked_loco_idx < n_locos && linked_loco_idx != map_new_loco_idx[linked_loco_idx])
            {
                rr->set_link_loco(map_new_loco_idx[linked_loco_idx]);
                RailroadGroups::data_changed = true;
            }

            if (active_loco_idx < n_locos && active_loco_idx != map_new_loco_idx[active_loco_idx])
            {
                rr->set_active_loco(map_new_loco_idx[active_loco_idx]);
                RailroadGroups::data_changed = true;
            }

            if (located_loco_idx < n_locos && located_loco_idx != map_new_loco_idx[located_loco_idx])
            {
                rr->set_located_loco(map_new_loco_idx[located_loco_idx]);
                RailroadGroups::data_changed = true;
            }
        }
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 *  RailroadGroups::add () - add a railroad
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
RailroadGroups::add (const RailroadGroup& railroad_group)
{
    uint_fast8_t rrgidx = RailroadGroups::n_railroad_groups;

    if (rrgidx < MAX_RAILROAD_GROUPS)
    {
        RailroadGroups::railroad_groups.push_back(railroad_group);
        RailroadGroups::n_railroad_groups++;
        RailroadGroups::data_changed = true;
    }
    else
    {
        rrgidx = 0xFF;
    }

    return rrgidx;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  RailroadGroups::group_del () - remove railroad group
 *------------------------------------------------------------------------------------------------------------------------
 */
void
RailroadGroups::del (uint_fast8_t rrgidx)
{
    RailroadGroups::railroad_groups.erase(RailroadGroups::railroad_groups.begin() + rrgidx);
    RailroadGroups::renumber();
}

/*------------------------------------------------------------------------------------------------------------------------
 *  RailroadGroups::set_new_id () - set new group id
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
RailroadGroups::set_new_id (uint_fast8_t rrgidx, uint_fast8_t new_rrgidx)
{
    uint_fast8_t   rtc                  = 0xFF;

    if (rrgidx < RailroadGroups::n_railroad_groups && new_rrgidx < RailroadGroups::n_railroad_groups)
    {
        if (rrgidx != new_rrgidx)
        {
            RailroadGroup   tmprailroad_group;
            uint_fast8_t    gidx;
            uint8_t *       map_new_rrgidx = (uint8_t *) calloc (RailroadGroups::n_railroad_groups, sizeof (uint8_t));

            for (gidx = 0; gidx < RailroadGroups::n_railroad_groups; gidx++)
            {
                map_new_rrgidx[gidx] = gidx;
            }

            tmprailroad_group = RailroadGroups::railroad_groups[rrgidx];

            if (new_rrgidx < rrgidx)
            {
                for (gidx = rrgidx; gidx > new_rrgidx; gidx--)
                {
                    RailroadGroups::railroad_groups[gidx] = RailroadGroups::railroad_groups[gidx - 1];
                    map_new_rrgidx[gidx - 1] = gidx;
                }
            }
            else // if (new_rrgidx > rrgidx)
            {
                for (gidx = rrgidx; gidx < new_rrgidx; gidx++)
                {
                    RailroadGroups::railroad_groups[gidx] = RailroadGroups::railroad_groups[gidx + 1];
                    map_new_rrgidx[gidx + 1] = gidx;
                }
            }

            RailroadGroups::railroad_groups[gidx] = tmprailroad_group;
            map_new_rrgidx[rrgidx] = gidx;

            for (gidx = 0; gidx < RailroadGroups::n_railroad_groups; gidx++)
            {
                Debug::printf (DEBUG_LEVEL_VERBOSE, "%2d -> %2d\n", gidx, map_new_rrgidx[gidx]);
            }

            S88::set_new_railroad_group_ids (map_new_rrgidx, n_railroad_groups);
            RCL::set_new_railroad_group_ids (map_new_rrgidx, n_railroad_groups);

            free (map_new_rrgidx);
            RailroadGroups::data_changed = true;
            rtc = new_rrgidx;
        }
        else
        {
            rtc = rrgidx;
        }
    }

    return rtc;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  get_n_railroad_groups () - get number of railroads
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
RailroadGroups::get_n_railroad_groups (void)
{
    return RailroadGroups::n_railroad_groups;
}


/*------------------------------------------------------------------------------------------------------------------------
 *  RailroadGroups::set_new_switch_ids () - set new switch ids
 *------------------------------------------------------------------------------------------------------------------------
 */
void
RailroadGroups::set_new_switch_ids (uint16_t * map_new_switch_idx, uint_fast16_t n_switches)
{
    uint_fast8_t    n_railroad_groups   = RailroadGroups::n_railroad_groups;
    uint_fast8_t    rrgidx;
    uint_fast8_t    rridx;

    for (rrgidx = 0; rrgidx < n_railroad_groups; rrgidx++)
    {
        RailroadGroup * rrg         = &RailroadGroups::railroad_groups[rrgidx];
        uint_fast8_t    n_railroads = rrg->get_n_railroads();

        for (rridx = 0; rridx < n_railroads; rridx++)
        {
            Railroad *      rr              = &(rrg->railroads[rridx]);
            uint_fast16_t   n_sub_switches  = rr->get_n_switches();
            uint_fast16_t   subidx;

            for (subidx = 0; subidx < n_sub_switches; subidx++)
            {
                uint_fast16_t   switch_idx = rr->get_switch_idx(subidx);

                if (switch_idx < n_switches)
                {
                    if (switch_idx != map_new_switch_idx[switch_idx])
                    {
                        rr->set_switch_idx(subidx, map_new_switch_idx[switch_idx]);
                        RailroadGroups::data_changed = true;
                    }
                }
            }
        }
    }
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * RailroadGroups::booster_on () - enable booster
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
RailroadGroups::booster_on (void)
{
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * RailroadGroups::booster_off () - enable booster
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
RailroadGroups::booster_off (void)
{
    uint_fast8_t rrgidx;

    for (rrgidx = 0; rrgidx < RailroadGroups::n_railroad_groups; rrgidx++)
    {
        RailroadGroups::railroad_groups[rrgidx].set_active_railroad (0xFF);
    }
}
