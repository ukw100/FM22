/*------------------------------------------------------------------------------------------------------------------------
 * http-led.cc - HTTP led routines
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
#include "led.h"
#include "func.h"
#include "base.h"
#include "http.h"
#include "http-common.h"
#include "http-led.h"

/*----------------------------------------------------------------------------------------------------------------------------------------
 * handle_led ()
 *----------------------------------------------------------------------------------------------------------------------------------------
 */
void
HTTP_Led::handle_led (void)
{
    String          title     = "LEDs";
    String          url       = "/led";
    const char *    action    = HTTP::parameter ("action");
    uint_fast16_t   led_group_idx  = 0;

    HTTP_Common::html_header (title, title, url, true);
    HTTP::response += (String) "<div style='margin-left:20px;'>\r\n";

    if (! strcmp (action, "saveled"))
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
    else if (! strcmp (action, "newled"))
    {
        const char *    sname     = HTTP::parameter ("name");
        uint_fast16_t   addr      = HTTP::parameter_number ("addr");
        uint_fast16_t   led_group_idx;

        led_group_idx = Leds::add ({});

        if (led_group_idx != 0xFFFF)
        {
            Leds::led_groups[led_group_idx].set_name (sname);
            Leds::led_groups[led_group_idx].set_addr (addr);
        }
    }
    else if (! strcmp (action, "changeled"))
    {
        const char *    sname     = HTTP::parameter ("name");
        uint_fast16_t   led_group_idx     = HTTP::parameter_number ("led_group_idx");
        uint_fast16_t   nled_group_idx    = HTTP::parameter_number ("nled_group_idx");
        uint_fast16_t   addr      = HTTP::parameter_number ("addr");

        if (led_group_idx != nled_group_idx)
        {
            nled_group_idx = Leds::set_new_id (led_group_idx, nled_group_idx);
        }

        if (nled_group_idx != 0xFFFF)
        {
            Leds::led_groups[nled_group_idx].set_name (sname);
            Leds::led_groups[nled_group_idx].set_addr (addr);
        }
    }
    else if (! strcmp (action, "delled"))
    {
        uint_fast16_t   led_group_idx     = HTTP::parameter_number ("led_group_idx");

        Leds::remove (led_group_idx);
    }

    const char * bg = "bgcolor='#e0e0e0'";

    HTTP::response += (String)
        "<script>"
        "function toggle_led(led_group_idx, led_idx)"
        "{"
        "  var omask = document.getElementById('state_' + led_group_idx);"
        "  if (omask)"
        "  {"
        "    var mask = omask.value;"
        "    var on;"
        "    if (mask & (1 << led_idx)) { on = 0; } else { on = 1; }"
        "    if (on)"
        "    {"
        "      mask |= (1<<led_idx);"
        "    }"
        "    else"
        "    {"
        "      mask &= ~(1<<led_idx);"
        "    }"
        "    omask.value = mask;"
        "    var http = new XMLHttpRequest();"
        "    http.open ('GET', '/action?action=setled&led_group_idx=' + led_group_idx + '&mask=' + mask);"
        "    http.addEventListener('load',"
        "      function(event)"
        "      {"
        "        var text = http.responseText;"
        "        if (http.status >= 200 && http.status < 300)"
        "        {"
        "          if (on)"
        "          {"
        "            document.getElementById('set_'  + led_group_idx + '_' + led_idx).style.color = 'white';"
        "            document.getElementById('set_'  + led_group_idx + '_' + led_idx).style.backgroundColor = 'green';"
        "          }"
        "          else"
        "          {"
        "            document.getElementById('set_' + led_group_idx + '_' + led_idx).style.color = '';"
        "            document.getElementById('set_'  + led_group_idx + '_' + led_idx).style.backgroundColor = '';"
        "          }"
        "        }"
        "      }"
        "    );"
        "    http.send (null);"
        "  }"
        "}\r\n";

    HTTP_Common::add_action_handler ("led", "", 200);

    HTTP::response += (String)
        "</script>\r\n"
        "<div id='stat'></div>\r\n"
        "<table style='border:1px lightgray solid;'>\r\n"
        "<tr " + bg + "><th align='right'>ID</th><th style='width:260px;'>Bezeichnung</th><th style='width:50px;'>Adr</th>"
        "<th>Status</th>";

    if (HTTP_Common::edit_mode)
    {
        HTTP::response += (String) "<th colspan='2'>Aktion</th>";
    }

    HTTP::response += (String) "</tr>\r\n";

    HTTP::flush ();

    String        checked;
    uint_fast16_t n_led_groups = Leds::get_n_led_groups ();

    for (led_group_idx = 0; led_group_idx < n_led_groups; led_group_idx++)
    {
        if (*bg)
        {
            bg = "";
        }
        else
        {
            bg = "bgcolor='#e0e0e0'";
        }

        String          lg              = std::to_string(led_group_idx);
        uint_fast8_t    current_state   = Leds::led_groups[led_group_idx].get_state();
        std::string     name            = Leds::led_groups[led_group_idx].get_name();
        uint_fast16_t   addr            = Leds::led_groups[led_group_idx].get_addr();
        uint_fast8_t    led_idx;

        HTTP::response += (String) "<tr " + bg + "><td align='right' id='id_" + std::to_string(led_group_idx) + "'>" +
            std::to_string(led_group_idx) + "&nbsp;</td><td>" +
            name +
            "</td><td align='right'>" +
            std::to_string(addr) +
            "</td>"
            "<td align='left' nowrap>"
            "<input id='state_" + lg + "' type='hidden' value='" + std::to_string(current_state) + "'>";

        for (led_idx = 0; led_idx < 8; led_idx++)
        {
            String lgi = std::to_string(led_idx);

            if (current_state & (1 << led_idx))
            {
                checked = (String) "style='color:white;background-color:green'";
            }
            else
            {
                checked = (String) "";
            }

            HTTP::response += (String)
                "  <button id='set_" + lg + "_" + lgi + "' " + checked + " onclick='toggle_led(" + lg + "," + lgi + ")'>" + lgi + "</button>\r\n";
        }

        HTTP::response += (String) "</td>";

        if (HTTP_Common::edit_mode)
        {
            HTTP::response += (String) "<td><button onclick=\"changeled("
                  + lg + ",'" + name + "'," + std::to_string(addr)
                  + ")\">Bearbeiten</button></td>";

            HTTP::response += (String) "<td><form method='get' action='" + url + "'>"
                      + "<input type='hidden' name='action' value='delled'>"
                      + "<input type='hidden' name='led_group_idx' value='" + lg + "'>"
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
        "function changeled(led_group_idx, name, addr)\r\n"
        "{\r\n"
        "  document.getElementById('action').value = 'changeled';\r\n"
        "  document.getElementById('led_group_idx').value = led_group_idx;\r\n"
        "  document.getElementById('nled_group_idx').value = led_group_idx;\r\n"
        "  document.getElementById('name').value = name;\r\n"
        "  document.getElementById('addr').value = addr;\r\n"
        "  document.getElementById('newid').style.display='';\r\n"
        "  document.getElementById('formled').style.display='';\r\n"
        "}\r\n"
        "function newled()\r\n"
        "{\r\n"
        "  document.getElementById('action').value = 'newled';\r\n"
        "  document.getElementById('name').value = '';\r\n"
        "  document.getElementById('addr').value = '';\r\n"
        "  document.getElementById('newid').style.display='none';\r\n"
        "  document.getElementById('formled').style.display='';\r\n"
        "}\r\n"
        "</script>\r\n";

    HTTP::flush ();

    if (Leds::data_changed)
    {
        HTTP::response += (String)
            "<BR><form method='get' action='" + url + "'>\r\n"
            "<input type='hidden' name='action' value='saveled'>\r\n"
            "<input type='submit' value='Ge&auml;nderte Konfiguration auf Datentr&auml;ger speichern'>"
            "</form>\r\n";
    }

    if (HTTP_Common::edit_mode)
    {
        HTTP::response += (String)
            "<BR><button onclick='newled()'>Neue LED-Gruppe</button>\r\n"
            "<form id='formled' style='display:none'>\r\n"
            "<table>\r\n"
            "<tr id='newid'>\r\n";

        HTTP_Common::print_id_select_list ("nled_group_idx", n_led_groups);

        HTTP::response += (String)
            "</tr>\r\n"
            "<tr><td>Name:</td><td><input type='text' id='name' name='name' style='width:280px'></td></tr>\r\n"
            "<tr><td>Adresse:</td><td><input type='text' id='addr' name='addr'  style='width:280px'></td></tr>\r\n"
            "<tr><td></td><td align='right'>"
            "<input type='hidden' name='action' id='action' value='newled'>"
            "<input type='hidden' name='led_group_idx' id='led_group_idx'>"
            "<input type='submit' value='Speichern'></td></tr>\r\n"
            "</table>\r\n"
            "</form>\r\n";
    }

    HTTP::response += (String) "</div>\r\n";
    HTTP_Common::html_trailer ();
}

/*----------------------------------------------------------------------------------------------------------------------------------------
 * handle_led_test ()
 *----------------------------------------------------------------------------------------------------------------------------------------
 */
void
HTTP_Led::handle_led_test (void)
{
    String          title   = "Ledtest";
    String          url     = "/ledtest";
    const char *    action  = HTTP::parameter ("action");
    const char *    saddr   = HTTP::parameter ("addr");
    uint_fast16_t   addr;

    HTTP_Common::html_header (title, title, url, true);
    HTTP::response += (String) "<div style='margin-left:20px;'>\r\n";
    HTTP_Common::add_action_handler ("head", "", 200, true);

    if (strcmp (action, "led") == 0)
    {
        uint_fast16_t   nled = HTTP::parameter_number ("led");

        addr = atoi (saddr);
        DCC::base_switch_set (addr, nled);
    }

    HTTP::response += (String)
        "<form method='get' action='" + url + "'>\r\n"
        "<table style='border:1px lightgray solid;'>\r\n"
        "  <tr><td>Adresse:</td><td><input type=text name='addr' maxlength=4 value='" + saddr + "'></td></tr>\r\n"
        "  <tr><td>Stellung:</td><td>\r\n"
        "    <input type='radio' name='led' id='go' value='1' checked><label for='go'>Ein</label>\r\n"
        "    <input type='radio' name='led' id='halt' value='0'><label for='halt'>Aus</label>\r\n"
        "  </td></tr>\r\n"
        "  <tr><td colspan='2' align='right'><button type='submit' name='action' value='led'>Schalten</button></td></tr>\r\n"
        "</table>\r\n"
        "</form>\r\n"
        "</div>\r\n";
    HTTP_Common::html_trailer ();
}

void
HTTP_Led::action_setled (void)
{
    uint_fast16_t   led_group_idx   = HTTP::parameter_number ("led_group_idx");
    uint_fast8_t    mask            = HTTP::parameter_number ("mask");

    Leds::led_groups[led_group_idx].set_state (mask);
    HTTP::response = std::to_string(mask);
}

void
HTTP_Led::action_led (void)
{
    uint_fast16_t   n_led_groups = Leds::get_n_led_groups ();
    uint_fast16_t   led_group_idx;

    HTTP_Common::head_action ();

    for (led_group_idx = 0; led_group_idx < n_led_groups; led_group_idx++)
    {
        String          lg = std::to_string(led_group_idx);
        uint_fast8_t    mask = Leds::led_groups[led_group_idx].get_state ();
        uint_fast8_t    led_idx;

        for (led_idx = 0; led_idx < 8; led_idx++)
        {
            String lgi = lg + "_" + std::to_string(led_idx);

            if (mask & (1 << led_idx))
            {
                HTTP_Common::add_action_content ((String) "set_" + lgi, "color", "white");
                HTTP_Common::add_action_content ((String) "set_" + lgi, "bgcolor", "green");
            }
            else
            {
                HTTP_Common::add_action_content ((String) "set_" + lgi, "color", "");
                HTTP_Common::add_action_content ((String) "set_" + lgi, "bgcolor", "");
            }
        }
    }
}
