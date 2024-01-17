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
#include "base.h"
#include "http.h"
#include "http-common.h"
#include "http-addon.h"

#define ADDON_SORT_IDX      0
#define ADDON_SORT_ADDR     1
#define ADDON_SORT_NAME     2

static int
addon_sort_name (const void * p1, const void * p2)
{
    const uint16_t *    idx1    = (uint16_t *) p1;
    const uint16_t *    idx2    = (uint16_t *) p2;
    String              s1      = AddOns::addons[*idx1].get_name();
    String              s2      = AddOns::addons[*idx2].get_name();

    std::transform(s1.begin(), s1.end(), s1.begin(), asciitolower);
    std::transform(s2.begin(), s2.end(), s2.begin(), asciitolower);

    return s1.compare(s2);
}

static int
addon_sort_addr (const void * p1, const void * p2)
{
    const uint16_t * idx1 = (uint16_t *) p1;
    const uint16_t * idx2 = (uint16_t *) p2;

    return (int) AddOns::addons[*idx1].get_addr() - (int) AddOns::addons[*idx2].get_addr();
}

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
    const int       nsort           = HTTP::parameter_number ("sort");
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

        aidx = AddOns::add ({});

        if (aidx != 0xFFFF)
        {
            AddOns::addons[aidx].set_name (name);
            AddOns::addons[aidx].set_addr (addr);

            if (lidx == 0xFFFF)
            {
                Locos::locos[lidx].reset_addon ();
                AddOns::addons[aidx].reset_loco ();
            }
            else
            {
                Locos::locos[lidx].set_addon (aidx);
                AddOns::addons[aidx].set_loco (lidx);
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
            naidx = AddOns::set_new_id (aidx, naidx);
        }

        if (naidx != 0xFFFF)
        {
            AddOns::addons[naidx].set_name (sname);
            AddOns::addons[naidx].set_addr (addr);

            if (lidx == 0xFFFF)
            {
                Locos::locos[lidx].reset_addon ();
                AddOns::addons[naidx].reset_loco ();
            }
            else
            {
                Locos::locos[lidx].set_addon (naidx);
                AddOns::addons[naidx].set_loco (lidx);
            }
        }
    }

    uint_fast16_t   n_addons = AddOns::get_n_addons ();
    uint_fast16_t   map_idx;
    uint16_t        addon_map[n_addons];
    String          scolor_id;
    String          scolor_name;
    String          scolor_addr;

    for (addon_idx = 0; addon_idx < n_addons; addon_idx++)
    {
        addon_map[addon_idx] = addon_idx;
    }

    if (nsort == ADDON_SORT_NAME)
    {
        qsort (addon_map, n_addons, sizeof (uint16_t), addon_sort_name);
        scolor_name = "style='color:green;'";
    }
    else if (nsort == ADDON_SORT_ADDR)
    {
        qsort (addon_map, n_addons, sizeof (uint16_t), addon_sort_addr);
        scolor_addr = "style='color:green;'";
    }
    else
    {
        scolor_id = "style='color:green;'";
    }

    const char * bg = "bgcolor='#e0e0e0'";

    HTTP_Common::add_action_handler ("locos", "", 200, true);

    HTTP::flush ();

    HTTP::response += (String)
        "<table style='border:1px lightgray solid;'>\r\n"
        "<tr " + bg + "><th align='right'><a href='?sort=0' " + scolor_id + ">ID</a></th>"
        "<th><a href='?sort=2' " + scolor_name + ">Bezeichnung</a></th>"
        "<th class='hide600' style='width:50px;'><a href='?sort=1' " + scolor_addr + ">Adresse</a></th>";

    if (HTTP_Common::edit_mode)
    {
        HTTP::response += (String) "<th>Aktion</th>";
    }

    HTTP::response += (String) "</tr>\r\n";

    String          status;
    String          online;

    for (map_idx = 0; map_idx < n_addons; map_idx++)
    {
        if (*bg)
        {
            bg = "";
        }
        else
        {
            bg = "bgcolor='#e0e0e0'";
        }

        addon_idx = addon_map[map_idx];
        uint_fast16_t loco_idx = AddOns::addons[addon_idx].get_loco ();

        HTTP::response += (String) "<tr " + bg + ">"
                  + "<td id='o" + std::to_string(addon_idx) + "' style='text-align:right'>" + std::to_string(addon_idx) + "&nbsp;</td>"
                  + "<td style='width:200px;overflow:hidden' nowrap><a href='/loco?action=loco&lidx=" + std::to_string(loco_idx) + "'>" + AddOns::addons[addon_idx].get_name() + "</a></td>"
                  + "<td class='hide600' align='right'>" + std::to_string(AddOns::addons[addon_idx].get_addr()) + "</td>";

        if (HTTP_Common::edit_mode)
        {
            HTTP::response += (String) "<td><button onclick=\""
                  + "changeaddon(" + std::to_string(addon_idx) + ",'"
                  + AddOns::addons[addon_idx].get_name() + "',"
                  + std::to_string(AddOns::addons[addon_idx].get_addr()) + ","
                  + std::to_string(loco_idx) + ")\""
                  + ">Bearbeiten</button></td>";
        }

        HTTP::response += (String) "</tr>\r\n";
        HTTP::flush ();
    }

    HTTP::response += (String)
        "</table>\r\n"
        "<script>\r\n"
        "function changeaddon(aidx, name, addr, lidx)\r\n"
        "{\r\n"
        "  document.getElementById('action').value = 'changeaddon';\r\n"
        "  document.getElementById('aidx').value = aidx;\r\n"
        "  document.getElementById('naidx').value = aidx;\r\n"
        "  document.getElementById('name').value = name;\r\n"
        "  document.getElementById('addr').value = addr;\r\n"
        "  document.getElementById('lidx').value = lidx;\r\n"
        "  document.getElementById('newid').style.display='';\r\n"
        "  document.getElementById('formaddon').style.display='';\r\n"
        "}\r\n"
        "function newaddon()\r\n"
        "{\r\n"
        "  document.getElementById('action').value = 'newaddon';\r\n"
        "  document.getElementById('name').value = '';\r\n"
        "  document.getElementById('addr').value = '';\r\n"
        "  document.getElementById('lidx').value = 65535;\r\n"
        "  document.getElementById('newid').style.display='none';\r\n"
        "  document.getElementById('formaddon').style.display='';\r\n"
        "}\r\n"
        "</script>\r\n";

    HTTP::flush ();

    if (AddOns::data_changed)
    {
        HTTP::response += (String)
            "<BR><form method='get' action='" + url + "'>\r\n"
            "<input type='hidden' name='action' value='saveaddon'>\r\n"
            "<input type='submit' value='Ge&auml;nderte Konfiguration auf Datentr&auml;ger speichern'>"
            "</form>\r\n";
    }

    if (HTTP_Common::edit_mode)
    {
        HTTP::response += (String)
            "<BR><button onclick='newaddon()'>Neuer Zusatzdecoder</button>\r\n"
            "<form id='formaddon' style='display:none'>\r\n"
            "<table>\r\n"
            "<tr id='newid'>\r\n";

        HTTP_Common::print_id_select_list ("naidx", n_addons);

        HTTP::response += (String)
            "</tr>\r\n"
            "<tr><td>Name:</td><td><input type='text' id='name' name='name'></td></tr>\r\n"
            "<tr><td>Adresse:</td><td><input type='text' id='addr' name='addr'></td></tr>\r\n"
            "<tr><td>Ist Zusatzdecoder von Lok:</td><td>";

        HTTP_Common::print_loco_select_list ("lidx", 0, LOCO_LIST_DO_DISPLAY);

        HTTP::response += (String)
            "</td></tr>\r\n"
            "<tr><td></td><td align='right'>"
            "<input type='hidden' name='action' id='action' value='newaddon'>"
            "<input type='hidden' name='aidx' id='aidx'>"
            "<input type='hidden' name='sort' value='" + std::to_string(nsort) + "'>"
            "<input type='submit' value='Speichern'></td></tr>\r\n"
            "</table>\r\n"
            "</form>\r\n";
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

    if (AddOns::addons[addon_idx].get_function (fidx))
    {
        value = 0;
    }
    else
    {
        value = 1;
    }

    AddOns::addons[addon_idx].set_function (fidx, value);
    HTTP::response = (String) "" + std::to_string(value);
}

void
HTTP_AddOn::action_getfa (void)
{
    uint_fast16_t   aidx        = HTTP::parameter_number ("aidx");
    uint_fast8_t    f           = HTTP::parameter_number ("f");
    uint_fast16_t   fn          = AddOns::addons[aidx].get_function_name_idx (f);
    bool            pulse       = AddOns::addons[aidx].get_function_pulse (f);
    bool            sound       = AddOns::addons[aidx].get_function_sound (f);

    HTTP::response = std::to_string(fn) + "\t" + std::to_string(pulse) + "\t" + std::to_string(sound);
}
