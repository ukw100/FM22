/*------------------------------------------------------------------------------------------------------------------------
 * http-s88.cc - HTTP s88 routines
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
            HTTP::response += (String) "Setze Geschwindigkeit";
            break;
        }

        case S88_ACTION_SET_LOCO_MIN_SPEED:                                         // parameters: START, LOCO_IDX, SPEED, TENTHS
        {
            HTTP::response += (String) "Setze Mindestgeschwindigkeit";
            break;
        }

        case S88_ACTION_SET_LOCO_MAX_SPEED:                                         // parameters: START, LOCO_IDX, SPEED, TENTHS
        {
            HTTP::response += (String) "Setze H&ouml;chstgeschwindigkeit";
            break;
        }

        case S88_ACTION_SET_LOCO_FORWARD_DIRECTION:                                 // parameters: START, LOCO_IDX, FWD
        {
            HTTP::response += (String) "Setze Richtung";
            break;
        }

        case S88_ACTION_SET_LOCO_ALL_FUNCTIONS_OFF:                                 // parameters: START, LOCO_IDX
        {
            HTTP::response += (String) "Schalte alle Funktionen aus";
            break;
        }

        case S88_ACTION_SET_LOCO_FUNCTION_OFF:                                      // parameters: START, LOCO_IDX, FUNC_IDX
        {
            HTTP::response += (String) "Schalte Funktion aus";
            break;
        }

        case S88_ACTION_SET_LOCO_FUNCTION_ON:                                       // parameters: START, LOCO_IDX, FUNC_IDX
        {
            HTTP::response += (String) "Schalte Funktion ein";
            break;
        }

        case S88_ACTION_EXECUTE_LOCO_MACRO:                                         // parameters: START, LOCO_IDX, MACRO_IDX
        {
            HTTP::response += (String) "F&uuml;hre Lok-Macro aus";
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

        case S88_ACTION_SET_LOCO_DESTINATION:                                       // parameters: LOCO_IDX, RRG_IDX
        {
            HTTP::response += (String) "Setze Ziel";
            break;
        }

        case S88_ACTION_SET_LED:                                                    // parameters: START, LG_IDX, LED_MASK, LED_ON
        {
            HTTP::response += (String) "Schalte LED";
            break;
        }

        case S88_ACTION_SET_SWITCH:                                                 // parameters: START, SW_IDX, SW_STATE
        {
            HTTP::response += (String) "Schalte Weiche";
            break;
        }

        case S88_ACTION_SET_SIGNAL:                                                 // parameters: START, SIG_IDX, SIG_STATE
        {
            HTTP::response += (String) "Schalte Signal";
            break;
        }
    }

    HTTP::response += (String) "</option>\r\n";
}

static void
print_s88_action_list (bool in, uint_fast8_t caidx, uint_fast8_t action)
{
    uint_fast8_t    i;
    String          scaidx  = std::to_string (caidx);

    if (in)
    {
        HTTP::response += (String) "<select name='iac" + scaidx + "' id='iac" + scaidx + "' onchange=\"changeca('i'," + scaidx + ")\">\r\n";
    }
    else
    {
        HTTP::response += (String) "<select name='oac" + scaidx + "' id='oac" + scaidx + "' onchange=\"changeca('o'," + scaidx + ")\">\r\n";
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

    n_contact_actions = S88::contacts[coidx].get_n_contact_actions (in);

    for (caidx = 0; caidx < S88_MAX_ACTIONS_PER_CONTACT; caidx++)
    {
        String              scaidx = std::to_string(caidx);
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

        HTTP::response += (String) "<tr><td>Aktion " + scaidx + ":</td><td>";

        if (caidx < n_contact_actions)
        {
            rtc = S88::contacts[coidx].get_contact_action (in, caidx, &ca);

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
            uint_fast8_t    destidx                 = 0xFF;
            uint_fast8_t    rridx                   = 0xFF;
            uint_fast8_t    fwd                     = 1;
            uint_fast8_t    f                       = 0;
            uint_fast8_t    lm                      = 0;
            uint_fast16_t   tenths                  = 0;
            uint_fast16_t   swidx                   = 0xFFFF;
            uint_fast8_t    swstate                 = DCC_SWITCH_STATE_UNDEFINED;
            uint_fast16_t   sigidx                  = 0xFFFF;
            uint_fast8_t    sigstate                = DCC_SIGNAL_STATE_UNDEFINED;
            uint_fast16_t   led_group_idx           = 0xFFFF;
            uint_fast8_t    ledmask                 = 0x00;
            uint_fast8_t    ledon                   = 0;
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
            uint_fast8_t    do_display_dest         = false;
            uint_fast8_t    do_display_led          = false;
            uint_fast8_t    do_display_sw           = false;
            uint_fast8_t    do_display_sig          = false;
            uint_fast8_t    pidx                    = 0;

            switch (ca.action)
            {
                case S88_ACTION_NONE:                                                       // no parameters
                {
                    break;
                }
                case S88_ACTION_SET_LOCO_SPEED:                                             // parameters: START, LOCO_IDX, SPEED
                {
                    start_tenths            = cap[caidx].parameters[pidx++];
                    do_display_start_tenths = true;
                    loco_idx                = cap[caidx].parameters[pidx++];
                    do_display_loco_list    = true;
                    speed                   = cap[caidx].parameters[pidx++];
                    do_display_speed        = true;
                    tenths                  = cap[caidx].parameters[pidx++];
                    do_display_tenths       = true;
                    break;
                }

                case S88_ACTION_SET_LOCO_MIN_SPEED:                                         // parameters: START, LOCO_IDX, SPEED
                {
                    start_tenths            = cap[caidx].parameters[pidx++];
                    do_display_start_tenths = true;
                    loco_idx                = cap[caidx].parameters[pidx++];
                    do_display_loco_list    = true;
                    speed                   = cap[caidx].parameters[pidx++];
                    do_display_speed        = true;
                    tenths                  = cap[caidx].parameters[pidx++];
                    do_display_tenths       = true;
                    break;
                }

                case S88_ACTION_SET_LOCO_MAX_SPEED:                                         // parameters: START, LOCO_IDX, SPEED
                {
                    start_tenths            = cap[caidx].parameters[pidx++];
                    do_display_start_tenths = true;
                    loco_idx                = cap[caidx].parameters[pidx++];
                    do_display_loco_list    = true;
                    speed                   = cap[caidx].parameters[pidx++];
                    do_display_speed        = true;
                    tenths                  = cap[caidx].parameters[pidx++];
                    do_display_tenths       = true;
                    break;
                }

                case S88_ACTION_SET_LOCO_FORWARD_DIRECTION:                                 // parameters: START, LOCO_IDX, FWD
                {
                    start_tenths            = cap[caidx].parameters[pidx++];
                    do_display_start_tenths = true;
                    loco_idx                = cap[caidx].parameters[pidx++];
                    do_display_loco_list    = true;
                    fwd                     = cap[caidx].parameters[pidx++];
                    do_display_fwd          = true;
                    break;
                }

                case S88_ACTION_SET_LOCO_ALL_FUNCTIONS_OFF:                                 // parameters: START, LOCO_IDX, TENTHS
                {
                    start_tenths            = cap[caidx].parameters[pidx++];
                    do_display_start_tenths = true;
                    loco_idx                = cap[caidx].parameters[pidx++];
                    do_display_loco_list    = true;
                    break;
                }

                case S88_ACTION_SET_LOCO_FUNCTION_OFF:                                      // parameters: START, LOCO_IDX, FUNC_IDX, TENTHS
                {
                    start_tenths            = cap[caidx].parameters[pidx++];
                    do_display_start_tenths = true;
                    loco_idx                = cap[caidx].parameters[pidx++];
                    do_display_loco_list    = true;
                    f                       = cap[caidx].parameters[pidx++];
                    do_display_f            = true;
                    break;
                }

                case S88_ACTION_SET_LOCO_FUNCTION_ON:                                        // parameters: START, LOCO_IDX, FUNC_IDX, TENTHS
                {
                    start_tenths            = cap[caidx].parameters[pidx++];
                    do_display_start_tenths = true;
                    loco_idx                = cap[caidx].parameters[pidx++];
                    do_display_loco_list    = true;
                    f                       = cap[caidx].parameters[pidx++];
                    do_display_f            = true;
                    break;
                }

                case S88_ACTION_EXECUTE_LOCO_MACRO:                                         // parameters: START, LOCO_IDX, FUNC_IDX, TENTHS
                {
                    start_tenths            = cap[caidx].parameters[pidx++];
                    do_display_start_tenths = true;
                    loco_idx                = cap[caidx].parameters[pidx++];
                    do_display_loco_list    = true;
                    lm                      = cap[caidx].parameters[pidx++];
                    do_display_lm           = true;
                    break;
                }

                case S88_ACTION_SET_ADDON_ALL_FUNCTIONS_OFF:                                 // parameters: START, ADDON_IDX, TENTHS
                {
                    start_tenths            = cap[caidx].parameters[pidx++];
                    do_display_start_tenths = true;
                    addon_idx               = cap[caidx].parameters[pidx++];
                    do_display_addon_list   = true;
                    break;
                }

                case S88_ACTION_SET_ADDON_FUNCTION_OFF:                                      // parameters: START, ADDON_IDX, FUNC_IDX, TENTHS
                {
                    start_tenths            = cap[caidx].parameters[pidx++];
                    do_display_start_tenths = true;
                    addon_idx               = cap[caidx].parameters[pidx++];
                    do_display_addon_list   = true;
                    f                       = cap[caidx].parameters[pidx++];
                    do_display_f            = true;
                    break;
                }

                case S88_ACTION_SET_ADDON_FUNCTION_ON:                                       // parameters: START, ADDON_IDX, FUNC_IDX, TENTHS
                {
                    start_tenths            = cap[caidx].parameters[pidx++];
                    do_display_start_tenths = true;
                    addon_idx               = cap[caidx].parameters[pidx++];
                    do_display_addon_list   = true;
                    f                       = cap[caidx].parameters[pidx++];
                    do_display_f            = true;
                    break;
                }

                case S88_ACTION_SET_RAILROAD:                                               // parameters: RRG_IDX, RR_IDX
                {
                    rrgidx                  = cap[caidx].parameters[pidx++];
                    rridx                   = cap[caidx].parameters[pidx++];
                    do_display_rr           = true;
                    break;
                }

                case S88_ACTION_SET_FREE_RAILROAD:                                          // parameters: RRG_IDX
                {
                    rrgidx                  = cap[caidx].parameters[pidx++];
                    do_display_rrg          = true;
                    break;
                }

                case S88_ACTION_SET_LOCO_DESTINATION:                                       // parameters: LOCO_IDX, RRG_IDX
                {
                    loco_idx                = cap[caidx].parameters[pidx++];
                    do_display_loco_list    = true;
                    destidx                 = cap[caidx].parameters[pidx++];
                    do_display_dest         = true;
                    break;
                }

                case S88_ACTION_SET_LED:                                                    // parameters: START, LG_IDX, LED_MASK, LED_ON
                {
                    start_tenths            = cap[caidx].parameters[pidx++];
                    do_display_start_tenths = true;
                    led_group_idx           = cap[caidx].parameters[pidx++];
                    ledmask                 = cap[caidx].parameters[pidx++];
                    ledon                   = cap[caidx].parameters[pidx++];
                    do_display_led          = true;
                    break;
                }

                case S88_ACTION_SET_SWITCH:                                                // parameters: START, SW_IDX, SW_STATE
                {
                    start_tenths            = cap[caidx].parameters[pidx++];
                    do_display_start_tenths = true;
                    swidx                   = cap[caidx].parameters[pidx++];
                    swstate                 = cap[caidx].parameters[pidx++];
                    do_display_sw           = true;
                    break;
                }

                case S88_ACTION_SET_SIGNAL:                                                // parameters: START, SIG_IDX, SIG_STATE
                {
                    start_tenths            = cap[caidx].parameters[pidx++];
                    do_display_start_tenths = true;
                    sigidx                  = cap[caidx].parameters[pidx++];
                    sigstate                = cap[caidx].parameters[pidx++];
                    do_display_sig          = true;
                    break;
                }
            }

            print_s88_action_list (in, caidx, ca.action);
            HTTP_Common::print_start_list (prefix + "st" + scaidx, start_tenths, do_display_start_tenths);

            HTTP_Common::print_loco_select_list (prefix + "ll" + scaidx, loco_idx, LOCO_LIST_SHOW_LINKED_LOCO | LOCO_LIST_SHOW_DETECTED_LOCO | (do_display_loco_list ? LOCO_LIST_DO_DISPLAY : 0));
            HTTP_Common::print_addon_select_list (prefix + "ad" + scaidx, addon_idx, (do_display_addon_list ? ADDON_LIST_DO_DISPLAY : 0));
            HTTP_Common::print_speed_list (prefix + "sp" + scaidx, speed, do_display_speed);
            HTTP_Common::print_fwd_list (prefix + "fwd" + scaidx, fwd, do_display_fwd);
            HTTP_Common::print_loco_function_list (prefix + "f" + scaidx, f, do_display_f);
            HTTP_Common::print_loco_macro_list (prefix + "lm" + scaidx, lm, do_display_lm);
            HTTP_Common::print_railroad_list (prefix + "rr" + scaidx, (rrgidx << 8) | rridx, false, do_display_rr);
            HTTP_Common::print_railroad_group_list (prefix + "rrg" + scaidx, rrgidx, false, do_display_rrg);
            HTTP_Common::print_tenths_list (prefix + "te" + scaidx, tenths, do_display_tenths);
            HTTP_Common::print_railroad_group_list (prefix + "dest" + scaidx, destidx, true, do_display_dest);
            HTTP_Common::print_led_group_list (prefix + "led" + scaidx, led_group_idx, do_display_led);
            HTTP_Common::print_led_mask_list (prefix + "ledmask" + scaidx, ledmask, do_display_led);
            HTTP_Common::print_ledon_list (prefix + "ledon" + scaidx, ledon, do_display_led);
            HTTP_Common::print_switch_list (prefix + "sw" + scaidx, swidx, do_display_sw);
            HTTP_Common::print_switch_state_list (prefix + "swst" + scaidx, swstate, do_display_sw);
            HTTP_Common::print_signal_list (prefix + "sig" + scaidx, sigidx, do_display_sig);
            HTTP_Common::print_signal_state_list (prefix + "sigst" + scaidx, sigstate, do_display_sig);
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
    uint_fast16_t   linkedrr            = S88::contacts[coidx].get_link_railroad ();

    HTTP_Common::html_header (browsertitle, title, url, true);
    HTTP::response += (String) "<div style='margin-left:20px;'>\r\n";
    HTTP_Common::add_action_handler ("head", "", 200, true);

    std::string s88_name = S88::contacts[coidx].get_name();

    HTTP::response += (String)
        "<script>\r\n"
        "function changeca(prefix, caidx)\r\n"
        "{\r\n"
        "  var obj = document.getElementById(prefix + 'ac' + caidx);\r\n"
        "  var st = 'none';\r\n"
        "  var ll = 'none';\r\n"
        "  var ad = 'none';\r\n"
        "  var sp = 'none';\r\n"
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
        "    case " + std::to_string(S88_ACTION_NONE)                           + ": { break; }\r\n"
        "    case " + std::to_string(S88_ACTION_SET_LOCO_SPEED)                 + ": { st = ''; ll = ''; sp = ''; te = ''; break; }\r\n"
        "    case " + std::to_string(S88_ACTION_SET_LOCO_MIN_SPEED)             + ": { st = ''; ll = ''; sp = ''; te = ''; break; }\r\n"
        "    case " + std::to_string(S88_ACTION_SET_LOCO_MAX_SPEED)             + ": { st = ''; ll = ''; sp = ''; te = ''; break; }\r\n"
        "    case " + std::to_string(S88_ACTION_SET_LOCO_FORWARD_DIRECTION)     + ": { st = ''; ll = ''; fwd = ''; break; }\r\n"
        "    case " + std::to_string(S88_ACTION_SET_LOCO_ALL_FUNCTIONS_OFF)     + ": { st = ''; ll = ''; break; }\r\n"
        "    case " + std::to_string(S88_ACTION_SET_LOCO_FUNCTION_OFF)          + ": { st = ''; ll = ''; f = ''; break; }\r\n"
        "    case " + std::to_string(S88_ACTION_SET_LOCO_FUNCTION_ON)           + ": { st = ''; ll = ''; f = ''; break; }\r\n"
        "    case " + std::to_string(S88_ACTION_EXECUTE_LOCO_MACRO)             + ": { st = ''; ll = ''; lm = ''; break; }\r\n"
        "    case " + std::to_string(S88_ACTION_SET_ADDON_ALL_FUNCTIONS_OFF)    + ": { st = ''; ad = ''; break; }\r\n"
        "    case " + std::to_string(S88_ACTION_SET_ADDON_FUNCTION_OFF)         + ": { st = ''; ad = ''; f = ''; break; }\r\n"
        "    case " + std::to_string(S88_ACTION_SET_ADDON_FUNCTION_ON)          + ": { st = ''; ad = ''; f = ''; break; }\r\n"
        "    case " + std::to_string(S88_ACTION_SET_RAILROAD)                   + ": { rr = ''; break; }\r\n"
        "    case " + std::to_string(S88_ACTION_SET_FREE_RAILROAD)              + ": { rrg = ''; break; }\r\n"
        "    case " + std::to_string(S88_ACTION_SET_LOCO_DESTINATION)           + ": { ll = ''; dest = ''; break; }\r\n"
        "    case " + std::to_string(S88_ACTION_SET_LED)                        + ": { st = ''; led = ''; break; }\r\n"
        "    case " + std::to_string(S88_ACTION_SET_SWITCH)                     + ": { st = ''; sw = ''; break; }\r\n"
        "    case " + std::to_string(S88_ACTION_SET_SIGNAL)                     + ": { st = ''; sig = ''; break; }\r\n"
        "  }\r\n"
        "  document.getElementById(prefix + 'st' + caidx).style.display = st;\r\n"
        "  document.getElementById(prefix + 'll' + caidx).style.display = ll;\r\n"
        "  document.getElementById(prefix + 'ad' + caidx).style.display = ad;\r\n"
        "  document.getElementById(prefix + 'sp' + caidx).style.display = sp;\r\n"
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
        "</script>\r\n";

    HTTP::flush ();

    HTTP_Common::print_script_led_mask_list ();
    HTTP::flush ();

    HTTP::response += (String) "<form method='get' action='" + url + "'>"
                + "<table style='border:1px lightgray solid;'>\r\n"
                + "<tr bgcolor='#e0e0e0'><td>ID:</td><td>" + std::to_string(coidx) + "</td></tr>\r\n"
                + "<tr><td>Name:</td><td><input type='text' style='width:200px' name='name' value='" + s88_name + "'></td></tr>\r\n";

    HTTP::flush ();

    HTTP::response += (String) "<tr><td>Gleis:</td><td>\r\n";
    HTTP_Common::print_railroad_list ((String) "rr", linkedrr, true, true);
    HTTP::response += (String) "</td></tr>\r\n";
    HTTP::flush ();

    HTTP::response += (String) "<tr><td colspan='2'>Aktionen bei Einfahrt:</td></tr>";
    print_s88_actions (true, coidx);
    HTTP::response += (String) "<tr><td colspan='2'>Aktionen bei Ausfahrt:</td></tr>";
    print_s88_actions (false, coidx);

    HTTP::flush ();

    HTTP::response += (String)
        "<tr><td><BR></td></tr>\r\n"
        "<tr><td><input type='button'  onclick=\"window.location.href='" + url + "';\" value='Zur&uuml;ck'></td>"
        "<td align='right'><input type='hidden' name='action' value='changes88'><input type='hidden' name='coidx' value='" + scoidx + "'>"
        "<button >Speichern</button></td></tr>"
        "</table>\r\n"
        "</form>\r\n"
        "</div>\r\n";
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

    n_contact_actions = S88::contacts[coidx].get_n_contact_actions (in);

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
        String sidx = std::to_string(idx);

        uint_fast16_t   st              = HTTP::parameter_number (prefix + "st"         + sidx);
        uint_fast8_t    ac              = HTTP::parameter_number (prefix + "ac"         + sidx);
        uint_fast16_t   ll              = HTTP::parameter_number (prefix + "ll"         + sidx);
        uint_fast16_t   ad              = HTTP::parameter_number (prefix + "ad"         + sidx);
        uint_fast8_t    sp              = HTTP::parameter_number (prefix + "sp"         + sidx);
        uint_fast8_t    te              = HTTP::parameter_number (prefix + "te"         + sidx);
        uint_fast8_t    fwd             = HTTP::parameter_number (prefix + "fwd"        + sidx);
        uint_fast8_t    f               = HTTP::parameter_number (prefix + "f"          + sidx);
        uint_fast8_t    lm              = HTTP::parameter_number (prefix + "lm"         + sidx);
        uint_fast8_t    rrg             = HTTP::parameter_number (prefix + "rrg"        + sidx);
        uint_fast16_t   rr              = HTTP::parameter_number (prefix + "rr"         + sidx);
        uint_fast8_t    dest            = HTTP::parameter_number (prefix + "dest"       + sidx);
        uint_fast16_t   led_group_idx   = HTTP::parameter_number (prefix + "led"        + sidx);
        uint_fast8_t    ledmask         = HTTP::parameter_number (prefix + "ledmask"    + sidx);
        uint_fast8_t    ledon           = HTTP::parameter_number (prefix + "ledon"      + sidx);
        uint_fast16_t   swidx           = HTTP::parameter_number (prefix + "sw"         + sidx);
        uint_fast16_t   swstate         = HTTP::parameter_number (prefix + "swst"       + sidx);
        uint_fast16_t   sigidx          = HTTP::parameter_number (prefix + "sig"        + sidx);
        uint_fast16_t   sigstate        = HTTP::parameter_number (prefix + "sigst"      + sidx);
        uint_fast8_t    pidx            = 0;

        ca.action       = ac;

        switch (ac)
        {
            case S88_ACTION_NONE:                                                       // no parameters
            {
                ca.n_parameters         = pidx;
                break;
            }
            case S88_ACTION_SET_LOCO_SPEED:                                             // parameters: START, LOCO_IDX, SPEED, TENTHS
            {
                ca.parameters[pidx++]   = st;
                ca.parameters[pidx++]   = ll;
                ca.parameters[pidx++]   = sp;
                ca.parameters[pidx++]   = te;
                ca.n_parameters         = pidx;
                break;
            }
            case S88_ACTION_SET_LOCO_MIN_SPEED:                                         // parameters: START, LOCO_IDX, SPEED, TENTHS
            {
                ca.parameters[pidx++]   = st;
                ca.parameters[pidx++]   = ll;
                ca.parameters[pidx++]   = sp;
                ca.parameters[pidx++]   = te;
                ca.n_parameters         = pidx;
                break;
            }
            case S88_ACTION_SET_LOCO_MAX_SPEED:                                         // parameters: START, LOCO_IDX, SPEED, TENTHS
            {
                ca.parameters[pidx++]   = st;
                ca.parameters[pidx++]   = ll;
                ca.parameters[pidx++]   = sp;
                ca.parameters[pidx++]   = te;
                ca.n_parameters         = pidx;
                break;
            }
            case S88_ACTION_SET_LOCO_FORWARD_DIRECTION:                                 // parameters: START, LOCO_IDX, FWD
            {
                ca.parameters[pidx++]   = st;
                ca.parameters[pidx++]   = ll;
                ca.parameters[pidx++]   = fwd;
                ca.n_parameters         = pidx;
                break;
            }
            case S88_ACTION_SET_LOCO_ALL_FUNCTIONS_OFF:                                 // parameters: START, LOCO_IDX
            {
                ca.parameters[pidx++]   = st;
                ca.parameters[pidx++]   = ll;
                ca.n_parameters         = pidx;
                break;
            }
            case S88_ACTION_SET_LOCO_FUNCTION_OFF:                                      // parameters: START, LOCO_IDX, FUNC_IDX
            {
                ca.parameters[pidx++]   = st;
                ca.parameters[pidx++]   = ll;
                ca.parameters[pidx++]   = f;
                ca.n_parameters         = pidx;
                break;
            }
            case S88_ACTION_SET_LOCO_FUNCTION_ON:                                       // parameters: START, LOCO_IDX, FUNC_IDX
            {
                ca.parameters[pidx++]   = st;
                ca.parameters[pidx++]   = ll;
                ca.parameters[pidx++]   = f;
                ca.n_parameters         = pidx;
                break;
            }
            case S88_ACTION_EXECUTE_LOCO_MACRO:                                         // parameters: START, LOCO_IDX, MACRO_IDX
            {
                ca.parameters[pidx++]   = st;
                ca.parameters[pidx++]   = ll;
                ca.parameters[pidx++]   = lm;
                ca.n_parameters         = pidx;
                break;
            }
            case S88_ACTION_SET_ADDON_ALL_FUNCTIONS_OFF:                                // parameters: START, ADDON_IDX
            {
                ca.parameters[pidx++]   = st;
                ca.parameters[pidx++]   = ad;
                ca.n_parameters         = pidx;
                break;
            }
            case S88_ACTION_SET_ADDON_FUNCTION_OFF:                                     // parameters: START, ADDON_IDX, FUNC_IDX
            {
                ca.parameters[pidx++]   = st;
                ca.parameters[pidx++]   = ad;
                ca.parameters[pidx++]   = f;
                ca.n_parameters         = pidx;
                break;
            }
            case S88_ACTION_SET_ADDON_FUNCTION_ON:                                      // parameters: START, LOCO_IDX, FUNC_IDX
            {
                ca.parameters[pidx++]   = st;
                ca.parameters[pidx++]   = ad;
                ca.parameters[pidx++]   = f;
                ca.n_parameters         = pidx;
                break;
            }
            case S88_ACTION_SET_RAILROAD:                                               // parameters: RRG_IDX, RR_IDX
            {
                ca.parameters[pidx++]   = rr >> 8;
                ca.parameters[pidx++]   = rr & 0xFF;
                ca.n_parameters         = pidx;
                break;
            }
            case S88_ACTION_SET_FREE_RAILROAD:                                          // parameters: RRG_IDX
            {
                ca.parameters[pidx++]   = rrg;
                ca.n_parameters         = pidx;
                break;
            }
            case S88_ACTION_SET_LOCO_DESTINATION:                                       // parameters: LOCO_IDX, RRG_IDX
            {
                ca.parameters[pidx++]   = ll;
                ca.parameters[pidx++]   = dest;
                ca.n_parameters         = pidx;
                break;
            }
            case S88_ACTION_SET_LED:                                                    // parameters: START, LG_IDX, LED_MASK, LED_STATE
            {
                ca.parameters[pidx++]   = st;
                ca.parameters[pidx++]   = led_group_idx;
                ca.parameters[pidx++]   = ledmask;
                ca.parameters[pidx++]   = ledon;
                ca.n_parameters         = pidx;
                break;
            }
            case S88_ACTION_SET_SWITCH:                                                 // parameters: START, SW_IDX, SW_STATE
            {
                ca.parameters[pidx++]   = st;
                ca.parameters[pidx++]   = swidx;
                ca.parameters[pidx++]   = swstate;
                ca.n_parameters         = pidx;
                break;
            }
            case S88_ACTION_SET_SIGNAL:                                                 // parameters: START, SIG_IDX, SIG_STATE
            {
                ca.parameters[pidx++]   = st;
                ca.parameters[pidx++]   = sigidx;
                ca.parameters[pidx++]   = sigstate;
                ca.n_parameters         = pidx;
                break;
            }
        }

        if (ac != S88_ACTION_NONE)
        {
            if (slotidx >= n_contact_actions)
            {
                S88::contacts[coidx].add_contact_action (in);
                n_contact_actions = S88::contacts[coidx].get_n_contact_actions (in);
            }

            S88::contacts[coidx].set_contact_action (in, slotidx, &ca);
            slotidx++;
        }
    }

    while (slotidx < n_contact_actions)
    {
        S88::contacts[coidx].delete_contact_action (in, slotidx);
        n_contact_actions = S88::contacts[coidx].get_n_contact_actions (in);
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
        std::string     sname = HTTP::parameter ("name");

        coidx = S88::add ({});

        if (coidx != 0xFFFF)
        {
            S88::contacts[coidx].set_name(sname);
        }
    }
    else if (! strcmp (action, "changes88"))
    {
        String  sname           = HTTP::parameter ("name");
        uint_fast16_t   rr      = HTTP::parameter_number ("rr");
        uint_fast8_t    rrgidx  = rr >> 8;
        uint_fast8_t    rridx   = rr & 0xFF;

        coidx = HTTP::parameter_number ("coidx");

        S88::contacts[coidx].set_name (sname);
        S88::contacts[coidx].set_link_railroad (rrgidx, rridx);

        change_s88_actions (coidx, true);
        change_s88_actions (coidx, false);
    }

    HTTP_Common::add_action_handler ("s88", "", 200, true);

    const char *  bg;
    const char *  style;

    HTTP::response += (String)
        "<table style='border:1px lightgray solid;'>\r\n"
        "<tr bgcolor='#e0e0e0'><th>ID</th><th style='width:280px'>Bezeichnung</th>";

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
                  + S88::contacts[coidx].get_name()
                  + "</td>";

        if (HTTP_Common::edit_mode)
        {
            HTTP::response += (String) "<td><button onclick=\"window.location.href='" + urledit + "?coidx=" + std::to_string(coidx) + "';\">Bearbeiten</button></td>";
        }

        HTTP::response += (String) "</tr>\r\n";
    }

    HTTP::response += (String) "</table>\r\n";
    HTTP::flush ();

    HTTP::response += (String)
        "<script>\r\n"
        "function news88()\r\n"
        "{\r\n"
        "  document.getElementById('action').value = 'news88';\r\n"
        "  document.getElementById('name').value = '';\r\n"
        "  document.getElementById('forms88').style.display='';\r\n"
        "}\r\n"
        "</script>\r\n";

    HTTP::flush ();

    if (S88::data_changed)
    {
        HTTP::response += (String)
            "<BR><form method='get' action='" + url + "'>\r\n"
            "<input type='hidden' name='action' value='saves88'>\r\n"
            "<input type='submit' value='Ge&auml;nderte Konfiguration auf Datentr&auml;ger speichern'>\r\n"
            "</form>\r\n";
    }

    if (HTTP_Common::edit_mode)
    {
        HTTP::response += (String)
            "<BR><button onclick='news88()'>Neuer S88-Kontakt</button>\r\n"
            "<form id='forms88' style='display:none'>\r\n"
            "<table>\r\n"
            "<tr><td>Name:</td><td><input type='text' id='name' name='name'></td></tr>\r\n"
            "<tr><td></td><td align='right'>"
            "<input type='hidden' name='action' id='action' value='news88'><input type='hidden' name='coidx' id='coidx'><input type='submit' value='Speichern'></td></tr>\r\n"
            "</table>\r\n"
            "</form>\r\n";
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
