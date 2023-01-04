/*------------------------------------------------------------------------------------------------------------------------
 * http-rcl.cc - HTTP rcl routines
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
#include "rcl.h"
#include "func.h"
#include "base.h"
#include "http.h"
#include "http-common.h"
#include "http-rcl.h"

static void
print_rcl_actions (bool in, uint_fast16_t trackidx)
{
    RCL_TRACK_ACTION    track_action;
    uint_fast8_t        track_action_idx;
    uint_fast8_t        n_track_actions;
    String              prefix;

    if (in)
    {
        prefix = "i";
    }
    else
    {
        prefix = "o";
    }

    n_track_actions = RCL::get_n_track_actions (in, trackidx);

    for (track_action_idx = 0; track_action_idx < RCL_MAX_ACTIONS_PER_RCL_TRACK; track_action_idx++)
    {
        uint_fast8_t rtc;

        HTTP::response += (String) "<tr><td>Aktion " + std::to_string(track_action_idx) + ":</td><td>";

        if (track_action_idx < n_track_actions)
        {
            rtc = RCL::get_track_action (in, trackidx, track_action_idx, &track_action);

            if (rtc && track_action.action == RCL_ACTION_NONE)
            {
                Debug::printf (DEBUG_LEVEL_NORMAL, "Internal error: trackidx = %u, track_action_idx = %u: action == RCL_ACTION_NONE\n", trackidx, track_action_idx);
            }
        }
        else
        {
            track_action.action     = RCL_ACTION_NONE;
            track_action.n_parameters = 0;
            rtc = 1;
        }

        if (rtc)
        {
            uint_fast16_t   start_tenths            = 0;
            uint_fast16_t   loco_idx                = 0xFFFF;
            uint_fast16_t   addon_idx               = 0xFFFF;
            uint_fast8_t    speed                   = 0x00;
            uint_fast16_t   tenths                  = 0;
            uint_fast8_t    rrgidx                  = 0xFF;
            uint_fast8_t    rridx                   = 0xFF;
            uint_fast8_t    fwd                     = 1;
            uint_fast8_t    f                       = 0;
            uint_fast8_t    m                       = 0;
            uint_fast16_t   coidx                   = 0xFFFF;
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
            uint_fast8_t    do_display_s88          = false;
            uint16_t *      param;

            if (in)
            {
                param = RCL::tracks[trackidx].track_actions_in[track_action_idx].parameters;
            }
            else
            {
                param = RCL::tracks[trackidx].track_actions_out[track_action_idx].parameters;
            }

            switch (track_action.action)
            {
                case RCL_ACTION_NONE:                                                       // no parameters
                {
                    break;
                }

                case RCL_ACTION_SET_LOCO_SPEED:                                             // parameters: START, LOCO_IDX, SPEED, TENTHS
                {
                    start_tenths            = param[0];
                    do_display_start_tenths = true;
                    loco_idx                = param[1];
                    do_display_loco_list    = true;
                    speed                   = param[2];
                    do_display_speed        = true;
                    tenths                  = param[3];
                    do_display_tenths       = true;
                    break;
                }

                case RCL_ACTION_SET_LOCO_MIN_SPEED:                                         // parameters: START, LOCO_IDX, SPEED, TENTHS
                {
                    start_tenths            = param[0];
                    do_display_start_tenths = true;
                    loco_idx                = param[1];
                    do_display_loco_list    = true;
                    speed                   = param[2];
                    do_display_speed        = true;
                    tenths                  = param[3];
                    do_display_tenths       = true;
                    break;
                }

                case RCL_ACTION_SET_LOCO_MAX_SPEED:                                         // parameters: START, LOCO_IDX, SPEED, TENTHS
                {
                    start_tenths            = param[0];
                    do_display_start_tenths = true;
                    loco_idx                = param[1];
                    do_display_loco_list    = true;
                    speed                   = param[2];
                    do_display_speed        = true;
                    tenths                  = param[3];
                    do_display_tenths       = true;
                    break;
                }

                case RCL_ACTION_SET_LOCO_FORWARD_DIRECTION:                                 // parameters: START, LOCO_IDX, FWD
                {
                    start_tenths            = param[0];
                    do_display_start_tenths = true;
                    loco_idx                = param[1];
                    do_display_loco_list    = true;
                    fwd                     = param[2];
                    do_display_fwd          = true;
                    break;
                }

                case RCL_ACTION_SET_LOCO_ALL_FUNCTIONS_OFF:                                 // parameters: START, LOCO_IDX
                {
                    start_tenths            = param[0];
                    do_display_start_tenths = true;
                    loco_idx                = param[1];
                    do_display_loco_list    = true;
                    break;
                }

                case RCL_ACTION_SET_LOCO_FUNCTION_OFF:                                      // parameters: START, LOCO_IDX, FUNC_IDX
                {
                    start_tenths            = param[0];
                    do_display_start_tenths = true;
                    loco_idx                = param[1];
                    do_display_loco_list    = true;
                    f                       = param[2];
                    do_display_f            = true;
                    break;
                }

                case RCL_ACTION_SET_LOCO_FUNCTION_ON:                                       // parameters: START, LOCO_IDX, FUNC_IDX
                {
                    start_tenths            = param[0];
                    do_display_start_tenths = true;
                    loco_idx                = param[1];
                    do_display_loco_list    = true;
                    f                       = param[2];
                    do_display_f            = true;
                    break;
                }

                case RCL_ACTION_EXECUTE_LOCO_MACRO:                                         // parameters: START, LOCO_IDX, MACRO_IDX
                {
                    start_tenths            = param[0];
                    do_display_start_tenths = true;
                    loco_idx                = param[1];
                    do_display_loco_list    = true;
                    m                       = param[2];
                    do_display_m            = true;
                    break;
                }

                case RCL_ACTION_SET_ADDON_ALL_FUNCTIONS_OFF:                                // parameters: START, ADDON_IDX
                {
                    start_tenths            = param[0];
                    do_display_start_tenths = true;
                    addon_idx               = param[1];
                    do_display_addon_list   = true;
                    break;
                }

                case RCL_ACTION_SET_ADDON_FUNCTION_OFF:                                     // parameters: START, ADDON_IDX, FUNC_IDX
                {
                    start_tenths            = param[0];
                    do_display_start_tenths = true;
                    addon_idx               = param[1];
                    do_display_addon_list   = true;
                    f                       = param[2];
                    do_display_f            = true;
                    break;
                }

                case RCL_ACTION_SET_ADDON_FUNCTION_ON:                                      // parameters: START, ADDON_IDX, FUNC_IDX
                {
                    start_tenths            = param[0];
                    do_display_start_tenths = true;
                    addon_idx               = param[1];
                    do_display_addon_list   = true;
                    f                       = param[2];
                    do_display_f            = true;
                    break;
                }

                case RCL_ACTION_SET_RAILROAD:                                               // parameters: RRG_IDX, RR_IDX
                {
                    rrgidx                  = param[0];
                    rridx                   = param[1];
                    do_display_rr           = true;
                    break;
                }

                case RCL_ACTION_SET_FREE_RAILROAD:                                          // parameters: RRG_IDX
                {
                    rrgidx                  = param[0];
                    do_display_rrg          = true;
                    break;
                }

                case RCL_ACTION_SET_LINKED_RAILROAD:                                        // parameters: RRG_IDX
                {
                    rrgidx                  = param[0];
                    do_display_rrg          = true;
                    break;
                }

                case RCL_ACTION_WAIT_FOR_FREE_S88_CONTACT:                                  // parameters: START, LOCO_IDX, COIDX, TENTHS
                {
                    start_tenths            = param[0];
                    do_display_start_tenths = true;
                    loco_idx                = param[1];
                    do_display_loco_list    = true;
                    coidx                   = param[2];
                    do_display_s88          = true;
                    tenths                  = param[3];
                    do_display_tenths       = true;
                    break;
                }
            }

            HTTP_Common::print_rcl_action_list (in, track_action_idx, track_action.action);
            HTTP_Common::print_start_list (prefix + "st" + std::to_string(track_action_idx), start_tenths, do_display_start_tenths);

            HTTP_Common::print_loco_select_list (prefix + "ll" + std::to_string(track_action_idx), loco_idx, LOCO_LIST_SHOW_THIS_DETECTED_LOCO | LOCO_LIST_SHOW_DETECTED_LOCO | (do_display_loco_list ? LOCO_LIST_DO_DISPLAY : 0));
            HTTP_Common::print_addon_select_list (prefix + "ad" + std::to_string(track_action_idx), addon_idx, (do_display_addon_list ? ADDON_LIST_DO_DISPLAY : 0));
            HTTP_Common::print_speed_list (prefix + "sp" + std::to_string(track_action_idx), speed, do_display_speed);
            HTTP_Common::print_fwd_list (prefix + "fwd" + std::to_string(track_action_idx), fwd, do_display_fwd);
            HTTP_Common::print_function_list (prefix + "f" + std::to_string(track_action_idx), f, do_display_f);
            HTTP_Common::print_loco_macro_list (prefix + "m" + std::to_string(track_action_idx), m, do_display_m);
            HTTP_Common::print_railroad_list (prefix + "rr" + std::to_string(track_action_idx), (rrgidx << 8) | rridx, false, do_display_rr);
            HTTP_Common::print_railroad_group_list (prefix + "rrg" + std::to_string(track_action_idx), rrgidx, do_display_rrg);
            HTTP_Common::print_s88_list (prefix + "co" + std::to_string(track_action_idx), coidx, do_display_s88);
            HTTP_Common::print_tenths_list (prefix + "te" + std::to_string(track_action_idx), tenths, do_display_tenths);
        }

        HTTP::response += (String) "</td></tr>\r\n";
    }

    while (track_action_idx < RCL_MAX_ACTIONS_PER_RCL_TRACK)
    {
        HTTP::response += (String) "<tr><td>Aktion " + std::to_string(track_action_idx) + ":</td><td></td></tr>\r\n";
        track_action_idx++;
    }
}

/*----------------------------------------------------------------------------------------------------------------------------------------
 * handle_rcl_edit ()
 *----------------------------------------------------------------------------------------------------------------------------------------
 */
void
HTTP_RCL::handle_rcl_edit (void)
{
    String          url                 = "/rcl";
    String          browsertitle        = (String) "RC-Detektor bearbeiten";
    String          title               = (String) "<a href='" + url + "'>RC-Detektoren</a> &rarr; " + "RC-Detektor bearbeiten";
    const char *    strackidx           = HTTP::parameter ("trackidx");
    uint_fast16_t   trackidx            = atoi (strackidx);
    char *          rcl_name            = RCL::get_name (trackidx);
    uint32_t        flags               = RCL::get_flags (trackidx);
    String          yes_selected        = "";
    String          no_selected         = "";

    HTTP_Common::html_header (browsertitle, title, url, true);
    HTTP::response += (String) "<div style='margin-left:20px;'>\r\n";
    HTTP_Common::add_action_handler ("head", "", 200, true);

    HTTP::response += (String) "<script>\r\n";
    HTTP::response += (String) "function changeca(prefix, caidx)\r\n";
    HTTP::response += (String) "{\r\n";
    HTTP::response += (String) "  var obj = document.getElementById(prefix + 'ac' + caidx);\r\n";

    HTTP::response += (String) "  var st = 'none'\r\n";
    HTTP::response += (String) "  var ll = 'none'\r\n";
    HTTP::response += (String) "  var ad = 'none'\r\n";
    HTTP::response += (String) "  var sp = 'none'\r\n";
    HTTP::response += (String) "  var co = 'none'\r\n";
    HTTP::response += (String) "  var fwd = 'none'\r\n";
    HTTP::response += (String) "  var f = 'none'\r\n";
    HTTP::response += (String) "  var m = 'none'\r\n";
    HTTP::response += (String) "  var rrg = 'none'\r\n";
    HTTP::response += (String) "  var rr = 'none'\r\n";
    HTTP::response += (String) "  var te = 'none'\r\n";

    HTTP::response += (String) "  switch (parseInt(obj.value))\r\n";
    HTTP::response += (String) "  {\r\n";
    HTTP::response += (String) "    case " + std::to_string(RCL_ACTION_NONE)                           + ": { break; }\r\n";
    HTTP::response += (String) "    case " + std::to_string(RCL_ACTION_SET_LOCO_SPEED)                 + ": { st = ''; ll = ''; sp = ''; te = ''; break; }\r\n";
    HTTP::response += (String) "    case " + std::to_string(RCL_ACTION_SET_LOCO_MIN_SPEED)             + ": { st = ''; ll = ''; sp = ''; te = ''; break; }\r\n";
    HTTP::response += (String) "    case " + std::to_string(RCL_ACTION_SET_LOCO_MAX_SPEED)             + ": { st = ''; ll = ''; sp = ''; te = ''; break; }\r\n";
    HTTP::response += (String) "    case " + std::to_string(RCL_ACTION_SET_LOCO_FORWARD_DIRECTION)     + ": { st = ''; ll = ''; fwd = ''; break; }\r\n";
    HTTP::response += (String) "    case " + std::to_string(RCL_ACTION_SET_LOCO_ALL_FUNCTIONS_OFF)     + ": { st = ''; ll = ''; break; }\r\n";
    HTTP::response += (String) "    case " + std::to_string(RCL_ACTION_SET_LOCO_FUNCTION_OFF)          + ": { st = ''; ll = ''; f = ''; break; }\r\n";
    HTTP::response += (String) "    case " + std::to_string(RCL_ACTION_SET_LOCO_FUNCTION_ON)           + ": { st = ''; ll = ''; f = ''; break; }\r\n";
    HTTP::response += (String) "    case " + std::to_string(RCL_ACTION_EXECUTE_LOCO_MACRO)             + ": { st = ''; ll = ''; m = ''; break; }\r\n";
    HTTP::response += (String) "    case " + std::to_string(RCL_ACTION_SET_ADDON_ALL_FUNCTIONS_OFF)    + ": { st = ''; ad = ''; break; }\r\n";
    HTTP::response += (String) "    case " + std::to_string(RCL_ACTION_SET_ADDON_FUNCTION_OFF)         + ": { st = ''; ad = ''; f = ''; break; }\r\n";
    HTTP::response += (String) "    case " + std::to_string(RCL_ACTION_SET_ADDON_FUNCTION_ON)          + ": { st = ''; ad = ''; f = ''; break; }\r\n";
    HTTP::response += (String) "    case " + std::to_string(RCL_ACTION_SET_RAILROAD)                   + ": { rr = ''; break; }\r\n";
    HTTP::response += (String) "    case " + std::to_string(RCL_ACTION_SET_FREE_RAILROAD)              + ": { rrg = ''; break; }\r\n";
    HTTP::response += (String) "    case " + std::to_string(RCL_ACTION_SET_LINKED_RAILROAD)            + ": { rrg = ''; break; }\r\n";
    HTTP::response += (String) "    case " + std::to_string(RCL_ACTION_WAIT_FOR_FREE_S88_CONTACT)      + ": { st = ''; co = ''; te = ''; break; }\r\n";
    HTTP::response += (String) "  }\r\n";

    HTTP::response += (String) "  document.getElementById(prefix + 'st' + caidx).style.display = st;\r\n";
    HTTP::response += (String) "  document.getElementById(prefix + 'll' + caidx).style.display = ll;\r\n";
    HTTP::response += (String) "  document.getElementById(prefix + 'ad' + caidx).style.display = ad;\r\n";
    HTTP::response += (String) "  document.getElementById(prefix + 'sp' + caidx).style.display = sp;\r\n";
    HTTP::response += (String) "  document.getElementById(prefix + 'co' + caidx).style.display = co;\r\n";
    HTTP::response += (String) "  document.getElementById(prefix + 'fwd' + caidx).style.display = fwd;\r\n";
    HTTP::response += (String) "  document.getElementById(prefix + 'f' + caidx).style.display = f;\r\n";
    HTTP::response += (String) "  document.getElementById(prefix + 'm' + caidx).style.display = m;\r\n";
    HTTP::response += (String) "  document.getElementById(prefix + 'rrg' + caidx).style.display = rrg;\r\n";
    HTTP::response += (String) "  document.getElementById(prefix + 'rr' + caidx).style.display = rr;\r\n";
    HTTP::response += (String) "  document.getElementById(prefix + 'te' + caidx).style.display = te;\r\n";

    HTTP::response += (String) "}\r\n";
    HTTP::response += (String) "</script>\r\n";

    HTTP::flush ();

    if (flags & RCL_TRACK_FLAG_BLOCK_PROTECTION)
    {
        yes_selected    = "selected";
    }
    else
    {
        no_selected     = "selected";
    }

    HTTP::response += (String) "<form method='get' action='" + url + "'>"
                + "<table style='border:1px lightgray solid;'>\r\n"
                + "<tr bgcolor='#e0e0e0'><td>ID:</td><td>" + std::to_string(trackidx) + "</td></tr>\r\n"
                + "<tr><td>Name:</td><td><input type='text' style='width:200px' name='name' value='" + rcl_name + "'></td></tr>\r\n"
                + "<tr><td>Blocksicherung:</td><td><select name='block'><option value='0' " + no_selected + ">Nein</option><option value='1' " + yes_selected + ">Ja</option></td></tr>\r\n";

    HTTP::flush ();

    HTTP::response += (String) "<tr><td colspan='2'>Aktionen bei Einfahrt:</td></tr>";
    print_rcl_actions (true, trackidx);
    HTTP::response += (String) "<tr><td colspan='2'>Aktionen bei Ausfahrt:</td></tr>";
    print_rcl_actions (false, trackidx);

    HTTP::response += (String) "<tr><td><BR></td></tr>\r\n";
    HTTP::response += (String) "<tr><td><input type='button'  onclick=\"window.location.href='" + url + "';\" value='Zur&uuml;ck'></td>";
    HTTP::response += (String) "<td align='right'><input type='hidden' name='action' value='changercl'><input type='hidden' name='trackidx' value='" + strackidx + "'>";
    HTTP::response += (String) "<button >Speichern</button></td></tr>";

    HTTP::response += (String) "</table>\r\n";
    HTTP::response += (String) "</form>\r\n";

    HTTP::response += (String) "</div>\r\n";
    HTTP_Common::html_trailer ();
}

static void
change_rcl_actions (uint16_t trackidx, bool in)
{
    String              prefix;
    uint_fast8_t        slotidx = 0;
    uint_fast8_t        idx;
    RCL_TRACK_ACTION    track_action;
    uint_fast8_t        n_track_actions;

    n_track_actions = RCL::get_n_track_actions (in, trackidx);

    if (in)
    {
        prefix = "i";
    }
    else
    {
        prefix = "o";
    }

    for (idx = 0; idx < RCL_MAX_ACTIONS_PER_RCL_TRACK; idx++)
    {
        uint_fast8_t    st  = HTTP::parameter_number (prefix + "st"      + std::to_string(idx));
        uint_fast16_t   ll  = HTTP::parameter_number (prefix + "ll"      + std::to_string(idx));
        uint_fast16_t   ad  = HTTP::parameter_number (prefix + "ad"      + std::to_string(idx));
        uint_fast8_t    ac  = HTTP::parameter_number (prefix + "ac"      + std::to_string(idx));
        uint_fast8_t    sp  = HTTP::parameter_number (prefix + "sp"      + std::to_string(idx));
        uint_fast8_t    co  = HTTP::parameter_number (prefix + "co"      + std::to_string(idx));
        uint_fast8_t    te  = HTTP::parameter_number (prefix + "te"      + std::to_string(idx));
        uint_fast8_t    fwd = HTTP::parameter_number (prefix + "fwd"     + std::to_string(idx));
        uint_fast8_t    f   = HTTP::parameter_number (prefix + "f"       + std::to_string(idx));
        uint_fast8_t    m   = HTTP::parameter_number (prefix + "m"       + std::to_string(idx));
        uint_fast8_t    rrg = HTTP::parameter_number (prefix + "rrg"     + std::to_string(idx));
        uint_fast8_t    rr  = HTTP::parameter_number (prefix + "rr"      + std::to_string(idx));

        track_action.action = ac;

        switch (ac)
        {
            case RCL_ACTION_NONE:                                                       // no parameters
            {
                track_action.n_parameters = 0;
                break;
            }
            case RCL_ACTION_SET_LOCO_SPEED:                                             // parameters: START, LOCO, SPEED, TENTHS
            {
                track_action.parameters[0] = st;
                track_action.parameters[1] = ll;
                track_action.parameters[2] = sp;
                track_action.parameters[3] = te;
                track_action.n_parameters = 4;
                break;
            }
            case RCL_ACTION_SET_LOCO_MIN_SPEED:                                         // parameters: START, LOCO, SPEED, TENTHS
            {
                track_action.parameters[0] = st;
                track_action.parameters[1] = ll;
                track_action.parameters[2] = sp;
                track_action.parameters[3] = te;
                track_action.n_parameters = 4;
                break;
            }
            case RCL_ACTION_SET_LOCO_MAX_SPEED:                                         // parameters: START, LOCO, SPEED, TENTHS
            {
                track_action.parameters[0] = st;
                track_action.parameters[1] = ll;
                track_action.parameters[2] = sp;
                track_action.parameters[3] = te;
                track_action.n_parameters = 4;
                break;
            }
            case RCL_ACTION_SET_LOCO_FORWARD_DIRECTION:                                 // parameters: START, LOCO, FWD
            {
                track_action.parameters[0] = st;
                track_action.parameters[1] = ll;
                track_action.parameters[2] = fwd;
                track_action.n_parameters = 3;
                break;
            }
            case RCL_ACTION_SET_LOCO_ALL_FUNCTIONS_OFF:                                 // parameters: START, LOCO
            {
                track_action.parameters[0] = st;
                track_action.parameters[1] = ll;
                track_action.n_parameters = 2;
                break;
            }
            case RCL_ACTION_SET_LOCO_FUNCTION_OFF:                                      // parameters: START, LOCO, FUNC_IDX
            {
                track_action.parameters[0] = st;
                track_action.parameters[1] = ll;
                track_action.parameters[2] = f;
                track_action.n_parameters = 3;
                break;
            }
            case RCL_ACTION_SET_LOCO_FUNCTION_ON:                                       // parameters: START, LOCO, FUNC_IDX
            {
                track_action.parameters[0] = st;
                track_action.parameters[1] = ll;
                track_action.parameters[2] = f;
                track_action.n_parameters = 3;
                break;
            }
            case RCL_ACTION_EXECUTE_LOCO_MACRO:                                         // parameters: START, LOCO, MACRO_IDX
            {
                track_action.parameters[0] = st;
                track_action.parameters[1] = ll;
                track_action.parameters[2] = m;
                track_action.n_parameters = 3;
                break;
            }
            case RCL_ACTION_SET_ADDON_ALL_FUNCTIONS_OFF:                                // parameters: START, LOCO
            {
                track_action.parameters[0] = st;
                track_action.parameters[1] = ad;
                track_action.n_parameters = 2;
                break;
            }
            case RCL_ACTION_SET_ADDON_FUNCTION_OFF:                                     // parameters: START, LOCO, FUNC_IDX
            {
                track_action.parameters[0] = st;
                track_action.parameters[1] = ad;
                track_action.parameters[2] = f;
                track_action.n_parameters = 3;
                break;
            }
            case RCL_ACTION_SET_ADDON_FUNCTION_ON:                                      // parameters: START, LOCO, FUNC_IDX
            {
                track_action.parameters[0] = st;
                track_action.parameters[1] = ad;
                track_action.parameters[2] = f;
                track_action.n_parameters = 3;
                break;
            }
            case RCL_ACTION_SET_RAILROAD:                                               // parameters: RRG_IDX, RR_IDX
            {
                track_action.parameters[0] = rrg;
                track_action.parameters[1] = rr;
                track_action.n_parameters = 2;
                break;
            }
            case RCL_ACTION_SET_FREE_RAILROAD:                                          // parameters: RRG_IDX
            {
                track_action.parameters[0] = rrg;
                track_action.n_parameters = 1;
                break;
            }
            case RCL_ACTION_SET_LINKED_RAILROAD:                                        // parameters: RRG_IDX
            {
                track_action.parameters[0] = rrg;
                track_action.n_parameters = 1;
                break;
            }
            case RCL_ACTION_WAIT_FOR_FREE_S88_CONTACT:                                  // parameters: START, LOCO, COIDX, TENTHS
            {
                track_action.parameters[0] = st;
                track_action.parameters[1] = ll;
                track_action.parameters[2] = co;
                track_action.parameters[3] = te;
                track_action.n_parameters = 4;
                break;
            }
        }

        if (ac != RCL_ACTION_NONE)
        {
            if (slotidx >= n_track_actions)
            {
                RCL::add_track_action (in, trackidx);
                n_track_actions = RCL::get_n_track_actions (in, trackidx);
            }

            RCL::set_track_action (in, trackidx, slotidx, &track_action);
            slotidx++;
        }
    }

    while (slotidx < n_track_actions)
    {
        RCL::delete_track_action (in, trackidx, slotidx);
        n_track_actions = RCL::get_n_track_actions (in, trackidx);
    }
}

/*----------------------------------------------------------------------------------------------------------------------------------------
 * handle_rc ()
 *----------------------------------------------------------------------------------------------------------------------------------------
 */
void
HTTP_RCL::handle_rcl (void)
{
    String          title   = "RC-Detektoren";
    String          url     = "/rcl";
    String          urledit = "/rcledit";
    const char *    action  = HTTP::parameter ("action");
    uint_fast16_t   trackidx;

    HTTP_Common::html_header (title, title, url, true);
    HTTP::response += (String) "<div style='margin-left:20px;'>\r\n";

    if (! strcmp (action, "savercl"))
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
    else if (! strcmp (action, "newrcl"))
    {
        char            name[RCL_MAX_NAME_SIZE];
        const char *    sname  = HTTP::parameter ("name");

        strncpy (name, sname, RCL_MAX_NAME_SIZE - 1);
        name[RCL_MAX_NAME_SIZE - 1] = '\0';

        trackidx = RCL::add_track ();

        if (trackidx != 0xFF)
        {
            RCL::set_name (trackidx, name);
        }
    }
    else if (! strcmp (action, "changercl"))
    {
        char                name[RCL_MAX_NAME_SIZE];
        const char *        sname   = HTTP::parameter ("name");
        int                 block   = atoi (HTTP::parameter ("block"));

        trackidx = HTTP::parameter_number ("trackidx");

        strncpy (name, sname, RCL_MAX_NAME_SIZE - 1);
        name[RCL_MAX_NAME_SIZE - 1] = '\0';

        RCL::set_name (trackidx, name);

        if (block)
        {
            RCL::set_flags (trackidx, RCL_TRACK_FLAG_BLOCK_PROTECTION);
        }
        else
        {
            RCL::reset_flags (trackidx, RCL_TRACK_FLAG_BLOCK_PROTECTION);
        }

        change_rcl_actions (trackidx, true);
        change_rcl_actions (trackidx, false);
    }

    const char *  bg;

    HTTP_Common::add_action_handler ("rcl", "", 200, true);

    HTTP::response += (String) "<table style='border:1px lightgray solid;white-space:nowrap;'>\r\n";
    HTTP::response += (String) "<tr bgcolor='#e0e0e0'><th>ID</th><th nowrap>Bezeichnung</th><th style='width:200px'>Erkannte Lok</th>";

    if (HTTP_Common::edit_mode)
    {
        HTTP::response += (String) "<th>Aktion</th>";
    }

    HTTP::response += (String) "</tr>\r\n";

    HTTP::flush ();

    uint_fast16_t   n_tracks    = RCL::get_n_tracks ();

    for (trackidx = 0; trackidx < n_tracks; trackidx++)
    {
        const char *    loconame;
        uint_fast16_t   loco_idx;

        if (trackidx % 2)
        {
            bg = "bgcolor='#e0e0e0'";
        }
        else
        {
            bg = "";
        }

        loco_idx = RCL::tracks[trackidx].loco_idx;
        loconame = Loco::get_name (loco_idx);

        if (! loconame)
        {
            loconame = "";
        }

        HTTP::response += (String) "<tr " + bg + "><td id='t" + std::to_string(trackidx) + "' align='right'>&nbsp;"
                  + std::to_string(trackidx) + "&nbsp;</td><td nowrap>"
                  + RCL::tracks[trackidx].name
                  + "</td><td id='loc" + std::to_string(trackidx)
                  + "' style='max-width:200px;overflow:hidden' nowrap>" + loconame + "</td>";

        if (HTTP_Common::edit_mode)
        {
            HTTP::response += (String) "<td><button onclick=\"window.location.href='" + urledit + "?trackidx=" + std::to_string(trackidx) + "';\">Bearbeiten</button></td>";
        }

        HTTP::response += (String) "</tr>\r\n";
    }

    HTTP::response += (String) "</table>\r\n";
    HTTP::flush ();

    HTTP::response += (String) "<script>\r\n";
    HTTP::response += (String) "function newrcl()\r\n";
    HTTP::response += (String) "{\r\n";
    HTTP::response += (String) "  document.getElementById('action').value = 'newrcl';\r\n";
    HTTP::response += (String) "  document.getElementById('name').value = '';\r\n";
    HTTP::response += (String) "  document.getElementById('formrcl').style.display='';\r\n";
    HTTP::response += (String) "}\r\n";
    HTTP::response += (String) "</script>\r\n";

    HTTP::flush ();

    if (RCL::data_changed)
    {
        HTTP::response += (String) "<BR><form method='get' action='" + url + "'>\r\n";
        HTTP::response += (String) "<input type='hidden' name='action' value='savercl'>\r\n";
        HTTP::response += (String) "<input type='submit' value='Ge&auml;nderte Konfiguration auf Datentr&auml;ger speichern'>\r\n";
        HTTP::response += (String) "</form>\r\n";
    }

    if (HTTP_Common::edit_mode)
    {
        HTTP::response += (String) "<BR><button onclick='newrcl()'>Neuer RCL-Kontakt</button>\r\n";
        HTTP::response += (String) "<form id='formrcl' style='display:none'>\r\n";
        HTTP::response += (String) "<table>\r\n";
        HTTP::response += (String) "<tr><td>Name:</td><td><input type='text' id='name' name='name'></td></tr>\r\n";
        HTTP::response += (String) "<tr><td></td><td align='right'>";
        HTTP::response += (String) "<input type='hidden' name='action' id='action' value='newrcl'><input type='hidden' name='trackidx' id='trackidx'><input type='submit' value='Speichern'></td></tr>\r\n";
        HTTP::response += (String) "</table>\r\n";
        HTTP::response += (String) "</form>\r\n";
    }

    HTTP::response += (String) "</div>\r\n";
    HTTP_Common::html_trailer ();
}

void
HTTP_RCL::action_rcl (void)
{
    uint_fast16_t   n_tracks = RCL::get_n_tracks ();
    uint_fast8_t    trackidx;

    HTTP_Common::head_action ();

    for (trackidx = 0; trackidx < n_tracks; trackidx++)
    {
        const char *    loconame;
        const char *    color;
        const char *    bgcolor;
        uint_fast16_t   loco_idx;

        loco_idx = RCL::tracks[trackidx].loco_idx;

        loconame = Loco::get_name (loco_idx);

        if (! loconame)
        {
            loconame    = "";
            color       = "";
            bgcolor     = "";
        }
        else
        {
            color       = "white";
            bgcolor     = "green";
        }

        HTTP_Common::add_action_content ( (String) "t" + std::to_string(trackidx), "color", color);
        HTTP_Common::add_action_content ( (String) "t" + std::to_string(trackidx), "bgcolor", bgcolor);
        HTTP_Common::add_action_content ( (String) "loc" + std::to_string(trackidx), "text", loconame);
    }
}
