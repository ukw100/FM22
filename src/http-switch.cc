/*------------------------------------------------------------------------------------------------------------------------
 * http-addon.cc - HTTP addon routines
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

        swidx = Switch::add ();

        if (swidx != 0xFFFF)
        {
            Switch::set_name (swidx, sname);
            Switch::set_addr (swidx, addr);
            Switch::set_flags (swidx, flags);
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
            nswidx = Switch::setid (swidx, nswidx);
        }

        if (threeway)
        {
            flags |= SWITCH_FLAG_3WAY;
        }

        if (nswidx != 0xFFFF)
        {
            Switch::set_name (nswidx, sname);
            Switch::set_addr (nswidx, addr);
            Switch::set_flags (nswidx, flags);
        }
    }
    else if (! strcmp (action, "delswitch"))
    {
        uint_fast16_t   swidx     = HTTP::parameter_number ("swidx");

        Switch::remove (swidx);
    }

    const char * bg = "bgcolor='#e0e0e0'";

    String        checked_straight;
    String        checked_branch;
    String        checked_branch2;

    HTTP::response += (String) "<script>function setswitch(swidx, f) { var http = new XMLHttpRequest(); http.open ('GET', '/action?action=setsw&swidx=' + swidx + '&f=' + f);";
    HTTP::response += (String) "http.addEventListener('load',";
    HTTP::response += (String) "function(event) { var text = http.responseText; if (http.status >= 200 && http.status < 300) { }});";
    HTTP::response += (String) "http.send (null);}\r\n";

    HTTP_Common::add_action_handler ("switch", "", 200);
    HTTP::response += (String) "</script>\r\n";
    HTTP::response += (String) "<div id='stat'></div>\r\n";

    HTTP::response += (String) "<table style='border:1px lightgray solid;'>\r\n";
    HTTP::response += (String) "<tr " + bg + "><th align='right'>ID</th><th style='width:120px;'>Bezeichnung</th><th style='width:50px;'>Adr</th>";
    HTTP::response += (String) "<th>Status</th>";

    if (HTTP_Common::edit_mode)
    {
        HTTP::response += (String) "<th>Aktion</th>";
    }

    HTTP::response += (String) "</tr>\r\n";

    HTTP::flush ();

    uint_fast16_t n_switches = Switch::get_n_switches ();

    for (sw_idx = 0; sw_idx < n_switches; sw_idx++)
    {
        if (*bg)
        {
            bg = "";
        }
        else
        {
            bg = "bgcolor='#e0e0e0'";
        }

        uint_fast8_t current_state = Switch::get_state (sw_idx);

        if (current_state == DCC_SWITCH_STATE_BRANCH)
        {
            checked_branch    = (String) "checked";
        }
        else if (current_state == DCC_SWITCH_STATE_BRANCH2)
        {
            checked_branch2    = (String) "checked";
        }
        else if (current_state == DCC_SWITCH_STATE_STRAIGHT)
        {
            checked_straight  = (String) "checked";
        }

        char *          name            = Switch::get_name (sw_idx);
        uint_fast16_t   addr            = Switch::get_addr (sw_idx);
        uint_fast8_t    flags           = Switch::get_flags (sw_idx);

        HTTP::response += (String) "<tr " + bg + "><td align='right'>"
                  + std::to_string(sw_idx) + "&nbsp;</td><td>"
                  + name
                  + "</td><td align='right'>"
                  + std::to_string(addr)
                  + "</td><td align='left'>"
                  + "  <input type='radio' id='gerade_"  + std::to_string(sw_idx) + "' name='state" + std::to_string(sw_idx) + "' value='1' " + checked_straight
                  + " onclick='setswitch(" + std::to_string(sw_idx) + "," + std::to_string(DCC_SWITCH_STATE_STRAIGHT) + ")'><label for='gerade_"  + std::to_string(sw_idx) + "'>Gerade</label>\r\n"
                  + "  <input type='radio' id='abzweig_" + std::to_string(sw_idx) + "' name='state" + std::to_string(sw_idx) + "' value='0' " + checked_branch
                  + " onclick='setswitch(" + std::to_string(sw_idx) + "," + std::to_string(DCC_SWITCH_STATE_BRANCH) + ")'><label for='abzweig_" + std::to_string(sw_idx) + "'>Abzweig</label>\r\n";

        if (flags & SWITCH_FLAG_3WAY)
        {
            HTTP::response += (String) "  <input type='radio' id='abzweig2_" + std::to_string(sw_idx) + "' name='state" + std::to_string(sw_idx) + "' value='2' " + checked_branch2
                      + " onclick='setswitch(" + std::to_string(sw_idx) + "," + std::to_string(DCC_SWITCH_STATE_BRANCH2) + ")'><label for='abzweig2_" + std::to_string(sw_idx) + "'>Abzweig2</label>\r\n";
        }

        HTTP::response += (String) "</td>";

        if (HTTP_Common::edit_mode)
        {
            HTTP::response += (String) "<td><button onclick=\"changeswitch("
                  + std::to_string(sw_idx) + ",'" + name + "'," + std::to_string(addr) + ","
                  + std::to_string(flags & SWITCH_FLAG_3WAY)
                  + ")\">Bearbeiten</td>";

            HTTP::response += (String) "<td><form method='get' action='" + url + "'>"
                      + "<input type='hidden' name='action' value='delswitch'>"
                      + "<input type='hidden' name='swidx' value='" + std::to_string(sw_idx) + "'>"
                      + "<input type='submit' value='L&ouml;schen'>"
                      + "</form></td>";
        }

        HTTP::response += (String) "</tr>\r\n";

        HTTP::flush ();
    }

    HTTP::response += (String) "</table>\r\n";
    HTTP::flush ();

    HTTP::response += (String) "<script>\r\n";
    HTTP::response += (String) "function changeswitch(swidx, name, addr, threeway)\r\n";
    HTTP::response += (String) "{\r\n";
    HTTP::response += (String) "  document.getElementById('action').value = 'changeswitch';\r\n";
    HTTP::response += (String) "  document.getElementById('swidx').value = swidx;\r\n";
    HTTP::response += (String) "  document.getElementById('nswidx').value = swidx;\r\n";
    HTTP::response += (String) "  document.getElementById('name').value = name;\r\n";
    HTTP::response += (String) "  document.getElementById('addr').value = addr;\r\n";
    HTTP::response += (String) "  if (threeway) { document.getElementById('threeway').checked = true; } else { document.getElementById('threeway').checked = false; }\r\n";
    HTTP::response += (String) "  document.getElementById('newid').style.display='';\r\n";
    HTTP::response += (String) "  document.getElementById('formswitch').style.display='';\r\n";
    HTTP::response += (String) "}\r\n";

    HTTP::flush ();

    HTTP::response += (String) "function newswitch()\r\n";
    HTTP::response += (String) "{\r\n";
    HTTP::response += (String) "  document.getElementById('action').value = 'newswitch';\r\n";
    HTTP::response += (String) "  document.getElementById('name').value = '';\r\n";
    HTTP::response += (String) "  document.getElementById('addr').value = '';\r\n";
    HTTP::response += (String) "  document.getElementById('threeway').checked = false;\r\n";
    HTTP::response += (String) "  document.getElementById('newid').style.display='none';\r\n";
    HTTP::response += (String) "  document.getElementById('formswitch').style.display='';\r\n";
    HTTP::response += (String) "}\r\n";
    HTTP::response += (String) "</script>\r\n";

    HTTP::flush ();

    if (Switch::data_changed)
    {
        HTTP::response += (String) "<BR><form method='get' action='" + url + "'>\r\n";
        HTTP::response += (String) "<input type='hidden' name='action' value='saveswitch'>\r\n";
        HTTP::response += (String) "<input type='submit' value='Ge&auml;nderte Konfiguration auf Datentr&auml;ger speichern'>";
        HTTP::response += (String) "</form>\r\n";
    }

    if (HTTP_Common::edit_mode)
    {
        HTTP::response += (String) "<BR><button onclick='newswitch()'>Neue Weiche</button>\r\n";
        HTTP::response += (String) "<form id='formswitch' style='display:none'>\r\n";
        HTTP::response += (String) "<table>\r\n";
        HTTP::response += (String) "<tr id='newid'>\r\n";
        HTTP_Common::print_id_select_list ("nswidx", n_switches);
        HTTP::response += (String) "</tr>\r\n";
        HTTP::response += (String) "<tr><td>Name:</td><td><input type='text' id='name' name='name'></td></tr>\r\n";
        HTTP::response += (String) "<tr><td>Adresse:</td><td><input type='text' id='addr' name='addr'></td></tr>\r\n";
        HTTP::response += (String) "<tr><td>3-Wege-Weiche:</td><td><input type='checkbox' id='threeway' name='threeway' value='1'></td></tr>\r\n";
        HTTP::response += (String) "<tr><td></td><td align='right'>";
        HTTP::response += (String) "<input type='hidden' name='action' id='action' value='newswitch'><input type='hidden' name='swidx' id='swidx'><input type='submit' value='Speichern'></td></tr>\r\n";
        HTTP::response += (String) "</table>\r\n";
        HTTP::response += (String) "</form>\r\n";
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

    HTTP::response += (String) "<form method='get' action=='" + url + "'>\r\n";
    HTTP::response += (String) "<table style='border:1px lightgray solid;'>\r\n";
    HTTP::response += (String) "  <tr><td>Adresse:</td><td><input type=text name='addr' maxlength=4 value='" + saddr + "'></td></tr>\r\n";
    HTTP::response += (String) "  <tr><td>Stellung:</td><td>\r\n";
    HTTP::response += (String) "    <input type='radio' name='switch' value='1' checked><label for='gerade'>Gerade</label>\r\n";
    HTTP::response += (String) "    <input type='radio' name='switch' value='0'><label for='abzweig'>Abzweig</label>\r\n";
    HTTP::response += (String) "    <input type='radio' name='switch' value='2'><label for='abzweig2'>Abzweig2</label>\r\n";
    HTTP::response += (String) "  </td></tr>\r\n";
    HTTP::response += (String) "  <tr><td colspan='2' align='right'><button type='submit' name='action' value='switch'>Schalten</button></td></tr>\r\n";
    HTTP::response += (String) "</table>\r\n";
    HTTP::response += (String) "</form>\r\n";

    HTTP::response += (String) "</div>\r\n";
    HTTP_Common::html_trailer ();
}

void
HTTP_Switch::action_setsw (void)
{
    uint_fast16_t   swidx       = HTTP::parameter_number ("swidx");
    uint_fast8_t    f           = HTTP::parameter_number ("f");

    Switch::set_state (swidx, f);
    HTTP::response = std::to_string(f);
}

void
HTTP_Switch::action_switch (void)
{
    uint_fast16_t   n_switches = Switch::get_n_switches ();
    uint_fast16_t   swidx;

    HTTP_Common::head_action ();

    for (swidx = 0; swidx < n_switches; swidx++)
    {
        uint_fast8_t    state = Switch::get_state (swidx);

        switch (state)
        {
            case DCC_SWITCH_STATE_BRANCH:
            {
                HTTP_Common::add_action_content ((String) "abzweig_" + std::to_string(swidx), "checked", "1");
                break;
            }
            case DCC_SWITCH_STATE_STRAIGHT:
            {
                HTTP_Common::add_action_content ((String) "gerade_" + std::to_string(swidx), "checked", "1");
                break;
            }
            case DCC_SWITCH_STATE_BRANCH2:
            {
                HTTP_Common::add_action_content ((String) "abzweig2_" + std::to_string(swidx), "checked", "1");
                break;
            }
            default:
            {
                HTTP_Common::add_action_content ((String) "abzweig_" + std::to_string(swidx), "checked", "0");
                HTTP_Common::add_action_content ((String) "gerade_" + std::to_string(swidx), "checked", "0");
                HTTP_Common::add_action_content ((String) "abzweig2_" + std::to_string(swidx), "checked", "0");
                break;
            }
        }
    }
}
