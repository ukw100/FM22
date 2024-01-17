/*------------------------------------------------------------------------------------------------------------------------
 * http-loco.cc - HTTP loco routines
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
#include <algorithm>
#include <cctype>
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
#include "addon.h"
#include "func.h"
#include "railroad.h"
#include "base.h"
#include "http.h"
#include "http-common.h"
#include "http-loco.h"

#define LOCO_SORT_IDX       0
#define LOCO_SORT_ADDR      1
#define LOCO_SORT_NAME      2

static void
print_loco_macro_action (uint_fast8_t idx, uint_fast8_t action)
{
    String sidx = std::to_string(idx);

    if (idx == action)
    {
        HTTP::response += (String) "<option value='" + sidx + "' selected>";
    }
    else
    {
        HTTP::response += (String) "<option value='" + sidx + "'>";
    }

    switch (idx)
    {
        case LOCO_ACTION_NONE:                                                       // no parameters
        {
            HTTP::response += (String) "&lt;keine&gt;";
            break;
        }
        case LOCO_ACTION_SET_LOCO_SPEED:                                            // parameters: START, SPEED, TENTHS
        {
            HTTP::response += (String) "Setze Geschwindigkeit";
            break;
        }

        case LOCO_ACTION_SET_LOCO_MIN_SPEED:                                        // parameters: START, SPEED, TENTHS
        {
            HTTP::response += (String) "Setze Mindestgeschwindigkeit";
            break;
        }

        case LOCO_ACTION_SET_LOCO_MAX_SPEED:                                        // parameters: START, SPEED, TENTHS
        {
            HTTP::response += (String) "Setze H&ouml;chstgeschwindigkeit";
            break;
        }

        case LOCO_ACTION_SET_LOCO_FORWARD_DIRECTION:                                // parameters: START, FWD
        {
            HTTP::response += (String) "Setze Richtung";
            break;
        }

        case LOCO_ACTION_SET_LOCO_ALL_FUNCTIONS_OFF:                                // parameters: START
        {
            HTTP::response += (String) "Schalte alle Funktionen aus";
            break;
        }

        case LOCO_ACTION_SET_LOCO_FUNCTION_OFF:                                     // parameters: START, FUNC_IDX
        {
            HTTP::response += (String) "Schalte Funktion aus";
            break;
        }

        case LOCO_ACTION_SET_LOCO_FUNCTION_ON:                                      // parameters: START, FUNC_IDX
        {
            HTTP::response += (String) "Schalte Funktion ein";
            break;
        }

        case LOCO_ACTION_SET_ADDON_ALL_FUNCTIONS_OFF:                               // parameters: START
        {
            HTTP::response += (String) "Schalte alle Funktionen aus von Zusatzdecoder";
            break;
        }

        case LOCO_ACTION_SET_ADDON_FUNCTION_OFF:                                // parameters: START, FUNC_IDX
        {
            HTTP::response += (String) "Schalte Funktion aus von Zusatzdecoder";
            break;
        }

        case LOCO_ACTION_SET_ADDON_FUNCTION_ON:                                     // parameters: START, FUNC_IDX
        {
            HTTP::response += (String) "Schalte Funktion ein von Zusatzdecoder";
            break;
        }
    }

    HTTP::response += (String) "</option>\r\n";
}

static void
print_loco_macro_action_list (uint_fast16_t addon_idx, uint_fast8_t caidx, uint_fast8_t action)
{
    uint_fast8_t    i;
    String          scaidx = std::to_string(caidx);

    HTTP::response += (String) "<select name='ac" + scaidx + "' id='ac" + scaidx + "' onchange=\"changeca(" + scaidx + ")\">\r\n";

    for (i = 0; i < LOCO_ACTIONS; i++)
    {
        if (addon_idx != 0xFFFF ||
            (i != LOCO_ACTION_SET_ADDON_ALL_FUNCTIONS_OFF &&
             i != LOCO_ACTION_SET_ADDON_FUNCTION_OFF &&
             i != LOCO_ACTION_SET_ADDON_FUNCTION_ON))
        {
            print_loco_macro_action (i, action);
        }
    }

    HTTP::response += (String) "</select>\r\n";
}

static void
print_loco_macro_actions (uint_fast16_t loco_idx, uint_fast8_t macroidx)
{
    LOCOACTION      la;
    uint_fast8_t    actionidx;
    uint_fast8_t    n_actions;
    auto&           Loco = Locos::locos[loco_idx];
    uint_fast16_t   addon_idx = Loco.get_addon ();

    n_actions = Loco.get_n_macro_actions (macroidx);

    for (actionidx = 0; actionidx < LOCO_MAX_ACTIONS_PER_MACRO; actionidx++)
    {
        String          sactionidx = std::to_string(actionidx);
        uint_fast8_t    rtc;

        HTTP::response += (String) "<tr><td>Aktion " + std::to_string(actionidx) + ":</td><td>";

        if (actionidx < n_actions)
        {
            rtc = Loco.get_macro_action (macroidx, actionidx, &la);

            if (rtc && la.action == LOCO_ACTION_NONE)
            {
                Debug::printf (DEBUG_LEVEL_NORMAL,
                                "print_loco_macro_actions: Internal error: loco_idx = %u, macroidx = %u, actionidx = %u: action == LOCO_ACTION_NONE\n",
                                loco_idx, macroidx, actionidx);
            }
        }
        else
        {
            la.action       = LOCO_ACTION_NONE;
            la.n_parameters = 0;
            rtc = 1;
        }

        if (rtc)
        {
            uint_fast16_t   start_tenths            = 0;
            uint_fast8_t    speed                   = 0x00;
            uint_fast8_t    fwd                     = 1;
            uint_fast8_t    f                       = 0;
            uint_fast8_t    fa                      = 0;
            uint_fast16_t   tenths                  = 0;
            uint_fast8_t    do_display_start_tenths = false;
            uint_fast8_t    do_display_speed        = false;
            uint_fast8_t    do_display_fwd          = false;
            uint_fast8_t    do_display_f            = false;
            uint_fast8_t    do_display_fa           = false;
            uint_fast8_t    do_display_tenths       = false;

            switch (la.action)
            {
                case LOCO_ACTION_NONE:                                                      // no parameters
                {
                    break;
                }
                case LOCO_ACTION_SET_LOCO_SPEED:                                            // parameters: START, SPEED, TENTHS
                {
                    start_tenths            = la.parameters[0];
                    do_display_start_tenths = true;
                    speed                   = la.parameters[1];
                    do_display_speed        = true;
                    tenths                  = la.parameters[2];
                    do_display_tenths       = true;
                    break;
                }

                case LOCO_ACTION_SET_LOCO_MIN_SPEED:                                        // parameters: START, SPEED, TENTHS
                {
                    start_tenths            = la.parameters[0];
                    do_display_start_tenths = true;
                    speed                   = la.parameters[1];
                    do_display_speed        = true;
                    tenths                  = la.parameters[2];
                    do_display_tenths       = true;
                    break;
                }

                case LOCO_ACTION_SET_LOCO_MAX_SPEED:                                        // parameters: START, SPEED, TENTHS
                {
                    start_tenths            = la.parameters[0];
                    do_display_start_tenths = true;
                    speed                   = la.parameters[1];
                    do_display_speed        = true;
                    tenths                  = la.parameters[2];
                    do_display_tenths       = true;
                    break;
                }

                case LOCO_ACTION_SET_LOCO_FORWARD_DIRECTION:                                // parameters: START, FWD
                {
                    start_tenths            = la.parameters[0];
                    do_display_start_tenths = true;
                    fwd                     = la.parameters[1];
                    do_display_fwd          = true;
                    break;
                }

                case LOCO_ACTION_SET_LOCO_ALL_FUNCTIONS_OFF:                                // parameters: START
                {
                    start_tenths            = la.parameters[0];
                    do_display_start_tenths = true;
                    break;
                }

                case LOCO_ACTION_SET_LOCO_FUNCTION_OFF:                                     // parameters: START, FUNC_IDX
                {
                    start_tenths            = la.parameters[0];
                    do_display_start_tenths = true;
                    f                       = la.parameters[1];
                    do_display_f            = true;
                    break;
                }

                case LOCO_ACTION_SET_LOCO_FUNCTION_ON:                                      // parameters: START, FUNC_IDX
                {
                    start_tenths            = la.parameters[0];
                    do_display_start_tenths = true;
                    f                       = la.parameters[1];
                    do_display_f            = true;
                    break;
                }

                case LOCO_ACTION_SET_ADDON_ALL_FUNCTIONS_OFF:                               // parameters: START
                {
                    start_tenths            = la.parameters[0];
                    do_display_start_tenths = true;
                    break;
                }

                case LOCO_ACTION_SET_ADDON_FUNCTION_OFF:                                    // parameters: START, FUNC_IDX
                {
                    start_tenths            = la.parameters[0];
                    do_display_start_tenths = true;
                    fa                      = la.parameters[1];
                    do_display_fa           = true;
                    break;
                }

                case LOCO_ACTION_SET_ADDON_FUNCTION_ON:                                     // parameters: START, FUNC_IDX
                {
                    start_tenths            = la.parameters[0];
                    do_display_start_tenths = true;
                    fa                      = la.parameters[1];
                    do_display_fa           = true;
                    break;
                }
            }

            print_loco_macro_action_list (addon_idx, actionidx, la.action);
            HTTP_Common::print_start_list ((String) "st" + sactionidx, start_tenths, do_display_start_tenths);

            HTTP_Common::print_speed_list ((String) "sp" + sactionidx, speed, do_display_speed);
            HTTP_Common::print_fwd_list ((String) "fwd" + sactionidx, fwd, do_display_fwd);
            HTTP_Common::print_loco_function_list (loco_idx, true, (String) "f" + sactionidx, f, do_display_f);

            if (addon_idx != 0xFFFF)
            {
                HTTP_Common::print_loco_function_list (addon_idx, false, (String) "fa" + sactionidx, fa, do_display_fa);
            }

            HTTP_Common::print_tenths_list ((String) "te" + sactionidx, tenths, do_display_tenths);
        }

        HTTP::response += (String) "</td></tr>\r\n";
    }

    while (actionidx < LOCO_MAX_ACTIONS_PER_MACRO)
    {
        String sactionidx = std::to_string(actionidx);

        HTTP::response += (String) "<tr><td>Aktion " + sactionidx + ":</td><td></td></tr>\r\n";
        actionidx++;
    }
}

/*----------------------------------------------------------------------------------------------------------------------------------------
 * handle_loco_macro_edit ()
 *----------------------------------------------------------------------------------------------------------------------------------------
 */
void
HTTP_Loco::handle_loco_macro_edit (void)
{
    String          browsertitle        = (String) "Macro bearbeiten";
    const char *    sloco_idx           = HTTP::parameter ("lidx");
    uint_fast16_t   loco_idx            = atoi (sloco_idx);
    String          sl                  = std::to_string(loco_idx);
    const char *    smacroidx           = HTTP::parameter ("midx");
    uint_fast16_t   macroidx            = atoi (smacroidx);
    auto&           Loco                = Locos::locos[loco_idx];
    String          loco_name           = Loco.get_name();
    String          url                 = (String) "/loco?action=loco&lidx=" + sl;
    String          title               = (String) "<a href='" + url + "'>" + loco_name + "</a> &rarr; " + "Macro bearbeiten";
    uint_fast16_t   addon_idx           = Loco.get_addon ();

    HTTP_Common::html_header (browsertitle, title, url, true);
    HTTP::response += (String) "<div style='margin-left:20px;'>\r\n";
    HTTP_Common::add_action_handler ("head", "", 200, true);

    HTTP::response += (String)
        "<script>\r\n"
        "function changeca(caidx)\r\n"
        "{\r\n"
        "  var obj = document.getElementById('ac' + caidx);\r\n"
        "  var st = 'none';\r\n"
        "  var sp = 'none';\r\n"
        "  var fwd = 'none';\r\n"
        "  var f = 'none';\r\n"
        "  var fa = 'none';\r\n"
        "  var te = 'none';\r\n"
        "  switch (parseInt(obj.value))\r\n"
        "  {\r\n"
        "    case " + std::to_string(LOCO_ACTION_NONE)                          + ": { break; }\r\n"
        "    case " + std::to_string(LOCO_ACTION_SET_LOCO_SPEED)                + ": { st = ''; sp = ''; te = ''; break }\r\n"
        "    case " + std::to_string(LOCO_ACTION_SET_LOCO_MIN_SPEED)            + ": { st = ''; sp = ''; te = ''; break; }\r\n"
        "    case " + std::to_string(LOCO_ACTION_SET_LOCO_MAX_SPEED)            + ": { st = ''; sp = ''; te = ''; break; }\r\n"
        "    case " + std::to_string(LOCO_ACTION_SET_LOCO_FORWARD_DIRECTION)    + ": { st = ''; fwd = ''; break; }\r\n"
        "    case " + std::to_string(LOCO_ACTION_SET_LOCO_ALL_FUNCTIONS_OFF)    + ": { st = ''; break; }\r\n"
        "    case " + std::to_string(LOCO_ACTION_SET_LOCO_FUNCTION_OFF)         + ": { st = ''; f = ''; break; }\r\n"
        "    case " + std::to_string(LOCO_ACTION_SET_LOCO_FUNCTION_ON)          + ": { st = ''; f = ''; break; }\r\n"
        "    case " + std::to_string(LOCO_ACTION_SET_ADDON_ALL_FUNCTIONS_OFF)   + ": { st = ''; break; }\r\n"
        "    case " + std::to_string(LOCO_ACTION_SET_ADDON_FUNCTION_OFF)        + ": { st = ''; fa = ''; break; }\r\n"
        "    case " + std::to_string(LOCO_ACTION_SET_ADDON_FUNCTION_ON)         + ": { st = ''; fa = ''; break; }\r\n"
        "  }\r\n"
        "  document.getElementById('st' + caidx).style.display = st;\r\n"
        "  document.getElementById('sp' + caidx).style.display = sp;\r\n"
        "  document.getElementById('fwd' + caidx).style.display = fwd;\r\n"
        "  document.getElementById('f' + caidx).style.display = f;\r\n";

    if (addon_idx != 0xFFFF)
    {
        HTTP::response += (String) "  document.getElementById('fa' + caidx).style.display = fa;\r\n";
    }

    HTTP::response += (String)
        "  document.getElementById('te' + caidx).style.display = te;\r\n"
        "}\r\n"
        "</script>\r\n";
    HTTP::flush ();

    HTTP::response += (String)
        "<form method='get' action='" + url + "'>"
        "<table style='border:1px lightgray solid;'>\r\n"
        "<tr bgcolor='#e0e0e0'><td>ID:</td><td>M";

    if (macroidx == 0)
    {
        HTTP::response += (String) "F";
    }
    else if (macroidx == 1)
    {
        HTTP::response += (String) "H";
    }
    else
    {
        HTTP::response += std::to_string(macroidx + 1);
    }

    HTTP::response += (String) "</td></tr>\r\n";

    HTTP::flush ();

    HTTP::response += (String) "<tr><td colspan='2'>Aktionen:</td></tr>";
    print_loco_macro_actions (loco_idx, macroidx);

    HTTP::flush ();

    HTTP::response += (String)
        "<tr><td><BR></td></tr>\r\n"
        "<tr><td><input type='button'  onclick=\"window.location.href='" + url + "?action=loco&lidx=" + sl + "';\" value='Zur&uuml;ck'></td>"
        "<td align='right'><input type='hidden' name='action' value='changelm'>"
        "<input type='hidden' name='lidx' value='" + sl + "'><input type='hidden' name='midx' value='" + smacroidx + "'>"
        "<button >Speichern</button></td></tr>"
        "</table>\r\n"
        "</form>\r\n"
        "</div>\r\n";

    HTTP_Common::html_trailer ();
}

static void
change_loco_macro_actions (uint_fast16_t loco_idx, uint_fast16_t macroidx)
{
    LOCOACTION          la;
    uint_fast8_t        n_actions;
    uint_fast8_t        slotidx = 0;
    uint_fast8_t        idx;
    auto&               Loco = Locos::locos[loco_idx];

    n_actions = Loco.get_n_macro_actions (macroidx);

    for (idx = 0; idx < LOCO_MAX_ACTIONS_PER_MACRO; idx++)
    {
        String          sidx = std::to_string(idx);

        uint_fast16_t   st  = HTTP::parameter_number ((String) "st"     + sidx);
        uint_fast8_t    ac  = HTTP::parameter_number ((String) "ac"     + sidx);
        uint_fast8_t    sp  = HTTP::parameter_number ((String) "sp"     + sidx);
        uint_fast8_t    te  = HTTP::parameter_number ((String) "te"     + sidx);
        uint_fast8_t    fwd = HTTP::parameter_number ((String) "fwd"    + sidx);
        uint_fast8_t    f   = HTTP::parameter_number ((String) "f"      + sidx);
        uint_fast8_t    fa  = HTTP::parameter_number ((String) "fa"     + sidx);

        la.action = ac;

        switch (ac)
        {
            case LOCO_ACTION_NONE:                                                      // no parameters
            {
                la.n_parameters = 0;
                break;
            }
            case LOCO_ACTION_SET_LOCO_SPEED:                                            // parameters: START, SPEED, TENTHS
            {
                la.parameters[0] = st;
                la.parameters[1] = sp;
                la.parameters[2] = te;
                la.n_parameters = 3;
                break;
            }
            case LOCO_ACTION_SET_LOCO_MIN_SPEED:                                        // parameters: START, SPEED, TENTHS
            {
                la.parameters[0] = st;
                la.parameters[1] = sp;
                la.parameters[2] = te;
                la.n_parameters = 3;
                break;
            }
            case LOCO_ACTION_SET_LOCO_MAX_SPEED:                                        // parameters: START, SPEED, TENTHS
            {
                la.parameters[0] = st;
                la.parameters[1] = sp;
                la.parameters[2] = te;
                la.n_parameters = 3;
                break;
            }
            case LOCO_ACTION_SET_LOCO_FORWARD_DIRECTION:                                // parameters: START, FWD
            {
                la.parameters[0] = st;
                la.parameters[1] = fwd;
                la.n_parameters = 2;
                break;
            }
            case LOCO_ACTION_SET_LOCO_ALL_FUNCTIONS_OFF:                                // parameters: START
            {
                la.parameters[0] = st;
                la.n_parameters = 1;
                break;
            }
            case LOCO_ACTION_SET_LOCO_FUNCTION_OFF:                                     // parameters: START, FUNC_IDX
            {
                la.parameters[0] = st;
                la.parameters[1] = f;
                la.n_parameters = 2;
                break;
            }
            case LOCO_ACTION_SET_LOCO_FUNCTION_ON:                                      // parameters: START, FUNC_IDX
            {
                la.parameters[0] = st;
                la.parameters[1] = f;
                la.n_parameters = 2;
                break;
            }
            case LOCO_ACTION_SET_ADDON_ALL_FUNCTIONS_OFF:                               // parameters: START
            {
                la.parameters[0] = st;
                la.n_parameters = 1;
                break;
            }
            case LOCO_ACTION_SET_ADDON_FUNCTION_OFF:                                    // parameters: START, FUNC_IDX
            {
                la.parameters[0] = st;
                la.parameters[1] = fa;
                la.n_parameters = 2;
                break;
            }
            case LOCO_ACTION_SET_ADDON_FUNCTION_ON:                                     // parameters: START, FUNC_IDX
            {
                la.parameters[0] = st;
                la.parameters[1] = fa;
                la.n_parameters = 2;
                break;
            }
        }

        if (ac != LOCO_ACTION_NONE)
        {
            if (slotidx >= n_actions)
            {
                Loco.add_macro_action (macroidx);
                n_actions = Loco.get_n_macro_actions (macroidx);
            }

            Loco.set_macro_action (macroidx, slotidx, &la);
            slotidx++;
        }
    }

    while (slotidx < n_actions)
    {
        Loco.delete_macro_action (macroidx, slotidx);
        n_actions = Loco.get_n_macro_actions (macroidx);
    }
}

static void
print_loco_function_select_list (String name, String onchange, bool show_none)
{
    HTTP::response += (String) "<select class='select' id='" + name + "' name='" + name + "' " + onchange + " style='width:250px'>\r\n";

    if (show_none)
    {
        HTTP::response += (String) "<option value='255'>---</option>\r\n";
    }

    for (uint_fast8_t i = 0; i < MAX_LOCO_FUNCTIONS; i++)
    {
        HTTP::response += (String) "<option value=" + std::to_string(i) + ">F" + std::to_string(i) + "</option>\r\n";
    }

    HTTP::response += (String) "</select>\r\n";
}

static void
print_function_select_list (String name)
{
    HTTP::response += (String)
        "<select class='select' name='" + name + "' id='" + name + "' style='width:250px'>\r\n"
        "<option value='65535'>--- Keine Funktion ---</option>\r\n";

    uint_fast16_t n_function_name_entries = Functions::get_n_entries ();

    for (uint_fast16_t i = 0; i < n_function_name_entries; i++)
    {
        HTTP::response += (String) "<option value='" + std::to_string(i) + "'>" + Functions::get (i) + "</option>\r\n";
    }
    HTTP::response += (String) "</select></td></tr>\r\n";
}

static void
print_functions (uint_fast16_t idx, bool is_addon)
{
    uint32_t        pulse_mask;
    uint32_t        sound_mask;
    uint32_t        functions;
    uint_fast16_t   addon_idx;
    const char *    bg          = "";
    uint_fast8_t    fidx;
    String          sidx        = std::to_string(idx);

    if (is_addon)
    {
        pulse_mask  = AddOns::addons[idx].get_function_pulse_mask ();
        sound_mask  = AddOns::addons[idx].get_function_sound_mask ();
        functions   = AddOns::addons[idx].get_functions ();
        addon_idx   = 0xFFFF;
    }
    else
    {
        pulse_mask  = Locos::locos[idx].get_function_pulse_mask ();
        sound_mask  = Locos::locos[idx].get_function_sound_mask ();
        functions   = Locos::locos[idx].get_functions ();
        addon_idx   = Locos::locos[idx].get_addon ();
    }

    for (fidx = 0; fidx < MAX_LOCO_FUNCTIONS; fidx++)
    {
        uint_fast16_t   function_name_idx;

        if (is_addon)
        {
            function_name_idx = AddOns::addons[idx].get_function_name_idx (fidx);
        }
        else
        {
            function_name_idx = Locos::locos[idx].get_function_name_idx (fidx);
        }

        if (function_name_idx != 0xFFFF)
        {
            String      pulse;
            String      sound;
            String      bgid;

            if (pulse_mask & (1 << fidx))
            {
                pulse = (String) "<font color=green>&#10004;</font>";
            }

            if (sound_mask & (1 << fidx))
            {
                sound = (String) "<font color=green>&#10004;</font>";
            }

            String style;

            if (functions & (1 << fidx))
            {
                style = "style='color: white; background-color: green'";
            }

            String fnname;

            if (function_name_idx < Functions::get_n_entries ())
            {
                fnname = Functions::get (function_name_idx);
            }
            else
            {
                fnname = "Unbekannt";
            }

            if (is_addon)
            {
                bgid = "bga";
            }
            else
            {
                bgid = "bg";
            }

            HTTP::response += (String)
                "<tr " + bg + "><td id='" + bgid + sidx + "_" + std::to_string(fidx) + "' " + style + " align='center'>F" + std::to_string(fidx) + "</td><td>" + fnname +
                "</td><td align='center'>" + pulse + "</td><td align='center'>" + sound + "</td>";

            if (addon_idx != 0xFFFF)
            {
                uint_fast8_t addon_fidx = Locos::locos[idx].get_coupled_function (fidx);

                if (addon_fidx != 0xFF)
                {
                    HTTP::response += (String) "<td align='right'>F" + std::to_string(addon_fidx) + "</td>";
                }
                else
                {
                    HTTP::response += (String) "<td></td>";
                }
            }
            else if (is_addon)
            {
                HTTP::response += (String) "<td></td>";
            }
        }

        if (function_name_idx != 0xFFFF)
        {
            if (is_addon)
            {
                HTTP::response += (String)
                    "<td align='center'><button id='ta" + sidx + "_" + std::to_string(fidx) + "' style='width:60px;' onclick='togglefa(" + sidx + "," + std::to_string(fidx) + ")'>";
            }
            else
            {
                HTTP::response += (String)
                    "<td align='center'><button id='t" + sidx + "_" + std::to_string(fidx) + "' style='width:60px;' onclick='togglef(" + sidx + "," + std::to_string(fidx) + ")'>";
            }

            if (functions & (1 << fidx))
            {
                HTTP::response += (String) "<font color='red'>Aus</font>";
            }
            else
            {
                HTTP::response += (String) "<font color='green'>Ein</font>";
            }

            HTTP::response += (String) "</button></td></tr>\r\n";

            if (*bg)
            {
                bg = "";
            }
            else
            {
                bg = "bgcolor='#e0e0e0'";
            }
        }
    }
}

static void
list_destinations (uint_fast16_t lidx, uint_fast8_t destination)
{
    uint_fast8_t    n = RailroadGroups::get_n_railroad_groups ();
    uint_fast8_t    destidx;

    HTTP::response += (String)
            "<script>"
            "function setdest(li)"
            "{"
            "  var desto = document.getElementById('dest');"
            "  if (desto)"
            "  {"
            "    var dest = desto.value;"
            "    var http = new XMLHttpRequest();"
            "    http.open ('GET', '/action?action=setdestination&loco_idx=' + li + '&dest=' + dest);"
            "    http.addEventListener('load',"
            "      function(event)"
            "      {"
            "        var o;"
            "        var text = http.responseText;"
            "        if (http.status >= 200 && http.status < 300)"
            "        {"
            "        }"
            "        else"
            "        {"
            "          alert (http.status);"
            "        }"
            "      }"
            "    );"
            "  }"
            "  http.send (null);"
            "}\r\n"
            "</script>\r\n";

    HTTP::response += (String) "Ziel:&nbsp;<select name='dest' id='dest' onchange='setdest(" + std::to_string(lidx) + ");'>";

    String selected = "";

    if (destination == 0xFF)
    {
        selected = "selected";
    }

    HTTP::response += (String) "<option value='255' " + selected + ">---</option>\r\n";

    for (destidx = 0; destidx < n; destidx++)
    {
        String sname = RailroadGroups::railroad_groups[destidx].get_name() ;

        if (destination == destidx)
        {
            selected = "selected";
        }
        else
        {
            selected = "";
        }

        HTTP::response += (String) "<option value='" + std::to_string(destidx) + "' " + selected + ">" + sname + "</option>\r\n";
    }

    HTTP::response += (String) "</select>\r\n";
}

static int
loco_sort_name (const void * p1, const void * p2)
{
    const uint16_t *    idx1    = (uint16_t *) p1;
    const uint16_t *    idx2    = (uint16_t *) p2;
    String              s1      = Locos::locos[*idx1].get_name();
    String              s2      = Locos::locos[*idx2].get_name();

    std::transform(s1.begin(), s1.end(), s1.begin(), asciitolower);
    std::transform(s2.begin(), s2.end(), s2.begin(), asciitolower);

    return s1.compare(s2);
}

int
loco_sort_addr (const void * p1, const void * p2)
{
    const uint16_t * idx1 = (uint16_t *) p1;
    const uint16_t * idx2 = (uint16_t *) p2;

    return (int) Locos::locos[*idx1].get_addr() - (int) Locos::locos[*idx2].get_addr();
}

/*----------------------------------------------------------------------------------------------------------------------------------------
 * handle_loco ()
 *----------------------------------------------------------------------------------------------------------------------------------------
 */
void
HTTP_Loco::handle_loco (void)
{
    String          browsertitle    = "Lokliste";
    String          title           = "Lokliste";
    String          url             = "/loco";
    String          urledit         = "/lmedit";
    const char *    action          = HTTP::parameter ("action");
    const int       nsort           = HTTP::parameter_number ("sort");
    uint_fast16_t   loco_idx        = 0;
    String          sl              = "";
    Loco *          lp;

    if (! strcmp (action, "loco") || ! strcmp (action, "changefunc") || ! strcmp (action, "changefuncaddon") || ! strcmp (action, "changelm"))
    {
        loco_idx    = HTTP::parameter_number ("lidx");
        lp          = &Locos::locos[loco_idx];
        sl          = std::to_string(loco_idx);

        browsertitle = lp->get_name();
        title = (String) "<a href='" + url + "'>Lokliste</a> &rarr; " + browsertitle;
    }

    HTTP_Common::html_header (browsertitle, title, url, true);
    HTTP::response += (String) "<div style='margin-left:20px;'>\r\n";

    if (! strcmp (action, "saveloco"))
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
    else if (! strcmp (action, "newloco"))
    {
        char            name[128];
        const char *    sname   = HTTP::parameter ("name");
        uint_fast16_t   addr    = HTTP::parameter_number ("addr");
        uint_fast8_t    steps   = HTTP::parameter_number ("steps");
        uint_fast8_t    active  = HTTP::parameter_number ("active");

        uint_fast16_t   lidx;

        strncpy (name, sname, 127);
        name[127] = 0;

        lidx = Locos::add ({});

        if (lidx != 0xFFFF && addr != 0)
        {
            Locos::locos[lidx].set_name (name);
            Locos::locos[lidx].set_addr (addr);
            Locos::locos[lidx].set_speed_steps (steps);

            if (active)
            {
                Locos::locos[lidx].activate ();
            }
            else
            {
                Locos::locos[lidx].deactivate ();
            }
        }
        else
        {
            HTTP::response += (String) "<font color='red'>Lok hinzuf&uuml;gen fehlgeschlagen.</font><P>";
        }
    }
    else if (! strcmp (action, "changeloco"))
    {
        const char *    sname   = HTTP::parameter ("name");
        uint_fast16_t   lidx    = HTTP::parameter_number ("lidx");
        uint_fast16_t   nlidx   = HTTP::parameter_number ("nlidx");
        uint_fast16_t   addr    = HTTP::parameter_number ("addr");
        uint_fast8_t    steps   = HTTP::parameter_number ("steps");
        uint_fast8_t    active  = HTTP::parameter_number ("active");

        if (lidx != nlidx)
        {
            nlidx = Locos::set_new_id (lidx, nlidx);
        }

        if (nlidx != 0xFFFF)
        {
            Locos::locos[nlidx].set_name (sname);
            Locos::locos[nlidx].set_addr (addr);
            Locos::locos[nlidx].set_speed_steps (steps);

            if (active)
            {
                Locos::locos[nlidx].activate ();
            }
            else
            {
                Locos::locos[nlidx].deactivate ();
            }
        }
    }
    else if (! strcmp (action, "changelm"))
    {
        uint_fast8_t    macroidx    = HTTP::parameter_number ("midx");
        change_loco_macro_actions (loco_idx, macroidx);
    }
    else if (! strcmp (action, "changefunc"))
    {
        uint_fast8_t    fidx                = HTTP::parameter_number ("fidx");
        uint_fast16_t   function_name_idx   = HTTP::parameter_number ("fnidx");
        uint_fast8_t    pulse               = HTTP::parameter_number ("pulse");
        uint_fast8_t    sound               = HTTP::parameter_number ("sound");
        uint_fast8_t    afidx               = HTTP::parameter_number ("afidx");

        lp->set_function_type (fidx, function_name_idx, pulse, sound);
        lp->set_coupled_function (fidx, afidx);     // also if afidx == 0xFF: reset coupled function
    }
    else if (! strcmp (action, "changefuncaddon"))
    {
        uint_fast16_t   addon_idx           = HTTP::parameter_number ("aidx");
        uint_fast8_t    fidx                = HTTP::parameter_number ("addonfidx");
        uint_fast16_t   function_name_idx   = HTTP::parameter_number ("addonfnidx");
        uint_fast8_t    pulse               = HTTP::parameter_number ("addonpulse");
        uint_fast8_t    sound               = HTTP::parameter_number ("addonsound");

        AddOns::addons[addon_idx].set_function_type (fidx, function_name_idx, pulse, sound);
    }

    HTTP::response += (String)
        "<script>\r\n"
        "function macro(li, idx)"
        "{"
        "  var http = new XMLHttpRequest();"
        "  http.open ('GET', '/action?action=macro&loco_idx=' + li + '&idx=' + idx);"
        "  http.addEventListener('load', function(event) {});"
        "  http.send (null);"
        "}\r\n"
        "</script>\r\n";

    if (! strcmp (action, "loco") || ! strcmp (action, "changefunc") || ! strcmp (action, "changefuncaddon") || ! strcmp (action, "changelm"))
    {
        char            param[64];
        uint_fast16_t   addon_idx;
        uint_fast8_t    speed;
        uint_fast8_t    fwd;
        String          addon_name = "";

        addon_idx   = lp->get_addon ();
        speed       = lp->get_speed ();
        fwd         = lp->get_fwd ();

        if (addon_idx != 0xFFFF)
        {
            addon_name = AddOns::addons[addon_idx].get_name ();
        }

        HTTP::response += (String)
            "<script>\r\n"
            "function togglef (li, fi)"
            "{"
            "  var http = new XMLHttpRequest();"
            "  http.open ('GET', '/action?action=togglefunction&loco_idx=' + li + '&f=' + fi);"
            "  http.addEventListener('load',"
            "    function(event)"
            "    {"
            "      var o; "
            "      var text = http.responseText;"
            "      if (http.status >= 200 && http.status < 300)"
            "      {"
            "        var color;"
            "        if (parseInt(text) == 1)"
            "        {"
            "          text='Aus';"
            "          color='red';"
            "        }"
            "        else"
            "        {"
            "          text='Ein';"
            "          color='green';"
            "        }"
            "        o = document.getElementById('t' + li + '_' + fi);"
            "        o.textContent = text;"
            "        o.color = color;"
            "      }"
            "      else"
            "      {"
            "        alert (http.status);"
            "      }"
            "    }"
            "  );"
            "  http.send (null);"
            "}\r\n"
            "function togglefa (ai, fi)"
            "{"
            "  var http = new XMLHttpRequest();"
            "  http.open ('GET', '/action?action=togglefunctionaddon&addon_idx=' + ai + '&f=' + fi);"
            "  http.addEventListener('load',"
            "    function(event)"
            "    {"
            "      var o;"
            "      var text = http.responseText;"
            "      if (http.status >= 200 && http.status < 300)"
            "      {"
            "        var color;"
            "        if (parseInt(text) == 1)"
            "        {"
            "          text='Aus';"
            "          color='red';"
            "        }"
            "        else"
            "        {"
            "          text='Ein';"
            "          color='green';"
            "        }"
            "        o = document.getElementById('t' + ai + '_' + fi);"
            "        o.textContent = text; o.color = color;"
            "      }"
            "      else"
            "      {"
            "        alert (http.status);"
            "      }"
            "    }"
            "  );"
            "  http.send (null);"
            "}\r\n";

        sprintf (param, "loco_idx=%u&addon_idx=%u", (unsigned int) loco_idx, (unsigned int) addon_idx);
        HTTP_Common::add_action_handler ("loco", param, 200);

        HTTP::response += (String)
            "function upd_fwd (f)"
            "{"
            "  var fo = document.getElementById('f');"
            "  var bo = document.getElementById('b');"
            "  var fwdo = document.getElementById('fwd');"
            "  fwdo.value = f;"
            "  if (f)"
            "  {"
            "    fo.style.backgroundColor = 'green';"
            "    fo.style.color = 'white';"
            "    bo.style.backgroundColor = 'inherit';"
            "    bo.style.color = 'inherit';"
            "  }"
            "  else"
            "  {"
            "    fo.style.backgroundColor = 'inherit';"
            "    fo.style.color = 'inherit';"
            "    bo.style.backgroundColor = 'green';"
            "    bo.style.color = 'white';"
            "  }"
            "}\r\n"
            "function upd_speed (s)"
            "{"
            "  var slider = document.getElementById('speed');"
            "  slider.value = s;"
            "  var o = document.getElementById('out');"
            "  o.textContent = s;"
            "}\r\n"
            "function incslider (li)"
            "{"
            "  var slider = document.getElementById('speed');"
            "  var speed = parseInt(slider.value);"
            "  speed += 4;"
            "  if (speed > 127)"
            "  {"
            "    speed = 127;"
            "  }"
            "  setspeed (li, speed);"
            "}\r\n"
            "function decslider (li)"
            "{"
            "  var slider = document.getElementById('speed');"
            "  var speed = parseInt(slider.value);"
            "  speed -= 4;"
            "  if (speed <= 1)"
            "  {"
            "    speed = 0;"
            "  }"
            "  setspeed (li, speed);"
            "}\r\n"
            "function setspeed (li, speed)"
            "{"
            "  var fwdo = document.getElementById('fwd');"
            "  var fwd = parseInt (fwdo.value);"
            "  if (fwd)"
            "  {"
            "    speed |= 0x80;"
            "  }"
            "  var http = new XMLHttpRequest();"
            "  http.open ('GET', '/action?action=setspeed&loco_idx=' + li + '&speed=' + speed);"
            "  http.addEventListener('load',"
            "    function(event)"
            "    {"
            "      var o;"
            "      var text = http.responseText;"
            "      if (http.status >= 200 && http.status < 300)"
            "      {"
            "        var fwd = 0;"
            "        var speed = parseInt(text);"
            "        if (speed & 0x80)"
            "        {"
            "          fwd = 1;"
            "          speed &= 0x7F;"
            "        }"
            "        upd_speed(speed);"
            "        upd_fwd (fwd);"
            "      }"
            "    }"
            "  );"
            "  http.send (null);"
            "}\r\n"
            "function bwd (li)"
            "{"
            "  var fwdo = document.getElementById('fwd');"
            "  fwdo.value = 0;"
            "  var slider = document.getElementById('speed');"
            "  var speed = parseInt(slider.value);"
            "  setspeed (li, speed);"
            "}\r\n"
            "function fwd (li)"
            "{"
            "  var fwdo = document.getElementById('fwd');"
            "  fwdo.value = 1;"
            "  var slider = document.getElementById('speed');"
            "  var speed = parseInt(slider.value) | 0x80;"
            "  setspeed (li, speed);"
            "}\r\n"
            "function upd_slider (li)"
            "{"
            "  var slider = document.getElementById('speed');"
            "  var speed = parseInt(slider.value);"
            "  if (speed == 1)"
            "  {"
            "    var output = document.getElementById('out');"
            "    var oldspeed = parseInt(output.textContent);"
            "    if (oldspeed == 0)"
            "    {"
            "      speed = 2;"
            "    }"
            "    else"
            "    {"
            "      speed = 0;"
            "    }"
            "  }"
            "  setspeed (li, speed);"
            "}\r\n"
            "function go (li)"
            "{"
            "  var http = new XMLHttpRequest();"
            "  http.open ('GET', '/action?action=go&loco_idx=' + li);"
            "  http.send (null);"
            "}\r\n"
            "</script>\r\n";

        HTTP::flush ();

        const char * fwd_color = "style='background-color:inherit;color:inherit'";
        const char * bwd_color = "style='background-color:inherit;color:inherit'";

        if (fwd)
        {
            fwd_color = "style='background-color:green;color:white'";
        }
        else
        {
            bwd_color = "style='background-color:green;color:white'";
        }

        if (speed == 1)
        {
            speed = 0;
        }

        String colspan;

        if (addon_idx != 0xFFFF)
        {
            colspan = "6";
        }
        else
        {
            colspan = "5";
        }

        String online;

        if (lp->is_online ())
        {
            online = (String) "<span id='on' style='background-color:green;color:white'>" + std::to_string(lp->get_addr()) + "</span>";
        }
        else
        {
            online = (String) "<span id='on'>" + std::to_string(lp->get_addr()) + "</span>";
        }

        HTTP::response += (String)
            "<table style='border:1px lightgray solid;'>\r\n"
            "<tr><td colspan='" + colspan + "'>"
            "  <div style='float:left;width:45%;display:block;text-align:left'>Adresse: " + online + "</div>"
            "  <div id='go' style='float:left;display:block;text-align:right;display:none'><button onclick='go(" + sl + ")' style='color:green'>Fahrt!</button></div>\r\n"
            "  <div id='loc' style='display:block;text-align:right'>&nbsp;</div></td></tr>\r\n"
            "<tr><td align='center' colspan='" + colspan + "'>\r\n"
            "<div style='width:350px;padding-top:0px;padding-bottom:10px'>"
            "  <input type='hidden' id='fwd' value='" + std::to_string(fwd) + "'>"
            "  <div style='width:300px;text-align:center;'>"
            "    <button id='m1' onclick='macro(" + sl + ", 0)'>MF</button>"
            "    <button id='m2' onclick='macro(" + sl + ", 1)'>MH</button>"
            "    <button id='m3' onclick='macro(" + sl + ", 2)'>M3</button>"
            "    <button id='m4' onclick='macro(" + sl + ", 3)'>M4</button>"
            "    <button id='m5' onclick='macro(" + sl + ", 4)'>M5</button>"
            "    <button id='m6' onclick='macro(" + sl + ", 5)'>M6</button>"
            "    <button id='m7' onclick='macro(" + sl + ", 6)'>M7</button>"
            "    <button id='m8' onclick='macro(" + sl + ", 7)'>M8</button>"
            "  </div>"
            "  <div id='out' style='width:300px;margin-top:10px;text-align:center;'>" + std::to_string(speed) + "</div>"
            "  <button style='vertical-align:top' onclick='decslider(" + sl + ")'>-</button>"
            "  <input type='range' min='0' max='127' value='" + std::to_string(speed) + "' id='speed' style='width:256px' onchange='upd_slider(" + sl + ")'>"
            "  <button style='vertical-align:top' onclick='incslider(" + sl + ")'>+</button>"
            "  <div style='width:300px;text-align:center'>"
            "    <button id='b' " + bwd_color + " onclick='bwd(" + sl + ")'>&#9664;</button>"
            "    <button onclick='setspeed(" + sl + ",0);'>&#9209;</button>"
            "    <button onclick='setspeed(" + sl + ",1);'>&#9210;</button>"
            "    <button id='f' " + fwd_color + " onclick='fwd(" + sl + ")'>&#9654;</button>"
            "  </div>"
            "</div>\r\n"
            "</td></tr>\r\n"
            "<tr><td align='center' colspan='" + colspan + "'>\r\n";

        HTTP::flush ();

        uint_fast8_t    destination = lp->get_destination();
        list_destinations (loco_idx, destination);

        HTTP::response += (String)
            "</td></tr>"
            "<tr bgcolor='#e0e0e0'>"
            "<th style='width:30px;'>F</th>"
            "<th style='width:250px;'>Funktion</th>"
            "<th style='width:50px;'>Puls</th>"
            "<th style='width:50px;'>Sound</th>";

        // Maerklin: Puls = "Momentfunktion"

        if (addon_idx != 0xFFFF)
        {
            HTTP::response += (String) "<th style='width:20px'>KF</th>";
        }

        HTTP::response += (String) "<th>Aktion</th></tr>\r\n";
        HTTP::flush ();

        print_functions (loco_idx, false);

        HTTP::response += (String)
            "</table>\r\n"
            "<script>\r\n"
            "function changef()"
            "{"
            "  var f = document.getElementById('fidx').value;"
            "  var url = '/action?action=getf&lidx=" + sl + "&f=' + f;"
            "  var http = new XMLHttpRequest();"
            "  http.open ('GET', url);"
            "  http.addEventListener('load',"
            "    function(event)"
            "    {"
            "      if (http.status >= 200 && http.status < 300)"
            "      {"
            "        var text = http.responseText;"
            "        const a = text.split('\t');"
            "        document.getElementById('fnidx').value = a[0];"
            "        document.getElementById('pulse').value = a[1];"
            "        document.getElementById('sound').value = a[2];"
            "        var oo; oo = document.getElementById('afidx');"
            "        if (oo)"
            "        {"
            "          oo.value = a[3];"
            "        }"
            "      }"
            "    }"
            "  );"
            "  http.send (null);"
            "}\r\n"
            "function changefunc()"
            "{"
            "  document.getElementById('formfunc').style.display = '';"
            "  document.getElementById('btnchgfunc').style.display = 'none';"
            "  changef();"
            "}\r\n"
            "</script>\r\n";

        if (HTTP_Common::edit_mode)
        {
            uint_fast8_t i;

            HTTP::response += (String)
                "<BR>Bearbeiten: "
                "<button onclick=\"window.location.href='" + urledit + "?lidx=" + sl + "&midx=0';\" style='margin-right:3px'>MF</button>"
                "<button onclick=\"window.location.href='" + urledit + "?lidx=" + sl + "&midx=1';\" style='margin-right:3px'>MH</button>";

            for (i = 2; i < MAX_LOCO_MACROS_PER_LOCO; i++)
            {
                HTTP::response += (String) "<button onclick=\"window.location.href='" + urledit + "?lidx=" + sl + "&midx=" + std::to_string(i) + "';\" style='margin-right:3px'>M"
                                + std::to_string(i + 1) + "</button>";
            }

            HTTP::response += (String) "<button id='btnchgfunc' onclick='changefunc()'>Fx</button><P>\r\n";
        }

        HTTP::response += (String)
            "<form id='formfunc' style='display:none'>\r\n"
            "<table>\r\n"
            "<tr><td>F-Nr.</td><td>";

        print_loco_function_select_list ("fidx", "onchange='changef()'", false);

        HTTP::flush ();

        HTTP::response += (String)
            "</td></tr>\r\n"
            "<tr><td>Funktion</td><td>\r\n";

        print_function_select_list ("fnidx");
        HTTP::flush ();

        // Maerklin: Puls = "Momentfunktion"

        HTTP::response += (String)
            "<tr><td>Puls:</td><td><select class='select' id='pulse' name='pulse' style='width:250px'><option value='0'>Nein</option><option value='1'>Ja</option></select></td></tr>\r\n"
            "<tr><td>Sound:</td><td><select class='select' id='sound' name='sound' style='width:250px'><option value='0'>Nein</option><option value='1'>Ja</option></select></td></tr>\r\n";

        if (addon_idx != 0xFFFF)
        {
            HTTP::response += (String) "<tr><td>Kopplung mit " + addon_name + ":</td><td>";
            print_loco_function_select_list ("afidx", "", true);
            HTTP::response += (String) "</td></tr>\r\n";
        }
        else
        {
            HTTP::response += (String) "<tr><td><input type='hidden' name='afidx' value='255'></td></tr>\r\n";
        }

        HTTP::response += (String)
            "<tr><td></td><td align='right'>"
            "<input type='hidden' name='action' value='changefunc'>\r\n"
            "<input type='hidden' name='lidx' value='" + sl + "'>\r\n"
            "<input type='submit' value='Speichern'></td></tr>\r\n"
            "</table>\r\n"
            "</form>\r\n";

        HTTP::flush ();

        if (addon_idx != 0xFFFF)
        {
            HTTP::response += (String)
                "<P><B>" + addon_name + "</B>\r\n"
                "<table style='border:1px lightgray solid;'>\r\n"
                "<tr bgcolor='#e0e0e0'><th style='width:30px;'>F</th><th style='width:250px;'>Funktion</th>"
                "<th style='width:50px;'>Puls</th><th style='width:50px;'>Sound</th><th style='width:20px'></th><th>Aktion</th></tr>\r\n";
            print_functions (addon_idx, true);
            HTTP::response += (String)
                "</table>\r\n"
                "<script>\r\n"
                "function changefaddon()"
                "{"
                "  var f = document.getElementById('addonfidx').value;"
                "  var url = '/action?action=getfa&aidx=" + std::to_string(addon_idx) + "&f=' + f;"
                "  var http = new XMLHttpRequest(); http.open ('GET', url);"
                "  http.addEventListener('load',"
                "    function(event)"
                "    {"
                "      if (http.status >= 200 && http.status < 300)"
                "      {"
                "        var text = http.responseText;"
                "        const a = text.split('\t');"
                "        document.getElementById('addonfnidx').value = a[0];"
                "        document.getElementById('addonpulse').value = a[1];"
                "        document.getElementById('addonsound').value = a[2];"
                "      }"
                "    }"
                "  );"
                "  http.send (null);"
                "}\r\n"
                "function changefuncaddon()"
                "{"
                "  document.getElementById('formfuncaddon').style.display='';"
                "  document.getElementById('btnchgfuncaddon').style.display='none';"
                "  changefaddon();"
                "}\r\n"
                "</script>\r\n";

            if (HTTP_Common::edit_mode)
            {
                HTTP::response += (String) "<BR><button id='btnchgfuncaddon' onclick='changefuncaddon()'>Bearbeiten</button><BR>\r\n";
            }

            HTTP::response += (String)
                "<form id='formfuncaddon' style='display:none'>\r\n"
                "<table>\r\n";

            HTTP::flush ();

            HTTP::response += (String) "<tr><td>F-Nr.</td><td>";
            print_loco_function_select_list ("addonfidx", "onchange='changefaddon()'", false);
            HTTP::response += (String) "</td></tr>\r\n";
            HTTP::flush ();

            HTTP::response += (String) "<tr><td>Funktion</td><td>\r\n";
            print_function_select_list ("addonfnidx");
            HTTP::flush ();

            HTTP::response += (String)
                "<tr><td>Puls:</td><td><select class='select' id='addonpulse' name='addonpulse' style='width:250px'><option value='0'>Nein</option><option value='1'>Ja</option></select></td></tr>\r\n"
                "<tr><td>Sound:</td><td><select class='select' id='addonsound' name='addonsound' style='width:250px'><option value='0'>Nein</option><option value='1'>Ja</option></select></td></tr>\r\n"
                "<tr><td></td><td align='right'>"
                "<input type='hidden' name='action' value='changefuncaddon'>\r\n"
                "<input type='hidden' name='lidx' value='" + sl + "'>\r\n";       // show loco details of loco lid
                "<input type='hidden' name='aidx' value='" + std::to_string(addon_idx) + "'>\r\n"
                "<input type='submit' value='Speichern'></td></tr>\r\n"
                "</table>\r\n"
                "</form>\r\n";
            HTTP::flush ();
        }
    }
    else
    {
        const char * bg = "bgcolor='#e0e0e0'";
        String          status;
        String          disabled;
        String          online;
        uint_fast16_t   n_locos = Locos::get_n_locos ();
        uint_fast16_t   map_idx;
        uint16_t        loco_map[n_locos];
        String          scolor_id;
        String          scolor_name;
        String          scolor_addr;

        for (loco_idx = 0; loco_idx < n_locos; loco_idx++)
        {
            loco_map[loco_idx] = loco_idx;
        }

        if (nsort == LOCO_SORT_NAME)
        {
            qsort (loco_map, n_locos, sizeof (uint16_t), loco_sort_name);
            scolor_name = "style='color:green;'";
        }
        else if (nsort == LOCO_SORT_ADDR)
        {
            qsort (loco_map, n_locos, sizeof (uint16_t), loco_sort_addr);
            scolor_addr = "style='color:green;'";
        }
        else
        {
            scolor_id = "style='color:green;'";
        }

        HTTP_Common::add_action_handler ("locos", "", 200, true);

        HTTP::flush ();

        HTTP::response += (String)
            "<table style='border:1px lightgray solid;'>\r\n"
            "<tr " + bg + ">"
            "<th align='right'><a href='?sort=0' " + scolor_id + ">ID</a></th>"
            "<th><a href='?sort=2' " + scolor_name + ">Bezeichnung</a></th>"
            "<th>Ort</th>"
            "<th>Macro</th>"
            "<th class='hide600' style='width:50px;'><a href='?sort=1' " + scolor_addr + ">Adresse</a></th>"
            "<th class='hide650'>RC2</th>";

        if (HTTP_Common::edit_mode)
        {
            HTTP::response += (String) "<th>Aktion</th>";
        }

        HTTP::response += (String) "</tr>\r\n";

        for (map_idx = 0; map_idx < n_locos; map_idx++)
        {
            if (*bg)
            {
                bg = "";
            }
            else
            {
                bg = "bgcolor='#e0e0e0'";
            }

            loco_idx    = loco_map[map_idx];
            lp          = &Locos::locos[loco_idx];
            String sl   = std::to_string(loco_idx);

            if (lp->get_flag_halt())
            {
                online = (String) "<td id='o" + sl + "' style='text-align:right;background-color:red;color:white'>" + sl + "&nbsp;</td>";
            }
            else if (lp->is_online ())
            {
                online = (String) "<td id='o" + sl + "' style='text-align:right;background-color:green;color:white'>" + sl + "&nbsp;</td>";
            }
            else
            {
                online = (String) "<td id='o" + sl + "' style='text-align:right'>" + sl + "&nbsp;</td>";
            }

            std::string slocation = HTTP_Common::get_location (loco_idx);

            HTTP::response += (String)
                "<tr " + bg + ">" + online + "<td style='width:200px;overflow:hidden' nowrap>";

            if (lp->is_active())
            {
                HTTP::response += (String) "<a href='/loco?action=loco&lidx=" + sl + "'>" + lp->get_name() + "</a></td>";
                disabled = "";
            }
            else
            {
                HTTP::response += (String) "<font color='gray'>" + lp->get_name() + "</font>";
                disabled = "disabled";
            }

            HTTP::response += (String)
                "</td>"
                "<td align='left' style='width:200px;overflow:hidden' id='loc" + sl + "' nowrap>" + slocation + "</td>"
                "<td nowrap>"
                "<button id='m1' onclick='macro(" + sl + ", 0)' " + disabled + ">MF</button>"
                "<button id='m2' onclick='macro(" + sl + ", 1)' " + disabled + ">MH</button>"
                "</td>"
                "<td class='hide600' align='right'>" + std::to_string(lp->get_addr()) + "</td>"
                "<td class='hide650' align='right' width='40' id='rc2r" + sl + "'>" + status + "</td>";

            if (HTTP_Common::edit_mode)
            {
                HTTP::response += (String) 
                    "<td><button onclick=\""
                    "changeloco(" + sl + ",'" + lp->get_name() +
                    "'," + std::to_string(lp->get_addr()) + "," + std::to_string(lp->get_speed_steps()) + "," + std::to_string(lp->is_active()) +
                    ")\">Bearbeiten</button></td>";
            }

            HTTP::response += (String) "</tr>\r\n";
            HTTP::flush ();
        }

        HTTP::response += (String) "</table>\r\n";

        HTTP::response += (String)
            "<script>\r\n"
            "function changeloco(lidx, name, addr, steps, act)"
            "{"
            "  document.getElementById('action').value = 'changeloco';"
            "  document.getElementById('lidx').value = lidx;"
            "  document.getElementById('nlidx').value = lidx;"
            "  document.getElementById('name').value = name;"
            "  document.getElementById('addr').value = addr;"
            "  document.getElementById('steps').value = steps;"
            "  document.getElementById('active').checked = act;"
            "  document.getElementById('newid').style.display='';"
            "  document.getElementById('formloco').style.display='';"
            "}\r\n"
            "function newloco()"
            "{"
            "  document.getElementById('action').value = 'newloco';"
            "  document.getElementById('name').value = '';"
            "  document.getElementById('addr').value = '';"
            "  document.getElementById('steps').value = '128';"
            "  document.getElementById('active').checked = true;"
            "  document.getElementById('newid').style.display='none';"
            "  document.getElementById('formloco').style.display='';"
            "}\r\n"
            "</script>\r\n";

        HTTP::flush ();

        if (Locos::data_changed)
        {
            HTTP::response += (String)
                "<BR><form method='get' action='" + url + "'>\r\n"
                "<input type='hidden' name='action' value='saveloco'>\r\n"
                "<input type='submit' value='Ge&auml;nderte Konfiguration auf Datentr&auml;ger speichern'>"
                "</form>\r\n";
        }

        if (HTTP_Common::edit_mode)
        {
            HTTP::response += (String)
                "<BR><button onclick='newloco()'>Neue Lok</button>\r\n"
                "<form id='formloco' style='display:none'>\r\n"
                "<table>\r\n"
                "<tr id='newid'>\r\n";

            HTTP_Common::print_id_select_list ("nlidx", n_locos);

            HTTP::response += (String)
                "</tr>\r\n"
                "<tr><td>Name:</td><td><input type='text' id='name' name='name'></td></tr>\r\n"
                "<tr><td>Adresse:</td><td><input type='text' id='addr' name='addr'></td></tr>\r\n"
                "<tr><td>Fahrstufen:</td><td><select class='select' id='steps' name='steps'><option value='14'>14</option><option value='28'>28</option><option value='128' selected>128</option></select></td></tr>\r\n"
                "<tr><td>Aktiv:</td><td><input type='checkbox' id='active' name='active' value='1'></td></tr>\r\n"
                "</td></tr>\r\n"
                "<tr><td></td><td align='right'>"
                "<input type='hidden' name='action' id='action' value='newloco'>"
                "<input type='hidden' name='lidx' id='lidx'>"
                "<input type='hidden' name='sort' value='" + std::to_string(nsort) + "'>"
                "<input type='submit' value='Speichern'></td></tr>\r\n"
                "</table>\r\n"
                "</form>\r\n";
        }
    }

    HTTP::response += (String) "</div>\r\n";
    HTTP_Common::html_trailer ();
}

void
HTTP_Loco::action_locos (void)
{
    uint_fast16_t   n_locos = Locos::get_n_locos ();
    uint_fast16_t   loco_idx;

    HTTP_Common::head_action ();

    for (loco_idx = 0; loco_idx < n_locos; loco_idx++)
    {
        auto&           Loco        = Locos::locos[loco_idx];
        String          sl          = std::to_string(loco_idx);
        uint_fast8_t    is_online   = Loco.is_online ();
        bool            is_halt     = Loco.get_flag_halt ();
        uint_fast8_t    rc2_rate    = Loco.get_rc2_rate ();
        String          id;

        id = (String) "o" + sl;

        if (is_halt)
        {
            HTTP_Common::add_action_content (id, "bgcolor", "red");
            HTTP_Common::add_action_content (id, "color", "white");
        }
        else if (is_online)
        {
            HTTP_Common::add_action_content (id, "bgcolor", "green");
            HTTP_Common::add_action_content (id, "color", "white");
        }
        else
        {
            HTTP_Common::add_action_content (id, "bgcolor", "");
            HTTP_Common::add_action_content (id, "color", "");
        }

        id = (String) "rc2r" + sl;
        HTTP_Common::add_action_content (id, "text", std::to_string(rc2_rate) + "%");

        std::string slocation = HTTP_Common::get_location (loco_idx);

        HTTP_Common::add_action_content ((String) "loc" + sl, "text", slocation);
    }
}

void
HTTP_Loco::action_loco (void)
{
    uint_fast16_t   loco_idx        = HTTP::parameter_number ("loco_idx");
    uint_fast16_t   addon_idx       = HTTP::parameter_number ("addon_idx");
    uint_fast8_t    speed;
    uint_fast8_t    fwd;
    uint_fast8_t    fidx;
    bool            is_online;
    bool            is_halted;
    uint_fast8_t    destination;
    uint32_t        functions;
    String          sspeed;
    String          sdestination;
    auto&           Loco            = Locos::locos[loco_idx];
    String          sl              = std::to_string (loco_idx);
    String          sa              = std::to_string (addon_idx);

    HTTP_Common::head_action ();

    speed       = Loco.get_speed ();
    fwd         = Loco.get_fwd ();
    functions   = Loco.get_functions ();
    is_online   = Loco.is_online ();
    is_halted   = Loco.get_flag_halt ();
    destination = Loco.get_destination ();

    std::string slocation = HTTP_Common::get_location (loco_idx);

    if (is_online)
    {
        HTTP_Common::add_action_content ("on", "bgcolor", "green");
        HTTP_Common::add_action_content ("on", "color", "white");
    }
    else
    {
        HTTP_Common::add_action_content ("on", "bgcolor", "");
        HTTP_Common::add_action_content ("on", "color", "");
    }

    if (is_halted)
    {
        HTTP_Common::add_action_content ("loc", "color", "red");
        HTTP_Common::add_action_content ("go", "display", "");
    }
    else
    {
        HTTP_Common::add_action_content ("loc", "color", "green");
        HTTP_Common::add_action_content ("go", "display", "none");
    }

    HTTP_Common::add_action_content ("loc", "text", slocation);

    sspeed = std::to_string(speed);
    HTTP_Common::add_action_content ("speed", "value", sspeed);
    HTTP_Common::add_action_content ("out", "text", sspeed);
    HTTP_Common::add_action_content ("fwd", "value", std::to_string(fwd));

    if (fwd)
    {
        HTTP_Common::add_action_content ("b", "color", "inherit");
        HTTP_Common::add_action_content ("b", "bgcolor", "inherit");
        HTTP_Common::add_action_content ("f", "color", "white");
        HTTP_Common::add_action_content ("f", "bgcolor", "green");
    }
    else
    {
        HTTP_Common::add_action_content ("b", "color", "white");
        HTTP_Common::add_action_content ("b", "bgcolor", "green");
        HTTP_Common::add_action_content ("f", "color", "inherit");
        HTTP_Common::add_action_content ("f", "bgcolor", "inherit");
    }

    sdestination = std::to_string(destination);
    HTTP_Common::add_action_content ("dest", "value", sdestination);

    for (fidx = 0; fidx < MAX_LOCO_FUNCTIONS; fidx++)
    {
        String          sfidx               = std::to_string (fidx);
        uint_fast16_t   function_name_idx   = Loco.get_function_name_idx (fidx);

        if (function_name_idx != 0xFFFF)
        {
            if (functions & (1 << fidx))
            {
                HTTP_Common::add_action_content ((String) "bg" + sl + "_" + sfidx, "color", "white");
                HTTP_Common::add_action_content ((String) "bg" + sl + "_" + sfidx, "bgcolor", "green");
                HTTP_Common::add_action_content ((String) "t"  + sl + "_" + sfidx, "text", "Aus");
                HTTP_Common::add_action_content ((String) "t"  + sl + "_" + sfidx, "color", "red");
            }
            else
            {
                HTTP_Common::add_action_content ((String) "bg" + sl + "_" + sfidx, "color", "inherit");
                HTTP_Common::add_action_content ((String) "bg" + sl + "_" + sfidx, "bgcolor", "inherit");
                HTTP_Common::add_action_content ((String) "t"  + sl + "_" + sfidx, "text", "Ein");
                HTTP_Common::add_action_content ((String) "t"  + sl + "_" + sfidx, "color", "green");
            }
        }
    }

    if (addon_idx != 0xFFFF)
    {
        functions = AddOns::addons[addon_idx].get_functions ();

        for (fidx = 0; fidx < MAX_LOCO_FUNCTIONS; fidx++)
        {
            String          sfidx               = std::to_string (fidx);
            uint_fast16_t   function_name_idx   = AddOns::addons[addon_idx].get_function_name_idx (fidx);

            if (function_name_idx != 0xFFFF)
            {
                if (functions & (1 << fidx))
                {
                    HTTP_Common::add_action_content ((String) "bga" + sa + "_" + sfidx, "color", "white");
                    HTTP_Common::add_action_content ((String) "bga" + sa + "_" + sfidx, "bgcolor", "green");
                    HTTP_Common::add_action_content ((String) "ta"  + sa + "_" + sfidx, "text", "Aus");
                    HTTP_Common::add_action_content ((String) "ta"  + sa + "_" + sfidx, "color", "red");
                }
                else
                {
                    HTTP_Common::add_action_content ((String) "bga" + sa + "_" + sfidx, "color", "inherit");
                    HTTP_Common::add_action_content ((String) "bga" + sa + "_" + sfidx, "bgcolor", "inherit");
                    HTTP_Common::add_action_content ((String) "ta"  + sa + "_" + sfidx, "text", "Ein");
                    HTTP_Common::add_action_content ((String) "ta"  + sa + "_" + sfidx, "color", "green");
                }
            }
        }
    }
}

void
HTTP_Loco::action_getf ()
{
    uint_fast16_t   lidx        = HTTP::parameter_number ("lidx");
    uint_fast8_t    f           = HTTP::parameter_number ("f");
    uint_fast16_t   fn          = Locos::locos[lidx].get_function_name_idx (f);
    bool            pulse       = Locos::locos[lidx].get_function_pulse (f);
    bool            sound       = Locos::locos[lidx].get_function_sound (f);
    uint_fast8_t    af          = Locos::locos[lidx].get_coupled_function (f);

    HTTP::response = std::to_string(fn) + "\t" + std::to_string(pulse) + "\t" + std::to_string(sound) + "\t" + std::to_string(af);
}

void
HTTP_Loco::action_setspeed (void)
{
    uint_fast16_t   loco_idx    = HTTP::parameter_number ("loco_idx");
    uint_fast8_t    speed       = HTTP::parameter_number ("speed");
    auto&           Loco        = Locos::locos[loco_idx];
    uint_fast8_t    fwd;

    if (speed & 0xf80)
    {
        speed &= 0x7F;
        fwd = 1;
    }
    else
    {
        fwd = 0;
    }

    Loco.set_speed (speed);
    Loco.set_fwd (fwd);

    if (fwd)
    {
        speed |= 0x80;
    }

    HTTP::response = (String) "" + std::to_string(speed);
}

void
HTTP_Loco::action_togglefunction (void)
{
    uint_fast16_t   loco_idx    = HTTP::parameter_number ("loco_idx");
    uint_fast8_t    fidx        = HTTP::parameter_number ("f");
    auto&           Loco        = Locos::locos[loco_idx];
    uint_fast8_t    value;

    if (Loco.get_function (fidx))
    {
        value = 0;
    }
    else
    {
        value = 1;
    }

    Loco.set_function (fidx, value);
    HTTP::response = (String) "" + std::to_string(value);
}

void
HTTP_Loco::action_setdestination (void)
{
    uint_fast16_t   loco_idx    = HTTP::parameter_number ("loco_idx");
    uint_fast8_t    dest        = HTTP::parameter_number ("dest");
    auto&           Loco        = Locos::locos[loco_idx];

    Loco.set_destination (dest);
    HTTP::response = (String) "" + std::to_string(dest);
}

void
HTTP_Loco::action_macro (void)
{
    uint_fast16_t   loco_idx    = HTTP::parameter_number ("loco_idx");
    uint_fast8_t    macroidx    = HTTP::parameter_number ("idx");
    auto&           Loco        = Locos::locos[loco_idx];

    Loco.execute_macro (macroidx);
}
