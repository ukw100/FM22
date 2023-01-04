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
#include "loco.h"
#include "addon.h"
#include "func.h"
#include "base.h"
#include "http.h"
#include "http-common.h"
#include "http-addon.h"

/*----------------------------------------------------------------------------------------------------------------------------------------
 * handle_addon ()
 *----------------------------------------------------------------------------------------------------------------------------------------
 */
void
HTTP_AddOn::handle_addon (void)
{
    String          browsertitle    = "Zusatzdecoder";
    String          title           = "Zusatzdecoder";
    String          url             = "/addon";
    const char *    action          = HTTP::parameter ("action");
    uint_fast16_t   addon_idx       = 0;

    HTTP_Common::html_header (browsertitle, title, url, true);
    HTTP::response += (String) "<div style='margin-left:20px;'>\r\n";

    if (! strcmp (action, "saveaddon"))
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
    else if (! strcmp (action, "newaddon"))
    {
        char            name[128];
        const char *    sname   = HTTP::parameter ("name");
        uint_fast16_t   addr    = HTTP::parameter_number ("addr");
        uint_fast16_t   lidx    = HTTP::parameter_number ("lidx");

        uint_fast16_t   aidx;

        strncpy (name, sname, 127);
        name[127] = 0;

        aidx = AddOn::add ();

        if (aidx != 0xFFFF)
        {
            AddOn::set_name (aidx, name);
            AddOn::set_addr (aidx, addr);

            if (lidx == 0xFFFF)
            {
                Loco::reset_addon (lidx);
                AddOn::reset_loco (aidx);
            }
            else
            {
                Loco::set_addon (lidx, aidx);
                AddOn::set_loco (aidx, lidx);
            }
        }
        else
        {
            HTTP::response += (String) "<font color='red'>Zusatzdecoder hinzuf&uuml;gen fehlgeschlagen.</font><P>";
        }
    }
    else if (! strcmp (action, "changeaddon"))
    {
        const char *    sname   = HTTP::parameter ("name");
        uint_fast16_t   aidx    = HTTP::parameter_number ("aidx");
        uint_fast16_t   naidx   = HTTP::parameter_number ("naidx");
        uint_fast16_t   addr    = HTTP::parameter_number ("addr");
        uint_fast16_t   lidx    = HTTP::parameter_number ("lidx");

        if (aidx != naidx)
        {
            naidx = AddOn::setid (aidx, naidx);
        }

        if (naidx != 0xFFFF)
        {
            AddOn::set_name (naidx, sname);
            AddOn::set_addr (naidx, addr);

            if (lidx == 0xFFFF)
            {
                Loco::reset_addon (lidx);
                AddOn::reset_loco (naidx);
            }
            else
            {
                Loco::set_addon (lidx, naidx);
                AddOn::set_loco (naidx, lidx);
            }
        }
    }

    const char * bg = "bgcolor='#e0e0e0'";

    HTTP_Common::add_action_handler ("locos", "", 200, true);

    HTTP::flush ();

    HTTP::response += (String) "<table style='border:1px lightgray solid;'>\r\n";
    HTTP::response += (String) "<tr " + bg + "><th align='right'>ID</th><th>Bezeichnung</th>";
    HTTP::response += (String) "<th class='hide600' style='width:50px;'>Adresse</th>";

    if (HTTP_Common::edit_mode)
    {
        HTTP::response += (String) "<th>Aktion</th>";
    }

    HTTP::response += (String) "</tr>\r\n";

    String          status;
    String          online;
    uint_fast16_t   n_addons = AddOn::get_n_addons ();

    for (addon_idx = 0; addon_idx < n_addons; addon_idx++)
    {
        if (*bg)
        {
            bg = "";
        }
        else
        {
            bg = "bgcolor='#e0e0e0'";
        }

        uint_fast16_t loco_idx = AddOn::get_loco (addon_idx);

        HTTP::response += (String) "<tr " + bg + ">"
                  + "<td id='o" + std::to_string(addon_idx) + "' style='text-align:right'>" + std::to_string(addon_idx) + "&nbsp;</td>"
                  + "<td style='width:200px;overflow:hidden' nowrap><a href='/loco?action=loco&lidx=" + std::to_string(loco_idx) + "'>" + AddOn::get_name(addon_idx) + "</a></td>"
                  + "<td class='hide600' align='right'>" + std::to_string(AddOn::get_addr(addon_idx)) + "</td>";

        if (HTTP_Common::edit_mode)
        {
            HTTP::response += (String) "<td><button onclick=\""
                  + "changeaddon(" + std::to_string(addon_idx) + ",'"
                  + AddOn::get_name(addon_idx) + "',"
                  + std::to_string(AddOn::get_addr(addon_idx)) + ","
                  + std::to_string(loco_idx) + ")\""
                  + ">Bearbeiten</button></td>";
        }

        HTTP::response += (String) "</tr>\r\n";
        HTTP::flush ();
    }

    HTTP::response += (String) "</table>\r\n";

    HTTP::response += (String) "<script>\r\n";
    HTTP::response += (String) "function changeaddon(aidx, name, addr, lidx)\r\n";
    HTTP::response += (String) "{\r\n";
    HTTP::response += (String) "  document.getElementById('action').value = 'changeaddon';\r\n";
    HTTP::response += (String) "  document.getElementById('aidx').value = aidx;\r\n";
    HTTP::response += (String) "  document.getElementById('naidx').value = aidx;\r\n";
    HTTP::response += (String) "  document.getElementById('name').value = name;\r\n";
    HTTP::response += (String) "  document.getElementById('addr').value = addr;\r\n";
    HTTP::response += (String) "  document.getElementById('lidx').value = lidx;\r\n";
    HTTP::response += (String) "  document.getElementById('newid').style.display='';\r\n";
    HTTP::response += (String) "  document.getElementById('formaddon').style.display='';\r\n";
    HTTP::response += (String) "}\r\n";
    HTTP::response += (String) "function newaddon()\r\n";
    HTTP::response += (String) "{\r\n";
    HTTP::response += (String) "  document.getElementById('action').value = 'newaddon';\r\n";
    HTTP::response += (String) "  document.getElementById('name').value = '';\r\n";
    HTTP::response += (String) "  document.getElementById('addr').value = '';\r\n";
    HTTP::response += (String) "  document.getElementById('lidx').value = 65535;\r\n";
    HTTP::response += (String) "  document.getElementById('newid').style.display='none';\r\n";
    HTTP::response += (String) "  document.getElementById('formaddon').style.display='';\r\n";
    HTTP::response += (String) "}\r\n";
    HTTP::response += (String) "</script>\r\n";

    HTTP::flush ();

    if (AddOn::data_changed)
    {
        HTTP::response += (String) "<BR><form method='get' action='" + url + "'>\r\n";
        HTTP::response += (String) "<input type='hidden' name='action' value='saveaddon'>\r\n";
        HTTP::response += (String) "<input type='submit' value='Ge&auml;nderte Konfiguration auf Datentr&auml;ger speichern'>";
        HTTP::response += (String) "</form>\r\n";
    }

    if (HTTP_Common::edit_mode)
    {
        HTTP::response += (String) "<BR><button onclick='newaddon()'>Neuer Zusatzdecoder</button>\r\n";

        HTTP::response += (String) "<form id='formaddon' style='display:none'>\r\n";
        HTTP::response += (String) "<table>\r\n";
        HTTP::response += (String) "<tr id='newid'>\r\n";
        HTTP_Common::print_id_select_list ("naidx", n_addons);
        HTTP::response += (String) "</tr>\r\n";
        HTTP::response += (String) "<tr><td>Name:</td><td><input type='text' id='name' name='name'></td></tr>\r\n";
        HTTP::response += (String) "<tr><td>Adresse:</td><td><input type='text' id='addr' name='addr'></td></tr>\r\n";
        HTTP::response += (String) "<tr><td>Ist Zusatzdecoder von Lok:</td><td>";
        HTTP_Common::print_loco_select_list ("lidx", 0, LOCO_LIST_DO_DISPLAY);
        HTTP::response += (String) "</td></tr>\r\n";
        HTTP::response += (String) "<tr><td></td><td align='right'>";
        HTTP::response += (String) "<input type='hidden' name='action' id='action' value='newaddon'><input type='hidden' name='aidx' id='aidx'><input type='submit' value='Speichern'></td></tr>\r\n";
        HTTP::response += (String) "</table>\r\n";
        HTTP::response += (String) "</form>\r\n";
    }

    HTTP::response += (String) "</div>\r\n";
    HTTP_Common::html_trailer ();
}

void
HTTP_AddOn::action_togglefunctionaddon (void)
{
    uint_fast16_t   addon_idx     = HTTP::parameter_number ("addon_idx");
    uint_fast8_t    fidx          = HTTP::parameter_number ("f");
    uint_fast8_t    value;

    if (AddOn::get_function (addon_idx, fidx))
    {
        value = 0;
    }
    else
    {
        value = 1;
    }

    AddOn::set_function (addon_idx, fidx, value);
    HTTP::response = (String) "" + std::to_string(value);
}

void
HTTP_AddOn::action_getfa (void)
{
    uint_fast16_t   aidx        = HTTP::parameter_number ("aidx");
    uint_fast8_t    f           = HTTP::parameter_number ("f");
    uint_fast16_t   fn          = AddOn::get_function_name_idx (aidx, f);
    bool            pulse       = AddOn::get_function_pulse (aidx, f);
    bool            sound       = AddOn::get_function_sound (aidx, f);

    HTTP::response = std::to_string(fn) + "\t" + std::to_string(pulse) + "\t" + std::to_string(sound);
}
