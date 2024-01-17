/*------------------------------------------------------------------------------------------------------------------------
 * http-rcl.cc - HTTP rcl routines
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

    n_track_actions = RCL::tracks[trackidx].get_n_track_actions (in);

    for (track_action_idx = 0; track_action_idx < RCL_MAX_ACTIONS_PER_RCL_TRACK; track_action_idx++)
    {
        String          strack_action_idx = std::to_string(track_action_idx);
        uint_fast8_t    rtc;

        HTTP::response += (String) "<tr><td>Aktion " + strack_action_idx + ":</td><td>";

        if (track_action_idx < n_track_actions)
        {
            rtc = RCL::tracks[trackidx].get_track_action (in, track_action_idx, &track_action);

            if (rtc && track_action.action == RCL_ACTION_NONE)
            {
                Debug::printf (DEBUG_LEVEL_NORMAL, "Internal error: trackidx = %u, track_action_idx = %u: action == RCL_ACTION_NONE\n", trackidx, track_action_idx);
            }
        }
        else
        {
            track_action.condition              = RCL_CONDITION_NEVER;
            track_action.condition_destination  = 0xFF;
            track_action.action                 = RCL_ACTION_NONE;
            track_action.n_parameters           = 0;
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
            uint_fast8_t    destidx                 = 0xFF;
            uint_fast8_t    fwd                     = 1;
            uint_fast8_t    f                       = 0;
            uint_fast8_t    m                       = 0;
            uint_fast16_t   coidx                   = 0xFFFF;
            uint_fast16_t   led_group_idx           = 0xFFFF;
            uint_fast8_t    ledmask                 = 0x00;
            uint_fast8_t    ledon                   = 0;
            uint_fast16_t   swidx                   = 0xFFFF;
            uint_fast8_t    swstate                 = DCC_SWITCH_STATE_UNDEFINED;
            uint_fast16_t   sigidx                  = 0xFFFF;
            uint_fast8_t    sigstate                = DCC_SIGNAL_STATE_UNDEFINED;
            uint_fast8_t    do_display_start_tenths = false;
            uint_fast8_t    do_display_loco_list    = false;
            uint_fast8_t    do_display_addon_list   = false;
            uint_fast8_t    do_display_speed        = false;
            uint_fast8_t    do_display_tenths       = false;
            uint_fast8_t    do_display_fwd          = false;
            uint_fast8_t    do_display_f            = false;
            uint_fast8_t    do_display_lm           = false;
            uint_fast8_t    do_display_rrg          = false;
            uint_fast8_t    do_display_rr           = false;
            uint_fast8_t    do_display_s88          = false;
            uint_fast8_t    do_display_dest         = false;
            uint_fast8_t    do_display_led          = false;
            uint_fast8_t    do_display_sw           = false;
            uint_fast8_t    do_display_sig          = false;
            uint16_t *      param;
            uint_fast8_t    pidx                    = 0;

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
                    start_tenths            = param[pidx++];
                    do_display_start_tenths = true;
                    loco_idx                = param[pidx++];
                    do_display_loco_list    = true;
                    speed                   = param[pidx++];
                    do_display_speed        = true;
                    tenths                  = param[pidx++];
                    do_display_tenths       = true;
                    break;
                }

                case RCL_ACTION_SET_LOCO_MIN_SPEED:                                         // parameters: START, LOCO_IDX, SPEED, TENTHS
                {
                    start_tenths            = param[pidx++];
                    do_display_start_tenths = true;
                    loco_idx                = param[pidx++];
                    do_display_loco_list    = true;
                    speed                   = param[pidx++];
                    do_display_speed        = true;
                    tenths                  = param[pidx++];
                    do_display_tenths       = true;
                    break;
                }

                case RCL_ACTION_SET_LOCO_MAX_SPEED:                                         // parameters: START, LOCO_IDX, SPEED, TENTHS
                {
                    start_tenths            = param[pidx++];
                    do_display_start_tenths = true;
                    loco_idx                = param[pidx++];
                    do_display_loco_list    = true;
                    speed                   = param[pidx++];
                    do_display_speed        = true;
                    tenths                  = param[pidx++];
                    do_display_tenths       = true;
                    break;
                }

                case RCL_ACTION_SET_LOCO_FORWARD_DIRECTION:                                 // parameters: START, LOCO_IDX, FWD
                {
                    start_tenths            = param[pidx++];
                    do_display_start_tenths = true;
                    loco_idx                = param[pidx++];
                    do_display_loco_list    = true;
                    fwd                     = param[pidx++];
                    do_display_fwd          = true;
                    break;
                }

                case RCL_ACTION_SET_LOCO_ALL_FUNCTIONS_OFF:                                 // parameters: START, LOCO_IDX
                {
                    start_tenths            = param[pidx++];
                    do_display_start_tenths = true;
                    loco_idx                = param[pidx++];
                    do_display_loco_list    = true;
                    break;
                }

                case RCL_ACTION_SET_LOCO_FUNCTION_OFF:                                      // parameters: START, LOCO_IDX, FUNC_IDX
                {
                    start_tenths            = param[pidx++];
                    do_display_start_tenths = true;
                    loco_idx                = param[pidx++];
                    do_display_loco_list    = true;
                    f                       = param[pidx++];
                    do_display_f            = true;
                    break;
                }

                case RCL_ACTION_SET_LOCO_FUNCTION_ON:                                       // parameters: START, LOCO_IDX, FUNC_IDX
                {
                    start_tenths            = param[pidx++];
                    do_display_start_tenths = true;
                    loco_idx                = param[pidx++];
                    do_display_loco_list    = true;
                    f                       = param[pidx++];
                    do_display_f            = true;
                    break;
                }

                case RCL_ACTION_EXECUTE_LOCO_MACRO:                                         // parameters: START, LOCO_IDX, MACRO_IDX
                {
                    start_tenths            = param[pidx++];
                    do_display_start_tenths = true;
                    loco_idx                = param[pidx++];
                    do_display_loco_list    = true;
                    m                       = param[pidx++];
                    do_display_lm           = true;
                    break;
                }

                case RCL_ACTION_SET_ADDON_ALL_FUNCTIONS_OFF:                                // parameters: START, ADDON_IDX
                {
                    start_tenths            = param[pidx++];
                    do_display_start_tenths = true;
                    addon_idx               = param[pidx++];
                    do_display_addon_list   = true;
                    break;
                }

                case RCL_ACTION_SET_ADDON_FUNCTION_OFF:                                     // parameters: START, ADDON_IDX, FUNC_IDX
                {
                    start_tenths            = param[pidx++];
                    do_display_start_tenths = true;
                    addon_idx               = param[pidx++];
                    do_display_addon_list   = true;
                    f                       = param[pidx++];
                    do_display_f            = true;
                    break;
                }

                case RCL_ACTION_SET_ADDON_FUNCTION_ON:                                      // parameters: START, ADDON_IDX, FUNC_IDX
                {
                    start_tenths            = param[pidx++];
                    do_display_start_tenths = true;
                    addon_idx               = param[pidx++];
                    do_display_addon_list   = true;
                    f                       = param[pidx++];
                    do_display_f            = true;
                    break;
                }

                case RCL_ACTION_SET_RAILROAD:                                               // parameters: RRG_IDX, RR_IDX
                {
                    rrgidx                  = param[pidx++];
                    rridx                   = param[pidx++];
                    do_display_rr           = true;
                    break;
                }

                case RCL_ACTION_SET_FREE_RAILROAD:                                          // parameters: RRG_IDX
                {
                    rrgidx                  = param[pidx++];
                    do_display_rrg          = true;
                    break;
                }

                case RCL_ACTION_SET_LINKED_RAILROAD:                                        // parameters: RRG_IDX
                {
                    rrgidx                  = param[pidx++];
                    do_display_rrg          = true;
                    break;
                }

                case RCL_ACTION_WAIT_FOR_FREE_S88_CONTACT:                                  // parameters: START, LOCO_IDX, COIDX, TENTHS
                {
                    start_tenths            = param[pidx++];
                    do_display_start_tenths = true;
                    loco_idx                = param[pidx++];
                    do_display_loco_list    = true;
                    coidx                   = param[pidx++];
                    do_display_s88          = true;
                    tenths                  = param[pidx++];
                    do_display_tenths       = true;
                    break;
                }

                case RCL_ACTION_SET_LOCO_DESTINATION:                                       // parameters: LOCO_IDX, RRG_IDX
                {
                    loco_idx                = param[pidx++];
                    do_display_loco_list    = true;
                    destidx                 = param[pidx++];
                    do_display_dest         = true;
                    break;
                }

                case RCL_ACTION_SET_LED:                                                    // parameters: START, LG_IDX, LED_MASK, LED_ON
                {
                    start_tenths            = param[pidx++];
                    do_display_start_tenths = true;
                    led_group_idx           = param[pidx++];
                    ledmask                 = param[pidx++];
                    ledon                   = param[pidx++];
                    do_display_led          = true;
                    break;
                }

                case RCL_ACTION_SET_SWITCH:                                                // parameters: START, SW_IDX, SW_STATE
                {
                    start_tenths            = param[pidx++];
                    do_display_start_tenths = true;
                    swidx                   = param[pidx++];
                    swstate                 = param[pidx++];
                    do_display_sw           = true;
                    break;
                }

                case RCL_ACTION_SET_SIGNAL:                                                // parameters: START, SW_IDX, SW_STATE
                {
                    start_tenths            = param[pidx++];
                    do_display_start_tenths = true;
                    sigidx                  = param[pidx++];
                    sigstate                = param[pidx++];
                    do_display_sig          = true;
                    break;
                }
            }

            String saidx = strack_action_idx;

            HTTP_Common::print_rcl_condition_list (in, track_action_idx, track_action.condition, track_action.condition_destination);

            String sctstyle;

            if (track_action.condition == RCL_CONDITION_NEVER)
            {
                sctstyle = " style='display:none;'";
            }

            HTTP::response += (String) "<span id=" + prefix + "ct" + saidx + sctstyle + ">\r\n";

            HTTP_Common::print_rcl_action_list (in, track_action_idx, track_action.action);
            HTTP_Common::print_start_list (prefix + "st" + saidx, start_tenths, do_display_start_tenths);

            HTTP_Common::print_loco_select_list (prefix + "ll" + saidx, loco_idx, LOCO_LIST_SHOW_THIS_DETECTED_LOCO | LOCO_LIST_SHOW_DETECTED_LOCO | (do_display_loco_list ? LOCO_LIST_DO_DISPLAY : 0));
            HTTP_Common::print_addon_select_list (prefix + "ad" + saidx, addon_idx, (do_display_addon_list ? ADDON_LIST_DO_DISPLAY : 0));
            HTTP_Common::print_speed_list (prefix + "sp" + saidx, speed, do_display_speed);
            HTTP_Common::print_fwd_list (prefix + "fwd" + saidx, fwd, do_display_fwd);
            HTTP_Common::print_loco_function_list (prefix + "f" + saidx, f, do_display_f);
            HTTP_Common::print_loco_macro_list (prefix + "lm" + saidx, m, do_display_lm);
            HTTP_Common::print_railroad_list (prefix + "rr" + saidx, (rrgidx << 8) | rridx, false, do_display_rr);
            HTTP_Common::print_railroad_group_list (prefix + "rrg" + saidx, rrgidx, false, do_display_rrg);
            HTTP_Common::print_s88_list (prefix + "co" + saidx, coidx, do_display_s88);
            HTTP_Common::print_tenths_list (prefix + "te" + saidx, tenths, do_display_tenths);
            HTTP_Common::print_railroad_group_list (prefix + "dest" + saidx, destidx, true, do_display_dest);
            HTTP_Common::print_led_group_list (prefix + "led" + saidx, led_group_idx, do_display_led);
            HTTP_Common::print_led_mask_list (prefix + "ledmask" + saidx, ledmask, do_display_led);
            HTTP_Common::print_ledon_list (prefix + "ledon" + saidx, ledon, do_display_led);
            HTTP_Common::print_switch_list (prefix + "sw" + saidx, swidx, do_display_sw);
            HTTP_Common::print_switch_state_list (prefix + "swst" + saidx, swstate, do_display_sw);
            HTTP_Common::print_signal_list (prefix + "sig" + saidx, sigidx, do_display_sig);
            HTTP_Common::print_signal_state_list (prefix + "sigst" + saidx, sigstate, do_display_sig);

            HTTP::response += (String) "</span>\r\n";
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
    std::string     rcl_name            = RCL::tracks[trackidx].get_name();
    uint32_t        flags               = RCL::tracks[trackidx].get_flags();
    String          yes_selected        = "";
    String          no_selected         = "";

    HTTP_Common::html_header (browsertitle, title, url, true);
    HTTP::response += (String) "<div style='margin-left:20px;'>\r\n";
    HTTP_Common::add_action_handler ("head", "", 200, true);

    HTTP::response += (String)
        "<script>\r\n"
        "function changeca(prefix, caidx)\r\n"
        "{\r\n"
        "  var obj = document.getElementById(prefix + 'ac' + caidx);\r\n"
        "  var st = 'none';\r\n"
        "  var ll = 'none';\r\n"
        "  var ad = 'none';\r\n"
        "  var sp = 'none';\r\n"
        "  var co = 'none';\r\n"
        "  var fwd = 'none';\r\n"
        "  var f = 'none';\r\n"
        "  var lm = 'none';\r\n"
        "  var rrg = 'none';\r\n"
        "  var rr = 'none';\r\n"
        "  var te = 'none';\r\n"
        "  var dest = 'none';\r\n"
        "  var led = 'none';\r\n"
        "  var sw = 'none';\r\n"
        "  var sig = 'none';\r\n"
        "  switch (parseInt(obj.value))\r\n"
        "  {\r\n"
        "    case " + std::to_string(RCL_ACTION_NONE)                           + ": { break; }\r\n"
        "    case " + std::to_string(RCL_ACTION_SET_LOCO_SPEED)                 + ": { st = ''; ll = ''; sp = ''; te = ''; break; }\r\n"
        "    case " + std::to_string(RCL_ACTION_SET_LOCO_MIN_SPEED)             + ": { st = ''; ll = ''; sp = ''; te = ''; break; }\r\n"
        "    case " + std::to_string(RCL_ACTION_SET_LOCO_MAX_SPEED)             + ": { st = ''; ll = ''; sp = ''; te = ''; break; }\r\n"
        "    case " + std::to_string(RCL_ACTION_SET_LOCO_FORWARD_DIRECTION)     + ": { st = ''; ll = ''; fwd = ''; break; }\r\n"
        "    case " + std::to_string(RCL_ACTION_SET_LOCO_ALL_FUNCTIONS_OFF)     + ": { st = ''; ll = ''; break; }\r\n"
        "    case " + std::to_string(RCL_ACTION_SET_LOCO_FUNCTION_OFF)          + ": { st = ''; ll = ''; f = ''; break; }\r\n"
        "    case " + std::to_string(RCL_ACTION_SET_LOCO_FUNCTION_ON)           + ": { st = ''; ll = ''; f = ''; break; }\r\n"
        "    case " + std::to_string(RCL_ACTION_EXECUTE_LOCO_MACRO)             + ": { st = ''; ll = ''; lm = ''; break; }\r\n"
        "    case " + std::to_string(RCL_ACTION_SET_ADDON_ALL_FUNCTIONS_OFF)    + ": { st = ''; ad = ''; break; }\r\n"
        "    case " + std::to_string(RCL_ACTION_SET_ADDON_FUNCTION_OFF)         + ": { st = ''; ad = ''; f = ''; break; }\r\n"
        "    case " + std::to_string(RCL_ACTION_SET_ADDON_FUNCTION_ON)          + ": { st = ''; ad = ''; f = ''; break; }\r\n"
        "    case " + std::to_string(RCL_ACTION_SET_RAILROAD)                   + ": { rr = ''; break; }\r\n"
        "    case " + std::to_string(RCL_ACTION_SET_FREE_RAILROAD)              + ": { rrg = ''; break; }\r\n"
        "    case " + std::to_string(RCL_ACTION_SET_LINKED_RAILROAD)            + ": { rrg = ''; break; }\r\n"
        "    case " + std::to_string(RCL_ACTION_WAIT_FOR_FREE_S88_CONTACT)      + ": { st = ''; co = ''; te = ''; break; }\r\n"
        "    case " + std::to_string(RCL_ACTION_SET_LOCO_DESTINATION)           + ": { ll = ''; dest = ''; break; }\r\n"
        "    case " + std::to_string(RCL_ACTION_SET_LED)                        + ": { st = ''; led = ''; break; }\r\n"
        "    case " + std::to_string(RCL_ACTION_SET_SWITCH)                     + ": { st = ''; sw = ''; break; }\r\n"
        "    case " + std::to_string(RCL_ACTION_SET_SIGNAL)                     + ": { st = ''; sig = ''; break; }\r\n"
        "  }\r\n"
        "  document.getElementById(prefix + 'st' + caidx).style.display = st;\r\n"
        "  document.getElementById(prefix + 'll' + caidx).style.display = ll;\r\n"
        "  document.getElementById(prefix + 'ad' + caidx).style.display = ad;\r\n"
        "  document.getElementById(prefix + 'sp' + caidx).style.display = sp;\r\n"
        "  document.getElementById(prefix + 'co' + caidx).style.display = co;\r\n"
        "  document.getElementById(prefix + 'fwd' + caidx).style.display = fwd;\r\n"
        "  document.getElementById(prefix + 'f' + caidx).style.display = f;\r\n"
        "  document.getElementById(prefix + 'lm' + caidx).style.display = lm;\r\n"
        "  document.getElementById(prefix + 'rrg' + caidx).style.display = rrg;\r\n"
        "  document.getElementById(prefix + 'rr' + caidx).style.display = rr;\r\n"
        "  document.getElementById(prefix + 'te' + caidx).style.display = te;\r\n"
        "  document.getElementById(prefix + 'dest' + caidx).style.display = dest;\r\n"
        "  document.getElementById(prefix + 'led' + caidx).style.display = led;\r\n"
        "  document.getElementById(prefix + 'ledmask' + caidx).style.display = led;\r\n"
        "  document.getElementById(prefix + 'ledon' + caidx).style.display = led;\r\n"
        "  document.getElementById(prefix + 'sw' + caidx).style.display = sw;\r\n"
        "  document.getElementById(prefix + 'swst' + caidx).style.display = sw;\r\n"
        "  document.getElementById(prefix + 'sig' + caidx).style.display = sig;\r\n"
        "  document.getElementById(prefix + 'sigst' + caidx).style.display = sig;\r\n"
        "}\r\n"
        "function changecc(prefix, caidx)\r\n"
        "{\r\n"
        "  var ccobj = document.getElementById(prefix + 'cc' + caidx);\r\n"
        "  var ctobj = document.getElementById(prefix + 'ct' + caidx);\r\n"
        "  var cdobj = document.getElementById(prefix + 'cd' + caidx);\r\n"
        "  var dest = 'none';\r\n"
        "  var cont = '';\r\n"
        "  switch (parseInt(ccobj.value))\r\n"
        "  {\r\n"
        "    case " + std::to_string(RCL_CONDITION_NEVER)               + ": { cont = 'none'; break; }\r\n"
        "    case " + std::to_string(RCL_CONDITION_IF_DESTINATION)      + ": { dest = ''; break; }\r\n"
        "    case " + std::to_string(RCL_CONDITION_IF_NOT_DESTINATION)  + ": { dest = ''; break; }\r\n"
        "  }\r\n"
        "  cdobj.style.display = dest;\r\n"
        "  ctobj.style.display = cont;\r\n"
        "  changeca(prefix, caidx)\r\n"
        "}\r\n"
        "</script>\r\n";

    HTTP::flush ();

    if (flags & RCL_TRACK_FLAG_BLOCK_PROTECTION)
    {
        yes_selected    = "selected";
    }
    else
    {
        no_selected     = "selected";
    }

    HTTP_Common::print_script_led_mask_list ();
    HTTP::flush ();

    HTTP::response += (String) "<form method='get' action='" + url + "'>"
                + "<table style='border:1px lightgray solid;'>\r\n"
                + "<tr bgcolor='#e0e0e0'><td>ID:</td><td>" + std::to_string(trackidx) + "</td></tr>\r\n"
                + "<tr><td>Name:</td><td><input type='text' style='width:200px' name='name' value='" + rcl_name + "'></td></tr>\r\n"
                + "<tr><td>Blocksicherung:</td><td><select name='block'><option value='0' " + no_selected + ">Nein</option><option value='1' " + yes_selected + ">Ja</option></select></td></tr>\r\n";

    HTTP::flush ();

    HTTP::response += (String) "<tr><td colspan='2'>Aktionen bei Einfahrt:</td></tr>";
    print_rcl_actions (true, trackidx);
    HTTP::response += (String) "<tr><td colspan='2'>Aktionen bei Ausfahrt:</td></tr>";
    print_rcl_actions (false, trackidx);

    HTTP::response += (String)
        "<tr><td><BR></td></tr>\r\n"
        "<tr><td><input type='button'  onclick=\"window.location.href='" + url + "';\" value='Zur&uuml;ck'></td>"
        "<td align='right'><input type='hidden' name='action' value='changercl'><input type='hidden' name='trackidx' value='" + strackidx + "'>"
        "<button >Speichern</button></td></tr>"
        "</table>\r\n"
        "</form>\r\n"
        "</div>\r\n";
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

    n_track_actions = RCL::tracks[trackidx].get_n_track_actions (in);

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
        String sidx = std::to_string(idx);

        uint_fast8_t    cc                  = HTTP::parameter_number (prefix + "cc"         + sidx);    // condition
        uint_fast8_t    cd                  = HTTP::parameter_number (prefix + "cd"         + sidx);    // condition_destination
        uint_fast8_t    st                  = HTTP::parameter_number (prefix + "st"         + sidx);
        uint_fast16_t   ll                  = HTTP::parameter_number (prefix + "ll"         + sidx);
        uint_fast16_t   ad                  = HTTP::parameter_number (prefix + "ad"         + sidx);
        uint_fast8_t    ac                  = HTTP::parameter_number (prefix + "ac"         + sidx);
        uint_fast8_t    sp                  = HTTP::parameter_number (prefix + "sp"         + sidx);
        uint_fast8_t    co                  = HTTP::parameter_number (prefix + "co"         + sidx);
        uint_fast8_t    te                  = HTTP::parameter_number (prefix + "te"         + sidx);
        uint_fast8_t    fwd                 = HTTP::parameter_number (prefix + "fwd"        + sidx);
        uint_fast8_t    f                   = HTTP::parameter_number (prefix + "f"          + sidx);
        uint_fast8_t    lm                  = HTTP::parameter_number (prefix + "lm"         + sidx);
        uint_fast8_t    rrg                 = HTTP::parameter_number (prefix + "rrg"        + sidx);
        uint_fast8_t    rr                  = HTTP::parameter_number (prefix + "rr"         + sidx);
        uint_fast8_t    dest                = HTTP::parameter_number (prefix + "dest"       + sidx);
        uint_fast16_t   led_group_idx       = HTTP::parameter_number (prefix + "led"        + sidx);
        uint_fast8_t    ledmask             = HTTP::parameter_number (prefix + "ledmask"    + sidx);
        uint_fast8_t    ledon               = HTTP::parameter_number (prefix + "ledon"      + sidx);
        uint_fast16_t   swidx               = HTTP::parameter_number (prefix + "sw"         + sidx);
        uint_fast16_t   swstate             = HTTP::parameter_number (prefix + "swst"       + sidx);
        uint_fast16_t   sigidx              = HTTP::parameter_number (prefix + "sig"        + sidx);
        uint_fast16_t   sigstate            = HTTP::parameter_number (prefix + "sigst"      + sidx);
        uint_fast8_t    pidx                = 0;

        track_action.condition              = cc;
        track_action.condition_destination  = cd;
        track_action.action                 = ac;

        switch (ac)
        {
            case RCL_ACTION_NONE:                                                       // no parameters
            {
                track_action.n_parameters = 0;
                break;
            }
            case RCL_ACTION_SET_LOCO_SPEED:                                             // parameters: START, LOCO, SPEED, TENTHS
            {
                track_action.parameters[pidx++] = st;
                track_action.parameters[pidx++] = ll;
                track_action.parameters[pidx++] = sp;
                track_action.parameters[pidx++] = te;
                track_action.n_parameters       = pidx;
                break;
            }
            case RCL_ACTION_SET_LOCO_MIN_SPEED:                                         // parameters: START, LOCO, SPEED, TENTHS
            {
                track_action.parameters[pidx++] = st;
                track_action.parameters[pidx++] = ll;
                track_action.parameters[pidx++] = sp;
                track_action.parameters[pidx++] = te;
                track_action.n_parameters       = pidx;
                break;
            }
            case RCL_ACTION_SET_LOCO_MAX_SPEED:                                         // parameters: START, LOCO, SPEED, TENTHS
            {
                track_action.parameters[pidx++] = st;
                track_action.parameters[pidx++] = ll;
                track_action.parameters[pidx++] = sp;
                track_action.parameters[pidx++] = te;
                track_action.n_parameters       = pidx;
                break;
            }
            case RCL_ACTION_SET_LOCO_FORWARD_DIRECTION:                                 // parameters: START, LOCO, FWD
            {
                track_action.parameters[pidx++] = st;
                track_action.parameters[pidx++] = ll;
                track_action.parameters[pidx++] = fwd;
                track_action.n_parameters       = pidx;
                break;
            }
            case RCL_ACTION_SET_LOCO_ALL_FUNCTIONS_OFF:                                 // parameters: START, LOCO
            {
                track_action.parameters[pidx++] = st;
                track_action.parameters[pidx++] = ll;
                track_action.n_parameters       = pidx;
                break;
            }
            case RCL_ACTION_SET_LOCO_FUNCTION_OFF:                                      // parameters: START, LOCO, FUNC_IDX
            {
                track_action.parameters[pidx++] = st;
                track_action.parameters[pidx++] = ll;
                track_action.parameters[pidx++] = f;
                track_action.n_parameters       = pidx;
                break;
            }
            case RCL_ACTION_SET_LOCO_FUNCTION_ON:                                       // parameters: START, LOCO, FUNC_IDX
            {
                track_action.parameters[pidx++] = st;
                track_action.parameters[pidx++] = ll;
                track_action.parameters[pidx++] = f;
                track_action.n_parameters       = pidx;
                break;
            }
            case RCL_ACTION_EXECUTE_LOCO_MACRO:                                         // parameters: START, LOCO, MACRO_IDX
            {
                track_action.parameters[pidx++] = st;
                track_action.parameters[pidx++] = ll;
                track_action.parameters[pidx++] = lm;
                track_action.n_parameters       = pidx;
                break;
            }
            case RCL_ACTION_SET_ADDON_ALL_FUNCTIONS_OFF:                                // parameters: START, LOCO
            {
                track_action.parameters[pidx++] = st;
                track_action.parameters[pidx++] = ad;
                track_action.n_parameters       = pidx;
                break;
            }
            case RCL_ACTION_SET_ADDON_FUNCTION_OFF:                                     // parameters: START, LOCO, FUNC_IDX
            {
                track_action.parameters[pidx++] = st;
                track_action.parameters[pidx++] = ad;
                track_action.parameters[pidx++] = f;
                track_action.n_parameters       = pidx;
                break;
            }
            case RCL_ACTION_SET_ADDON_FUNCTION_ON:                                      // parameters: START, LOCO, FUNC_IDX
            {
                track_action.parameters[pidx++] = st;
                track_action.parameters[pidx++] = ad;
                track_action.parameters[pidx++] = f;
                track_action.n_parameters       = pidx;
                break;
            }
            case RCL_ACTION_SET_RAILROAD:                                               // parameters: RRG_IDX, RR_IDX
            {
                track_action.parameters[pidx++] = rrg;
                track_action.parameters[pidx++] = rr;
                track_action.n_parameters       = pidx;
                break;
            }
            case RCL_ACTION_SET_FREE_RAILROAD:                                          // parameters: RRG_IDX
            {
                track_action.parameters[pidx++] = rrg;
                track_action.n_parameters       = pidx;
                break;
            }
            case RCL_ACTION_SET_LINKED_RAILROAD:                                        // parameters: RRG_IDX
            {
                track_action.parameters[pidx++] = rrg;
                track_action.n_parameters       = pidx;
                break;
            }
            case RCL_ACTION_WAIT_FOR_FREE_S88_CONTACT:                                  // parameters: START, LOCO, COIDX, TENTHS
            {
                track_action.parameters[pidx++] = st;
                track_action.parameters[pidx++] = ll;
                track_action.parameters[pidx++] = co;
                track_action.parameters[pidx++] = te;
                track_action.n_parameters       = pidx;
                break;
            }
            case RCL_ACTION_SET_LOCO_DESTINATION:                                       // parameters: LOCO_IDX, RRG_IDX
            {
                track_action.parameters[pidx++] = ll;
                track_action.parameters[pidx++] = dest;
                track_action.n_parameters       = pidx;
                break;
            }
            case RCL_ACTION_SET_LED:                                                    // parameters: START, LG_IDX, LED_MASK, LED_STATE
            {
                track_action.parameters[pidx++] = st;
                track_action.parameters[pidx++] = led_group_idx;
                track_action.parameters[pidx++] = ledmask;
                track_action.parameters[pidx++] = ledon;
                track_action.n_parameters       = pidx;
                break;
            }
            case RCL_ACTION_SET_SWITCH:                                                 // parameters: START, SW_IDX, SW_STATE
            {
                track_action.parameters[pidx++] = st;
                track_action.parameters[pidx++] = swidx;
                track_action.parameters[pidx++] = swstate;
                track_action.n_parameters       = pidx;
                break;
            }
            case RCL_ACTION_SET_SIGNAL:                                                 // parameters: START, SIG_IDX, SIG_STATE
            {
                track_action.parameters[pidx++] = st;
                track_action.parameters[pidx++] = sigidx;
                track_action.parameters[pidx++] = sigstate;
                track_action.n_parameters       = pidx;
                break;
            }
        }

        if (cc != RCL_CONDITION_NEVER && ac != RCL_ACTION_NONE)
        {
            if (slotidx >= n_track_actions)
            {
                RCL::tracks[trackidx].add_track_action (in);
                n_track_actions = RCL::tracks[trackidx].get_n_track_actions (in);
            }

            RCL::tracks[trackidx].set_track_action (in, slotidx, &track_action);
            slotidx++;
        }
    }

    while (slotidx < n_track_actions)
    {
        RCL::tracks[trackidx].delete_track_action (in, slotidx);
        n_track_actions = RCL::tracks[trackidx].get_n_track_actions (in);
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
        std::string     sname = HTTP::parameter ("name");

        trackidx = RCL::add({});

        if (trackidx != 0xFF)
        {
            RCL::tracks[trackidx].set_name(sname);
        }
    }
    else if (! strcmp (action, "changercl"))
    {
        String  sname   = HTTP::parameter ("name");
        int     block   = atoi (HTTP::parameter ("block"));

        trackidx = HTTP::parameter_number ("trackidx");

        RCL::tracks[trackidx].set_name (sname);

        if (block)
        {
            RCL::tracks[trackidx].set_flags (RCL_TRACK_FLAG_BLOCK_PROTECTION);
        }
        else
        {
            RCL::tracks[trackidx].reset_flags (RCL_TRACK_FLAG_BLOCK_PROTECTION);
        }

        change_rcl_actions (trackidx, true);
        change_rcl_actions (trackidx, false);
    }

    const char *  bg;

    HTTP_Common::add_action_handler ("rcl", "", 200, true);

    HTTP::response += (String)
        "<table style='border:1px lightgray solid;white-space:nowrap;'>\r\n"
        "<tr bgcolor='#e0e0e0'><th>ID</th><th style='width:240px' nowrap>Bezeichnung</th><th style='width:200px'>Erkannte Lok</th>";

    if (HTTP_Common::edit_mode)
    {
        HTTP::response += (String) "<th>Aktion</th>";
    }

    HTTP::response += (String) "</tr>\r\n";

    HTTP::flush ();

    uint_fast16_t   n_tracks    = RCL::get_n_tracks ();

    for (trackidx = 0; trackidx < n_tracks; trackidx++)
    {
        String          strackidx = std::to_string(trackidx);
        std::string     loconame;
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

        if (loco_idx != 0xFFFF)
        {
            loconame = Locos::locos[loco_idx].get_name();
        }
        else
        {
            loconame = "";
        }

        HTTP::response += (String) "<tr " + bg + "><td id='t" + strackidx + "' align='right'>&nbsp;"
                  + strackidx + "&nbsp;</td><td nowrap>"
                  + RCL::tracks[trackidx].get_name()
                  + "</td><td id='loc" + strackidx
                  + "' style='max-width:200px;overflow:hidden' nowrap>" + loconame + "</td>";

        if (HTTP_Common::edit_mode)
        {
            HTTP::response += (String) "<td><button onclick=\"window.location.href='" + urledit + "?trackidx=" + strackidx + "';\">Bearbeiten</button></td>";
        }

        HTTP::response += (String) "</tr>\r\n";
    }

    HTTP::response += (String) "</table>\r\n";
    HTTP::flush ();

    HTTP::response += (String)
        "<script>\r\n"
        "function newrcl()\r\n"
        "{\r\n"
        "  document.getElementById('action').value = 'newrcl';\r\n"
        "  document.getElementById('name').value = '';\r\n"
        "  document.getElementById('formrcl').style.display='';\r\n"
        "}\r\n"
        "</script>\r\n";

    HTTP::flush ();

    if (RCL::data_changed)
    {
        HTTP::response += (String)
            "<BR><form method='get' action='" + url + "'>\r\n"
        "<input type='hidden' name='action' value='savercl'>\r\n"
        "<input type='submit' value='Ge&auml;nderte Konfiguration auf Datentr&auml;ger speichern'>\r\n"
        "</form>\r\n";
    }

    if (HTTP_Common::edit_mode)
    {
        HTTP::response += (String)
            "<BR><button onclick='newrcl()'>Neuer RCL-Kontakt</button>\r\n"
            "<form id='formrcl' style='display:none'>\r\n"
            "<table>\r\n"
            "<tr><td>Name:</td><td><input type='text' id='name' name='name'></td></tr>\r\n"
            "<tr><td></td><td align='right'>"
            "<input type='hidden' name='action' id='action' value='newrcl'><input type='hidden' name='trackidx' id='trackidx'><input type='submit' value='Speichern'></td></tr>\r\n"
            "</table>\r\n"
            "</form>\r\n";
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
        std::string     loconame;
        const char *    color;
        const char *    bgcolor;
        uint_fast16_t   loco_idx;

        loco_idx = RCL::tracks[trackidx].loco_idx;

        if (loco_idx != 0xFFFF)
        {
            loconame    = Locos::locos[loco_idx].get_name ();
            color       = "white";
            bgcolor     = "green";
        }
        else
        {
            loconame    = "";
            color       = "";
            bgcolor     = "";
        }

        HTTP_Common::add_action_content ( (String) "t" + std::to_string(trackidx), "color", color);
        HTTP_Common::add_action_content ( (String) "t" + std::to_string(trackidx), "bgcolor", bgcolor);
        HTTP_Common::add_action_content ( (String) "loc" + std::to_string(trackidx), "text", loconame);
    }
}
