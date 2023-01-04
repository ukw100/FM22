/*------------------------------------------------------------------------------------------------------------------------
 * http-s88.cc - HTTP s88 routines
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
#include <string>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <stdarg.h>

#include "debug.h"
#include "fileio.h"
#include "dcc.h"
#include "loco.h"
#include "s88.h"
#include "func.h"
#include "base.h"
#include "http.h"
#include "http-common.h"
#include "http-s88.h"

static void
print_s88_action (uint_fast8_t idx, uint_fast8_t action)
{
    if (idx == action)
    {
        HTTP::response += (String) "<option value='" + std::to_string(idx) + "' selected>";
    }
    else
    {
        HTTP::response += (String) "<option value='" + std::to_string(idx) + "'>";
    }

    switch (idx)
    {
        case S88_ACTION_NONE:                                                       // no parameters
        {
            HTTP::response += (String) "&lt;keine&gt;";
            break;
        }
        case S88_ACTION_SET_LOCO_SPEED:                                             // parameters: START, LOCO_IDX, SPEED, TENTHS
        {
            HTTP::response += (String) "Setze Geschwindigkeit von Lok";
            break;
        }

        case S88_ACTION_SET_LOCO_MIN_SPEED:                                         // parameters: START, LOCO_IDX, SPEED, TENTHS
        {
            HTTP::response += (String) "Setze Mindestgeschwindigkeit von Lok";
            break;
        }

        case S88_ACTION_SET_LOCO_MAX_SPEED:                                         // parameters: START, LOCO_IDX, SPEED, TENTHS
        {
            HTTP::response += (String) "Setze H&ouml;chstgeschwindigkeit von Lok";
            break;
        }

        case S88_ACTION_SET_LOCO_FORWARD_DIRECTION:                                 // parameters: START, LOCO_IDX, FWD
        {
            HTTP::response += (String) "Setze Richtung von Lok";
            break;
        }

        case S88_ACTION_SET_LOCO_ALL_FUNCTIONS_OFF:                                 // parameters: START, LOCO_IDX
        {
            HTTP::response += (String) "Schalte alle Funktionen aus von Lok";
            break;
        }

        case S88_ACTION_SET_LOCO_FUNCTION_OFF:                                      // parameters: START, LOCO_IDX, FUNC_IDX
        {
            HTTP::response += (String) "Schalte Funktion aus von Lok";
            break;
        }

        case S88_ACTION_SET_LOCO_FUNCTION_ON:                                       // parameters: START, LOCO_IDX, FUNC_IDX
        {
            HTTP::response += (String) "Schalte Funktion ein von Lok";
            break;
        }

        case S88_ACTION_EXECUTE_LOCO_MACRO:                                         // parameters: START, LOCO_IDX, MACRO_IDX
        {
            HTTP::response += (String) "F&uuml;hre Macro aus f&uuml;r Lok";
            break;
        }

        case S88_ACTION_SET_ADDON_ALL_FUNCTIONS_OFF:                                 // parameters: START, ADDON_IDX
        {
            HTTP::response += (String) "Schalte alle Funktionen aus von Zusatzdecoder";
            break;
        }

        case S88_ACTION_SET_ADDON_FUNCTION_OFF:                                      // parameters: START, ADDON_IDX, FUNC_IDX
        {
            HTTP::response += (String) "Schalte Funktion aus von Zusatzdecoder";
            break;
        }

        case S88_ACTION_SET_ADDON_FUNCTION_ON:                                      // parameters: START, ADDON_IDX, FUNC_IDX
        {
            HTTP::response += (String) "Schalte Funktion ein von Zusatzdecoder";
            break;
        }

        case S88_ACTION_SET_RAILROAD:                                               // parameters: RRG_IDX, RR_IDX
        {
            HTTP::response += (String) "Schalte Gleis von Fahrstra&szlig;e";
            break;
        }

        case S88_ACTION_SET_FREE_RAILROAD:                                          // parameters: RRG_IDX
        {
            HTTP::response += (String) "Schalte freies Gleis von Fahrstra&szlig;e";
            break;
        }
    }

    HTTP::response += (String) "</option>\r\n";
}

static void
print_s88_action_list (bool in, uint_fast8_t caidx, uint_fast8_t action)
{
    uint_fast8_t    i;

    if (in)
    {
        HTTP::response += (String) "<select name='iac" + std::to_string(caidx) + "' id='iac" + std::to_string(caidx) + "' onchange=\"changeca('i'," + std::to_string(caidx) + ")\">\r\n";
    }
    else
    {
        HTTP::response += (String) "<select name='oac" + std::to_string(caidx) + "' id='oac" + std::to_string(caidx) + "' onchange=\"changeca('o'," + std::to_string(caidx) + ")\">\r\n";
    }

    for (i = 0; i < S88_ACTIONS; i++)
    {
        print_s88_action (i, action);
    }

    HTTP::response += (String) "</select>\r\n";
}

static void
print_s88_actions (bool in, uint_fast16_t coidx)
{
    CONTACT_ACTION  ca;
    uint_fast8_t    caidx;
    uint_fast8_t    n_contact_actions;
    String          prefix;

    if (in)
    {
        prefix = "i";
    }
    else
    {
        prefix = "o";
    }

    n_contact_actions = S88::get_n_contact_actions (in, coidx);

    for (caidx = 0; caidx < S88_MAX_ACTIONS_PER_CONTACT; caidx++)
    {
        CONTACT_ACTION *    cap;
        uint_fast8_t        rtc;

        if (in)
        {
            cap = S88::contacts[coidx].contact_actions_in;
        }
        else
        {
            cap = S88::contacts[coidx].contact_actions_out;
        }

        HTTP::response += (String) "<tr><td>Aktion " + std::to_string(caidx) + ":</td><td>";

        if (caidx < n_contact_actions)
        {
            rtc = S88::get_contact_action (in, coidx, caidx, &ca);

            if (rtc && ca.action == S88_ACTION_NONE)
            {
                Debug::printf (DEBUG_LEVEL_NORMAL, "print_s88_actions: Internal error: coidx = %u, caidx = %u: action == S88_ACTION_NONE\n", coidx, caidx);
            }
        }
        else
        {
            ca.action       = S88_ACTION_NONE;
            ca.n_parameters = 0;
            rtc = 1;
        }

        if (rtc)
        {
            uint_fast16_t   start_tenths            = 0;
            uint_fast16_t   loco_idx                = 0xFFFF;
            uint_fast16_t   addon_idx               = 0xFFFF;
            uint_fast8_t    speed                   = 0x00;
            uint_fast8_t    rrgidx                  = 0xFF;
            uint_fast8_t    rridx                   = 0xFF;
            uint_fast8_t    fwd                     = 1;
            uint_fast8_t    f                       = 0;
            uint_fast8_t    m                       = 0;
            uint_fast16_t   tenths                  = 0;
            uint_fast8_t    do_display_start_tenths = false;
            uint_fast8_t    do_display_loco_list    = false;
            uint_fast8_t    do_display_addon_list   = false;
            uint_fast8_t    do_display_speed        = false;
            uint_fast8_t    do_display_tenths       = false;
            uint_fast8_t    do_display_fwd          = false;
            uint_fast8_t    do_display_f            = false;
            uint_fast8_t    do_display_m            = false;
            uint_fast8_t    do_display_rrg          = false;
            uint_fast8_t    do_display_rr           = false;

            switch (ca.action)
            {
                case S88_ACTION_NONE:                                                       // no parameters
                {
                    break;
                }
                case S88_ACTION_SET_LOCO_SPEED:                                             // parameters: LOCO_IDX, SPEED
                {
                    start_tenths            = cap[caidx].parameters[0];
                    do_display_start_tenths = true;
                    loco_idx                = cap[caidx].parameters[1];
                    do_display_loco_list    = true;
                    speed                   = cap[caidx].parameters[2];
                    do_display_speed        = true;
                    tenths                  = cap[caidx].parameters[3];
                    do_display_tenths       = true;
                    break;
                }

                case S88_ACTION_SET_LOCO_MIN_SPEED:                                         // parameters: LOCO_IDX, SPEED
                {
                    start_tenths            = cap[caidx].parameters[0];
                    do_display_start_tenths = true;
                    loco_idx                = cap[caidx].parameters[1];
                    do_display_loco_list    = true;
                    speed                   = cap[caidx].parameters[2];
                    do_display_speed        = true;
                    tenths                  = cap[caidx].parameters[3];
                    do_display_tenths       = true;
                    break;
                }

                case S88_ACTION_SET_LOCO_MAX_SPEED:                                         // parameters: LOCO_IDX, SPEED
                {
                    start_tenths            = cap[caidx].parameters[0];
                    do_display_start_tenths = true;
                    loco_idx                = cap[caidx].parameters[1];
                    do_display_loco_list    = true;
                    speed                   = cap[caidx].parameters[2];
                    do_display_speed        = true;
                    tenths                  = cap[caidx].parameters[3];
                    do_display_tenths       = true;
                    break;
                }

                case S88_ACTION_SET_LOCO_FORWARD_DIRECTION:                                 // parameters: LOCO_IDX, FWD
                {
                    start_tenths            = cap[caidx].parameters[0];
                    do_display_start_tenths = true;
                    loco_idx                = cap[caidx].parameters[1];
                    do_display_loco_list    = true;
                    fwd                     = cap[caidx].parameters[2];
                    do_display_fwd          = true;
                    break;
                }

                case S88_ACTION_SET_LOCO_ALL_FUNCTIONS_OFF:                                 // parameters: LOCO_IDX, TENTHS
                {
                    start_tenths            = cap[caidx].parameters[0];
                    do_display_start_tenths = true;
                    loco_idx                = cap[caidx].parameters[1];
                    do_display_loco_list    = true;
                    break;
                }

                case S88_ACTION_SET_LOCO_FUNCTION_OFF:                                      // parameters: LOCO_IDX, FUNC_IDX, TENTHS
                {
                    start_tenths            = cap[caidx].parameters[0];
                    do_display_start_tenths = true;
                    loco_idx                = cap[caidx].parameters[1];
                    do_display_loco_list    = true;
                    f                       = cap[caidx].parameters[2];
                    do_display_f            = true;
                    break;
                }

                case S88_ACTION_SET_LOCO_FUNCTION_ON:                                        // parameters: LOCO_IDX, FUNC_IDX, TENTHS
                {
                    start_tenths            = cap[caidx].parameters[0];
                    do_display_start_tenths = true;
                    loco_idx                = cap[caidx].parameters[1];
                    do_display_loco_list    = true;
                    f                       = cap[caidx].parameters[2];
                    do_display_f            = true;
                    break;
                }

                case S88_ACTION_EXECUTE_LOCO_MACRO:                                         // parameters: LOCO_IDX, FUNC_IDX, TENTHS
                {
                    start_tenths            = cap[caidx].parameters[0];
                    do_display_start_tenths = true;
                    loco_idx                = cap[caidx].parameters[1];
                    do_display_loco_list    = true;
                    m                       = cap[caidx].parameters[2];
                    do_display_m            = true;
                    break;
                }

                case S88_ACTION_SET_ADDON_ALL_FUNCTIONS_OFF:                                 // parameters: ADDON_IDX, TENTHS
                {
                    start_tenths            = cap[caidx].parameters[0];
                    do_display_start_tenths = true;
                    addon_idx               = cap[caidx].parameters[1];
                    do_display_addon_list   = true;
                    break;
                }

                case S88_ACTION_SET_ADDON_FUNCTION_OFF:                                      // parameters: ADDON_IDX, FUNC_IDX, TENTHS
                {
                    start_tenths            = cap[caidx].parameters[0];
                    do_display_start_tenths = true;
                    addon_idx               = cap[caidx].parameters[1];
                    do_display_addon_list   = true;
                    f                       = cap[caidx].parameters[2];
                    do_display_f            = true;
                    break;
                }

                case S88_ACTION_SET_ADDON_FUNCTION_ON:                                       // parameters: ADDON_IDX, FUNC_IDX, TENTHS
                {
                    start_tenths            = cap[caidx].parameters[0];
                    do_display_start_tenths = true;
                    addon_idx               = cap[caidx].parameters[1];
                    do_display_addon_list   = true;
                    f                       = cap[caidx].parameters[2];
                    do_display_f            = true;
                    break;
                }

                case S88_ACTION_SET_RAILROAD:                                               // parameters: RRG_IDX, RR_IDX
                {
                    rrgidx                  = cap[caidx].parameters[0];
                    rridx                   = cap[caidx].parameters[1];
                    do_display_rr           = true;
                    break;
                }

                case S88_ACTION_SET_FREE_RAILROAD:                                          // parameters: RRG_IDX
                {
                    rrgidx                  = cap[caidx].parameters[0];
                    do_display_rrg          = true;
                    break;
                }
            }

            print_s88_action_list (in, caidx, ca.action);
            HTTP_Common::print_start_list (prefix + "st" + std::to_string(caidx), start_tenths, do_display_start_tenths);

            HTTP_Common::print_loco_select_list (prefix + "ll" + std::to_string(caidx), loco_idx, LOCO_LIST_SHOW_LINKED_LOCO | LOCO_LIST_SHOW_DETECTED_LOCO | (do_display_loco_list ? LOCO_LIST_DO_DISPLAY : 0));
            HTTP_Common::print_addon_select_list (prefix + "ad" + std::to_string(caidx), addon_idx, (do_display_addon_list ? ADDON_LIST_DO_DISPLAY : 0));
            HTTP_Common::print_speed_list (prefix + "sp" + std::to_string(caidx), speed, do_display_speed);
            HTTP_Common::print_fwd_list (prefix + "fwd" + std::to_string(caidx), fwd, do_display_fwd);
            HTTP_Common::print_function_list (prefix + "f" + std::to_string(caidx), f, do_display_f);
            HTTP_Common::print_loco_macro_list (prefix + "m" + std::to_string(caidx), m, do_display_m);
            HTTP_Common::print_railroad_list (prefix + "rr" + std::to_string(caidx), (rrgidx << 8) | rridx, false, do_display_rr);
            HTTP_Common::print_railroad_group_list (prefix + "rrg" + std::to_string(caidx), rrgidx, do_display_rrg);
            HTTP_Common::print_tenths_list (prefix + "te" + std::to_string(caidx), tenths, do_display_tenths);
        }

        HTTP::response += (String) "</td></tr>\r\n";
    }

    while (caidx < S88_MAX_ACTIONS_PER_CONTACT)
    {
        HTTP::response += (String) "<tr><td>Aktion " + std::to_string(caidx) + ":</td><td></td></tr>\r\n";
        caidx++;
    }
}

/*----------------------------------------------------------------------------------------------------------------------------------------
 * handle_s88_edit ()
 *----------------------------------------------------------------------------------------------------------------------------------------
 */
void
HTTP_S88::handle_s88_edit (void)
{
    String          url                 = "/s88";
    String          browsertitle        = (String) "S88 bearbeiten";
    String          title               = (String) "<a href='" + url + "'>S88</a> &rarr; " + "S88 bearbeiten";
    const char *    scoidx              = HTTP::parameter ("coidx");
    uint_fast16_t   coidx               = atoi (scoidx);
    uint_fast16_t   linkedrr            = S88::get_link_railroad (coidx);

    HTTP_Common::html_header (browsertitle, title, url, true);
    HTTP::response += (String) "<div style='margin-left:20px;'>\r\n";
    HTTP_Common::add_action_handler ("head", "", 200, true);

    char * s88_name = S88::get_name (coidx);

    HTTP::response += (String) "<script>\r\n";
    HTTP::response += (String) "function changeca(prefix, caidx)\r\n";
    HTTP::response += (String) "{\r\n";
    HTTP::response += (String) "  var obj = document.getElementById(prefix + 'ac' + caidx);\r\n";
    HTTP::response += (String) "  var st = 'none'\r\n";
    HTTP::response += (String) "  var ll = 'none'\r\n";
    HTTP::response += (String) "  var ad = 'none'\r\n";
    HTTP::response += (String) "  var sp = 'none'\r\n";
    HTTP::response += (String) "  var fwd = 'none'\r\n";
    HTTP::response += (String) "  var f = 'none'\r\n";
    HTTP::response += (String) "  var m = 'none'\r\n";
    HTTP::response += (String) "  var rrg = 'none'\r\n";
    HTTP::response += (String) "  var rr = 'none'\r\n";
    HTTP::response += (String) "  var te = 'none'\r\n";

    HTTP::response += (String) "  switch (parseInt(obj.value))\r\n";
    HTTP::response += (String) "  {\r\n";
    HTTP::response += (String) "    case " + std::to_string(S88_ACTION_NONE)                           + ": { break; }\r\n";
    HTTP::response += (String) "    case " + std::to_string(S88_ACTION_SET_LOCO_SPEED)                 + ": { st = ''; ll = ''; sp = ''; te = ''; break; }\r\n";
    HTTP::response += (String) "    case " + std::to_string(S88_ACTION_SET_LOCO_MIN_SPEED)             + ": { st = ''; ll = ''; sp = ''; te = ''; break; }\r\n";
    HTTP::response += (String) "    case " + std::to_string(S88_ACTION_SET_LOCO_MAX_SPEED)             + ": { st = ''; ll = ''; sp = ''; te = ''; break; }\r\n";
    HTTP::response += (String) "    case " + std::to_string(S88_ACTION_SET_LOCO_FORWARD_DIRECTION)     + ": { st = ''; ll = ''; fwd = ''; break; }\r\n";
    HTTP::response += (String) "    case " + std::to_string(S88_ACTION_SET_LOCO_ALL_FUNCTIONS_OFF)     + ": { st = ''; ll = ''; break; }\r\n";
    HTTP::response += (String) "    case " + std::to_string(S88_ACTION_SET_LOCO_FUNCTION_OFF)          + ": { st = ''; ll = ''; f = ''; break; }\r\n";
    HTTP::response += (String) "    case " + std::to_string(S88_ACTION_SET_LOCO_FUNCTION_ON)           + ": { st = ''; ll = ''; f = ''; break; }\r\n";
    HTTP::response += (String) "    case " + std::to_string(S88_ACTION_EXECUTE_LOCO_MACRO)             + ": { st = ''; ll = ''; m = ''; break; }\r\n";
    HTTP::response += (String) "    case " + std::to_string(S88_ACTION_SET_ADDON_ALL_FUNCTIONS_OFF)    + ": { st = ''; ad = ''; break; }\r\n";
    HTTP::response += (String) "    case " + std::to_string(S88_ACTION_SET_ADDON_FUNCTION_OFF)         + ": { st = ''; ad = ''; f = ''; break; }\r\n";
    HTTP::response += (String) "    case " + std::to_string(S88_ACTION_SET_ADDON_FUNCTION_ON)          + ": { st = ''; ad = ''; f = ''; break; }\r\n";
    HTTP::response += (String) "    case " + std::to_string(S88_ACTION_SET_RAILROAD)                   + ": { rr = ''; break; }\r\n";
    HTTP::response += (String) "    case " + std::to_string(S88_ACTION_SET_FREE_RAILROAD)              + ": { rrg = ''; break; }\r\n";
    HTTP::response += (String) "  }\r\n";

    HTTP::response += (String) "  document.getElementById(prefix + 'st' + caidx).style.display = st;\r\n";
    HTTP::response += (String) "  document.getElementById(prefix + 'll' + caidx).style.display = ll;\r\n";
    HTTP::response += (String) "  document.getElementById(prefix + 'ad' + caidx).style.display = ad;\r\n";
    HTTP::response += (String) "  document.getElementById(prefix + 'sp' + caidx).style.display = sp;\r\n";
    HTTP::response += (String) "  document.getElementById(prefix + 'fwd' + caidx).style.display = fwd;\r\n";
    HTTP::response += (String) "  document.getElementById(prefix + 'f' + caidx).style.display = f;\r\n";
    HTTP::response += (String) "  document.getElementById(prefix + 'm' + caidx).style.display = m;\r\n";
    HTTP::response += (String) "  document.getElementById(prefix + 'rrg' + caidx).style.display = rrg;\r\n";
    HTTP::response += (String) "  document.getElementById(prefix + 'rr' + caidx).style.display = rr;\r\n";
    HTTP::response += (String) "  document.getElementById(prefix + 'te' + caidx).style.display = te;\r\n";

    HTTP::response += (String) "}\r\n";
    HTTP::response += (String) "</script>\r\n";

    HTTP::flush ();

    HTTP::response += (String) "<form method='get' action='" + url + "'>"
                + "<table style='border:1px lightgray solid;'>\r\n"
                + "<tr bgcolor='#e0e0e0'><td>ID:</td><td>" + std::to_string(coidx) + "</td></tr>\r\n"
                + "<tr><td>Name:</td><td><input type='text' style='width:200px' name='name' value='" + s88_name + "'></td></tr>\r\n";

    HTTP::response += (String) "<tr><td>Gleis:</td><td>\r\n";
    HTTP_Common::print_railroad_list ((String) "rr", linkedrr, true, true);
    HTTP::response += (String) "</td></tr>\r\n";
    HTTP::flush ();

    HTTP::response += (String) "<tr><td colspan='2'>Aktionen bei Einfahrt:</td></tr>";
    print_s88_actions (true, coidx);
    HTTP::response += (String) "<tr><td colspan='2'>Aktionen bei Ausfahrt:</td></tr>";
    print_s88_actions (false, coidx);

    HTTP::flush ();

    HTTP::response += (String) "<tr><td><BR></td></tr>\r\n";
    HTTP::response += (String) "<tr><td><input type='button'  onclick=\"window.location.href='" + url + "';\" value='Zur&uuml;ck'></td>";
    HTTP::response += (String) "<td align='right'><input type='hidden' name='action' value='changes88'><input type='hidden' name='coidx' value='" + scoidx + "'>";
    HTTP::response += (String) "<button >Speichern</button></td></tr>";

    HTTP::response += (String) "</table>\r\n";
    HTTP::response += (String) "</form>\r\n";

    HTTP::response += (String) "</div>\r\n";
    HTTP_Common::html_trailer ();
}

static void
change_s88_actions (uint_fast16_t coidx, bool in)
{
    String              prefix;
    CONTACT_ACTION      ca;
    uint_fast8_t        n_contact_actions;
    uint_fast8_t        slotidx = 0;
    uint_fast8_t        idx;

    n_contact_actions = S88::get_n_contact_actions (in, coidx);

    if (in)
    {
        prefix = "i";
    }
    else
    {
        prefix = "o";
    }

    for (idx = 0; idx < S88_MAX_ACTIONS_PER_CONTACT; idx++)
    {
        uint_fast16_t   st  = HTTP::parameter_number (prefix + "st" + std::to_string(idx));
        uint_fast8_t    ac  = HTTP::parameter_number (prefix + "ac" + std::to_string(idx));
        uint_fast16_t   ll  = HTTP::parameter_number (prefix + "ll" + std::to_string(idx));
        uint_fast16_t   ad  = HTTP::parameter_number (prefix + "ad" + std::to_string(idx));
        uint_fast8_t    sp  = HTTP::parameter_number (prefix + "sp" + std::to_string(idx));
        uint_fast8_t    te  = HTTP::parameter_number (prefix + "te" + std::to_string(idx));
        uint_fast8_t    fwd = HTTP::parameter_number (prefix + "fwd" + std::to_string(idx));
        uint_fast8_t    f   = HTTP::parameter_number (prefix + "f" + std::to_string(idx));
        uint_fast8_t    m   = HTTP::parameter_number (prefix + "m" + std::to_string(idx));
        uint_fast8_t    rrg = HTTP::parameter_number (prefix + "rrg" + std::to_string(idx));
        uint_fast16_t   rr  = HTTP::parameter_number (prefix + "rr" + std::to_string(idx));

        ca.action       = ac;

        switch (ac)
        {
            case S88_ACTION_NONE:                                                       // no parameters
            {
                ca.n_parameters = 0;
                break;
            }
            case S88_ACTION_SET_LOCO_SPEED:                                             // parameters: START, LOCO_IDX, SPEED, TENTHS
            {
                ca.parameters[0] = st;
                ca.parameters[1] = ll;
                ca.parameters[2] = sp;
                ca.parameters[3] = te;
                ca.n_parameters = 4;
                break;
            }
            case S88_ACTION_SET_LOCO_MIN_SPEED:                                         // parameters: START, LOCO_IDX, SPEED, TENTHS
            {
                ca.parameters[0] = st;
                ca.parameters[1] = ll;
                ca.parameters[2] = sp;
                ca.parameters[3] = te;
                ca.n_parameters = 4;
                break;
            }
            case S88_ACTION_SET_LOCO_MAX_SPEED:                                         // parameters: START, LOCO_IDX, SPEED, TENTHS
            {
                ca.parameters[0] = st;
                ca.parameters[1] = ll;
                ca.parameters[2] = sp;
                ca.parameters[3] = te;
                ca.n_parameters = 4;
                break;
            }
            case S88_ACTION_SET_LOCO_FORWARD_DIRECTION:                                 // parameters: START, LOCO_IDX, FWD
            {
                ca.parameters[0] = st;
                ca.parameters[1] = ll;
                ca.parameters[2] = fwd;
                ca.n_parameters = 3;
                break;
            }
            case S88_ACTION_SET_LOCO_ALL_FUNCTIONS_OFF:                                 // parameters: START, LOCO_IDX
            {
                ca.parameters[0] = st;
                ca.parameters[1] = ll;
                ca.n_parameters = 2;
                break;
            }
            case S88_ACTION_SET_LOCO_FUNCTION_OFF:                                      // parameters: START, LOCO_IDX, FUNC_IDX
            {
                ca.parameters[0] = st;
                ca.parameters[1] = ll;
                ca.parameters[2] = f;
                ca.n_parameters = 3;
                break;
            }
            case S88_ACTION_SET_LOCO_FUNCTION_ON:                                       // parameters: START, LOCO_IDX, FUNC_IDX
            {
                ca.parameters[0] = st;
                ca.parameters[1] = ll;
                ca.parameters[2] = f;
                ca.n_parameters = 3;
                break;
            }
            case S88_ACTION_EXECUTE_LOCO_MACRO:                                         // parameters: START, LOCO_IDX, MACRO_IDX
            {
                ca.parameters[0] = st;
                ca.parameters[1] = ll;
                ca.parameters[2] = m;
                ca.n_parameters = 3;
                break;
            }
            case S88_ACTION_SET_ADDON_ALL_FUNCTIONS_OFF:                                // parameters: START, ADDON_IDX
            {
                ca.parameters[0] = st;
                ca.parameters[1] = ad;
                ca.n_parameters = 2;
                break;
            }
            case S88_ACTION_SET_ADDON_FUNCTION_OFF:                                     // parameters: START, ADDON_IDX, FUNC_IDX
            {
                ca.parameters[0] = st;
                ca.parameters[1] = ad;
                ca.parameters[2] = f;
                ca.n_parameters = 3;
                break;
            }
            case S88_ACTION_SET_ADDON_FUNCTION_ON:                                      // parameters: START, LOCO_IDX, FUNC_IDX
            {
                ca.parameters[0] = st;
                ca.parameters[1] = ad;
                ca.parameters[2] = f;
                ca.n_parameters = 3;
                break;
            }
            case S88_ACTION_SET_RAILROAD:                                               // parameters: RRG_IDX, RR_IDX
            {
                ca.parameters[0] = rr >> 8;
                ca.parameters[1] = rr & 0xFF;
                ca.n_parameters = 2;
                break;
            }
            case S88_ACTION_SET_FREE_RAILROAD:                                          // parameters: RRG_IDX
            {
                ca.parameters[0] = rrg;
                ca.n_parameters = 1;
                break;
            }
        }

        if (ac != S88_ACTION_NONE)
        {
            if (slotidx >= n_contact_actions)
            {
                S88::add_contact_action (in, coidx);
                n_contact_actions = S88::get_n_contact_actions (in, coidx);
            }

            S88::set_contact_action (in, coidx, slotidx, &ca);
            slotidx++;
        }
    }

    while (slotidx < n_contact_actions)
    {
        S88::delete_contact_action (in, coidx, slotidx);
        n_contact_actions = S88::get_n_contact_actions (in, coidx);
    }
}

/*----------------------------------------------------------------------------------------------------------------------------------------
 * handle_s88 ()
 *----------------------------------------------------------------------------------------------------------------------------------------
 */
void
HTTP_S88::handle_s88 (void)
{
    String          title   = "S88";
    String          url     = "/s88";
    String          urledit = "/s88edit";
    const char *    action  = HTTP::parameter ("action");
    uint_fast16_t   coidx;

    HTTP_Common::html_header (title, title, url, true);
    HTTP::response += (String) "<div style='margin-left:20px;'>\r\n";

    if (! strcmp (action, "saves88"))
    {
        uint_fast8_t rtc;

        rtc = FileIO::write_all_ini_files ();

        if (rtc)
        {
            HTTP::response += (String) "<font color='green'>Konfiguration wurde auf Datentr&auml;ger gespeichert</font><P>";
        }
        else
        {
            HTTP::response += (String) "<font color='red'>Konfiguration konnte nicht auf Datentr&auml;ger gespeichert werden</font><P>";
        }
    }
    else if (! strcmp (action, "news88"))
    {
        char            name[S88_MAX_NAME_SIZE];
        const char *    sname  = HTTP::parameter ("name");

        strncpy (name, sname, S88_MAX_NAME_SIZE - 1);
        name[S88_MAX_NAME_SIZE - 1] = '\0';

        coidx = S88::add_contact ();

        if (coidx != 0xFFFF)
        {
            S88::set_name (coidx, name);
        }
    }
    else if (! strcmp (action, "changes88"))
    {
        char            name[S88_MAX_NAME_SIZE];
        const char *    sname   = HTTP::parameter ("name");
        uint_fast16_t   rr      = HTTP::parameter_number ("rr");
        uint_fast8_t    rrgidx  = rr >> 8;
        uint_fast8_t    rridx   = rr & 0xFF;

        coidx = HTTP::parameter_number ("coidx");

        strncpy (name, sname, S88_MAX_NAME_SIZE - 1);
        name[S88_MAX_NAME_SIZE - 1] = '\0';

        S88::set_name (coidx, name);
        S88::set_link_railroad (coidx, rrgidx, rridx);

        change_s88_actions (coidx, true);
        change_s88_actions (coidx, false);
    }

    HTTP_Common::add_action_handler ("s88", "", 200, true);

    const char *  bg;
    const char *  style;

    HTTP::response += (String) "<table style='border:1px lightgray solid;'>\r\n";
    HTTP::response += (String) "<tr bgcolor='#e0e0e0'><th>ID</th><th>Bezeichnung</th>";

    if (HTTP_Common::edit_mode)
    {
        HTTP::response += (String) "<th>Aktion</th>";
    }

    HTTP::response += (String) "</tr>\r\n";
    HTTP::flush ();

    uint_fast16_t   n_contacts = S88::get_n_contacts ();

    for (coidx = 0; coidx < n_contacts; coidx++)
    {
        if (coidx % 2)
        {
            bg = "bgcolor='#e0e0e0'";
        }
        else
        {
            bg = "";
        }

        if (S88::get_state_bit (coidx))
        {
            style = "style='color:white;background-color:red'";
        }
        else
        {
            style = "";
        }

        HTTP::response += (String) "<tr " + bg + "><td id='s" + std::to_string(coidx) + "' " + style + " align='right'>"
                  + std::to_string(coidx) + "&nbsp;</td><td>"
                  + S88::contacts[coidx].name
                  + "</td>";

        if (HTTP_Common::edit_mode)
        {
            HTTP::response += (String) "<td><button onclick=\"window.location.href='" + urledit + "?coidx=" + std::to_string(coidx) + "';\">Bearbeiten</button></td>";
        }

        HTTP::response += (String) "</tr>\r\n";
    }

    HTTP::response += (String) "</table>\r\n";
    HTTP::flush ();

    HTTP::response += (String) "<script>\r\n";
    HTTP::response += (String) "function news88()\r\n";
    HTTP::response += (String) "{\r\n";
    HTTP::response += (String) "  document.getElementById('action').value = 'news88';\r\n";
    HTTP::response += (String) "  document.getElementById('name').value = '';\r\n";
    HTTP::response += (String) "  document.getElementById('forms88').style.display='';\r\n";
    HTTP::response += (String) "}\r\n";
    HTTP::response += (String) "</script>\r\n";

    HTTP::flush ();

    if (S88::data_changed)
    {
        HTTP::response += (String) "<BR><form method='get' action='" + url + "'>\r\n";
        HTTP::response += (String) "<input type='hidden' name='action' value='saves88'>\r\n";
        HTTP::response += (String) "<input type='submit' value='Ge&auml;nderte Konfiguration auf Datentr&auml;ger speichern'>\r\n";
        HTTP::response += (String) "</form>\r\n";
    }

    if (HTTP_Common::edit_mode)
    {
        HTTP::response += (String) "<BR><button onclick='news88()'>Neuer S88-Kontakt</button>\r\n";
        HTTP::response += (String) "<form id='forms88' style='display:none'>\r\n";
        HTTP::response += (String) "<table>\r\n";
        HTTP::response += (String) "<tr><td>Name:</td><td><input type='text' id='name' name='name'></td></tr>\r\n";
        HTTP::response += (String) "<tr><td></td><td align='right'>";
        HTTP::response += (String) "<input type='hidden' name='action' id='action' value='news88'><input type='hidden' name='coidx' id='coidx'><input type='submit' value='Speichern'></td></tr>\r\n";
        HTTP::response += (String) "</table>\r\n";
        HTTP::response += (String) "</form>\r\n";
    }

    HTTP::response += (String) "</div>\r\n";
    HTTP_Common::html_trailer ();
}

void
HTTP_S88::action_s88 (void)
{
    uint_fast16_t   n_contacts = S88::get_n_contacts ();
    uint_fast8_t    coidx;

    HTTP_Common::head_action ();

    for (coidx = 0; coidx < n_contacts; coidx++)
    {
        String id = (String) "s" + std::to_string(coidx);

        if (S88::get_state_bit (coidx) == S88_STATE_OCCUPIED)
        {
            HTTP_Common::add_action_content (id, "bgcolor", "green");
            HTTP_Common::add_action_content (id, "color", "white");
        }
        else
        {
            HTTP_Common::add_action_content (id, "bgcolor", "");
            HTTP_Common::add_action_content (id, "color", "");
        }
    }
}
