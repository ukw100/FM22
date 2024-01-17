/*------------------------------------------------------------------------------------------------------------------------
 * http-addon.cc - HTTP addon routines
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
#include "switch.h"
#include "func.h"
#include "base.h"
#include "http.h"
#include "http-common.h"
#include "http-switch.h"

/*----------------------------------------------------------------------------------------------------------------------------------------
 * handle_switch ()
 *----------------------------------------------------------------------------------------------------------------------------------------
 */
void
HTTP_Switch::handle_switch (void)
{
    String          title     = "Weichen";
    String          url       = "/switch";
    const char *    action    = HTTP::parameter ("action");
    uint_fast16_t   sw_idx  = 0;

    HTTP_Common::html_header (title, title, url, true);
    HTTP::response += (String) "<div style='margin-left:20px;'>\r\n";

    if (! strcmp (action, "saveswitch"))
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
    else if (! strcmp (action, "newswitch"))
    {
        const char *    sname     = HTTP::parameter ("name");
        uint_fast16_t   addr      = HTTP::parameter_number ("addr");
        uint_fast8_t    threeway  = HTTP::parameter_number ("threeway");
        uint_fast16_t   swidx;

        uint_fast8_t    flags     = 0;

        if (threeway)
        {
            flags |= SWITCH_FLAG_3WAY;
        }

        swidx = Switches::add ({});

        if (swidx != 0xFFFF)
        {
            Switches::switches[swidx].set_name (sname);
            Switches::switches[swidx].set_addr (addr);
            Switches::switches[swidx].set_flags (flags);
        }
    }
    else if (! strcmp (action, "changeswitch"))
    {
        const char *    sname     = HTTP::parameter ("name");
        uint_fast16_t   swidx     = HTTP::parameter_number ("swidx");
        uint_fast16_t   nswidx    = HTTP::parameter_number ("nswidx");
        uint_fast16_t   addr      = HTTP::parameter_number ("addr");
        uint_fast8_t    threeway  = HTTP::parameter_number ("threeway");
        uint_fast8_t    flags     = 0;

        if (swidx != nswidx)
        {
            nswidx = Switches::set_new_id (swidx, nswidx);
        }

        if (threeway)
        {
            flags |= SWITCH_FLAG_3WAY;
        }

        if (nswidx != 0xFFFF)
        {
            Switches::switches[nswidx].set_name (sname);
            Switches::switches[nswidx].set_addr (addr);
            Switches::switches[nswidx].set_flags (flags);
        }
    }
    else if (! strcmp (action, "delswitch"))
    {
        uint_fast16_t   swidx     = HTTP::parameter_number ("swidx");

        Switches::remove (swidx);
    }

    const char * bg = "bgcolor='#e0e0e0'";

    String        checked_straight;
    String        checked_branch;
    String        checked_branch2;

    HTTP::response += (String)
        "<script>function setswitch(swidx, f) { var http = new XMLHttpRequest(); http.open ('GET', '/action?action=setsw&swidx=' + swidx + '&f=' + f);"
        "http.addEventListener('load',"
        "function(event) { var text = http.responseText; if (http.status >= 200 && http.status < 300)"
        " {"
        "    var o2 = document.getElementById('abzweig2_' + swidx);"
        "    if (f == 1)"
        "    {"
        "      document.getElementById('abzweig_' + swidx).style.color = 'inherit';"
        "      if (o2) o2.style.color = 'inherit';"
        "      document.getElementById('gerade_' + swidx).style.color = 'green';"
        "    }"
        "    else if (f == 0)"
        "    {"
        "      document.getElementById('abzweig_' + swidx).style.color = 'green';"
        "      if (o2) o2.style.color = 'inherit';"
        "      document.getElementById('gerade_' + swidx).style.color = 'inherit';"
        "    }"
        "    else"
        "    {"
        "      document.getElementById('abzweig_' + swidx).style.color = 'inherit';"
        "      if (o2) o2.style.color = 'green';"
        "      document.getElementById('gerade_' + swidx).style.color = 'inherit';"
        "    }"
        " }});"
        "http.send (null);}\r\n";

    HTTP_Common::add_action_handler ("switch", "", 200);
    HTTP::response += (String)  "</script>\r\n"
                                "<div id='stat'></div>\r\n"
                                "<table style='border:1px lightgray solid;'>\r\n"
                                "<tr " + bg + ">"
                                "<th align='right'>ID</th>"
                                "<th style='width:280px;'>Bezeichnung</th><th style='width:40px;'>Adr</th>"
                                "<th>Status</th>";

    if (HTTP_Common::edit_mode)
    {
        HTTP::response += (String) "<th colspan='2'>Aktion</th>";
    }

    HTTP::response += (String) "</tr>\r\n";
    HTTP::flush ();

    uint_fast16_t n_switches = Switches::get_n_switches ();

    for (sw_idx = 0; sw_idx < n_switches; sw_idx++)
    {
        String ssw_idx = std::to_string (sw_idx);

        if (*bg)
        {
            bg = "";
        }
        else
        {
            bg = "bgcolor='#e0e0e0'";
        }

        uint_fast8_t current_state = Switches::switches[sw_idx].get_state ();

        if (current_state == DCC_SWITCH_STATE_BRANCH)
        {
            checked_branch      = (String) "style='color:green'";
            checked_branch2     = (String) "";
            checked_straight    = (String) "";
        }
        else if (current_state == DCC_SWITCH_STATE_BRANCH2)
        {
            checked_branch      = (String) "";
            checked_branch2     = (String) "style='color:red'";
            checked_straight    = (String) "";
        }
        else if (current_state == DCC_SWITCH_STATE_STRAIGHT)
        {
            checked_branch      = (String) "";
            checked_branch2     = (String) "";
            checked_straight    = (String) "style='color:green'";
        }

        std::string     name            = Switches::switches[sw_idx].get_name();
        uint_fast16_t   addr            = Switches::switches[sw_idx].get_addr();
        uint_fast8_t    flags           = Switches::switches[sw_idx].get_flags();

        HTTP::response += (String) "<tr " + bg + "><td align='right'>"
                  + ssw_idx + "&nbsp;</td><td nowrap>"
                  + name
                  + "</td><td align='right'>"
                  + std::to_string(addr)
                  + "</td><td align='left' nowrap>"
                  + "  <button id='gerade_"  + ssw_idx + "' name='state" + ssw_idx + "' value='1' " + checked_straight
                  + " onclick='setswitch(" + ssw_idx + "," + std::to_string(DCC_SWITCH_STATE_STRAIGHT) + ")'>Gerade</button>\r\n"
                  + "  <button id='abzweig_" + ssw_idx + "' name='state" + ssw_idx + "' value='0' " + checked_branch
                  + " onclick='setswitch(" + ssw_idx + "," + std::to_string(DCC_SWITCH_STATE_BRANCH) + ")'>Abzweig</button>\r\n";

        if (flags & SWITCH_FLAG_3WAY)
        {
            HTTP::response += (String)
                "  <button id='abzweig2_" + ssw_idx + "' name='state" + ssw_idx + "' value='2' " + checked_branch2
                + " onclick='setswitch(" + ssw_idx + "," + std::to_string(DCC_SWITCH_STATE_BRANCH2) + ")'>Abzweig2</button>\r\n";
        }

        HTTP::response += (String) "</td>";

        if (HTTP_Common::edit_mode)
        {
            HTTP::response += (String) "<td><button onclick=\"changeswitch("
                  + ssw_idx + ",'" + name + "'," + std::to_string(addr) + ","
                  + std::to_string(flags & SWITCH_FLAG_3WAY)
                  + ")\">Bearbeiten</td>";

            HTTP::response += (String) "<td><form method='get' action='" + url + "'>"
                      + "<input type='hidden' name='action' value='delswitch'>"
                      + "<input type='hidden' name='swidx' value='" + ssw_idx + "'>"
                      + "<input type='submit' value='L&ouml;schen'>"
                      + "</form></td>";
        }

        HTTP::response += (String) "</tr>\r\n";

        HTTP::flush ();
    }

    HTTP::response += (String) "</table>\r\n";
    HTTP::flush ();

    HTTP::response += (String)
        "<script>\r\n"
        "function changeswitch(swidx, name, addr, threeway)\r\n"
        "{\r\n"
        "  document.getElementById('action').value = 'changeswitch';\r\n"
        "  document.getElementById('swidx').value = swidx;\r\n"
        "  document.getElementById('nswidx').value = swidx;\r\n"
        "  document.getElementById('name').value = name;\r\n"
        "  document.getElementById('addr').value = addr;\r\n"
        "  if (threeway) { document.getElementById('threeway').checked = true; } else { document.getElementById('threeway').checked = false; }\r\n"
        "  document.getElementById('newid').style.display='';\r\n"
        "  document.getElementById('formswitch').style.display='';\r\n"
        "}\r\n";

    HTTP::flush ();

    HTTP::response += (String)
        "function newswitch()\r\n"
        "{\r\n"
        "  document.getElementById('action').value = 'newswitch';\r\n"
        "  document.getElementById('name').value = '';\r\n"
        "  document.getElementById('addr').value = '';\r\n"
        "  document.getElementById('threeway').checked = false;\r\n"
        "  document.getElementById('newid').style.display='none';\r\n"
        "  document.getElementById('formswitch').style.display='';\r\n"
        "}\r\n"
        "</script>\r\n";

    HTTP::flush ();

    if (Switches::data_changed)
    {
        HTTP::response += (String)
            "<BR><form method='get' action='" + url + "'>\r\n"
            "<input type='hidden' name='action' value='saveswitch'>\r\n"
            "<input type='submit' value='Ge&auml;nderte Konfiguration auf Datentr&auml;ger speichern'>"
            "</form>\r\n";
    }

    if (HTTP_Common::edit_mode)
    {
        HTTP::response += (String)
            "<BR><button onclick='newswitch()'>Neue Weiche</button>\r\n"
            "<form id='formswitch' style='display:none'>\r\n"
            "<table>\r\n"
            "<tr id='newid'>\r\n";

        HTTP_Common::print_id_select_list ("nswidx", n_switches);

        HTTP::response += (String)
            "</tr>\r\n"
            "<tr><td>Name:</td><td><input type='text' id='name' name='name'></td></tr>\r\n"
            "<tr><td>Adresse:</td><td><input type='text' id='addr' name='addr'></td></tr>\r\n"
            "<tr><td>3-Wege-Weiche:</td><td><input type='checkbox' id='threeway' name='threeway' value='1'></td></tr>\r\n"
            "<tr><td></td><td align='right'>"
            "<input type='hidden' name='action' id='action' value='newswitch'><input type='hidden' name='swidx' id='swidx'><input type='submit' value='Speichern'></td></tr>\r\n"
            "</table>\r\n"
            "</form>\r\n";
    }

    HTTP::response += (String) "</div>\r\n";
    HTTP_Common::html_trailer ();
}

/*----------------------------------------------------------------------------------------------------------------------------------------
 * handle_switch_test ()
 *----------------------------------------------------------------------------------------------------------------------------------------
 */
void
HTTP_Switch::handle_switch_test (void)
{
    String          title   = "Weichentest";
    String          url     = "/swtest";
    const char *    action  = HTTP::parameter ("action");
    const char *    saddr   = HTTP::parameter ("addr");
    uint_fast16_t   addr;

    HTTP_Common::html_header (title, title, url, true);
    HTTP::response += (String) "<div style='margin-left:20px;'>\r\n";
    HTTP_Common::add_action_handler ("head", "", 200, true);

    if (strcmp (action, "switch") == 0)
    {
        uint_fast16_t   nswitch = HTTP::parameter_number ("switch");

        addr = atoi (saddr);
        DCC::base_switch_set (addr, nswitch);
    }

    HTTP::response += (String)
        "<form method='get' action='" + url + "'>\r\n"
        "<table style='border:1px lightgray solid;'>\r\n"
        "  <tr><td>Adresse:</td><td><input type=text name='addr' maxlength=4 value='" + saddr + "'></td></tr>\r\n"
        "  <tr><td>Stellung:</td><td>\r\n"
        "    <input type='radio' name='switch' id='gerade' value='1' checked><label for='gerade'>Gerade</label>\r\n"
        "    <input type='radio' name='switch' id='abzweig' value='0'><label for='abzweig'>Abzweig</label>\r\n"
        "    <input type='radio' name='switch' id='abzweig2' value='2'><label for='abzweig2'>Abzweig2</label>\r\n"
        "  </td></tr>\r\n"
        "  <tr><td colspan='2' align='right'><button type='submit' name='action' value='switch'>Schalten</button></td></tr>\r\n"
        "</table>\r\n"
        "</form>\r\n"
        "</div>\r\n";
    HTTP_Common::html_trailer ();
}

void
HTTP_Switch::action_setsw (void)
{
    uint_fast16_t   swidx       = HTTP::parameter_number ("swidx");
    uint_fast8_t    f           = HTTP::parameter_number ("f");

    Switches::switches[swidx].set_state (f);
    HTTP::response = std::to_string(f);
}

void
HTTP_Switch::action_switch (void)
{
    uint_fast16_t   n_switches = Switches::get_n_switches ();
    uint_fast16_t   swidx;

    HTTP_Common::head_action ();

    for (swidx = 0; swidx < n_switches; swidx++)
    {
        String          sswidx = std::to_string (swidx);
        uint_fast8_t    state = Switches::switches[swidx].get_state ();

        switch (state)
        {
            case DCC_SWITCH_STATE_BRANCH:
            {
                HTTP_Common::add_action_content ((String) "abzweig_"  + sswidx, "color", "green");
                HTTP_Common::add_action_content ((String) "abzweig2_" + sswidx, "color", "inherit");
                HTTP_Common::add_action_content ((String) "gerade_"   + sswidx, "color", "inherit");
                break;
            }
            case DCC_SWITCH_STATE_STRAIGHT:
            {
                HTTP_Common::add_action_content ((String) "abzweig_"  + sswidx, "color", "inherit");
                HTTP_Common::add_action_content ((String) "abzweig2_" + sswidx, "color", "inherit");
                HTTP_Common::add_action_content ((String) "gerade_"   + sswidx, "color", "green");
                break;
            }
            case DCC_SWITCH_STATE_BRANCH2:
            {
                HTTP_Common::add_action_content ((String) "abzweig_"  + sswidx, "color", "inherit");
                HTTP_Common::add_action_content ((String) "abzweig2_" + sswidx, "color", "green");
                HTTP_Common::add_action_content ((String) "gerade_"   + sswidx, "color", "inherit");
                break;
            }
            default:
            {
                HTTP_Common::add_action_content ((String) "abzweig_"  + sswidx, "color", "inherit");
                HTTP_Common::add_action_content ((String) "abzweig2_" + sswidx, "color", "inherit");
                HTTP_Common::add_action_content ((String) "gerade_"   + sswidx, "color", "inherit");
                break;
            }
        }
    }
}
