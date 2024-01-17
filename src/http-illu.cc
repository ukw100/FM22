/*------------------------------------------------------------------------------------------------------------------------
 * http-illu.cc - HTTP illumination routines
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
#include "illu.h"
#include "func.h"
#include "base.h"
#include "http.h"
#include "http-common.h"
#include "http-illu.h"

static void
print_illu_macro_action (uint_fast8_t idx, uint_fast8_t action)
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
        case ILLU_ACTION_NONE:                                                       // no parameters
        {
            HTTP::response += (String) "&lt;keine&gt;";
            break;
        }
        case ILLU_ACTION_SET_ILLU_SPEED:                                            // parameters: START, SPEED, TENTHS
        {
            HTTP::response += (String) "Setze Helligkeit";
            break;
        }

        case ILLU_ACTION_SET_ILLU_MIN_SPEED:                                        // parameters: START, SPEED, TENTHS
        {
            HTTP::response += (String) "Setze Mindesthelligkeit";
            break;
        }

        case ILLU_ACTION_SET_ILLU_MAX_SPEED:                                        // parameters: START, SPEED, TENTHS
        {
            HTTP::response += (String) "Setze H&ouml;chsthelligkeit";
            break;
        }

        case ILLU_ACTION_SET_ILLU_FORWARD_DIRECTION:                                // parameters: START, FWD
        {
            HTTP::response += (String) "Setze Richtung";
            break;
        }

        case ILLU_ACTION_SET_ILLU_ALL_FUNCTIONS_OFF:                                // parameters: START
        {
            HTTP::response += (String) "Schalte alle Funktionen aus";
            break;
        }

        case ILLU_ACTION_SET_ILLU_FUNCTION_OFF:                                     // parameters: START, FUNC_IDX
        {
            HTTP::response += (String) "Schalte Funktion aus";
            break;
        }

        case ILLU_ACTION_SET_ILLU_FUNCTION_ON:                                      // parameters: START, FUNC_IDX
        {
            HTTP::response += (String) "Schalte Funktion ein";
            break;
        }
    }

    HTTP::response += (String) "</option>\r\n";
}

static void
print_illu_macro_action_list (uint_fast8_t caidx, uint_fast8_t action)
{
    uint_fast8_t    i;

    HTTP::response += (String) "<select name='ac" + std::to_string(caidx) + "' id='ac" + std::to_string(caidx) + "' onchange=\"changeca(" + std::to_string(caidx) + ")\">\r\n";

    for (i = 0; i < ILLU_ACTIONS; i++)
    {
        print_illu_macro_action (i, action);
    }

    HTTP::response += (String) "</select>\r\n";
}

static void
print_illu_macro_actions (uint_fast16_t illu_idx, uint_fast8_t macroidx)
{
    ILLUACTION      la;
    uint_fast8_t    actionidx;
    uint_fast8_t    n_actions;

    n_actions = Illus::illus[illu_idx].get_n_macro_actions (macroidx);

    // printf ("illu_idx=%u, macroidx=%u, n_actions = %u\n", illu_idx, macroidx, n_actions);
    for (actionidx = 0; actionidx < ILLU_MAX_ACTIONS_PER_MACRO; actionidx++)
    {
        uint_fast8_t    rtc;

        HTTP::response += (String) "<tr><td>Aktion " + std::to_string(actionidx) + ":</td><td>";

        if (actionidx < n_actions)
        {
            rtc = Illus::illus[illu_idx].get_macro_action (macroidx, actionidx, &la);

            if (rtc && la.action == ILLU_ACTION_NONE)
            {
                Debug::printf (DEBUG_LEVEL_NORMAL,
                                "print_illu_macro_actions: Internal error: illu_idx = %u, macroidx = %u, actionidx = %u: action == ILLU_ACTION_NONE\n",
                                illu_idx, macroidx, actionidx);
            }
        }
        else
        {
            la.action       = ILLU_ACTION_NONE;
            la.n_parameters = 0;
            rtc = 1;
        }

        if (rtc)
        {
            uint_fast16_t   start_tenths            = 0;
            uint_fast8_t    speed                   = 0x00;
            uint_fast8_t    fwd                     = 1;
            uint_fast8_t    f                       = 0;
            uint_fast16_t   tenths                  = 0;
            uint_fast8_t    do_display_start_tenths = false;
            uint_fast8_t    do_display_speed        = false;
            uint_fast8_t    do_display_fwd          = false;
            uint_fast8_t    do_display_f            = false;
            uint_fast8_t    do_display_tenths       = false;

            switch (la.action)
            {
                case ILLU_ACTION_NONE:                                                      // no parameters
                {
                    break;
                }
                case ILLU_ACTION_SET_ILLU_SPEED:                                            // parameters: START, SPEED, TENTHS
                {
                    start_tenths            = la.parameters[0];
                    do_display_start_tenths = true;
                    speed                   = la.parameters[1];
                    do_display_speed        = true;
                    tenths                  = la.parameters[2];
                    do_display_tenths       = true;
                    break;
                }

                case ILLU_ACTION_SET_ILLU_MIN_SPEED:                                        // parameters: START, SPEED, TENTHS
                {
                    start_tenths            = la.parameters[0];
                    do_display_start_tenths = true;
                    speed                   = la.parameters[1];
                    do_display_speed        = true;
                    tenths                  = la.parameters[2];
                    do_display_tenths       = true;
                    break;
                }

                case ILLU_ACTION_SET_ILLU_MAX_SPEED:                                        // parameters: START, SPEED, TENTHS
                {
                    start_tenths            = la.parameters[0];
                    do_display_start_tenths = true;
                    speed                   = la.parameters[1];
                    do_display_speed        = true;
                    tenths                  = la.parameters[2];
                    do_display_tenths       = true;
                    break;
                }

                case ILLU_ACTION_SET_ILLU_FORWARD_DIRECTION:                                // parameters: START, FWD
                {
                    start_tenths            = la.parameters[0];
                    do_display_start_tenths = true;
                    fwd                     = la.parameters[1];
                    do_display_fwd          = true;
                    break;
                }

                case ILLU_ACTION_SET_ILLU_ALL_FUNCTIONS_OFF:                                // parameters: START
                {
                    start_tenths            = la.parameters[0];
                    do_display_start_tenths = true;
                    break;
                }

                case ILLU_ACTION_SET_ILLU_FUNCTION_OFF:                                     // parameters: START, FUNC_IDX
                {
                    start_tenths            = la.parameters[0];
                    do_display_start_tenths = true;
                    f                       = la.parameters[1];
                    do_display_f            = true;
                    break;
                }

                case ILLU_ACTION_SET_ILLU_FUNCTION_ON:                                      // parameters: START, FUNC_IDX
                {
                    start_tenths            = la.parameters[0];
                    do_display_start_tenths = true;
                    f                       = la.parameters[1];
                    do_display_f            = true;
                    break;
                }
            }

            print_illu_macro_action_list (actionidx, la.action);
            HTTP_Common::print_start_list ((String) "st" + std::to_string(actionidx), start_tenths, do_display_start_tenths);

            HTTP_Common::print_speed_list ((String) "sp" + std::to_string(actionidx), speed, do_display_speed);
            HTTP_Common::print_fwd_list ((String) "fwd" + std::to_string(actionidx), fwd, do_display_fwd);
            HTTP_Common::print_illu_function_list (illu_idx, (String) "f" + std::to_string(actionidx), f, do_display_f);
            HTTP_Common::print_tenths_list ((String) "te" + std::to_string(actionidx), tenths, do_display_tenths);
        }

        HTTP::response += (String) "</td></tr>\r\n";
    }

    while (actionidx < ILLU_MAX_ACTIONS_PER_MACRO)
    {
        HTTP::response += (String) "<tr><td>Aktion " + std::to_string(actionidx) + ":</td><td></td></tr>\r\n";
        actionidx++;
    }
}

/*----------------------------------------------------------------------------------------------------------------------------------------
 * handle_illu_macro_edit ()
 *----------------------------------------------------------------------------------------------------------------------------------------
 */
void
HTTP_Illu::handle_illu_macro_edit (void)
{
    String          browsertitle        = (String) "Macro bearbeiten";
    const char *    sillu_idx           = HTTP::parameter ("lidx");
    uint_fast16_t   illu_idx            = atoi (sillu_idx);
    const char *    smacroidx           = HTTP::parameter ("midx");
    uint_fast16_t   macroidx            = atoi (smacroidx);
    String          illu_name           = Illus::illus[illu_idx].get_name();
    String          url                 = (String) "/illu?action=illu&lidx=" + sillu_idx;
    String          title               = (String) "<a href='" + url + "'>" + illu_name + "</a> &rarr; " + "Macro bearbeiten";

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
        "    case " + std::to_string(ILLU_ACTION_NONE)                          + ": { break; }\r\n"
        "    case " + std::to_string(ILLU_ACTION_SET_ILLU_SPEED)                + ": { st = ''; sp = ''; te = ''; break }\r\n"
        "    case " + std::to_string(ILLU_ACTION_SET_ILLU_MIN_SPEED)            + ": { st = ''; sp = ''; te = ''; break; }\r\n"
        "    case " + std::to_string(ILLU_ACTION_SET_ILLU_MAX_SPEED)            + ": { st = ''; sp = ''; te = ''; break; }\r\n"
        "    case " + std::to_string(ILLU_ACTION_SET_ILLU_FORWARD_DIRECTION)    + ": { st = ''; fwd = ''; break; }\r\n"
        "    case " + std::to_string(ILLU_ACTION_SET_ILLU_ALL_FUNCTIONS_OFF)    + ": { st = ''; break; }\r\n"
        "    case " + std::to_string(ILLU_ACTION_SET_ILLU_FUNCTION_OFF)         + ": { st = ''; f = ''; break; }\r\n"
        "    case " + std::to_string(ILLU_ACTION_SET_ILLU_FUNCTION_ON)          + ": { st = ''; f = ''; break; }\r\n"
        "  }\r\n"
        "  document.getElementById('st' + caidx).style.display = st;\r\n"
        "  document.getElementById('sp' + caidx).style.display = sp;\r\n"
        "  document.getElementById('fwd' + caidx).style.display = fwd;\r\n"
        "  document.getElementById('f' + caidx).style.display = f;\r\n"
        "  document.getElementById('te' + caidx).style.display = te;\r\n"
        "}\r\n"
        "</script>\r\n";

    HTTP::flush ();

    HTTP::response += (String)
        "<form method='get' action='" + url + "'>"
        "<table style='border:1px lightgray solid;'>\r\n"
        "<tr bgcolor='#e0e0e0'><td>ID:</td><td>M";

    HTTP::response += std::to_string(macroidx + 1);
    HTTP::response += (String) "</td></tr>\r\n";

    HTTP::flush ();

    HTTP::response += (String) "<tr><td colspan='2'>Aktionen:</td></tr>";
    print_illu_macro_actions (illu_idx, macroidx);

    HTTP::flush ();

    HTTP::response += (String)
        "<tr><td><BR></td></tr>\r\n"
        "<tr><td><input type='button'  onclick=\"window.location.href='" + url + "?action=illu&lidx=" + std::to_string(illu_idx) + "';\" value='Zur&uuml;ck'></td>"
        "<td align='right'><input type='hidden' name='action' value='changeim'>"
        "<input type='hidden' name='lidx' value='" + sillu_idx + "'><input type='hidden' name='midx' value='" + smacroidx + "'>"
        "<button >Speichern</button></td></tr>"
        "</table>\r\n"
        "</form>\r\n"
        "</div>\r\n";

    HTTP_Common::html_trailer ();
}

static void
change_illu_macro_actions (uint_fast16_t illu_idx, uint_fast16_t macroidx)
{
    ILLUACTION          la;
    uint_fast8_t        n_actions;
    uint_fast8_t        slotidx = 0;
    uint_fast8_t        idx;

    n_actions = Illus::illus[illu_idx].get_n_macro_actions (macroidx);

    for (idx = 0; idx < ILLU_MAX_ACTIONS_PER_MACRO; idx++)
    {
        uint_fast16_t   st  = HTTP::parameter_number ((String) "st" + std::to_string(idx));
        uint_fast8_t    ac  = HTTP::parameter_number ((String) "ac" + std::to_string(idx));
        uint_fast8_t    sp  = HTTP::parameter_number ((String) "sp" + std::to_string(idx));
        uint_fast8_t    te  = HTTP::parameter_number ((String) "te" + std::to_string(idx));
        uint_fast8_t    fwd = HTTP::parameter_number ((String) "fwd" + std::to_string(idx));
        uint_fast8_t    f   = HTTP::parameter_number ((String) "f" + std::to_string(idx));

        la.action = ac;

        switch (ac)
        {
            case ILLU_ACTION_NONE:                                                      // no parameters
            {
                la.n_parameters = 0;
                break;
            }
            case ILLU_ACTION_SET_ILLU_SPEED:                                            // parameters: START, SPEED, TENTHS
            {
                la.parameters[0] = st;
                la.parameters[1] = sp;
                la.parameters[2] = te;
                la.n_parameters = 3;
                break;
            }
            case ILLU_ACTION_SET_ILLU_MIN_SPEED:                                        // parameters: START, SPEED, TENTHS
            {
                la.parameters[0] = st;
                la.parameters[1] = sp;
                la.parameters[2] = te;
                la.n_parameters = 3;
                break;
            }
            case ILLU_ACTION_SET_ILLU_MAX_SPEED:                                        // parameters: START, SPEED, TENTHS
            {
                la.parameters[0] = st;
                la.parameters[1] = sp;
                la.parameters[2] = te;
                la.n_parameters = 3;
                break;
            }
            case ILLU_ACTION_SET_ILLU_FORWARD_DIRECTION:                                // parameters: START, FWD
            {
                la.parameters[0] = st;
                la.parameters[1] = fwd;
                la.n_parameters = 2;
                break;
            }
            case ILLU_ACTION_SET_ILLU_ALL_FUNCTIONS_OFF:                                // parameters: START
            {
                la.parameters[0] = st;
                la.n_parameters = 1;
                break;
            }
            case ILLU_ACTION_SET_ILLU_FUNCTION_OFF:                                     // parameters: START, FUNC_IDX
            {
                la.parameters[0] = st;
                la.parameters[1] = f;
                la.n_parameters = 2;
                break;
            }
            case ILLU_ACTION_SET_ILLU_FUNCTION_ON:                                      // parameters: START, FUNC_IDX
            {
                la.parameters[0] = st;
                la.parameters[1] = f;
                la.n_parameters = 2;
                break;
            }
        }

        if (ac != ILLU_ACTION_NONE)
        {
            if (slotidx >= n_actions)
            {
                Illus::illus[illu_idx].add_macro_action (macroidx);
                n_actions = Illus::illus[illu_idx].get_n_macro_actions (macroidx);
            }

            Illus::illus[illu_idx].set_macro_action (macroidx, slotidx, &la);
            slotidx++;
        }
    }

    while (slotidx < n_actions)
    {
        Illus::illus[illu_idx].delete_macro_action (macroidx, slotidx);
        n_actions = Illus::illus[illu_idx].get_n_macro_actions (macroidx);
    }
}

static void
print_illu_function_select_list (String name, String onchange, bool show_none)
{
    HTTP::response += (String) "<select class='select' id='" + name + "' name='" + name + "' " + onchange + " style='width:250px'>\r\n";

    if (show_none)
    {
        HTTP::response += (String) "<option value='255'>---</option>\r\n";
    }

    for (uint_fast8_t i = 0; i < MAX_ILLU_FUNCTIONS; i++)
    {
        HTTP::response += (String) "<option value=" + std::to_string(i) + ">F" + std::to_string(i) + "</option>\r\n";
    }

    HTTP::response += (String) "</select>\r\n";
}

static void
print_functions (uint_fast16_t idx)
{
    uint32_t        functions;
    const char *    bg          = "";
    uint_fast8_t    fidx;

    functions   = Illus::illus[idx].get_functions ();

    for (fidx = 0; fidx < MAX_ILLU_FUNCTIONS; fidx++)
    {
        String function_name;

        function_name = Illus::illus[idx].get_function_name (fidx);

        if (function_name != "")
        {
            String      bgid;
            String      style;

            if (functions & (1 << fidx))
            {
                style = "style='color: white; background-color: green'";
            }

            bgid = "bg";

            HTTP::response += (String) "<tr " + bg + "><td id='" + bgid + std::to_string(idx) + "_" + std::to_string(fidx) + "' " + style + " align='center'>F" + std::to_string(fidx) + "</td><td>"
                      + function_name + "</td>";

            HTTP::response += (String) "<td align='center'><button id='t" + std::to_string(idx) + "_" + std::to_string(fidx) + "' style='width:60px;' onclick='togglef(" + std::to_string(idx) + "," + std::to_string(fidx) + ")'>";

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
 * handle_illu ()
 *----------------------------------------------------------------------------------------------------------------------------------------
 */
void
HTTP_Illu::handle_illu (void)
{
    String          browsertitle    = "Beleuchtungen";
    String          title           = "Beleuchtungen";
    String          url             = "/illu";
    String          urledit         = "/imedit";
    const char *    action          = HTTP::parameter ("action");
    uint_fast16_t   illu_idx        = 0;

    if (! strcmp (action, "illu") || ! strcmp (action, "changefunc") || ! strcmp (action, "changeim"))
    {
        illu_idx = HTTP::parameter_number ("lidx");

        browsertitle = Illus::illus[illu_idx].get_name();
        title = (String) "<a href='" + url + "'>Beleuchtungen</a> &rarr; " + browsertitle;
    }

    HTTP_Common::html_header (browsertitle, title, url, true);
    HTTP::response += (String) "<div style='margin-left:20px;'>\r\n";

    if (! strcmp (action, "saveillu"))
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
    else if (! strcmp (action, "newillu"))
    {
        char            name[128];
        const char *    sname   = HTTP::parameter ("name");
        uint_fast16_t   addr    = HTTP::parameter_number ("addr");

        uint_fast16_t   lidx;

        strncpy (name, sname, 127);
        name[127] = 0;

        lidx = Illus::add ({});

        if (lidx != 0xFFFF && addr != 0)
        {
            Illus::illus[lidx].set_name (name);
            Illus::illus[lidx].set_addr (addr);
            Illus::illus[lidx].activate ();
        }
        else
        {
            HTTP::response += (String) "<font color='red'>Beleuchtung hinzuf&uuml;gen fehlgeschlagen.</font><P>";
        }
    }
    else if (! strcmp (action, "changeillu"))
    {
        const char *    sname   = HTTP::parameter ("name");
        uint_fast16_t   lidx    = HTTP::parameter_number ("lidx");
        uint_fast16_t   nlidx   = HTTP::parameter_number ("nlidx");
        uint_fast16_t   addr    = HTTP::parameter_number ("addr");

        if (lidx != nlidx)
        {
            nlidx = Illus::set_new_id (lidx, nlidx);
        }

        if (nlidx != 0xFFFF)
        {
            Illus::illus[nlidx].set_name (sname);
            Illus::illus[lidx].set_addr (addr);
        }
    }
    else if (! strcmp (action, "changeim"))
    {
        uint_fast8_t    macroidx    = HTTP::parameter_number ("midx");
        change_illu_macro_actions (illu_idx, macroidx);
    }
    else if (! strcmp (action, "changefunc"))
    {
        uint_fast8_t    fidx                = HTTP::parameter_number ("fidx");
        String          function_name       = HTTP::parameter ("fnname");

        Illus::illus[illu_idx].set_function_name (fidx, function_name);
    }

    HTTP::response += (String)
        "<script>\r\n"
        "function imacro(li, idx)\r\n"
        "{\r\n"
        "  var http = new XMLHttpRequest();\r\n"
        "  http.open ('GET', '/action?action=imacro&illu_idx=' + li + '&idx=' + idx);\r\n"
        "  http.addEventListener('load', function(event) {});\r\n"
        "  http.send (null);\r\n"
        "}\r\n"
        "</script>\r\n";

    if (! strcmp (action, "illu") || ! strcmp (action, "changefunc") || ! strcmp (action, "changeim"))
    {
        char            param[64];
        uint_fast8_t    speed;
        uint_fast8_t    fwd;

        speed       = Illus::illus[illu_idx].get_speed ();
        fwd         = Illus::illus[illu_idx].get_fwd ();

        HTTP::response += (String)
            "<script>\r\n"
            "function togglef (li, fi) { var http = new XMLHttpRequest(); http.open ('GET', '/action?action=togglefunctionillu&illu_idx=' + li + '&f=' + fi);"
            "http.addEventListener('load',"
            "function(event) { var o; var text = http.responseText; if (http.status >= 200 && http.status < 300) { "
            "var color; if (parseInt(text) == 1) { text='Aus'; color='red' } else { text='Ein'; color='green' } "
            "o = document.getElementById('t' + li + '_' + fi); o.textContent = text; o.color = color; } else alert (http.status); });"
            "http.send (null);}\r\n";

        sprintf (param, "illu_idx=%u", (unsigned int) illu_idx);
        HTTP_Common::add_action_handler ("illu", param, 200);

        HTTP::response += (String)
            "function upd_fwd (f) { var fo = document.getElementById('f'); var bo = document.getElementById('b');\r\n"
            "var fwdo = document.getElementById('fwd'); fwdo.value = f;\r\n"
            "if (f) { fo.style.backgroundColor = 'green'; fo.style.color = 'white'; bo.style.backgroundColor = 'inherit'; bo.style.color = 'inherit'; }\r\n"
            "else { fo.style.backgroundColor = 'inherit'; fo.style.color = 'inherit'; bo.style.backgroundColor = 'green'; bo.style.color = 'white'; }}\r\n"
            "function upd_speed (s) { var slider = document.getElementById('speed'); slider.value = s; var o = document.getElementById('out'); o.textContent = s; }\r\n"
            "function incslider (li) { var slider = document.getElementById('speed'); var speed = parseInt(slider.value);\r\n"
            "speed += 4; if (speed > 127) { speed = 127; } setspeed (li, speed); }\r\n"
            "function decslider (li) { var slider = document.getElementById('speed'); var speed = parseInt(slider.value);\r\n"
            "speed -= 4; if (speed <= 1) speed = 0; setspeed (li, speed);}\r\n";

        HTTP::flush ();

        HTTP::response += (String)
            "function setspeed (li, speed) { var fwdo = document.getElementById('fwd'); var fwd = parseInt (fwdo.value); if (fwd) { speed |= 0x80; }\r\n"
            "var http = new XMLHttpRequest(); http.open ('GET', '/action?action=setspeedillu&illu_idx=' + li + '&speed=' + speed);\r\n"
            "http.addEventListener('load', function(event) { var o; var text = http.responseText; if (http.status >= 200 && http.status < 300) { "
            "var fwd = 0; var speed = parseInt(text); if (speed & 0x80) { fwd = 1; speed &= 0x7F; } upd_speed(speed); upd_fwd (fwd); }});"
            "http.send (null);}\r\n"
            "function bwd (li) { var fwdo = document.getElementById('fwd'); fwdo.value = 0;"
            "var slider = document.getElementById('speed'); var speed = parseInt(slider.value); setspeed (li, speed); }\r\n"
            "function fwd (li) { var fwdo = document.getElementById('fwd'); fwdo.value = 1;"
            "var slider = document.getElementById('speed'); var speed = parseInt(slider.value) | 0x80; setspeed (li, speed); }\r\n"
            "function upd_slider (li) { var slider = document.getElementById('speed'); var speed = parseInt(slider.value);\r\n"
            "if (speed == 1) { var output = document.getElementById('out'); var oldspeed = parseInt(output.textContent);\r\n"
            "if (oldspeed == 0) { speed = 2; } else { speed = 0; }} setspeed (li, speed); }\r\n"
            "function go (li) { var http = new XMLHttpRequest(); http.open ('GET', '/action?action=go&illu_idx=' + li);\r\n"
            "http.send (null);}\r\n"
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

        String colspan = "5";

        HTTP::response += (String)
            "<table style='border:1px lightgray solid;'>\r\n"
            "<tr><td colspan='" + colspan + "'>"
            "  <div style='float:left;width:45%;display:block;text-align:left'>Adresse: " + std::to_string(Illus::illus[illu_idx].get_addr()) + "</div>"
            "  <div id='go' style='float:left;display:block;text-align:right;display:none'><button onclick='go(" + std::to_string(illu_idx) + ")' style='color:green'>Fahrt!</button></div>\r\n"
            "  <div id='loc' style='display:block;text-align:right'></td></tr>\r\n"
            "<tr><td align='center' colspan='" + colspan + "'>\r\n"
            "<div style='width:350px;padding-top:0px;padding-bottom:10px'>"
            "  <input type='hidden' id='fwd' value='" + std::to_string(fwd) + "'>"
            "  <div style='width:300px;text-align:center;'>";

        uint_fast8_t i;

        for (i = 0; i < MAX_ILLU_MACROS_PER_ILLU; i++)
        {
            String si0 = std::to_string(i);
            String si1 = std::to_string(i + 1);
            HTTP::response += (String) "    <button id='m" + si1 + "' title='" + Illus::illus[illu_idx].get_macro_name(i) + "' onclick='imacro(" + std::to_string(illu_idx) + ", " + si0 + ")'>M" + si1 + "</button>";
        }

        HTTP::response += (String)
            "  </div>"
            "  <div id='out' style='width:300px;margin-top:10px;text-align:center;'>" + std::to_string(speed) + "</div>"
            "  <button style='vertical-align:top' onclick='decslider(" + std::to_string(illu_idx) + ")'>-</button>"
            "  <input type='range' min='0' max='127' value='" + std::to_string(speed) + "' id='speed' style='width:256px' onchange='upd_slider(" + std::to_string(illu_idx) + ")'>"
            "  <button style='vertical-align:top' onclick='incslider(" + std::to_string(illu_idx) + ")'>+</button>"
            "  <div style='width:300px;text-align:center'>"
            "    <button id='b' " + bwd_color + " onclick='bwd(" + std::to_string(illu_idx) + ")'>&#9664;</button>"
            "    <button onclick='setspeed(" + std::to_string(illu_idx) + ",0);'>&#9209;</button>"
            "    <button onclick='setspeed(" + std::to_string(illu_idx) + ",1);'>&#9210;</button>"
            "    <button id='f' " + fwd_color + " onclick='fwd(" + std::to_string(illu_idx) + ")'>&#9654;</button>"
            "  </div>"
            "</div>\r\n"
            "</td></tr>\r\n";

        HTTP::flush ();

        HTTP::response += (String)
            "<tr bgcolor='#e0e0e0'><th style='width:30px;'>F</th><th style='width:250px;'>Funktion</th>"
            "<th>Aktion</th></tr>\r\n";

        print_functions (illu_idx);

        HTTP::response += (String)
            "</table>\r\n"
            "<script>\r\n"
            "function changef()\r\n"
            "{\r\n"
            "  var f = document.getElementById('fidx').value;\r\n"
            "  var url = '/action?action=getfi&lidx=" + std::to_string(illu_idx) + "&f=' + f;\r\n"
            "  var http = new XMLHttpRequest(); http.open ('GET', url);\r\n"
            "  http.addEventListener('load', function(event) { if (http.status >= 200 && http.status < 300) { "
            "    var text = http.responseText;\r\n"
            "    const a = text.split('\t');\r\n"
            "    document.getElementById('fnname').value = a[0];\r\n"
            "  }});\r\n"
            "  http.send (null);\r\n"
            "}\r\n"
            "function changefunc()\r\n"
            "{\r\n"
            "  document.getElementById('formfunc').style.display='';\r\n"
            "  document.getElementById('btnchgfunc').style.display='none';\r\n"
            "  changef();\r\n"
            "}\r\n"
            "</script>\r\n";

        if (HTTP_Common::edit_mode)
        {
            HTTP::response += (String) "<BR>Bearbeiten: ";

            for (i = 0; i < MAX_ILLU_MACROS_PER_ILLU; i++)
            {
                HTTP::response += (String) "<button onclick=\"window.location.href='" + urledit + "?lidx=" + std::to_string(illu_idx) + "&midx=" + std::to_string(i) + "';\" title='"
                                + Illus::illus[illu_idx].get_macro_name(i) + "' style='margin-right:3px'>M"
                                + std::to_string(i + 1) + "</button>";
            }

            HTTP::response += (String) "<button id='btnchgfunc' onclick='changefunc()'>Fx</button><P>";
        }

        HTTP::response += (String)
            "<form id='formfunc' style='display:none'>\r\n"
            "<table>\r\n";

        HTTP::flush ();

        HTTP::response += (String) "<tr><td>F-Nr.</td><td>";
        print_illu_function_select_list ("fidx", "onchange='changef()'", false);

        HTTP::response += (String)
            "</td></tr>\r\n"
            "<tr><td>Funktion</td><td><input type='text' style='width:250px' name='fnname' id='fnname' value=''</td></tr>\r\n"
            "<tr><td></td><td align='right'>"
            "<input type='hidden' name='action' value='changefunc'>\r\n"
            "<input type='hidden' name='lidx' value='" + std::to_string(illu_idx) + "'>\r\n"
            "<input type='submit' value='Speichern'></td></tr>\r\n"
            "</table>\r\n"
            "</form>\r\n";

        HTTP::flush ();
    }
    else
    {
        const char * bg = "bgcolor='#e0e0e0'";

        HTTP_Common::add_action_handler ("illus", "", 200, true);

        HTTP::flush ();

        HTTP::response += (String)
            "<table style='border:1px lightgray solid;'>\r\n"
            "<tr " + bg + "><th align='right'>ID</th><th>Bezeichnung</th><th>Macro</th>"
            "<th class='hide600' style='width:50px;'>Adresse</th>";

        if (HTTP_Common::edit_mode)
        {
            HTTP::response += (String) "<th>Aktion</th>";
        }

        HTTP::response += (String) "</tr>\r\n";

        String          online;
        uint_fast16_t   n_illus = Illus::get_n_illus ();

        for (illu_idx = 0; illu_idx < n_illus; illu_idx++)
        {
            if (*bg)
            {
                bg = "";
            }
            else
            {
                bg = "bgcolor='#e0e0e0'";
            }

            if (Illus::illus[illu_idx].is_online ())
            {
                online = (String) "<td id='o" + std::to_string(illu_idx) + "' style='text-align:right;background-color:green;color:white'>" + std::to_string(illu_idx) + "&nbsp;</td>";
            }
            else
            {
                online = (String) "<td id='o" + std::to_string(illu_idx) + "' style='text-align:right'>" + std::to_string(illu_idx) + "&nbsp;</td>";
            }

            HTTP::response += (String)
                "<tr " + bg + ">" + online +
                "<td style='width:200px;overflow:hidden' nowrap><a href='/illu?action=illu&lidx=" + std::to_string(illu_idx) + "'>" + Illus::illus[illu_idx].get_name() + "</a></td>"
                "<td nowrap><button id='m1' onclick='imacro(" + std::to_string(illu_idx) + ", 0)'>M1</button>"
                "<button id='m2' onclick='imacro(" + std::to_string(illu_idx) + ", 1)'>M2</button></td>"
                "<td class='hide600' align='right'>" + std::to_string(Illus::illus[illu_idx].get_addr()) + "</td>";

            if (HTTP_Common::edit_mode)
            {
                HTTP::response += (String) 
                    "<td><button onclick=\"changeillu(" + std::to_string(illu_idx) + ",'" + Illus::illus[illu_idx].get_name() + "',"
                      + std::to_string(Illus::illus[illu_idx].get_addr()) + ")\">Bearbeiten</button></td>";
            }

            HTTP::response += (String) "</tr>\r\n";
            HTTP::flush ();
        }

        HTTP::response += (String)
            "</table>\r\n"
            "<script>\r\n"
            "function changeillu(lidx, name, addr)\r\n"
            "{\r\n"
            "  document.getElementById('action').value = 'changeillu';\r\n"
            "  document.getElementById('lidx').value = lidx;\r\n"
            "  document.getElementById('nlidx').value = lidx;\r\n"
            "  document.getElementById('name').value = name;\r\n"
            "  document.getElementById('addr').value = addr;\r\n"
            "  document.getElementById('newid').style.display='';\r\n"
            "  document.getElementById('formillu').style.display='';\r\n"
            "}\r\n"
            "function newillu()\r\n"
            "{\r\n"
            "  document.getElementById('action').value = 'newillu';\r\n"
            "  document.getElementById('name').value = '';\r\n"
            "  document.getElementById('addr').value = '';\r\n"
            "  document.getElementById('newid').style.display='none';\r\n"
            "  document.getElementById('formillu').style.display='';\r\n"
            "}\r\n"
            "</script>\r\n";

        HTTP::flush ();

        if (Illus::data_changed)
        {
            HTTP::response += (String)
                "<BR><form method='get' action='" + url + "'>\r\n"
                "<input type='hidden' name='action' value='saveillu'>\r\n"
                "<input type='submit' value='Ge&auml;nderte Konfiguration auf Datentr&auml;ger speichern'>"
                "</form>\r\n";
        }

        if (HTTP_Common::edit_mode)
        {
            HTTP::response += (String)
                "<BR><button onclick='newillu()'>Neue Beleuchtung</button>\r\n"
                "<form id='formillu' style='display:none'>\r\n"
                "<table>\r\n"
                "<tr id='newid'>\r\n";

            HTTP_Common::print_id_select_list ("nlidx", n_illus);

            HTTP::response += (String)
                "</tr>\r\n"
                "<tr><td>Name:</td><td><input type='text' id='name' name='name'></td></tr>\r\n"
                "<tr><td>Adresse:</td><td><input type='text' id='addr' name='addr'></td></tr>\r\n"
                "</td></tr>\r\n"
                "<tr><td></td><td align='right'>"
                "<input type='hidden' name='action' id='action' value='newillu'><input type='hidden' name='lidx' id='lidx'><input type='submit' value='Speichern'></td></tr>\r\n"
                "</table>\r\n"
                "</form>\r\n";
        }
    }

    HTTP::response += (String) "</div>\r\n";
    HTTP_Common::html_trailer ();
}

void
HTTP_Illu::action_illus (void)
{
    uint_fast16_t   n_illus = Illus::get_n_illus ();
    uint_fast16_t   illu_idx;

    HTTP_Common::head_action ();

    for (illu_idx = 0; illu_idx < n_illus; illu_idx++)
    {
        uint_fast8_t    is_online   = Illus::illus[illu_idx].is_online ();
        String          id;

        id = (String) "o" + std::to_string(illu_idx);

        if (is_online)
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

void
HTTP_Illu::action_illu (void)
{
    uint_fast16_t   illu_idx        = HTTP::parameter_number ("illu_idx");
    uint_fast8_t    speed;
    uint_fast8_t    fwd;
    uint_fast8_t    fidx;
    uint32_t        functions;
    String          sspeed;

    HTTP_Common::head_action ();

    speed       = Illus::illus[illu_idx].get_speed ();
    fwd         = Illus::illus[illu_idx].get_fwd ();
    functions   = Illus::illus[illu_idx].get_functions ();

    std::string slocation = HTTP_Common::get_location (illu_idx);

    HTTP_Common::add_action_content ("loc", "color", "green");
    HTTP_Common::add_action_content ("go", "display", "none");
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

    for (fidx = 0; fidx < MAX_ILLU_FUNCTIONS; fidx++)
    {
        String function_name = Illus::illus[illu_idx].get_function_name (fidx);

        if (function_name != "")
        {
            if (functions & (1 << fidx))
            {
                HTTP_Common::add_action_content ((String) "bg" + std::to_string (illu_idx) + "_" + std::to_string (fidx), "color", "white");
                HTTP_Common::add_action_content ((String) "bg" + std::to_string (illu_idx) + "_" + std::to_string (fidx), "bgcolor", "green");
                HTTP_Common::add_action_content ((String) "t"  + std::to_string (illu_idx) + "_" + std::to_string (fidx), "text", "Aus");
                HTTP_Common::add_action_content ((String) "t"  + std::to_string (illu_idx) + "_" + std::to_string (fidx), "color", "red");
            }
            else
            {
                HTTP_Common::add_action_content ((String) "bg" + std::to_string (illu_idx) + "_" + std::to_string (fidx), "color", "inherit");
                HTTP_Common::add_action_content ((String) "bg" + std::to_string (illu_idx) + "_" + std::to_string (fidx), "bgcolor", "inherit");
                HTTP_Common::add_action_content ((String) "t"  + std::to_string (illu_idx) + "_" + std::to_string (fidx), "text", "Ein");
                HTTP_Common::add_action_content ((String) "t"  + std::to_string (illu_idx) + "_" + std::to_string (fidx), "color", "green");
            }
        }
    }
}

void
HTTP_Illu::action_getf ()
{
    uint_fast16_t   lidx        = HTTP::parameter_number ("lidx");
    uint_fast8_t    f           = HTTP::parameter_number ("f");
    String          fn          = Illus::illus[lidx].get_function_name (f);

    HTTP::response = fn;
}

void
HTTP_Illu::action_setspeed (void)
{
    uint_fast16_t   illu_idx      = HTTP::parameter_number ("illu_idx");
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

    Illus::illus[illu_idx].set_speed (speed);
    Illus::illus[illu_idx].set_fwd (fwd);

    if (fwd)
    {
        speed |= 0x80;
    }

    HTTP::response = (String) "" + std::to_string(speed);
}

void
HTTP_Illu::action_togglefunction (void)
{
    uint_fast16_t   illu_idx      = HTTP::parameter_number ("illu_idx");
    uint_fast8_t    fidx          = HTTP::parameter_number ("f");
    uint_fast8_t    value;

    if (Illus::illus[illu_idx].get_function (fidx))
    {
        value = 0;
    }
    else
    {
        value = 1;
    }

    Illus::illus[illu_idx].set_function (fidx, value);
    HTTP::response = (String) "" + std::to_string(value);
}

void
HTTP_Illu::action_macro (void)
{
    uint_fast16_t   illu_idx      = HTTP::parameter_number ("illu_idx");
    uint_fast8_t    macroidx      = HTTP::parameter_number ("idx");

    Illus::illus[illu_idx].execute_macro (macroidx);
}
