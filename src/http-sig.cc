/*------------------------------------------------------------------------------------------------------------------------
 * http-sig.cc - HTTP sig routines
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
#include "sig.h"
#include "func.h"
#include "base.h"
#include "http.h"
#include "http-common.h"
#include "http-sig.h"

/*----------------------------------------------------------------------------------------------------------------------------------------
 * handle_sig ()
 *----------------------------------------------------------------------------------------------------------------------------------------
 */
void
HTTP_Signal::handle_sig (void)
{
    String          title     = "Signale";
    String          url       = "/sig";
    const char *    action    = HTTP::parameter ("action");
    uint_fast16_t   sigidx  = 0;

    HTTP_Common::html_header (title, title, url, true);
    HTTP::response += (String) "<div style='margin-left:20px;'>\r\n";

    if (! strcmp (action, "savesig"))
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
    else if (! strcmp (action, "newsig"))
    {
        const char *    sname     = HTTP::parameter ("name");
        uint_fast16_t   addr      = HTTP::parameter_number ("addr");
        uint_fast16_t   sigidx;

        sigidx = Signals::add ({});

        if (sigidx != 0xFFFF)
        {
            Signals::signals[sigidx].set_name (sname);
            Signals::signals[sigidx].set_addr (addr);
        }
    }
    else if (! strcmp (action, "changesig"))
    {
        const char *    sname   = HTTP::parameter ("name");
        uint_fast16_t   sigidx  = HTTP::parameter_number ("sigidx");
        uint_fast16_t   nsigidx = HTTP::parameter_number ("nsigidx");
        uint_fast16_t   addr    = HTTP::parameter_number ("addr");

        if (sigidx != nsigidx)
        {
            nsigidx = Signals::set_new_id (sigidx, nsigidx);
        }

        if (nsigidx != 0xFFFF)
        {
            Signals::signals[nsigidx].set_name (sname);
            Signals::signals[nsigidx].set_addr (addr);
        }
    }
    else if (! strcmp (action, "delsig"))
    {
        uint_fast16_t   sigidx     = HTTP::parameter_number ("sigidx");

        Signals::remove (sigidx);
    }

    const char * bg = "bgcolor='#e0e0e0'";

    String        checked_straight;
    String        checked_branch;

    HTTP::response += (String)
        "<script>function setsig(sigidx, f) { var http = new XMLHttpRequest(); http.open ('GET', '/action?action=setsig&sigidx=' + sigidx + '&f=' + f);"
        "http.addEventListener('load',"
        "function(event) { var text = http.responseText; if (http.status >= 200 && http.status < 300)"
        " {"
        "    if (f)"
        "    {"
        "      document.getElementById('halt_' + sigidx).style.color = 'black';"
        "      document.getElementById('go_' + sigidx).style.color = 'green';"
        "      document.getElementById('id_' + sigidx).style.backgroundColor = 'green';"
        "      document.getElementById('id_' + sigidx).style.color = 'white';"
        "    }"
        "    else"
        "    {"
        "      document.getElementById('halt_' + sigidx).style.color = 'green';"
        "      document.getElementById('go_' + sigidx).style.color = 'black';"
        "      document.getElementById('id_' + sigidx).style.backgroundColor = 'red';"
        "      document.getElementById('id_' + sigidx).style.color = 'white';"
        "    }"
        " }});"
        "http.send (null);}\r\n";

    HTTP_Common::add_action_handler ("sig", "", 200);

    HTTP::response += (String)
        "</script>\r\n"
        "<div id='stat'></div>\r\n"
        "<table style='border:1px lightgray solid;'>\r\n"
        "<tr " + bg + "><th align='right'>ID</th><th style='width:240px;'>Bezeichnung</th><th style='width:50px;'>Adr</th>"
        "<th>Status</th>";

    if (HTTP_Common::edit_mode)
    {
        HTTP::response += (String) "<th colspan='2'>Aktion</th>";
    }

    HTTP::response += (String) "</tr>\r\n";

    HTTP::flush ();

    uint_fast16_t n_signals = Signals::get_n_signals ();

    for (sigidx = 0; sigidx < n_signals; sigidx++)
    {
        if (*bg)
        {
            bg = "";
        }
        else
        {
            bg = "bgcolor='#e0e0e0'";
        }

        uint_fast8_t current_state = Signals::signals[sigidx].get_state ();

        if (current_state == DCC_SIGNAL_STATE_HALT)
        {
            checked_branch    = (String) "style='color:red'";
            checked_straight  = (String) "";
        }
        else if (current_state == DCC_SIGNAL_STATE_GO)
        {
            checked_branch    = (String) "";
            checked_straight  = (String) "style='color:green'";
        }
        else
        {
            checked_branch    = (String) "";
            checked_straight  = (String) "";
        }

        std::string     name            = Signals::signals[sigidx].get_name();
        uint_fast16_t   addr            = Signals::signals[sigidx].get_addr();
        String          ssigidx         = std::to_string (sigidx);

        HTTP::response += (String) "<tr " + bg + "><td align='right' id='id_" + ssigidx + "'>"
                  + ssigidx + "&nbsp;</td><td nowrap>"
                  + name
                  + "</td><td align='right'>"
                  + std::to_string(addr)
                  + "</td><td align='left' nowrap>"
                  + "  <button id='halt_" + ssigidx + "' name='state" + ssigidx + "' value='0' " + checked_branch
                  + " onclick='setsig(" + ssigidx + "," + std::to_string(DCC_SIGNAL_STATE_HALT) + ")'>Halt</button>\r\n"
                  + "  <button id='go_"  + ssigidx + "' name='state" + ssigidx + "' value='1' " + checked_straight
                  + " onclick='setsig(" + ssigidx + "," + std::to_string(DCC_SIGNAL_STATE_GO) + ")'>Fahrt</button>\r\n";

        HTTP::response += (String) "</td>";

        if (HTTP_Common::edit_mode)
        {
            HTTP::response += (String) "<td><button onclick=\"changesig("
                  + ssigidx + ",'" + name + "'," + std::to_string(addr)
                  + ")\">Bearbeiten</td>";

            HTTP::response += (String) "<td><form method='get' action='" + url + "'>"
                      + "<input type='hidden' name='action' value='delsig'>"
                      + "<input type='hidden' name='sigidx' value='" + ssigidx + "'>"
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
        "function changesig(sigidx, name, addr)\r\n"
        "{\r\n"
        "  document.getElementById('action').value = 'changesig';\r\n"
        "  document.getElementById('sigidx').value = sigidx;\r\n"
        "  document.getElementById('nsigidx').value = sigidx;\r\n"
        "  document.getElementById('name').value = name;\r\n"
        "  document.getElementById('addr').value = addr;\r\n"
        "  document.getElementById('newid').style.display='';\r\n"
        "  document.getElementById('formsig').style.display='';\r\n"
        "}\r\n"
        "function newsig()\r\n"
        "{\r\n"
        "  document.getElementById('action').value = 'newsig';\r\n"
        "  document.getElementById('name').value = '';\r\n"
        "  document.getElementById('addr').value = '';\r\n"
        "  document.getElementById('newid').style.display='none';\r\n"
        "  document.getElementById('formsig').style.display='';\r\n"
        "}\r\n"
        "</script>\r\n";

    HTTP::flush ();

    if (Signals::data_changed)
    {
        HTTP::response += (String)
            "<BR><form method='get' action='" + url + "'>\r\n"
            "<input type='hidden' name='action' value='savesig'>\r\n"
            "<input type='submit' value='Ge&auml;nderte Konfiguration auf Datentr&auml;ger speichern'>"
            "</form>\r\n";
    }

    if (HTTP_Common::edit_mode)
    {
        HTTP::response += (String)
            "<BR><button onclick='newsig()'>Neues Signal</button>\r\n"
            "<form id='formsig' style='display:none'>\r\n"
            "<table>\r\n"
            "<tr id='newid'>\r\n";

        HTTP_Common::print_id_select_list ("nsigidx", n_signals);

        HTTP::response += (String)
            "</tr>\r\n"
            "<tr><td>Name:</td><td><input type='text' id='name' name='name'></td></tr>\r\n"
            "<tr><td>Adresse:</td><td><input type='text' id='addr' name='addr'></td></tr>\r\n"
            "<tr><td></td><td align='right'>"
            "<input type='hidden' name='action' id='action' value='newsig'><input type='hidden' name='sigidx' id='sigidx'><input type='submit' value='Speichern'></td></tr>\r\n"
            "</table>\r\n"
            "</form>\r\n";
    }

    HTTP::response += (String) "</div>\r\n";
    HTTP_Common::html_trailer ();
}

/*----------------------------------------------------------------------------------------------------------------------------------------
 * handle_sig_test ()
 *----------------------------------------------------------------------------------------------------------------------------------------
 */
void
HTTP_Signal::handle_sig_test (void)
{
    String          title   = "Signaltest";
    String          url     = "/sigtest";
    const char *    action  = HTTP::parameter ("action");
    const char *    saddr   = HTTP::parameter ("addr");
    uint_fast16_t   addr;

    HTTP_Common::html_header (title, title, url, true);
    HTTP::response += (String) "<div style='margin-left:20px;'>\r\n";
    HTTP_Common::add_action_handler ("head", "", 200, true);

    if (strcmp (action, "sig") == 0)
    {
        uint_fast16_t   nsig = HTTP::parameter_number ("sig");

        addr = atoi (saddr);
        DCC::base_switch_set (addr, nsig);
    }

    HTTP::response += (String)
        "<form method='get' action='" + url + "'>\r\n"
        "<table style='border:1px lightgray solid;'>\r\n"
        "  <tr><td>Adresse:</td><td><input type=text name='addr' maxlength=4 value='" + saddr + "'></td></tr>\r\n"
        "  <tr><td>Stellung:</td><td>\r\n"
        "    <input type='radio' name='sig' id='go' value='1' checked><label for='go'>Fahrt</label>\r\n"
        "    <input type='radio' name='sig' id='halt' value='0'><label for='halt'>Halt</label>\r\n"
        "  </td></tr>\r\n"
        "  <tr><td colspan='2' align='right'><button type='submit' name='action' value='sig'>Schalten</button></td></tr>\r\n"
        "</table>\r\n"
        "</form>\r\n"
        "</div>\r\n";
    HTTP_Common::html_trailer ();
}

void
HTTP_Signal::action_setsig (void)
{
    uint_fast16_t   sigidx  = HTTP::parameter_number ("sigidx");
    uint_fast8_t    f       = HTTP::parameter_number ("f");

    Signals::signals[sigidx].set_state (f);
    HTTP::response = std::to_string(f);
}

void
HTTP_Signal::action_sig (void)
{
    uint_fast16_t   n_signals = Signals::get_n_signals ();
    uint_fast16_t   sigidx;

    HTTP_Common::head_action ();

    for (sigidx = 0; sigidx < n_signals; sigidx++)
    {
        String          ssigidx = std::to_string (sigidx);
        uint_fast8_t    state   = Signals::signals[sigidx].get_state ();

        switch (state)
        {
            case DCC_SIGNAL_STATE_HALT:
            {
                HTTP_Common::add_action_content ((String) "halt_" + ssigidx, "color", "red");
                HTTP_Common::add_action_content ((String) "go_" + ssigidx, "color", "inherit");
                HTTP_Common::add_action_content ((String) "id_" + ssigidx, "bgcolor", "red");
                HTTP_Common::add_action_content ((String) "id_" + ssigidx, "color", "white");
                break;
            }
            case DCC_SIGNAL_STATE_GO:
            {
                HTTP_Common::add_action_content ((String) "halt_" + ssigidx, "color", "inherit");
                HTTP_Common::add_action_content ((String) "go_" + ssigidx, "color", "green");
                HTTP_Common::add_action_content ((String) "id_" + ssigidx, "bgcolor", "green");
                HTTP_Common::add_action_content ((String) "id_" + ssigidx, "color", "white");
                break;
            }
            default:
            {
                HTTP_Common::add_action_content ((String) "halt_" + ssigidx, "color", "inherit");
                HTTP_Common::add_action_content ((String) "go_" + ssigidx, "color", "inherit");
                HTTP_Common::add_action_content ((String) "id_" + ssigidx, "bgcolor", "inherit");
                HTTP_Common::add_action_content ((String) "id_" + ssigidx, "color", "inherit");
                break;
            }
        }
    }
}
