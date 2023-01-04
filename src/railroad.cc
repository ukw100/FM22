/*------------------------------------------------------------------------------------------------------------------------
 * railroad.c - railroad management functions
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
#include <stdlib.h>
#include <string.h>
#include "switch.h"
#include "loco.h"
#include "s88.h"
#include "rcl.h"
#include "railroad.h"
#include "debug.h"

bool                            Railroad::data_changed = false;
RAILROAD_GROUP                  Railroad::railroad_groups[MAX_RAILROAD_GROUPS];
uint_fast8_t                    Railroad::n_railroad_groups = 0;

/*------------------------------------------------------------------------------------------------------------------------
 *  group_add () - add a railroad
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
Railroad::group_add (void)
{
    uint_fast8_t rrgidx = Railroad::n_railroad_groups;

    if (rrgidx < MAX_RAILROAD_GROUPS - 1)
    {
        Railroad::railroad_groups[rrgidx].active_railroad_idx = 0xFF;
        Railroad::n_railroad_groups++;
        Railroad::data_changed = true;
    }
    else
    {
        rrgidx = 0xFF;
    }

    return rrgidx;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  group_setid () - set new group id
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
Railroad::group_setid (uint_fast8_t rrgidx, uint_fast8_t new_rrgidx)
{
    uint_fast8_t   rtc                  = 0xFF;

    if (rrgidx < Railroad::n_railroad_groups && new_rrgidx < Railroad::n_railroad_groups)
    {
        if (rrgidx != new_rrgidx)
        {
            RAILROAD_GROUP  tmprailroad_group;
            uint_fast8_t    gidx;
            uint8_t *       map_new_rrgidx = (uint8_t *) calloc (Railroad::n_railroad_groups, sizeof (uint8_t));

            for (gidx = 0; gidx < Railroad::n_railroad_groups; gidx++)
            {
                map_new_rrgidx[gidx] = gidx;
            }

            memcpy (&tmprailroad_group, Railroad::railroad_groups + rrgidx, sizeof (RAILROAD_GROUP));

            if (new_rrgidx < rrgidx)
            {
                for (gidx = rrgidx; gidx > new_rrgidx; gidx--)
                {
                    memcpy (Railroad::railroad_groups + gidx, Railroad::railroad_groups + gidx - 1, sizeof (RAILROAD_GROUP));
                    map_new_rrgidx[gidx - 1] = gidx;
                }
            }
            else // if (new_rrgidx > rrgidx)
            {
                for (gidx = rrgidx; gidx < new_rrgidx; gidx++)
                {
                    memcpy (Railroad::railroad_groups + gidx, Railroad::railroad_groups + gidx + 1, sizeof (RAILROAD_GROUP));
                    map_new_rrgidx[gidx + 1] = gidx;
                }
            }

            memcpy (Railroad::railroad_groups + gidx, &tmprailroad_group, sizeof (RAILROAD_GROUP));
            map_new_rrgidx[rrgidx] = gidx;

            for (gidx = 0; gidx < Railroad::n_railroad_groups; gidx++)
            {
                Debug::printf (DEBUG_LEVEL_NORMAL, "%2d -> %2d\n", gidx, map_new_rrgidx[gidx]);
            }

            S88::set_new_railroad_group_ids (map_new_rrgidx, n_railroad_groups);
            RCL::set_new_railroad_group_ids (map_new_rrgidx, n_railroad_groups);

            free (map_new_rrgidx);
            Railroad::data_changed = true;
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
Railroad::get_n_railroad_groups (void)
{
    return Railroad::n_railroad_groups;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  group_setname () - set name
 *------------------------------------------------------------------------------------------------------------------------
 */
void
Railroad::group_setname (uint_fast8_t rrgidx, const char * name)
{
    int l = strlen (name);

    if (! Railroad::railroad_groups[rrgidx].name || strcmp (Railroad::railroad_groups[rrgidx].name, name) != 0)
    {   
        if (Railroad::railroad_groups[rrgidx].name)
        {
            if ((int) strlen (Railroad::railroad_groups[rrgidx].name) < l)
            {
                free (Railroad::railroad_groups[rrgidx].name);
                Railroad::railroad_groups[rrgidx].name = (char *) NULL;
            }
        }

        if (! Railroad::railroad_groups[rrgidx].name)
        {
            Railroad::railroad_groups[rrgidx].name = (char *) malloc (l + 1);
        }

        strcpy (Railroad::railroad_groups[rrgidx].name, name);
        Railroad::data_changed = true;
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 *  Railroad::group_getname () - get name
 *------------------------------------------------------------------------------------------------------------------------
 */
char *
Railroad::group_getname (uint_fast8_t rrgidx)
{
    return Railroad::railroad_groups[rrgidx].name;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  Railroad::add () - add a railroad
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
Railroad::add (uint_fast8_t rrgidx)
{
    uint_fast8_t rridx;

    if (rrgidx < MAX_RAILROAD_GROUPS)
    {
        rridx = Railroad::railroad_groups[rrgidx].n_railroads;

        if (rridx < MAX_RAILROADS_PER_RAILROAD_GROUP - 1)
        {
            Railroad::railroad_groups[rrgidx].railroads[rridx].linked_loco_idx = 0xFFFF;
            Railroad::railroad_groups[rrgidx].railroads[rridx].active_loco_idx = 0xFFFF;
            Railroad::railroad_groups[rrgidx].railroads[rridx].located_loco_idx = 0xFFFF;
            Railroad::railroad_groups[rrgidx].n_railroads++;
        }
        else
        {
            rridx = 0xFF;
        }

        Railroad::data_changed = true;
    }
    else
    {
        rridx = 0xFF;
    }

    return rridx;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  setid () - set new id
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
Railroad::setid (uint_fast8_t rrgidx, uint_fast8_t rridx, uint_fast8_t new_rridx)
{
    uint_fast8_t    n_railroads = Railroad::railroad_groups[rrgidx].n_railroads;
    uint_fast8_t    rtc = 0xFF;

    if (rridx < n_railroads && new_rridx < n_railroads)
    {
        if (rridx != new_rridx)
        {
            RAILROAD        tmprailroad;
            uint_fast8_t    ridx;

            uint8_t *      map_new_rridx = (uint8_t *) calloc (n_railroads, sizeof (uint8_t));

            for (ridx = 0; ridx < n_railroads; ridx++)
            {
                map_new_rridx[ridx] = ridx;
            }

            memcpy (&tmprailroad, Railroad::railroad_groups[rrgidx].railroads + rridx, sizeof (RAILROAD));

            if (new_rridx < rridx)
            {
                for (ridx = rridx; ridx > new_rridx; ridx--)
                {
                    memcpy (Railroad::railroad_groups[rrgidx].railroads + ridx, Railroad::railroad_groups[rrgidx].railroads + ridx - 1, sizeof (RAILROAD));
                    map_new_rridx[ridx - 1] = ridx;
                }
            }
            else // if (new_rridx > rridx)
            {
                for (ridx = rridx; ridx < new_rridx; ridx++)
                {
                    memcpy (Railroad::railroad_groups[rrgidx].railroads + ridx, Railroad::railroad_groups[rrgidx].railroads + ridx + 1, sizeof (RAILROAD));
                    map_new_rridx[ridx + 1] = ridx;
                }
            }

            memcpy (Railroad::railroad_groups[rrgidx].railroads + ridx, &tmprailroad, sizeof (RAILROAD));
            map_new_rridx[rridx] = ridx;

            for (ridx = 0; ridx < n_railroads; ridx++)
            {
                Debug::printf (DEBUG_LEVEL_NORMAL, "%2d -> %2d\n", ridx, map_new_rridx[ridx]);
            }

            S88::set_new_railroad_ids (rrgidx, map_new_rridx, n_railroads);
            RCL::set_new_railroad_ids (rrgidx, map_new_rridx, n_railroads);

            free (map_new_rridx);
            Railroad::data_changed = true;
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
 *  Railroad::get_n_railroads () - get number of railroads
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
Railroad::get_n_railroads (uint_fast8_t rrgidx)
{
    return Railroad::railroad_groups[rrgidx].n_railroads;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  Railroad::set_name () - set name
 *------------------------------------------------------------------------------------------------------------------------
 */
void
Railroad::set_name (uint_fast8_t rrgidx, uint_fast8_t rridx, const char * name)
{
    int l = strlen (name);

    if (! Railroad::railroad_groups[rrgidx].railroads[rridx].name || strcmp (Railroad::railroad_groups[rrgidx].railroads[rridx].name, name) != 0)
    {
        if (Railroad::railroad_groups[rrgidx].railroads[rridx].name)
        {
            if ((int) strlen (Railroad::railroad_groups[rrgidx].railroads[rridx].name) < l)
            {
                free (Railroad::railroad_groups[rrgidx].railroads[rridx].name);
                Railroad::railroad_groups[rrgidx].railroads[rridx].name = (char *) NULL;
            }
        }

        if (! Railroad::railroad_groups[rrgidx].railroads[rridx].name)
        {
            Railroad::railroad_groups[rrgidx].railroads[rridx].name = (char *) malloc (l + 1);
        }

        strcpy (Railroad::railroad_groups[rrgidx].railroads[rridx].name, name);
        Railroad::data_changed = true;
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 *  Railroad::get_name () - get name
 *------------------------------------------------------------------------------------------------------------------------
 */
char *
Railroad::get_name (uint_fast8_t rrgidx, uint_fast8_t rridx)
{
    return Railroad::railroad_groups[rrgidx].railroads[rridx].name;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  Railroad::set_link_loco () - link railroad with loco
 *------------------------------------------------------------------------------------------------------------------------
 */
void
Railroad::set_link_loco (uint_fast8_t rrgidx, uint_fast8_t rridx, uint_fast16_t loco_idx)
{
    if (rrgidx < Railroad::n_railroad_groups && rridx < Railroad::railroad_groups[rrgidx].n_railroads)
    {   // don't check loco_idx, 0xFFFF is allowed to reset!
        if (Railroad::railroad_groups[rrgidx].railroads[rridx].linked_loco_idx != loco_idx)
        {
            Railroad::railroad_groups[rrgidx].railroads[rridx].linked_loco_idx = loco_idx;
            Railroad::data_changed = true;
        }
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 *  Railroad::get_link_loco () - get linked loco
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast16_t
Railroad::get_link_loco (uint_fast8_t rrgidx, uint_fast8_t rridx)
{
    uint_fast16_t rtc = 0xFFFF;

    if (rrgidx < Railroad::n_railroad_groups && rridx < Railroad::railroad_groups[rrgidx].n_railroads)
    {
        rtc = Railroad::railroad_groups[rrgidx].railroads[rridx].linked_loco_idx;
    }

    return rtc;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  Railroad::get_link_railroad () - get linked railroad
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
Railroad::get_link_railroad (uint_fast8_t rrgidx, uint_fast16_t loco_idx)
{
    uint_fast8_t rridx;
    uint_fast8_t rtc = 0xFF;

    if (rrgidx < Railroad::n_railroad_groups)
    {
        for (rridx = 0; rridx < Railroad::railroad_groups[rrgidx].n_railroads; rridx++)
        {
            if (Railroad::railroad_groups[rrgidx].railroads[rridx].linked_loco_idx == loco_idx)
            {
                rtc = rridx;
                break;
            }
        }
    }

    return rtc;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  Railroad::get_active_loco () - get active loco
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast16_t
Railroad::get_active_loco (uint_fast8_t rrgidx, uint_fast8_t rridx)
{
    uint_fast16_t rtc = 0xFFFF;

    if (rrgidx < Railroad::n_railroad_groups && rridx < Railroad::railroad_groups[rrgidx].n_railroads)
    {
        rtc = Railroad::railroad_groups[rrgidx].railroads[rridx].active_loco_idx;
    }

    return rtc;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  Railroad::set_active_loco () - set active loco
 *------------------------------------------------------------------------------------------------------------------------
 */
void
Railroad::set_active_loco (uint_fast8_t rrgidx, uint_fast8_t rridx, uint_fast16_t loco_idx)
{
    Debug::printf (DEBUG_LEVEL_VERBOSE, "Railroad::set_active_loco: rrgidx=%u rridx=%u loco_idx=%u\n", rrgidx, rridx, loco_idx);

    if (rrgidx < Railroad::n_railroad_groups && rridx < Railroad::railroad_groups[rrgidx].n_railroads)
    {
        Railroad::railroad_groups[rrgidx].railroads[rridx].active_loco_idx = loco_idx;
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 *  Railroad::get_located_loco () - get located loco
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast16_t
Railroad::get_located_loco (uint_fast8_t rrgidx, uint_fast8_t rridx)
{
    uint_fast16_t rtc = 0xFFFF;

    if (rrgidx < Railroad::n_railroad_groups && rridx < Railroad::railroad_groups[rrgidx].n_railroads)
    {
        rtc = Railroad::railroad_groups[rrgidx].railroads[rridx].located_loco_idx;
    }

    return rtc;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  Railroad::set_located_loco () - set located loco
 *------------------------------------------------------------------------------------------------------------------------
 */
void
Railroad::set_located_loco (uint_fast8_t rrgidx, uint_fast8_t rridx, uint_fast16_t loco_idx)
{
    Debug::printf (DEBUG_LEVEL_VERBOSE, "Railroad::set_located_loco: rrgidx=%u rridx=%u loco_idx=%u\n", rrgidx, rridx, loco_idx);

    if (rrgidx < Railroad::n_railroad_groups && rridx < Railroad::railroad_groups[rrgidx].n_railroads)
    {
        Railroad::railroad_groups[rrgidx].railroads[rridx].located_loco_idx = loco_idx;
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 *  Railroad::add_switch () - add a switch to railroad
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
Railroad::add_switch (uint_fast8_t rrgidx, uint_fast8_t rridx)
{
    uint_fast8_t subidx;

    if (rrgidx < MAX_RAILROAD_GROUPS)
    {
        subidx = Railroad::railroad_groups[rrgidx].railroads[rridx].n_switches;

        if (subidx < MAX_SWITCHES_PER_RAILROAD - 1)
        {
            Railroad::railroad_groups[rrgidx].railroads[rridx].n_switches++;
        }
        else
        {
            subidx = 0xFF;
        }

        Railroad::data_changed = true;
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
Railroad::get_n_switches (uint_fast8_t rrgidx, uint_fast8_t rridx)
{
    return Railroad::railroad_groups[rrgidx].railroads[rridx].n_switches;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  Railroad::set_switch_idx () - set switch index
 *------------------------------------------------------------------------------------------------------------------------
 */
void
Railroad::set_switch_idx (uint_fast8_t rrgidx, uint_fast8_t rridx, uint_fast8_t subidx, uint_fast16_t swidx)
{
    if (Railroad::railroad_groups[rrgidx].railroads[rridx].switches[subidx] != swidx)
    {
        Railroad::railroad_groups[rrgidx].railroads[rridx].switches[subidx] = swidx;
        Railroad::data_changed = true;
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 *  Railroad::get_switch_idx () - get switch index
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast16_t
Railroad::get_switch_idx (uint_fast8_t rrgidx, uint_fast8_t rridx, uint_fast8_t subidx)
{
    return Railroad::railroad_groups[rrgidx].railroads[rridx].switches[subidx];
}

/*------------------------------------------------------------------------------------------------------------------------
 *  Railroad::set_switch_state () - set switch state
 *------------------------------------------------------------------------------------------------------------------------
 */
void
Railroad::set_switch_state (uint_fast8_t rrgidx, uint_fast8_t rridx, uint_fast8_t subidx, uint_fast8_t state)
{
    Railroad::railroad_groups[rrgidx].railroads[rridx].states[subidx] = state;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  Railroad::get_switch_state () - get switch state
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
Railroad::get_switch_state (uint_fast8_t rrgidx, uint_fast8_t rridx, uint_fast8_t subidx)
{
    return Railroad::railroad_groups[rrgidx].railroads[rridx].states[subidx];
}

/*------------------------------------------------------------------------------------------------------------------------
 *  Railroad::set_new_switch_ids () - set new switch ids
 *------------------------------------------------------------------------------------------------------------------------
 */
void
Railroad::set_new_switch_ids (uint16_t * map_new_switch_idx, uint_fast16_t n_switches)
{
    uint_fast8_t    n_railroad_groups   = Railroad::n_railroad_groups;
    uint_fast8_t    n_railroads;
    uint_fast8_t    rrgidx;
    uint_fast8_t    rridx;

    for (rrgidx = 0; rrgidx < n_railroad_groups; rrgidx++)
    {
        n_railroads = Railroad::railroad_groups[rrgidx].n_railroads;

        for (rridx = 0; rridx < n_railroads; rridx++)
        {
            RAILROAD *      rrp        = &(Railroad::railroad_groups[rrgidx].railroads[rridx]);
            uint_fast16_t   swidx;

            for (swidx = 0; swidx < rrp->n_switches; swidx++)
            {
                if (rrp->switches[swidx] < n_switches)
                {
                    if (rrp->switches[swidx] != map_new_switch_idx[rrp->switches[swidx]])
                    {
                        rrp->switches[swidx] = map_new_switch_idx[rrp->switches[swidx]];
                        Railroad::data_changed = true;
                    }
                }
            }
        }
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 *  Railroad::del_switch () - remove switch from railroad
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
Railroad::del_switch (uint_fast8_t rrgidx, uint_fast8_t rridx, uint_fast8_t subidx)
{
    uint_fast8_t    n_rr_switches   = Railroad::railroad_groups[rrgidx].railroads[rridx].n_switches;
    uint16_t *      rrsp            = Railroad::railroad_groups[rrgidx].railroads[rridx].switches;
    uint_fast8_t    idx;
    uint_fast8_t    rtc = 0;

    if (subidx < n_rr_switches)
    {
        n_rr_switches--;

        for (idx = subidx; idx < n_rr_switches; idx++)
        {
            memcpy (rrsp + idx, rrsp + (idx + 1), sizeof (uint16_t));
        }

        Railroad::railroad_groups[rrgidx].railroads[rridx].n_switches = n_rr_switches;
        Railroad::data_changed = true;
        rtc = 1;
    }

    return rtc;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  Railroad::del () - remove railroad
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
Railroad::del (uint_fast8_t rrgidx, uint_fast8_t rridx)
{
    RAILROAD *      rrp             = Railroad::railroad_groups[rrgidx].railroads;
    uint_fast8_t    n_rr            = Railroad::railroad_groups[rrgidx].n_railroads;
    char *          savename;
    uint_fast8_t    idx;
    uint_fast8_t    rtc = 0;

    Railroad::railroad_groups[rrgidx].railroads[rridx].n_switches = 0;

    if (rridx < n_rr)
    {
        n_rr--;

        savename = Railroad::railroad_groups[rrgidx].railroads[rridx].name;

        for (idx = rridx; idx < n_rr; idx++)
        {
            memcpy (rrp + idx, rrp + (idx + 1), sizeof (RAILROAD));
        }

        if (rridx < n_rr)                      // it's not the last entry
        {
            Railroad::railroad_groups[rrgidx].railroads[n_rr].name = savename;
        }

        Railroad::railroad_groups[rrgidx].n_railroads = n_rr;
        Railroad::data_changed = true;
        rtc = 1;
    }

    return rtc;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  Railroad::group_del () - remove railroad group
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
Railroad::group_del (uint_fast8_t rrgidx)
{
    RAILROAD_GROUP *    rrgp        = Railroad::railroad_groups;
    uint_fast8_t        n_rrg       = Railroad::n_railroad_groups;
    char *              savename;
    uint_fast8_t        rridx;
    uint_fast8_t        idx;
    uint_fast8_t        rtc         = 0;

    if (rrgidx < n_rrg)
    {
        for (rridx = 0; rridx < Railroad::railroad_groups[rrgidx].n_railroads; rridx++)
        {
            Railroad::railroad_groups[rrgidx].railroads[rridx].n_switches = 0;
        }

        Railroad::railroad_groups[rrgidx].n_railroads = 0;

        n_rrg--;

        savename = Railroad::railroad_groups[rrgidx].name;

        for (idx = rrgidx; idx < n_rrg; idx++)
        {
            memcpy (rrgp + idx, rrgp + (idx + 1), sizeof (RAILROAD_GROUP));
        }

        if (rrgidx < n_rrg)                     // it's not the last entry
        {
            Railroad::railroad_groups[n_rrg].name = savename;
        }

        Railroad::n_railroad_groups = n_rrg;
        Railroad::data_changed = true;
        rtc = 1;
    }

    return rtc;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  Railroad::set () - set railroad
 *------------------------------------------------------------------------------------------------------------------------
 */
void
Railroad::set (uint_fast8_t rrgidx, uint_fast8_t rridx, uint_fast16_t loco_idx)
{
    uint_fast8_t subidx;

    if (rrgidx < Railroad::n_railroad_groups && rridx < Railroad::railroad_groups[rrgidx].n_railroads)
    {
        if (rridx < Railroad::railroad_groups[rrgidx].n_railroads)
        {
            for (subidx = 0; subidx < Railroad::railroad_groups[rrgidx].railroads[rridx].n_switches; subidx++)
            {
                Debug::printf (DEBUG_LEVEL_VERBOSE, "Railroad::set: Switch::set_state(%d, %d)\r\n",
                                Railroad::railroad_groups[rrgidx].railroads[rridx].switches[subidx], Railroad::railroad_groups[rrgidx].railroads[rridx].states[subidx]);

                Switch::set_state (Railroad::railroad_groups[rrgidx].railroads[rridx].switches[subidx], Railroad::railroad_groups[rrgidx].railroads[rridx].states[subidx]);
            }
        }

        Railroad::railroad_groups[rrgidx].active_railroad_idx = rridx;        // may be 0xFF (undefined)
        Railroad::set_active_loco (rrgidx, rridx, loco_idx);
    }
}


/*------------------------------------------------------------------------------------------------------------------------
 *  Railroad::set () - set railroad
 *------------------------------------------------------------------------------------------------------------------------
 */
void
Railroad::set (uint_fast8_t rrgidx, uint_fast8_t rridx)
{
    Railroad::set (rrgidx, rridx, 0xFFFF);
}

/*------------------------------------------------------------------------------------------------------------------------
 *  Railroad::get () - get active railroad
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
Railroad::get (uint_fast8_t rrgidx)
{
    uint_fast8_t rridx = 0xFF;

    if (rrgidx < Railroad::n_railroad_groups)
    {
        rridx = Railroad::railroad_groups[rrgidx].active_railroad_idx;        // may be 0xFF (undefined)
    }

    return rridx;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * Railroad::set_new_loco_ids () - set new loco ids
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
Railroad::set_new_loco_ids (uint16_t * map_new_loco_idx, uint_fast16_t n_locos)
{
    uint_fast8_t    n_railroad_groups   = Railroad::n_railroad_groups;
    uint_fast8_t    n_railroads;
    uint_fast8_t    rrgidx;
    uint_fast8_t    rridx;

    for (rrgidx = 0; rrgidx < n_railroad_groups; rrgidx++)
    {
        n_railroads = Railroad::railroad_groups[rrgidx].n_railroads;

        for (rridx = 0; rridx < n_railroads; rridx++)
        {
            RAILROAD * rrp        = &(Railroad::railroad_groups[rrgidx].railroads[rridx]);

            if (rrp->linked_loco_idx < n_locos &&
                rrp->linked_loco_idx != map_new_loco_idx[rrp->linked_loco_idx])
            {
                rrp->linked_loco_idx = map_new_loco_idx[rrp->linked_loco_idx];
                Railroad::data_changed = true;
            }

            if (rrp->active_loco_idx < n_locos)
            {
                rrp->active_loco_idx = map_new_loco_idx[rrp->active_loco_idx];
            }

            if (rrp->located_loco_idx < n_locos)
            {
                rrp->located_loco_idx = map_new_loco_idx[rrp->located_loco_idx];
            }
        }
    }
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * Railroad::booster_on () - enable booster
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
Railroad::booster_on (void)
{
    Debug::printf (DEBUG_LEVEL_VERBOSE, "Railroad::booster_on: Railroad::n_railroad_groups=%d\r\n", Railroad::n_railroad_groups);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * Railroad::booster_off () - enable booster
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
Railroad::booster_off (void)
{
    uint_fast8_t rrgidx;

    for (rrgidx = 0; rrgidx < Railroad::n_railroad_groups; rrgidx++)
    {
        Railroad::railroad_groups[rrgidx].active_railroad_idx = 0xFF;
    }
}
