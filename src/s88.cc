/*------------------------------------------------------------------------------------------------------------------------
 * s88.cc - s88 management functions
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
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "millis.h"
#include "event.h"
#include "dcc.h"
#include "switch.h"
#include "loco.h"
#include "addon.h"
#include "railroad.h"
#include "rcl.h"
#include "debug.h"
#include "s88.h"

#define S88_MAX_CONTACT_BYTES   (S88_MAX_CONTACTS / sizeof (uint8_t))

bool                            S88::data_changed = false;
uint8_t                         S88::new_bits[S88_MAX_CONTACT_BYTES];
uint8_t                         S88::current_bits[S88_MAX_CONTACT_BYTES];

/*----
 * if LOCO_IDX = 0xFFFF, get RAILROAD of CONTACT, get HOME_LOCO_IDX of RAILROAD
 * --> we need home_loco_idx in RR-struct
 *----
 */

CONTACT                         S88::contacts[S88_MAX_CONTACTS];
uint_fast16_t                   S88::n_contacts = 0;                        // number of contacts
bool                            S88::n_contacts_changed = true;             // flag: number of contacts changed

/*------------------------------------------------------------------------------------------------------------------------
 *  S88::get_link_loco () get link to loco by linked railroad
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast16_t
S88::get_link_loco (bool in, uint_fast8_t coidx, uint_fast8_t caidx, uint_fast8_t paidx)
{
    uint_fast16_t   loco_idx;

    if (in)
    {
        loco_idx    = contacts[coidx].contact_actions_in[caidx].parameters[paidx];
    }
    else
    {
        loco_idx    = contacts[coidx].contact_actions_out[caidx].parameters[paidx];
    }

    if (loco_idx == 0xFFFF)
    {
        uint_fast8_t    rrgidx  = contacts[coidx].rrgidx;
        uint_fast8_t    rridx   = contacts[coidx].rridx;

        loco_idx = Railroad::get_link_loco (rrgidx, rridx);
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
 *  S88::get_free_railroad () - get free railroad
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast16_t
S88::get_free_railroad (uint_fast8_t rrgidx)
{
    uint_fast16_t   coidx;
    uint_fast8_t    rridx = 0xFF;

    for (coidx = 0; coidx < S88::n_contacts; coidx++)
    {
        if (contacts[coidx].rrgidx == rrgidx && S88::get_state_bit (coidx) == S88_STATE_FREE)
        {
            rridx   = contacts[coidx].rridx;
            break;
        }
    }

    return rridx;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  S88::execute_contact_actions () - execute contact actions
 *------------------------------------------------------------------------------------------------------------------------
 */
void
S88::execute_contact_actions (bool in, uint_fast16_t coidx)
{
    CONTACT_ACTION  *   cap;
    uint_fast8_t        n_contact_actions;
    uint_fast8_t        caidx;

    if (in)
    {
        n_contact_actions = contacts[coidx].n_contact_actions_in;
        cap = contacts[coidx].contact_actions_in;
    }
    else
    {
        n_contact_actions = contacts[coidx].n_contact_actions_out;
        cap = contacts[coidx].contact_actions_out;
    }

    for (caidx = 0; caidx < n_contact_actions; caidx++)
    {
        switch (cap[caidx].action)
        {
            case S88_ACTION_SET_LOCO_SPEED:                                             // parameters: START, LOCO_IDX, SPEED, TENTHS
            {
                uint_fast32_t   start       = cap[caidx].parameters[0];
                uint_fast16_t   loco_idx    = S88::get_link_loco (in, coidx, caidx, 1);
                uint_fast8_t    speed       = cap[caidx].parameters[2];
                uint_fast16_t   tenths      = cap[caidx].parameters[3];

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

            case S88_ACTION_SET_LOCO_MIN_SPEED:                                         // parameters: START, LOCO_IDX, SPEED, TENTHS
            {
                uint_fast32_t   start       = cap[caidx].parameters[0];
                uint_fast16_t   loco_idx    = S88::get_link_loco (in, coidx, caidx, 1);
                uint_fast8_t    speed       = cap[caidx].parameters[2];
                uint_fast16_t   tenths      = cap[caidx].parameters[3];

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

            case S88_ACTION_SET_LOCO_MAX_SPEED:                                         // parameters: START, LOCO_IDX, SPEED, TENTHS
            {
                uint_fast32_t   start       = cap[caidx].parameters[0];
                uint_fast16_t   loco_idx    = S88::get_link_loco (in, coidx, caidx, 1);
                uint_fast8_t    speed       = cap[caidx].parameters[2];
                uint_fast16_t   tenths      = cap[caidx].parameters[3];

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

            case S88_ACTION_SET_LOCO_FORWARD_DIRECTION:                                 // parameters: START, LOCO_IDX, FWD
            {
                uint_fast32_t   start       = cap[caidx].parameters[0];
                uint_fast16_t   loco_idx    = S88::get_link_loco (in, coidx, caidx, 1);
                uint_fast8_t    fwd         = cap[caidx].parameters[2];

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

            case S88_ACTION_SET_LOCO_ALL_FUNCTIONS_OFF:                                     // parameters: START, LOCO_IDX
            {
                uint_fast32_t   start       = cap[caidx].parameters[0];
                uint_fast16_t   loco_idx    = S88::get_link_loco (in, coidx, caidx, 1);

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

            case S88_ACTION_SET_LOCO_FUNCTION_OFF:                                          // parameters: START, LOCO_IDX, FUNC_IDX
            {
                uint_fast32_t   start       = cap[caidx].parameters[0];
                uint_fast16_t   loco_idx    = S88::get_link_loco (in, coidx, caidx, 1);
                uint_fast8_t    f           = cap[caidx].parameters[2];

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

            case S88_ACTION_SET_LOCO_FUNCTION_ON:                                           // parameters: START, LOCO_IDX, FUNC_IDX
            {
                uint_fast32_t   start       = cap[caidx].parameters[0];
                uint_fast16_t   loco_idx    = S88::get_link_loco (in, coidx, caidx, 1);
                uint_fast8_t    f           = cap[caidx].parameters[2];

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

            case S88_ACTION_EXECUTE_LOCO_MACRO:                                             // parameters: START, LOCO_IDX, MACRO_IDX
            {
                uint_fast32_t   start       = cap[caidx].parameters[0];
                uint_fast16_t   loco_idx    = S88::get_link_loco (in, coidx, caidx, 1);
                uint_fast8_t    macro_idx   = cap[caidx].parameters[2];

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

            case S88_ACTION_SET_ADDON_ALL_FUNCTIONS_OFF:                                     // parameters: START, ADDON_IDX
            {
                uint_fast32_t   start       = cap[caidx].parameters[0];
                uint_fast16_t   addon_idx   = cap[caidx].parameters[1];

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

            case S88_ACTION_SET_ADDON_FUNCTION_OFF:                                          // parameters: START, ADDON_IDX, FUNC_IDX
            {
                uint_fast32_t   start       = cap[caidx].parameters[0];
                uint_fast16_t   addon_idx   = cap[caidx].parameters[1];
                uint_fast8_t    f           = cap[caidx].parameters[2];

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

            case S88_ACTION_SET_ADDON_FUNCTION_ON:                                          // parameters: START, ADDON_IDX, FUNC_IDX
            {
                uint_fast32_t   start       = cap[caidx].parameters[0];
                uint_fast16_t   addon_idx   = cap[caidx].parameters[1];
                uint_fast8_t    f           = cap[caidx].parameters[2];

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

            case S88_ACTION_SET_RAILROAD:                                               // parameters: RRG_IDX, RR_IDX
            {
                uint_fast8_t    rrgidx  = cap[caidx].parameters[0];
                uint_fast8_t    rridx   = cap[caidx].parameters[1];

                Railroad::set (rrgidx, rridx);
                break;
            }

            case S88_ACTION_SET_FREE_RAILROAD:                                          // parameters: RRG_IDX
            {
                uint_fast8_t    rrgidx  = cap[caidx].parameters[0];
                uint_fast8_t    rridx   = S88::get_free_railroad (rrgidx);

                if (rridx != 0xFF)
                {
                    Railroad::set (rrgidx, rridx);
                }
                else
                {
                    Debug::printf (DEBUG_LEVEL_VERBOSE, "S88::execute_contact_actions: cannot find free railroad\n");
                }
                break;
            }
        }
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 *  S88::add_contact () - add a contact
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast16_t
S88::add_contact (void)
{
    uint_fast16_t   coidx;

    if (n_contacts < S88_MAX_CONTACTS - 1)
    {
        coidx = n_contacts;
        contacts[coidx].rrgidx                  = 0xFF;
        contacts[coidx].rridx                   = 0xFF;
        contacts[coidx].n_contact_actions_in    = 0;
        contacts[coidx].n_contact_actions_out   = 0;
        n_contacts++;                                                       // 2nd increment number of contacts
        S88::n_contacts_changed = true;
        S88::data_changed = true;
    }
    else
    {
        coidx = 0xFFFF;
    }

    return coidx;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  setid () - set new id
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast16_t
S88::setid (uint_fast16_t coidx, uint_fast16_t new_coidx)
{
    uint_fast16_t   rtc = 0xFFFF;

    if (coidx < S88::n_contacts && new_coidx < S88::n_contacts)
    {
        if (coidx != new_coidx)
        {
            CONTACT         tmpcontact;
            uint_fast16_t   cidx;

            uint16_t *      map_new_coidx = (uint16_t *) calloc (S88::n_contacts, sizeof (uint16_t));

            for (cidx = 0; cidx < S88::n_contacts; cidx++)
            {
                map_new_coidx[cidx] = cidx;
            }

            memcpy (&tmpcontact, S88::contacts + coidx, sizeof (CONTACT));

            if (new_coidx < coidx)
            {
                for (cidx = coidx; cidx > new_coidx; cidx--)
                {
                    memcpy (S88::contacts + cidx, S88::contacts + cidx - 1, sizeof (CONTACT));
                    map_new_coidx[cidx - 1] = cidx;
                }
            }
            else // if (new_coidx > coidx)
            {
                for (cidx = coidx; cidx < new_coidx; cidx++)
                {
                    memcpy (S88::contacts + cidx, S88::contacts + cidx + 1, sizeof (CONTACT));
                    map_new_coidx[cidx + 1] = cidx;
                }
            }

            memcpy (S88::contacts + cidx, &tmpcontact, sizeof (CONTACT));
            map_new_coidx[coidx] = cidx;

            for (cidx = 0; cidx < S88::n_contacts; cidx++)
            {
                Debug::printf (DEBUG_LEVEL_NORMAL, "%2d -> %2d\n", cidx, map_new_coidx[cidx]);
            }

            free (map_new_coidx);
            S88::data_changed = true;
            rtc = new_coidx;
        }
        else
        {
            rtc = coidx;
        }
    }

    return rtc;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  S88::get_n_contacts () - get number of contacts
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast16_t
S88::get_n_contacts (void)
{
    return n_contacts;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  S88::get_n_contacts_changed () - get flag:  number of contacts changed
 *------------------------------------------------------------------------------------------------------------------------
 */
bool
S88::get_n_contacts_changed (void)
{
    bool    rtc = n_contacts_changed;
    n_contacts_changed = false;
    return rtc;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  S88::set_name (coidx, name) - set name of contact
 *------------------------------------------------------------------------------------------------------------------------
 */
void
S88::set_name (uint_fast16_t coidx, char * name)
{
    int l = strlen (name);

    if (contacts[coidx].name)
    {
        if ((int) strlen (contacts[coidx].name) < l)
        {
            free (contacts[coidx].name);
            contacts[coidx].name = (char *) NULL;
        }
    }

    if (! contacts[coidx].name)
    {
        contacts[coidx].name = (char *) malloc (l + 1);
    }

    strcpy (contacts[coidx].name, name);
    S88::data_changed = true;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  S88::get_name (coidx) - get name of contact
 *------------------------------------------------------------------------------------------------------------------------
 */
char  *
S88::get_name (uint_fast16_t coidx)
{
    if (coidx < n_contacts)
    {
        return contacts[coidx].name;
    }

    return (char *) NULL;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  S88::set_link_railroad (coidx, rrgidx, rridx) - link contact to railroad
 *------------------------------------------------------------------------------------------------------------------------
 */
void
S88::set_link_railroad (uint_fast16_t coidx, uint_fast8_t rrgidx, uint_fast8_t rridx)
{
    contacts[coidx].rrgidx  = rrgidx;
    contacts[coidx].rridx   = rridx;
    S88::data_changed = true;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  S88::get_link_railroad (coidx) - get railroad linked to contact
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast16_t
S88::get_link_railroad (uint_fast16_t coidx)
{
    uint_fast16_t rtc = (contacts[coidx].rrgidx << 8) | contacts[coidx].rridx;
    return rtc;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  S88::add_contact_action () - add a contact action
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
S88::add_contact_action (bool in, uint_fast16_t coidx)
{
    uint_fast8_t caidx;

    if (in)
    {
        if (contacts[coidx].n_contact_actions_in < S88_MAX_ACTIONS_PER_CONTACT - 1)
        {
            caidx = contacts[coidx].n_contact_actions_in;
            contacts[coidx].contact_actions_in[caidx].action        = S88_ACTION_NONE;
            contacts[coidx].contact_actions_in[caidx].n_parameters  = 0;
            contacts[coidx].n_contact_actions_in++;
            S88::data_changed = true;
        }
        else
        {
            caidx = 0xFF;
        }
    }
    else
    {
        if (contacts[coidx].n_contact_actions_out < S88_MAX_ACTIONS_PER_CONTACT - 1)
        {
            caidx = contacts[coidx].n_contact_actions_out;
            contacts[coidx].contact_actions_out[caidx].action       = S88_ACTION_NONE;
            contacts[coidx].contact_actions_out[caidx].n_parameters = 0;
            contacts[coidx].n_contact_actions_out++;
            S88::data_changed = true;
        }
        else
        {
            caidx = 0xFF;
        }
    }

    return caidx;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  S88::delete_contact_action () - delete a contact action
 *------------------------------------------------------------------------------------------------------------------------
 */
void
S88::delete_contact_action (bool in, uint_fast16_t coidx, uint_fast8_t caidx)
{
    uint_fast8_t    n_parameters;
    uint_fast8_t    pidx;

    if (in)
    {
        if (caidx < contacts[coidx].n_contact_actions_in)
        {
            while (caidx < contacts[coidx].n_contact_actions_in - 1)
            {
                n_parameters = contacts[coidx].contact_actions_in[caidx + 1].n_parameters;

                contacts[coidx].contact_actions_in[caidx].action        = contacts[coidx].contact_actions_in[caidx + 1].action;
                contacts[coidx].contact_actions_in[caidx].n_parameters  = n_parameters;

                for (pidx = 0; pidx < n_parameters; pidx++)
                {
                    contacts[coidx].contact_actions_in[caidx].parameters[pidx]  = contacts[coidx].contact_actions_in[caidx + 1].parameters[pidx];
                }

                caidx++;
            }

            contacts[coidx].n_contact_actions_in--;
            S88::data_changed = true;
        }
    }
    else
    {
        if (caidx < contacts[coidx].n_contact_actions_out)
        {
            while (caidx < contacts[coidx].n_contact_actions_out - 1)
            {
                n_parameters = contacts[coidx].contact_actions_out[caidx + 1].n_parameters;

                contacts[coidx].contact_actions_out[caidx].action       = contacts[coidx].contact_actions_out[caidx + 1].action;
                contacts[coidx].contact_actions_out[caidx].n_parameters = n_parameters;

                for (pidx = 0; pidx < n_parameters; pidx++)
                {
                    contacts[coidx].contact_actions_out[caidx].parameters[pidx] = contacts[coidx].contact_actions_out[caidx + 1].parameters[pidx];
                }

                caidx++;
            }

            contacts[coidx].n_contact_actions_out--;
            S88::data_changed = true;
        }
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 *  S88::get_n_contact_actions () - get number of contact actions
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
S88::get_n_contact_actions (bool in, uint_fast16_t coidx)
{
    uint_fast8_t    rtc;

    if (in)
    {
        rtc = contacts[coidx].n_contact_actions_in;
    }
    else
    {
        rtc = contacts[coidx].n_contact_actions_out;
    }

    return rtc;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  S88::set_contact_action () - set contact action
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
S88::set_contact_action (bool in, uint_fast16_t coidx, uint_fast8_t caidx, CONTACT_ACTION * cap)
{
    uint_fast8_t rtc = 0;

    if (in)
    {
        if (caidx < contacts[coidx].n_contact_actions_in)
        {
            uint_fast8_t paidx;

            contacts[coidx].contact_actions_in[caidx].action = cap->action;
            contacts[coidx].contact_actions_in[caidx].n_parameters = cap->n_parameters;

            for (paidx = 0; paidx < cap->n_parameters; paidx++)
            {
                contacts[coidx].contact_actions_in[caidx].parameters[paidx] = cap->parameters[paidx];
            }

            S88::data_changed = true;
            rtc = 1;
        }
    }
    else
    {
        if (caidx < contacts[coidx].n_contact_actions_out)
        {
            uint_fast8_t paidx;

            contacts[coidx].contact_actions_out[caidx].action = cap->action;
            contacts[coidx].contact_actions_out[caidx].n_parameters = cap->n_parameters;

            for (paidx = 0; paidx < cap->n_parameters; paidx++)
            {
                contacts[coidx].contact_actions_out[caidx].parameters[paidx] = cap->parameters[paidx];
            }

            S88::data_changed = true;
            rtc = 1;
        }
    }

    return rtc;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  S88::get_contact_action () - get contact action
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
S88::get_contact_action (bool in, uint_fast16_t coidx, uint_fast8_t caidx, CONTACT_ACTION * cap)
{
    uint_fast8_t rtc = 0;

    if (in)
    {
        if (caidx < contacts[coidx].n_contact_actions_in)
        {
            uint_fast8_t paidx;

            cap->action         = contacts[coidx].contact_actions_in[caidx].action;
            cap->n_parameters   = contacts[coidx].contact_actions_in[caidx].n_parameters;

            for (paidx = 0; paidx < cap->n_parameters; paidx++)
            {
                cap->parameters[paidx] = contacts[coidx].contact_actions_in[caidx].parameters[paidx];
            }
            rtc = 1;
        }
    }
    else
    {
        if (caidx < contacts[coidx].n_contact_actions_out)
        {
            uint_fast8_t paidx;

            cap->action         = contacts[coidx].contact_actions_out[caidx].action;
            cap->n_parameters   = contacts[coidx].contact_actions_out[caidx].n_parameters;

            for (paidx = 0; paidx < cap->n_parameters; paidx++)
            {
                cap->parameters[paidx] = contacts[coidx].contact_actions_out[caidx].parameters[paidx];
            }
            rtc = 1;
        }
    }

    return rtc;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  S88::number_of_status_bytes ()
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast16_t
S88::number_of_status_bytes (void)
{
    uint_fast16_t nbytes;

    nbytes = S88::n_contacts / 8;

    if (S88::n_contacts % 8)
    {
        nbytes++;
    }

    return nbytes;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  S88::booster_on ()
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
S88::booster_on (void)
{
    uint_fast16_t idx;
    uint_fast16_t nbytes = S88::number_of_status_bytes ();

    for (idx = 0; idx < nbytes; idx++)
    {
        S88::current_bits[idx] = 0x00;
        S88::n_contacts_changed = true;
    }

    return 0;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  S88::booster_off ()
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
S88::booster_off (void)
{
    uint_fast16_t idx;
    uint_fast16_t nbytes = S88::number_of_status_bytes ();

    for (idx = 0; idx < nbytes; idx++)
    {
        S88::current_bits[idx] = 0x00;
        S88::n_contacts_changed = true;
    }

    return 0;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  S88::schedule ()
 *------------------------------------------------------------------------------------------------------------------------
 */
void
S88::schedule (void)
{
    uint_fast16_t   coidx;

    if (DCC::booster_is_on)
    {
        coidx = 0;

        for (coidx = 0; coidx < n_contacts; coidx++)
        {
            if (! S88::get_state_bit (coidx))
            {
                if (S88::get_newstate_bit (coidx))                                  // contact gets occupied
                {
                    uint_fast16_t   rrgrridx;
                    uint_fast8_t    rrgidx;
                    uint_fast8_t    rridx;
                    uint_fast16_t   active_loco_idx;

                    S88::set_state_bit (coidx, S88_STATE_OCCUPIED);

                    rrgrridx        = S88::get_link_railroad (coidx);
                    rrgidx          = rrgrridx >> 8;
                    rridx           = rrgrridx & 0xFF;
                    active_loco_idx = Railroad::get_active_loco (rrgidx, rridx);

                    Railroad::set_located_loco (rrgidx, rridx, active_loco_idx);
                    Railroad::set_active_loco (rrgidx, rridx, 0xFFFF);
                    Loco::set_rrlocation (active_loco_idx, rrgrridx);

                    if (Millis::elapsed () - DCC::booster_is_on_time > 3000)
                    {
                        S88::execute_contact_actions (true, coidx);
                        Debug::printf (DEBUG_LEVEL_VERBOSE, "contact=%u gets occupied\r\n", (uint16_t) coidx);
                    }
                    else
                    {
                        Debug::printf (DEBUG_LEVEL_VERBOSE, "contact=%u gets occupied... ignored\r\n", (uint16_t) coidx);
                    }
                }
            }
            else if (! get_newstate_bit (coidx))                                    // contact gets free
            {
                uint_fast16_t   rrgrridx;
                uint_fast8_t    rrgidx;
                uint_fast8_t    rridx;
                uint_fast16_t   located_loco_idx;

                rrgrridx            = S88::get_link_railroad (coidx);
                rrgidx              = rrgrridx >> 8;
                rridx               = rrgrridx & 0xFF;
                located_loco_idx    = Railroad::get_located_loco (rrgidx, rridx);

                Railroad::set_located_loco (rrgidx, rridx, 0xFFFF);
                Loco::set_rrlocation (located_loco_idx, 0xFFFF);

                S88::set_state_bit (coidx, S88_STATE_FREE);
                S88::execute_contact_actions (false, coidx);
                Debug::printf (DEBUG_LEVEL_VERBOSE, "contact=%u gets free\n", (uint16_t) coidx);
            }
        }
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 *  S88::get_state_bit ()
 *------------------------------------------------------------------------------------------------------------------------
 */
bool
S88::get_state_bit (uint_fast16_t coidx)
{
    uint_fast8_t    byte_idx    = coidx / 8;
    uint_fast8_t    pin_idx     = coidx % 8;

    if (S88::current_bits[byte_idx] & (1 << pin_idx))
    {
        return S88_STATE_OCCUPIED;
    }
    return S88_STATE_FREE;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  S88::get_state_byte ()
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
S88::get_state_byte (uint_fast16_t byteidx)
{
    return S88::current_bits[byteidx];
}

/*------------------------------------------------------------------------------------------------------------------------
 *  S88::set_state_bit ()
 *------------------------------------------------------------------------------------------------------------------------
 */
void
S88::set_state_bit (uint_fast16_t coidx, bool value)
{
    uint_fast8_t    byte_idx    = coidx / 8;
    uint_fast8_t    pin_idx     = coidx % 8;

    if (value)
    {
        S88::current_bits[byte_idx] |= S88_STATE_OCCUPIED << pin_idx;
    }
    else
    {
        S88::current_bits[byte_idx] &= ~(S88_STATE_OCCUPIED << pin_idx);
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 *  S88::set_state_byte ()
 *------------------------------------------------------------------------------------------------------------------------
 */
void
S88::set_state_byte (uint_fast8_t byte_idx, uint_fast8_t value)
{
    S88::current_bits[byte_idx] = value;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  S88::get_newstate_bit ()
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
S88::get_newstate_bit (uint_fast16_t coidx)
{
    uint_fast8_t    byte_idx    = coidx / 8;
    uint_fast8_t    pin_idx     = coidx % 8;

    if (S88::new_bits[byte_idx] & (1 << pin_idx))
    {
        return S88_STATE_OCCUPIED;
    }
    return S88_STATE_FREE;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  S88::get_newstate_byte ()
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
S88::get_newstate_byte (uint_fast16_t byteidx)
{
    return S88::new_bits[byteidx];
}

/*------------------------------------------------------------------------------------------------------------------------
 *  S88::set_newstate_bit ()
 *------------------------------------------------------------------------------------------------------------------------
 */
void
S88::set_newstate_bit (uint_fast16_t coidx, bool value)
{
    uint_fast8_t    byte_idx    = coidx / 8;
    uint_fast8_t    pin_idx     = coidx % 8;

    if (value)
    {
        S88::new_bits[byte_idx] |= S88_STATE_OCCUPIED << pin_idx;
    }
    else
    {
        S88::new_bits[byte_idx] &= ~(S88_STATE_OCCUPIED << pin_idx);
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 *  S88::set_newstate_byte ()
 *------------------------------------------------------------------------------------------------------------------------
 */
void
S88::set_newstate_byte (uint_fast16_t byte_idx, uint_fast8_t value)
{
    S88::new_bits[byte_idx] = value;
}

void
S88::set_new_loco_ids_for_contact (bool in, uint_fast16_t coidx, uint16_t * map_new_loco_idx, uint16_t n_locos)
{
    if (coidx < n_contacts)
    {
        CONTACT_ACTION  *   cap;
        uint_fast8_t        n_contact_actions;
        uint_fast8_t        caidx;

        if (in)
        {
            n_contact_actions = contacts[coidx].n_contact_actions_in;
            cap = contacts[coidx].contact_actions_in;
        }
        else
        {
            n_contact_actions = contacts[coidx].n_contact_actions_out;
            cap = contacts[coidx].contact_actions_out;
        }

        for (caidx = 0; caidx < n_contact_actions; caidx++)
        {
            switch (cap[caidx].action)
            {
                case S88_ACTION_SET_LOCO_SPEED:                                             // parameters: START, LOCO_IDX, SPEED, TENTHS
                case S88_ACTION_SET_LOCO_MIN_SPEED:                                         // parameters: START, LOCO_IDX, SPEED, TENTHS
                case S88_ACTION_SET_LOCO_MAX_SPEED:                                         // parameters: START, LOCO_IDX, SPEED, TENTHS
                case S88_ACTION_SET_LOCO_FORWARD_DIRECTION:                                 // parameters: START, LOCO_IDX, FWD
                case S88_ACTION_SET_LOCO_ALL_FUNCTIONS_OFF:                                 // parameters: START, LOCO_IDX
                case S88_ACTION_SET_LOCO_FUNCTION_OFF:                                      // parameters: START, LOCO_IDX, FUNC_IDX
                case S88_ACTION_SET_LOCO_FUNCTION_ON:                                       // parameters: START, LOCO_IDX, FUNC_IDX
                case S88_ACTION_EXECUTE_LOCO_MACRO:                                         // parameters: START, LOCO_IDX, MACRO_IDX
                {
                    uint_fast16_t   loco_idx    = cap[caidx].parameters[1];

                    if (loco_idx < n_locos && loco_idx != map_new_loco_idx[loco_idx])
                    {
                        if (in)
                        {
                            contacts[coidx].contact_actions_in[caidx].parameters[1] = map_new_loco_idx[loco_idx];
                        }
                        else
                        {
                            contacts[coidx].contact_actions_out[caidx].parameters[1] = map_new_loco_idx[loco_idx];
                        }

                        S88::data_changed = true;
                    }

                    break;
                }
            }
        }
    }
}

void
S88::set_new_addon_ids_for_contact (bool in, uint_fast16_t coidx, uint16_t * map_new_addon_idx, uint16_t n_addons)
{
    if (coidx < n_contacts)
    {
        CONTACT_ACTION  *   cap;
        uint_fast8_t        n_contact_actions;
        uint_fast8_t        caidx;

        if (in)
        {
            n_contact_actions = contacts[coidx].n_contact_actions_in;
            cap = contacts[coidx].contact_actions_in;
        }
        else
        {
            n_contact_actions = contacts[coidx].n_contact_actions_out;
            cap = contacts[coidx].contact_actions_out;
        }

        for (caidx = 0; caidx < n_contact_actions; caidx++)
        {
            switch (cap[caidx].action)
            {
                case S88_ACTION_SET_ADDON_ALL_FUNCTIONS_OFF:                                 // parameters: START, ADDON_IDX
                case S88_ACTION_SET_ADDON_FUNCTION_OFF:                                      // parameters: START, ADDON_IDX, FUNC_IDX
                case S88_ACTION_SET_ADDON_FUNCTION_ON:                                       // parameters: START, ADDON_IDX, FUNC_IDX
                {
                    uint_fast16_t   addon_idx    = cap[caidx].parameters[1];

                    if (addon_idx < n_addons && addon_idx != map_new_addon_idx[addon_idx])
                    {
                        if (in)
                        {
                            contacts[coidx].contact_actions_in[caidx].parameters[1] = map_new_addon_idx[addon_idx];
                        }
                        else
                        {
                            contacts[coidx].contact_actions_out[caidx].parameters[1] = map_new_addon_idx[addon_idx];
                        }

                        S88::data_changed = true;
                    }

                    break;
                }
            }
        }
    }
}

void
S88::set_new_railroad_group_ids_for_contact (bool in, uint_fast16_t coidx, uint8_t * map_new_rrgidx, uint_fast8_t n_railroad_groups)
{
    if (coidx < n_contacts)
    {
        CONTACT_ACTION  *   cap;
        uint_fast8_t        n_contact_actions;
        uint_fast8_t        caidx;

        if (in)
        {
            n_contact_actions = contacts[coidx].n_contact_actions_in;
            cap = contacts[coidx].contact_actions_in;
        }
        else
        {
            n_contact_actions = contacts[coidx].n_contact_actions_out;
            cap = contacts[coidx].contact_actions_out;
        }

        for (caidx = 0; caidx < n_contact_actions; caidx++)
        {
            switch (cap[caidx].action)
            {
                case S88_ACTION_SET_RAILROAD:                                               // parameters: RRG_IDX, RR_IDX
                case S88_ACTION_SET_FREE_RAILROAD:                                          // parameters: RRG_IDX
                {
                    uint_fast16_t   rrgidx    = cap[caidx].parameters[0];

                    if (rrgidx < n_railroad_groups && rrgidx != map_new_rrgidx[rrgidx])
                    {
                        if (in)
                        {
                            contacts[coidx].contact_actions_in[caidx].parameters[0] = map_new_rrgidx[rrgidx];
                        }
                        else
                        {
                            contacts[coidx].contact_actions_out[caidx].parameters[0] = map_new_rrgidx[rrgidx];
                        }

                        S88::data_changed = true;
                    }

                    break;
                }
            }
        }
    }
}

void
S88::set_new_railroad_ids_for_contact (bool in, uint_fast16_t coidx, uint_fast8_t current_rrgidx, uint8_t * map_new_rridx, uint_fast8_t n_railroads)
{
    if (coidx < n_contacts)
    {
        CONTACT_ACTION  *   cap;
        uint_fast8_t        n_contact_actions;
        uint_fast8_t        caidx;

        if (in)
        {
            n_contact_actions = contacts[coidx].n_contact_actions_in;
            cap = contacts[coidx].contact_actions_in;
        }
        else
        {
            n_contact_actions = contacts[coidx].n_contact_actions_out;
            cap = contacts[coidx].contact_actions_out;
        }

        for (caidx = 0; caidx < n_contact_actions; caidx++)
        {
            switch (cap[caidx].action)
            {
                case S88_ACTION_SET_RAILROAD:                                               // parameters: RRG_IDX, RR_IDX
                {
                    uint_fast16_t   rrgidx   = cap[caidx].parameters[0];
                    uint_fast16_t   rridx    = cap[caidx].parameters[1];

                    if (current_rrgidx == rrgidx && rridx < n_railroads && rridx != map_new_rridx[rridx])
                    {
                        if (in)
                        {
                            contacts[coidx].contact_actions_in[caidx].parameters[1] = map_new_rridx[rridx];
                        }
                        else
                        {
                            contacts[coidx].contact_actions_out[caidx].parameters[1] = map_new_rridx[rridx];
                        }

                        S88::data_changed = true;
                    }

                    break;
                }
            }
        }
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 *  S88::set_new_loco_ids ()
 *------------------------------------------------------------------------------------------------------------------------
 */
void
S88::set_new_loco_ids (uint16_t * map_new_loco_idx, uint16_t n_locos)
{
    uint_fast16_t     coidx;

    for (coidx = 0; coidx < S88::n_contacts; coidx++)
    {
        S88::set_new_loco_ids_for_contact (true, coidx, map_new_loco_idx, n_locos);
        S88::set_new_loco_ids_for_contact (false, coidx, map_new_loco_idx, n_locos);
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 *  S88::set_new_addon_ids ()
 *------------------------------------------------------------------------------------------------------------------------
 */
void
S88::set_new_addon_ids (uint16_t * map_new_addon_idx, uint16_t n_addons)
{
    uint_fast16_t     coidx;

    for (coidx = 0; coidx < S88::n_contacts; coidx++)
    {
        S88::set_new_addon_ids_for_contact (true, coidx, map_new_addon_idx, n_addons);
        S88::set_new_addon_ids_for_contact (false, coidx, map_new_addon_idx, n_addons);
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 *  S88::set_new_railroad_group_ids ()
 *------------------------------------------------------------------------------------------------------------------------
 */
void
S88::set_new_railroad_group_ids (uint8_t * map_new_rrgidx, uint8_t n_railroad_groups)
{
    uint_fast16_t     coidx;

    for (coidx = 0; coidx < S88::n_contacts; coidx++)
    {
        if (contacts[coidx].rrgidx < n_railroad_groups)
        {
            if (contacts[coidx].rrgidx != map_new_rrgidx[contacts[coidx].rrgidx])
            {
                contacts[coidx].rrgidx = map_new_rrgidx[contacts[coidx].rrgidx];
                S88::data_changed = true;
            }
        }

        S88::set_new_railroad_group_ids_for_contact (true, coidx, map_new_rrgidx, n_railroad_groups);
        S88::set_new_railroad_group_ids_for_contact (false, coidx, map_new_rrgidx, n_railroad_groups);
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 *  S88::set_new_railroad_ids ()
 *------------------------------------------------------------------------------------------------------------------------
 */
void
S88::set_new_railroad_ids (uint_fast8_t rrgidx, uint8_t * map_new_rridx, uint_fast8_t n_railroads)
{
    uint_fast16_t     coidx;

    for (coidx = 0; coidx < S88::n_contacts; coidx++)
    {
        if (contacts[coidx].rrgidx == rrgidx)
        {
            if (contacts[coidx].rridx < n_railroads)
            {
                if (contacts[coidx].rridx != map_new_rridx[contacts[coidx].rridx])
                {
                    contacts[coidx].rridx = map_new_rridx[contacts[coidx].rridx];
                    S88::data_changed = true;
                }
            }
        }

        S88::set_new_railroad_ids_for_contact (true, coidx, rrgidx, map_new_rridx, n_railroads);
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 *  S88::init ()
 *------------------------------------------------------------------------------------------------------------------------
 */
void
S88::init (void)
{
}
