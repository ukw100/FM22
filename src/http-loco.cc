/*------------------------------------------------------------------------------------------------------------------------
 * http-loco.cc - HTTP loco routines
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
#include "addon.h"
#include "func.h"
#include "base.h"
#include "http.h"
#include "http-common.h"
#include "http-loco.h"

static void
print_loco_macro_action (uint_fast8_t idx, uint_fast8_t action)
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

    HTTP::response += (String) "<select name='ac" + std::to_string(caidx) + "' id='ac" + std::to_string(caidx) + "' onchange=\"changeca(" + std::to_string(caidx) + ")\">\r\n";

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
    uint_fast16_t   addon_idx = Loco::get_addon (loco_idx);

    n_actions = Loco::get_n_macro_actions (loco_idx, macroidx);

    for (actionidx = 0; actionidx < LOCO_MAX_ACTIONS_PER_MACRO; actionidx++)
    {
        uint_fast8_t    rtc;

        HTTP::response += (String) "<tr><td>Aktion " + std::to_string(actionidx) + ":</td><td>";

        if (actionidx < n_actions)
        {
            rtc = Loco::get_macro_action (loco_idx, macroidx, actionidx, &la);

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
            HTTP_Common::print_start_list ((String) "st" + std::to_string(actionidx), start_tenths, do_display_start_tenths);

            HTTP_Common::print_speed_list ((String) "sp" + std::to_string(actionidx), speed, do_display_speed);
            HTTP_Common::print_fwd_list ((String) "fwd" + std::to_string(actionidx), fwd, do_display_fwd);
            HTTP_Common::print_function_list (loco_idx, true, (String) "f" + std::to_string(actionidx), f, do_display_f);

            if (addon_idx != 0xFFFF)
            {
                HTTP_Common::print_function_list (addon_idx, false, (String) "fa" + std::to_string(actionidx), fa, do_display_fa);
            }

            HTTP_Common::print_tenths_list ((String) "te" + std::to_string(actionidx), tenths, do_display_tenths);
        }

        HTTP::response += (String) "</td></tr>\r\n";
    }

    while (actionidx < LOCO_MAX_ACTIONS_PER_MACRO)
    {
        HTTP::response += (String) "<tr><td>Aktion " + std::to_string(actionidx) + ":</td><td></td></tr>\r\n";
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
    const char *    smacroidx           = HTTP::parameter ("midx");
    uint_fast16_t   macroidx            = atoi (smacroidx);
    char *          loco_name           = Loco::get_name (loco_idx);
    String          url                 = (String) "/loco?action=loco&lidx=" + sloco_idx;
    String          title               = (String) "<a href='" + url + "'>" + loco_name + "</a> &rarr; " + "Macro bearbeiten";
    uint_fast16_t   addon_idx           = Loco::get_addon (loco_idx);

    HTTP_Common::html_header (browsertitle, title, url, true);
    HTTP::response += (String) "<div style='margin-left:20px;'>\r\n";
    HTTP_Common::add_action_handler ("head", "", 200, true);

    HTTP::response += (String) "<script>\r\n";
    HTTP::response += (String) "function changeca(caidx)\r\n";
    HTTP::response += (String) "{\r\n";
    HTTP::response += (String) "  var obj = document.getElementById('ac' + caidx);\r\n";
    HTTP::response += (String) "  var st = 'none'\r\n";
    HTTP::response += (String) "  var sp = 'none'\r\n";
    HTTP::response += (String) "  var fwd = 'none'\r\n";
    HTTP::response += (String) "  var f = 'none'\r\n";
    HTTP::response += (String) "  var fa = 'none'\r\n";
    HTTP::response += (String) "  var te = 'none'\r\n";

    HTTP::response += (String) "  switch (parseInt(obj.value))\r\n";
    HTTP::response += (String) "  {\r\n";
    HTTP::response += (String) "    case " + std::to_string(LOCO_ACTION_NONE)                          + ": { break; }\r\n";
    HTTP::response += (String) "    case " + std::to_string(LOCO_ACTION_SET_LOCO_SPEED)                + ": { st = ''; sp = ''; te = ''; break }\r\n";
    HTTP::response += (String) "    case " + std::to_string(LOCO_ACTION_SET_LOCO_MIN_SPEED)            + ": { st = ''; sp = ''; te = ''; break; }\r\n";
    HTTP::response += (String) "    case " + std::to_string(LOCO_ACTION_SET_LOCO_MAX_SPEED)            + ": { st = ''; sp = ''; te = ''; break; }\r\n";
    HTTP::response += (String) "    case " + std::to_string(LOCO_ACTION_SET_LOCO_FORWARD_DIRECTION)    + ": { st = ''; fwd = ''; break; }\r\n";
    HTTP::response += (String) "    case " + std::to_string(LOCO_ACTION_SET_LOCO_ALL_FUNCTIONS_OFF)    + ": { st = ''; break; }\r\n";
    HTTP::response += (String) "    case " + std::to_string(LOCO_ACTION_SET_LOCO_FUNCTION_OFF)         + ": { st = ''; f = ''; break; }\r\n";
    HTTP::response += (String) "    case " + std::to_string(LOCO_ACTION_SET_LOCO_FUNCTION_ON)          + ": { st = ''; f = ''; break; }\r\n";
    HTTP::response += (String) "    case " + std::to_string(LOCO_ACTION_SET_ADDON_ALL_FUNCTIONS_OFF)   + ": { st = ''; break; }\r\n";
    HTTP::response += (String) "    case " + std::to_string(LOCO_ACTION_SET_ADDON_FUNCTION_OFF)        + ": { st = ''; fa = ''; break; }\r\n";
    HTTP::response += (String) "    case " + std::to_string(LOCO_ACTION_SET_ADDON_FUNCTION_ON)         + ": { st = ''; fa = ''; break; }\r\n";
    HTTP::response += (String) "  }\r\n";

    HTTP::response += (String) "  document.getElementById('st' + caidx).style.display = st;\r\n";
    HTTP::response += (String) "  document.getElementById('sp' + caidx).style.display = sp;\r\n";
    HTTP::response += (String) "  document.getElementById('fwd' + caidx).style.display = fwd;\r\n";
    HTTP::response += (String) "  document.getElementById('f' + caidx).style.display = f;\r\n";

    if (addon_idx != 0xFFFF)
    {
        HTTP::response += (String) "  document.getElementById('fa' + caidx).style.display = fa;\r\n";
    }

    HTTP::response += (String) "  document.getElementById('te' + caidx).style.display = te;\r\n";
    HTTP::response += (String) "}\r\n";
    HTTP::response += (String) "</script>\r\n";

    HTTP::flush ();

    HTTP::response += (String) "<form method='get' action='" + url + "'>"
                + "<table style='border:1px lightgray solid;'>\r\n"
                + "<tr bgcolor='#e0e0e0'><td>ID:</td><td>M" + std::to_string(macroidx + 1) + "</td></tr>\r\n";

    HTTP::flush ();

    HTTP::response += (String) "<tr><td colspan='2'>Aktionen:</td></tr>";
    print_loco_macro_actions (loco_idx, macroidx);

    HTTP::flush ();

    HTTP::response += (String) "<tr><td><BR></td></tr>\r\n";
    HTTP::response += (String) "<tr><td><input type='button'  onclick=\"window.location.href='" + url + "?action=loco&lidx=" + std::to_string(loco_idx) + "';\" value='Zur&uuml;ck'></td>";
    HTTP::response += (String) "<td align='right'><input type='hidden' name='action' value='changelm'>";
    HTTP::response += (String) "<input type='hidden' name='lidx' value='" + sloco_idx + "'><input type='hidden' name='midx' value='" + smacroidx + "'>";
    HTTP::response += (String) "<button >Speichern</button></td></tr>";

    HTTP::response += (String) "</table>\r\n";
    HTTP::response += (String) "</form>\r\n";

    HTTP::response += (String) "</div>\r\n";
    HTTP_Common::html_trailer ();
}

static void
change_loco_macro_actions (uint_fast16_t loco_idx, uint_fast16_t macroidx)
{
    LOCOACTION          la;
    uint_fast8_t        n_actions;
    uint_fast8_t        slotidx = 0;
    uint_fast8_t        idx;

    n_actions = Loco::get_n_macro_actions (loco_idx, macroidx);

    for (idx = 0; idx < LOCO_MAX_ACTIONS_PER_MACRO; idx++)
    {
        uint_fast16_t   st  = HTTP::parameter_number ((String) "st" + std::to_string(idx));
        uint_fast8_t    ac  = HTTP::parameter_number ((String) "ac" + std::to_string(idx));
        uint_fast8_t    sp  = HTTP::parameter_number ((String) "sp" + std::to_string(idx));
        uint_fast8_t    te  = HTTP::parameter_number ((String) "te" + std::to_string(idx));
        uint_fast8_t    fwd = HTTP::parameter_number ((String) "fwd" + std::to_string(idx));
        uint_fast8_t    f   = HTTP::parameter_number ((String) "f" + std::to_string(idx));
        uint_fast8_t    fa  = HTTP::parameter_number ((String) "fa" + std::to_string(idx));

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
                Loco::add_macro_action (loco_idx, macroidx);
                n_actions = Loco::get_n_macro_actions (loco_idx, macroidx);
            }

            Loco::set_macro_action (loco_idx, macroidx, slotidx, &la);
            slotidx++;
        }
    }

    while (slotidx < n_actions)
    {
        Loco::delete_macro_action (loco_idx, macroidx, slotidx);
        n_actions = Loco::get_n_macro_actions (loco_idx, macroidx);
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
    HTTP::response += (String) "<select class='select' name='" + name + "' id='" + name + "' style='width:250px'>\r\n";
    HTTP::response += (String) "<option value='65535'>--- Keine Funktion ---</option>\r\n";

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

    if (is_addon)
    {
        pulse_mask  = AddOn::get_function_pulse_mask (idx);
        sound_mask  = AddOn::get_function_sound_mask (idx);
        functions   = AddOn::get_functions (idx);
        addon_idx   = 0xFFFF;
    }
    else
    {
        pulse_mask  = Loco::get_function_pulse_mask (idx);
        sound_mask  = Loco::get_function_sound_mask (idx);
        functions   = Loco::get_functions (idx);
        addon_idx   = Loco::get_addon (idx);
    }

    for (fidx = 0; fidx < MAX_LOCO_FUNCTIONS; fidx++)
    {
        uint_fast16_t   function_name_idx;

        if (is_addon)
        {
            function_name_idx = AddOn::get_function_name_idx (idx, fidx);
        }
        else
        {
            function_name_idx = Loco::get_function_name_idx (idx, fidx);
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

            HTTP::response += (String) "<tr " + bg + "><td id='" + bgid + std::to_string(idx) + "_" + std::to_string(fidx) + "' " + style + " align='center'>F" + std::to_string(fidx) + "</td><td>"
                      + fnname + "</td><td align='center'>" + pulse + "</td><td align='center'>" + sound + "</td>";

            if (addon_idx != 0xFFFF)
            {
                uint_fast8_t addon_fidx = Loco::get_coupled_function (idx, fidx);

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
                HTTP::response += (String) "<td align='center'><button id='ta" + std::to_string(idx) + "_" + std::to_string(fidx) + "' style='width:60px;' onclick='togglefa(" + std::to_string(idx) + "," + std::to_string(fidx) + ")'>";
            }
            else
            {
                HTTP::response += (String) "<td align='center'><button id='t" + std::to_string(idx) + "_" + std::to_string(fidx) + "' style='width:60px;' onclick='togglef(" + std::to_string(idx) + "," + std::to_string(fidx) + ")'>";
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
    uint_fast16_t   loco_idx        = 0;

    if (! strcmp (action, "loco") || ! strcmp (action, "changefunc") || ! strcmp (action, "changefuncaddon") || ! strcmp (action, "changelm"))
    {
        loco_idx = HTTP::parameter_number ("lidx");

        browsertitle = (String) Loco::get_name(loco_idx);
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

        uint_fast16_t   lidx;

        strncpy (name, sname, 127);
        name[127] = 0;

        lidx = Loco::add ();

        if (lidx != 0xFFFF && addr != 0)
        {
            Loco::set_name (lidx, name);
            Loco::set_addr (lidx, addr);
            Loco::set_speed_steps (lidx, steps);
            Loco::activate (lidx);
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

        if (lidx != nlidx)
        {
            nlidx = Loco::setid (lidx, nlidx);
        }

        if (nlidx != 0xFFFF)
        {
            Loco::set_name (nlidx, sname);
            Loco::set_addr (nlidx, addr);
            Loco::set_speed_steps (nlidx, steps);
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

        Loco::set_function_type (loco_idx, fidx, function_name_idx, pulse, sound);
        Loco::set_coupled_function (loco_idx, fidx, afidx);     // also if afidx == 0xFF: reset coupled function
    }
    else if (! strcmp (action, "changefuncaddon"))
    {
        uint_fast16_t   addon_idx           = HTTP::parameter_number ("aidx");
        uint_fast8_t    fidx                = HTTP::parameter_number ("addonfidx");
        uint_fast16_t   function_name_idx   = HTTP::parameter_number ("addonfnidx");
        uint_fast8_t    pulse               = HTTP::parameter_number ("addonpulse");
        uint_fast8_t    sound               = HTTP::parameter_number ("addonsound");

        AddOn::set_function_type (addon_idx, fidx, function_name_idx, pulse, sound);
    }

    HTTP::response += (String) "<script>\r\n";
    HTTP::response += (String) "function macro(li, idx)\r\n";
    HTTP::response += (String) "{\r\n";
    HTTP::response += (String) "  var http = new XMLHttpRequest();\r\n";
    HTTP::response += (String) "  http.open ('GET', '/action?action=macro&loco_idx=' + li + '&idx=' + idx);\r\n";
    HTTP::response += (String) "  http.addEventListener('load', function(event) {});\r\n";
    HTTP::response += (String) "  http.send (null);\r\n";
    HTTP::response += (String) "}\r\n";
    HTTP::response += (String) "</script>\r\n";

    if (! strcmp (action, "loco") || ! strcmp (action, "changefunc") || ! strcmp (action, "changefuncaddon") || ! strcmp (action, "changelm"))
    {
        char            param[64];
        uint_fast16_t   addon_idx;
        uint_fast8_t    speed;
        uint_fast8_t    fwd;
        String          addon_name = "";

        addon_idx   = Loco::get_addon (loco_idx);
        speed       = Loco::get_speed (loco_idx);
        fwd         = Loco::get_fwd (loco_idx);

        if (addon_idx != 0xFFFF)
        {
            addon_name = AddOn::get_name (addon_idx);
        }

        HTTP::response += (String) "<script>\r\n";
        HTTP::response += (String) "function togglef (li, fi) { var http = new XMLHttpRequest(); http.open ('GET', '/action?action=togglefunction&loco_idx=' + li + '&f=' + fi);";
        HTTP::response += (String) "http.addEventListener('load',";
        HTTP::response += (String) "function(event) { var o; var text = http.responseText; if (http.status >= 200 && http.status < 300) { ";
        HTTP::response += (String) "var color; if (parseInt(text) == 1) { text='Aus'; color='red' } else { text='Ein'; color='green' } ";
        HTTP::response += (String) "o = document.getElementById('t' + li + '_' + fi); o.textContent = text; o.color = color; } else alert (http.status); });";
        HTTP::response += (String) "http.send (null);}\r\n";

        HTTP::response += (String) "function togglefa (ai, fi) { var http = new XMLHttpRequest(); http.open ('GET', '/action?action=togglefunctionaddon&addon_idx=' + ai + '&f=' + fi);";
        HTTP::response += (String) "http.addEventListener('load',";
        HTTP::response += (String) "function(event) { var o; var text = http.responseText; if (http.status >= 200 && http.status < 300) { ";
        HTTP::response += (String) "var color; if (parseInt(text) == 1) { text='Aus'; color='red' } else { text='Ein'; color='green' } ";
        HTTP::response += (String) "o = document.getElementById('t' + ai + '_' + fi); o.textContent = text; o.color = color; } else alert (http.status); });";
        HTTP::response += (String) "http.send (null);}\r\n";

        sprintf (param, "loco_idx=%u&addon_idx=%u", (unsigned int) loco_idx, (unsigned int) addon_idx);
        HTTP_Common::add_action_handler ("loco", param, 200);

        HTTP::response += (String) "function upd_fwd (f) { var fo = document.getElementById('f'); var bo = document.getElementById('b');\r\n";
        HTTP::response += (String) "var fwdo = document.getElementById('fwd'); fwdo.value = f;\r\n";
        HTTP::response += (String) "if (f) { fo.style.backgroundColor = 'green'; fo.style.color = 'white'; bo.style.backgroundColor = 'inherit'; bo.style.color = 'inherit'; }\r\n";
        HTTP::response += (String) "else { fo.style.backgroundColor = 'inherit'; fo.style.color = 'inherit'; bo.style.backgroundColor = 'green'; bo.style.color = 'white'; }}\r\n";

        HTTP::response += (String) "function upd_speed (s) { var slider = document.getElementById('speed'); slider.value = s; var o = document.getElementById('out'); o.textContent = s; }\r\n";

        HTTP::response += (String) "function incslider (li) { var slider = document.getElementById('speed'); var speed = parseInt(slider.value);\r\n";
        HTTP::response += (String) "speed += 4; if (speed > 127) { speed = 127; } setspeed (li, speed); }\r\n";
        HTTP::response += (String) "function decslider (li) { var slider = document.getElementById('speed'); var speed = parseInt(slider.value);\r\n";
        HTTP::response += (String) "speed -= 4; if (speed <= 1) speed = 0; setspeed (li, speed);}\r\n";
        HTTP::flush ();

        HTTP::response += (String) "function setspeed (li, speed) { var fwdo = document.getElementById('fwd'); var fwd = parseInt (fwdo.value); if (fwd) { speed |= 0x80; }\r\n";
        HTTP::response += (String) "var http = new XMLHttpRequest(); http.open ('GET', '/action?action=setspeed&loco_idx=' + li + '&speed=' + speed);\r\n";
        HTTP::response += (String) "http.addEventListener('load', function(event) { var o; var text = http.responseText; if (http.status >= 200 && http.status < 300) { ";
        HTTP::response += (String) "var fwd = 0; var speed = parseInt(text); if (speed & 0x80) { fwd = 1; speed &= 0x7F; } upd_speed(speed); upd_fwd (fwd); }});";
        HTTP::response += (String) "http.send (null);}\r\n";

        HTTP::response += (String) "function bwd (li) { var fwdo = document.getElementById('fwd'); fwdo.value = 0;";
        HTTP::response += (String) "var slider = document.getElementById('speed'); var speed = parseInt(slider.value); setspeed (li, speed); }\r\n";
        HTTP::response += (String) "function fwd (li) { var fwdo = document.getElementById('fwd'); fwdo.value = 1;";
        HTTP::response += (String) "var slider = document.getElementById('speed'); var speed = parseInt(slider.value) | 0x80; setspeed (li, speed); }\r\n";
        HTTP::response += (String) "function upd_slider (li) { var slider = document.getElementById('speed'); var speed = parseInt(slider.value);\r\n";
        HTTP::response += (String) "if (speed == 1) { var output = document.getElementById('out'); var oldspeed = parseInt(output.textContent);\r\n";
        HTTP::response += (String) "if (oldspeed == 0) { speed = 2; } else { speed = 0; }} setspeed (li, speed); }\r\n";
        HTTP::response += (String) "function go (li) { var http = new XMLHttpRequest(); http.open ('GET', '/action?action=go&loco_idx=' + li);\r\n";
//        HTTP::response += (String) "http.addEventListener('load', function(event) { });\r\n";
        HTTP::response += (String) "http.send (null);}\r\n";

        HTTP::response += (String) "</script>\r\n";

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

        HTTP::response += (String) "<table style='border:1px lightgray solid;'>\r\n";
        HTTP::response += (String) "<tr><td colspan='" + colspan + "'>";
        HTTP::response += (String) "  <div style='float:left;width:45%;display:block;text-align:left'>Adresse: " + std::to_string(Loco::get_addr(loco_idx)) + "</div>";
        HTTP::response += (String) "  <div id='go' style='float:left;display:block;text-align:right;display:none'><button onclick='go(" + std::to_string(loco_idx) + ")' style='color:green'>Fahrt!</button></div>\r\n";
        HTTP::response += (String) "  <div id='loc' style='display:block;text-align:right'></td></tr>\r\n";
        HTTP::response += (String) "<tr><td align='center' colspan='" + colspan + "'>\r\n";
        HTTP::response += (String) "<div style='width:350px;padding-top:0px;padding-bottom:10px'>";
        HTTP::response += (String) "  <input type='hidden' id='fwd' value='" + std::to_string(fwd) + "'>";
        HTTP::response += (String) "  <div style='width:300px;text-align:center;'>";
        HTTP::response += (String) "    <button id='m1' onclick='macro(" + std::to_string(loco_idx) + ", 0)'>MF</button>";
        HTTP::response += (String) "    <button id='m2' onclick='macro(" + std::to_string(loco_idx) + ", 1)'>MH</button>";
        HTTP::response += (String) "    <button id='m3' onclick='macro(" + std::to_string(loco_idx) + ", 2)'>M3</button>";
        HTTP::response += (String) "    <button id='m4' onclick='macro(" + std::to_string(loco_idx) + ", 3)'>M4</button>";
        HTTP::response += (String) "    <button id='m5' onclick='macro(" + std::to_string(loco_idx) + ", 4)'>M5</button>";
        HTTP::response += (String) "    <button id='m6' onclick='macro(" + std::to_string(loco_idx) + ", 5)'>M6</button>";
        HTTP::response += (String) "    <button id='m7' onclick='macro(" + std::to_string(loco_idx) + ", 6)'>M7</button>";
        HTTP::response += (String) "    <button id='m8' onclick='macro(" + std::to_string(loco_idx) + ", 7)'>M8</button>";
        HTTP::response += (String) "  </div>";
        HTTP::response += (String) "  <div id='out' style='width:300px;margin-top:10px;text-align:center;'>" + std::to_string(speed) + "</div>";
        HTTP::response += (String) "  <button style='vertical-align:top' onclick='decslider(" + std::to_string(loco_idx) + ")'>-</button>";
        HTTP::response += (String) "  <input type='range' min='0' max='127' value='" + std::to_string(speed) + "' id='speed' style='width:256px' onchange='upd_slider(" + std::to_string(loco_idx) + ")'>";
        HTTP::response += (String) "  <button style='vertical-align:top' onclick='incslider(" + std::to_string(loco_idx) + ")'>+</button>";
        HTTP::response += (String) "  <div style='width:300px;text-align:center'>";
        HTTP::response += (String) "    <button id='b' " + bwd_color + " onclick='bwd(" + std::to_string(loco_idx) + ")'>&#9664;</button>";
        HTTP::response += (String) "    <button onclick='setspeed(" + std::to_string(loco_idx) + ",0);'>&#9209;</button>";
        HTTP::response += (String) "    <button onclick='setspeed(" + std::to_string(loco_idx) + ",1);'>&#9210;</button>";
        HTTP::response += (String) "    <button id='f' " + fwd_color + " onclick='fwd(" + std::to_string(loco_idx) + ")'>&#9654;</button>";
        HTTP::response += (String) "  </div>";
        HTTP::response += (String) "</div>\r\n";
        HTTP::response += (String) "</td></tr>\r\n";

        HTTP::flush ();

        HTTP::response += (String) "<tr bgcolor='#e0e0e0'><th style='width:30px;'>F</th><th style='width:250px;'>Funktion</th>";
        HTTP::response += (String) "<th style='width:50px;'>Puls</th><th style='width:50px;'>Sound</th>";

        if (addon_idx != 0xFFFF)
        {
            HTTP::response += (String) "<th style='width:20px'>KF</th>";
        }

        HTTP::response += (String) "<th>Aktion</th></tr>\r\n";

        print_functions (loco_idx, false);

        HTTP::response += (String) "</table>\r\n";
        HTTP::response += (String) "<script>\r\n";
        HTTP::response += (String) "function changef()\r\n";
        HTTP::response += (String) "{\r\n";
        HTTP::response += (String) "  var f = document.getElementById('fidx').value;\r\n";
        HTTP::response += (String) "  var url = '/action?action=getf&lidx=" + std::to_string(loco_idx) + "&f=' + f;\r\n";
        HTTP::response += (String) "  var http = new XMLHttpRequest(); http.open ('GET', url);\r\n";
        HTTP::response += (String) "  http.addEventListener('load', function(event) { if (http.status >= 200 && http.status < 300) { ";
        HTTP::response += (String) "    var text = http.responseText;\r\n";
        HTTP::response += (String) "    const a = text.split('\t');\r\n";
        HTTP::response += (String) "    document.getElementById('fnidx').value = a[0];\r\n";
        HTTP::response += (String) "    document.getElementById('pulse').value = a[1];\r\n";
        HTTP::response += (String) "    document.getElementById('sound').value = a[2];\r\n";
        HTTP::response += (String) "    var oo; oo = document.getElementById('afidx');\r\n";
        HTTP::response += (String) "    if (oo) { oo.value = a[3]; }\r\n";
        HTTP::response += (String) "  }});\r\n";
        HTTP::response += (String) "  http.send (null);\r\n";
        HTTP::response += (String) "}\r\n";
        HTTP::response += (String) "function changefunc()\r\n";
        HTTP::response += (String) "{\r\n";
        HTTP::response += (String) "  document.getElementById('formfunc').style.display='';\r\n";
        HTTP::response += (String) "  document.getElementById('btnchgfunc').style.display='none';\r\n";
        HTTP::response += (String) "  changef();\r\n";
        HTTP::response += (String) "}\r\n";
        HTTP::response += (String) "</script>\r\n";

        if (HTTP_Common::edit_mode)
        {
            uint_fast8_t i;

            HTTP::response += (String) "<BR>Bearbeiten: ";
            HTTP::response += (String) "<button onclick=\"window.location.href='" + urledit + "?lidx=" + std::to_string(loco_idx) + "&midx=0';\">MF</button>";
            HTTP::response += (String) "<button onclick=\"window.location.href='" + urledit + "?lidx=" + std::to_string(loco_idx) + "&midx=1';\">MH</button>";

            for (i = 2; i < MAX_LOCO_MACROS_PER_LOCO; i++)
            {
                HTTP::response += (String) "<button onclick=\"window.location.href='" + urledit + "?lidx=" + std::to_string(loco_idx) + "&midx=" + std::to_string(i) + "';\">M" + std::to_string(i + 1) + "</button>\r\n";
            }

            HTTP::response += (String) "<button id='btnchgfunc' onclick='changefunc()'>Fx</button><P>\r\n";
        }

        HTTP::response += (String) "<form id='formfunc' style='display:none'>\r\n";
        HTTP::response += (String) "<table>\r\n";

        HTTP::flush ();

        HTTP::response += (String) "<tr><td>F-Nr.</td><td>";
        print_loco_function_select_list ("fidx", "onchange='changef()'", false);
        HTTP::response += (String) "</td></tr>\r\n";

        HTTP::flush ();

        HTTP::response += (String) "<tr><td>Funktion</td><td>\r\n";
        print_function_select_list ("fnidx");
        HTTP::flush ();

        HTTP::response += (String) "<tr><td>Puls:</td><td><select class='select' id='pulse' name='pulse' style='width:250px'><option value='0'>Nein</option><option value='1'>Ja</option></select></td></tr>\r\n";
        HTTP::response += (String) "<tr><td>Sound:</td><td><select class='select' id='sound' name='sound' style='width:250px'><option value='0'>Nein</option><option value='1'>Ja</option></select></td></tr>\r\n";

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

        HTTP::response += (String) "<tr><td></td><td align='right'>";
        HTTP::response += (String) "<input type='hidden' name='action' value='changefunc'>\r\n";
        HTTP::response += (String) "<input type='hidden' name='lidx' value='" + std::to_string(loco_idx) + "'>\r\n";
        HTTP::response += (String) "<input type='submit' value='Speichern'></td></tr>\r\n";
        HTTP::response += (String) "</table>\r\n";
        HTTP::response += (String) "</form>\r\n";
        HTTP::flush ();

        if (addon_idx != 0xFFFF)
        {
            HTTP::response += (String) "<P><B>" + addon_name + "</B>\r\n";
            HTTP::response += (String) "<table style='border:1px lightgray solid;'>\r\n";
            HTTP::response += (String) "<tr bgcolor='#e0e0e0'><th style='width:30px;'>F</th><th style='width:250px;'>Funktion</th>";
            HTTP::response += (String) "<th style='width:50px;'>Puls</th><th style='width:50px;'>Sound</th><th style='width:20px'></th><th>Aktion</th></tr>\r\n";
            print_functions (addon_idx, true);
            HTTP::response += (String) "</table>\r\n";
            HTTP::response += (String) "<script>\r\n";
            HTTP::response += (String) "function changefaddon()\r\n";
            HTTP::response += (String) "{\r\n";
            HTTP::response += (String) "  var f = document.getElementById('addonfidx').value;\r\n";
            HTTP::response += (String) "  var url = '/action?action=getfa&aidx=" + std::to_string(addon_idx) + "&f=' + f;\r\n";
            HTTP::response += (String) "  var http = new XMLHttpRequest(); http.open ('GET', url);\r\n";
            HTTP::response += (String) "  http.addEventListener('load', function(event) { if (http.status >= 200 && http.status < 300) { ";
            HTTP::response += (String) "    var text = http.responseText;\r\n";
            HTTP::response += (String) "    const a = text.split('\t');\r\n";
            HTTP::response += (String) "    document.getElementById('addonfnidx').value = a[0];\r\n";
            HTTP::response += (String) "    document.getElementById('addonpulse').value = a[1];\r\n";
            HTTP::response += (String) "    document.getElementById('addonsound').value = a[2];\r\n";
            HTTP::response += (String) "  }});\r\n";
            HTTP::response += (String) "  http.send (null);\r\n";
            HTTP::response += (String) "}\r\n";
            HTTP::response += (String) "function changefuncaddon()\r\n";
            HTTP::response += (String) "{\r\n";
            HTTP::response += (String) "  document.getElementById('formfuncaddon').style.display='';\r\n";
            HTTP::response += (String) "  document.getElementById('btnchgfuncaddon').style.display='none';\r\n";
            HTTP::response += (String) "  changefaddon();\r\n";
            HTTP::response += (String) "}\r\n";
            HTTP::response += (String) "</script>\r\n";

            if (HTTP_Common::edit_mode)
            {
                HTTP::response += (String) "<BR><button id='btnchgfuncaddon' onclick='changefuncaddon()'>Bearbeiten</button><BR>\r\n";
            }

            HTTP::response += (String) "<form id='formfuncaddon' style='display:none'>\r\n";
            HTTP::response += (String) "<table>\r\n";

            HTTP::flush ();

            HTTP::response += (String) "<tr><td>F-Nr.</td><td>";
            print_loco_function_select_list ("addonfidx", "onchange='changefaddon()'", false);
            HTTP::response += (String) "</td></tr>\r\n";
            HTTP::flush ();

            HTTP::response += (String) "<tr><td>Funktion</td><td>\r\n";
            print_function_select_list ("addonfnidx");
            HTTP::flush ();

            HTTP::response += (String) "<tr><td>Puls:</td><td><select class='select' id='addonpulse' name='addonpulse' style='width:250px'><option value='0'>Nein</option><option value='1'>Ja</option></select></td></tr>\r\n";
            HTTP::response += (String) "<tr><td>Sound:</td><td><select class='select' id='addonsound' name='addonsound' style='width:250px'><option value='0'>Nein</option><option value='1'>Ja</option></select></td></tr>\r\n";
            HTTP::response += (String) "<tr><td></td><td align='right'>";
            HTTP::response += (String) "<input type='hidden' name='action' value='changefuncaddon'>\r\n";
            HTTP::response += (String) "<input type='hidden' name='lidx' value='" + std::to_string(loco_idx) + "'>\r\n";       // show loco details of loco lidx
            HTTP::response += (String) "<input type='hidden' name='aidx' value='" + std::to_string(addon_idx) + "'>\r\n";
            HTTP::response += (String) "<input type='submit' value='Speichern'></td></tr>\r\n";
            HTTP::response += (String) "</table>\r\n";
            HTTP::response += (String) "</form>\r\n";
            HTTP::flush ();
        }
    }
    else
    {
        const char * bg = "bgcolor='#e0e0e0'";

        HTTP_Common::add_action_handler ("locos", "", 200, true);

        HTTP::flush ();

        HTTP::response += (String) "<table style='border:1px lightgray solid;'>\r\n";
        HTTP::response += (String) "<tr " + bg + "><th align='right'>ID</th><th>Bezeichnung</th><th>Ort</th><th>Macro</th>";
        HTTP::response += (String) "<th class='hide600' style='width:50px;'>Adresse</th><th class='hide650'>RC2</th>";

        if (HTTP_Common::edit_mode)
        {
            HTTP::response += (String) "<th>Aktion</th>";
        }

        HTTP::response += (String) "</tr>\r\n";

        String          status;
        String          online;
        uint_fast16_t   n_locos = Loco::get_n_locos ();

        for (loco_idx = 0; loco_idx < n_locos; loco_idx++)
        {
            if (*bg)
            {
                bg = "";
            }
            else
            {
                bg = "bgcolor='#e0e0e0'";
            }

            if (Loco::get_flag_halt(loco_idx))
            {
                online = (String) "<td id='o" + std::to_string(loco_idx) + "' style='text-align:right;background-color:red;color:white'>" + std::to_string(loco_idx) + "&nbsp;</td>";
            }
            else if (Loco::is_online (loco_idx))
            {
                online = (String) "<td id='o" + std::to_string(loco_idx) + "' style='text-align:right;background-color:green;color:white'>" + std::to_string(loco_idx) + "&nbsp;</td>";
            }
            else
            {
                online = (String) "<td id='o" + std::to_string(loco_idx) + "' style='text-align:right'>" + std::to_string(loco_idx) + "&nbsp;</td>";
            }

            if (Loco::get_flags (loco_idx) & LOCO_FLAG_LOCKED)
            {
                status = "<font color='red'>Gesperrt</font>";
            }
            else
            {
                uint_fast8_t rc2_rate = Loco::get_rc2_rate (loco_idx);
                status = std::to_string(rc2_rate) + "%";
            }

            const char * slocation = HTTP_Common::get_location (loco_idx);

            HTTP::response += (String) "<tr " + bg + ">"
                      + online
                      + "<td style='width:200px;overflow:hidden' nowrap><a href='/loco?action=loco&lidx=" + std::to_string(loco_idx) + "'>" + Loco::get_name(loco_idx) + "</a></td>"
                      + "<td align='left' style='width:200px;overflow:hidden' id='loc" + std::to_string(loco_idx) + "' nowrap>" + slocation + "</td>"
                      + "<td nowrap><button id='m1' onclick='macro(" + std::to_string(loco_idx) + ", 0)'>MF</button>"
                      + "<button id='m2' onclick='macro(" + std::to_string(loco_idx) + ", 1)'>MH</button></td>"
                      + "<td class='hide600' align='right'>" + std::to_string(Loco::get_addr(loco_idx)) + "</td>"
                      + "<td class='hide650' align='right' width='40' id='rc2r" + std::to_string(loco_idx) + "'>" + status + "</td>";

            if (HTTP_Common::edit_mode)
            {
                HTTP::response += (String) 
                      "<td><button onclick=\""
                      + "changeloco(" + std::to_string(loco_idx) + ",'"
                      + Loco::get_name(loco_idx) + "',"
                      + std::to_string(Loco::get_addr(loco_idx)) + ","
                      + std::to_string(Loco::get_speed_steps(loco_idx)) + ")\""
                      + ">Bearbeiten</button></td>";
            }

            HTTP::response += (String) "</tr>\r\n";
            HTTP::flush ();
        }

        HTTP::response += (String) "</table>\r\n";

        HTTP::response += (String) "<script>\r\n";
        HTTP::response += (String) "function changeloco(lidx, name, addr, steps)\r\n";
        HTTP::response += (String) "{\r\n";
        HTTP::response += (String) "  document.getElementById('action').value = 'changeloco';\r\n";
        HTTP::response += (String) "  document.getElementById('lidx').value = lidx;\r\n";
        HTTP::response += (String) "  document.getElementById('nlidx').value = lidx;\r\n";
        HTTP::response += (String) "  document.getElementById('name').value = name;\r\n";
        HTTP::response += (String) "  document.getElementById('addr').value = addr;\r\n";
        HTTP::response += (String) "  document.getElementById('steps').value = steps;\r\n";
        HTTP::response += (String) "  document.getElementById('newid').style.display='';\r\n";
        HTTP::response += (String) "  document.getElementById('formloco').style.display='';\r\n";
        HTTP::response += (String) "}\r\n";
        HTTP::response += (String) "function newloco()\r\n";
        HTTP::response += (String) "{\r\n";
        HTTP::response += (String) "  document.getElementById('action').value = 'newloco';\r\n";
        HTTP::response += (String) "  document.getElementById('name').value = '';\r\n";
        HTTP::response += (String) "  document.getElementById('addr').value = '';\r\n";
        HTTP::response += (String) "  document.getElementById('steps').value = '128';\r\n";
        HTTP::response += (String) "  document.getElementById('newid').style.display='none';\r\n";
        HTTP::response += (String) "  document.getElementById('formloco').style.display='';\r\n";
        HTTP::response += (String) "}\r\n";
        HTTP::response += (String) "</script>\r\n";

        HTTP::flush ();

        if (Loco::data_changed)
        {
            HTTP::response += (String) "<BR><form method='get' action='" + url + "'>\r\n";
            HTTP::response += (String) "<input type='hidden' name='action' value='saveloco'>\r\n";
            HTTP::response += (String) "<input type='submit' value='Ge&auml;nderte Konfiguration auf Datentr&auml;ger speichern'>";
            HTTP::response += (String) "</form>\r\n";
        }

        if (HTTP_Common::edit_mode)
        {
            HTTP::response += (String) "<BR><button onclick='newloco()'>Neue Lok</button>\r\n";
            HTTP::response += (String) "<form id='formloco' style='display:none'>\r\n";
            HTTP::response += (String) "<table>\r\n";
            HTTP::response += (String) "<tr id='newid'>\r\n";
            HTTP_Common::print_id_select_list ("nlidx", n_locos);
            HTTP::response += (String) "</tr>\r\n";
            HTTP::response += (String) "<tr><td>Name:</td><td><input type='text' id='name' name='name'></td></tr>\r\n";
            HTTP::response += (String) "<tr><td>Adresse:</td><td><input type='text' id='addr' name='addr'></td></tr>\r\n";
            HTTP::response += (String) "<tr><td>Fahrstufen:</td><td><select class='select' id='steps' name='steps'><option value='14'>14</option><option value='28'>28</option><option value='128' selected>128</option></select></td></tr>\r\n";
            HTTP::response += (String) "</td></tr>\r\n";
            HTTP::response += (String) "<tr><td></td><td align='right'>";
            HTTP::response += (String) "<input type='hidden' name='action' id='action' value='newloco'><input type='hidden' name='lidx' id='lidx'><input type='submit' value='Speichern'></td></tr>\r\n";
            HTTP::response += (String) "</table>\r\n";
            HTTP::response += (String) "</form>\r\n";
        }
    }

    HTTP::response += (String) "</div>\r\n";
    HTTP_Common::html_trailer ();
}

void
HTTP_Loco::action_locos (void)
{
    static uint32_t n_calls;
    uint_fast16_t   n_locos = Loco::get_n_locos ();
    uint_fast16_t   loco_idx;

    HTTP_Common::head_action ();

    for (loco_idx = 0; loco_idx < n_locos; loco_idx++)
    {
        uint_fast8_t    is_online   = Loco::is_online (loco_idx);
        bool            is_halt     = Loco::get_flag_halt (loco_idx);
        String          id;

        id = (String) "o" + std::to_string(loco_idx);

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

        n_calls++;

        if (n_calls == 10)
        {
            uint_fast8_t    rc2_rate    = Loco::get_rc2_rate (loco_idx);
            id = (String) "rc2r" + std::to_string(loco_idx);
            HTTP_Common::add_action_content (id, "text", std::to_string(rc2_rate) + "%");
            n_calls = 0;
        }

        const char * slocation = HTTP_Common::get_location (loco_idx);

        HTTP_Common::add_action_content ((String) "loc" + std::to_string(loco_idx), "text", slocation);
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
    bool            is_halted;
    uint32_t        functions;
    String          sspeed;

    HTTP_Common::head_action ();

    speed       = Loco::get_speed (loco_idx);
    fwd         = Loco::get_fwd (loco_idx);
    functions   = Loco::get_functions (loco_idx);
    is_halted   = Loco::get_flag_halt (loco_idx);

    const char * slocation = HTTP_Common::get_location (loco_idx);

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

    for (fidx = 0; fidx < MAX_LOCO_FUNCTIONS; fidx++)
    {
        uint_fast16_t  function_name_idx = Loco::get_function_name_idx (loco_idx, fidx);

        if (function_name_idx != 0xFFFF)
        {
            if (functions & (1 << fidx))
            {
                HTTP_Common::add_action_content ((String) "bg" + std::to_string (loco_idx) + "_" + std::to_string (fidx), "color", "white");
                HTTP_Common::add_action_content ((String) "bg" + std::to_string (loco_idx) + "_" + std::to_string (fidx), "bgcolor", "green");
                HTTP_Common::add_action_content ((String) "t"  + std::to_string (loco_idx) + "_" + std::to_string (fidx), "text", "Aus");
                HTTP_Common::add_action_content ((String) "t"  + std::to_string (loco_idx) + "_" + std::to_string (fidx), "color", "red");
            }
            else
            {
                HTTP_Common::add_action_content ((String) "bg" + std::to_string (loco_idx) + "_" + std::to_string (fidx), "color", "inherit");
                HTTP_Common::add_action_content ((String) "bg" + std::to_string (loco_idx) + "_" + std::to_string (fidx), "bgcolor", "inherit");
                HTTP_Common::add_action_content ((String) "t"  + std::to_string (loco_idx) + "_" + std::to_string (fidx), "text", "Ein");
                HTTP_Common::add_action_content ((String) "t"  + std::to_string (loco_idx) + "_" + std::to_string (fidx), "color", "green");
            }
        }
    }

    if (addon_idx != 0xFFFF)
    {
        functions = AddOn::get_functions (addon_idx);

        for (fidx = 0; fidx < MAX_LOCO_FUNCTIONS; fidx++)
        {
            uint_fast16_t  function_name_idx = AddOn::get_function_name_idx (addon_idx, fidx);

            if (function_name_idx != 0xFFFF)
            {
                if (functions & (1 << fidx))
                {
                    HTTP_Common::add_action_content ((String) "bga" + std::to_string (addon_idx) + "_" + std::to_string (fidx), "color", "white");
                    HTTP_Common::add_action_content ((String) "bga" + std::to_string (addon_idx) + "_" + std::to_string (fidx), "bgcolor", "green");
                    HTTP_Common::add_action_content ((String) "ta"  + std::to_string (addon_idx) + "_" + std::to_string (fidx), "text", "Aus");
                    HTTP_Common::add_action_content ((String) "ta"  + std::to_string (addon_idx) + "_" + std::to_string (fidx), "color", "red");
                }
                else
                {
                    HTTP_Common::add_action_content ((String) "bga" + std::to_string (addon_idx) + "_" + std::to_string (fidx), "color", "inherit");
                    HTTP_Common::add_action_content ((String) "bga" + std::to_string (addon_idx) + "_" + std::to_string (fidx), "bgcolor", "inherit");
                    HTTP_Common::add_action_content ((String) "ta"  + std::to_string (addon_idx) + "_" + std::to_string (fidx), "text", "Ein");
                    HTTP_Common::add_action_content ((String) "ta"  + std::to_string (addon_idx) + "_" + std::to_string (fidx), "color", "green");
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
    uint_fast16_t   fn          = Loco::get_function_name_idx (lidx, f);
    bool            pulse       = Loco::get_function_pulse (lidx, f);
    bool            sound       = Loco::get_function_sound (lidx, f);
    uint_fast8_t    af          = Loco::get_coupled_function (lidx, f);

    HTTP::response = std::to_string(fn) + "\t" + std::to_string(pulse) + "\t" + std::to_string(sound) + "\t" + std::to_string(af);
}

void
HTTP_Loco::action_setspeed (void)
{
    uint_fast16_t   loco_idx      = HTTP::parameter_number ("loco_idx");
    uint_fast8_t    speed         = HTTP::parameter_number ("speed");
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

    Loco::set_speed (loco_idx, speed);
    Loco::set_fwd (loco_idx, fwd);

    if (fwd)
    {
        speed |= 0x80;
    }

    HTTP::response = (String) "" + std::to_string(speed);
}

void
HTTP_Loco::action_togglefunction (void)
{
    uint_fast16_t   loco_idx      = HTTP::parameter_number ("loco_idx");
    uint_fast8_t    fidx          = HTTP::parameter_number ("f");
    uint_fast8_t    value;

    if (Loco::get_function (loco_idx, fidx))
    {
        value = 0;
    }
    else
    {
        value = 1;
    }

    Loco::set_function (loco_idx, fidx, value);
    HTTP::response = (String) "" + std::to_string(value);
}

void
HTTP_Loco::action_macro (void)
{
    uint_fast16_t   loco_idx      = HTTP::parameter_number ("loco_idx");
    uint_fast8_t    macroidx      = HTTP::parameter_number ("idx");

    Loco::execute_macro (loco_idx, macroidx);
}
