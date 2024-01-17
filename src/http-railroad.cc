/*------------------------------------------------------------------------------------------------------------------------
 * http-railroad.cc - HTTP railroad routines
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
#include "switch.h"
#include "railroad.h"
#include "func.h"
#include "base.h"
#include "http.h"
#include "http-common.h"
#include "http-railroad.h"

/*----------------------------------------------------------------------------------------------------------------------------------------
 * print_select_switch ()
 *
 * swidx begins with 1, 0 is: not defined
 *----------------------------------------------------------------------------------------------------------------------------------------
 */
static void
print_select_switch (uint_fast8_t subidx, uint_fast16_t swidx)
{
    uint_fast16_t idx;

    HTTP::response += (String)
        "<select class='select' name='rr" + std::to_string(subidx) + "'>\r\n"
        "<option value='0'>---</option>\r\n";

    uint_fast16_t n_switches = Switches::get_n_switches ();

    for (idx = 0; idx < n_switches; idx++)
    {
        std::string name    = Switches::switches[idx].get_name();

        HTTP::response += (String) "<option value='" + std::to_string(idx + 1) + "'";

        if ((idx + 1) == swidx)
        {
            HTTP::response += (String) " selected";
        }

        HTTP::response += (String) ">";
        HTTP::response += name;
        HTTP::response += (String) "</option>\r\n";
    }
    HTTP::response += (String) "</select>\r\n";
}

/*----------------------------------------------------------------------------------------------------------------------------------------
 * print_switch_state ()
 *----------------------------------------------------------------------------------------------------------------------------------------
 */
static void
print_switch_state (uint_fast8_t subidx, uint_fast8_t state)
{
    String checked_straight;
    String checked_branch;
    String checked_branch2;

    if (state == DCC_SWITCH_STATE_STRAIGHT)
    {
        checked_straight = "checked";
    }
    else if (state == DCC_SWITCH_STATE_BRANCH)
    {
        checked_branch = "checked";
    }
    else if (state == DCC_SWITCH_STATE_BRANCH2)
    {
        checked_branch2 = "checked";
    }

    HTTP::response += (String)
        "<input type='radio' id='gerade" + std::to_string(subidx) + "' name='state" + std::to_string(subidx) + "' value='1' " + checked_straight + ">"
        "<label for='gerade" + std::to_string(subidx) + "'>Gerade</label>\r\n"
        "<input type='radio' id='abzweig" + std::to_string(subidx) + "' name='state" + std::to_string(subidx) + "' value='0' " + checked_branch + ">"
        "<label for='abzweig" + std::to_string(subidx) + "'>Abzweig</label>\r\n"
        "<input type='radio' id='abzweig2" + std::to_string(subidx) + "' name='state" + std::to_string(subidx) + "' value='2' " + checked_branch2 + ">"
        "<label for='abzweig2" + std::to_string(subidx) + "'>Abzweig2</label>\r\n";
}

/*----------------------------------------------------------------------------------------------------------------------------------------
 * handle_rr ()
 *----------------------------------------------------------------------------------------------------------------------------------------
 */
void
HTTP_Railroad::handle_rr (void)
{
    String          title     = "Fahrstra&szlig;en";
    String          url       = "/rr";
    String          urledit   = "/rredit";
    const char *    action    = HTTP::parameter ("action");

    HTTP_Common::html_header (title, title, url, true);
    HTTP::response += (String) "<div style='margin-left:20px;'>\r\n";

    if (! strcmp (action, "saverrg"))
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
    else if (! strcmp (action, "newrrg"))
    {
        char            name[128];
        const char *    sname     = HTTP::parameter ("name");
        uint_fast8_t    rrgidx;

        strncpy (name, sname, 127);
        name[127] = '\0';

        rrgidx = RailroadGroups::add ({});

        if (rrgidx != 0xFF)
        {
            RailroadGroups::railroad_groups[rrgidx].set_name (name);
        }
    }
    else if (! strcmp (action, "changerrg"))
    {
        const char *    sname   = HTTP::parameter ("name");
        uint_fast8_t    rrgidx  = HTTP::parameter_number ("rrgidx");
        uint_fast8_t    nrrgidx = HTTP::parameter_number ("nrrgidx");

        if (rrgidx != nrrgidx)
        {
            nrrgidx = RailroadGroups::set_new_id (rrgidx, nrrgidx);
        }

        if (nrrgidx != 0xFF)
        {
            RailroadGroups::railroad_groups[nrrgidx].set_name (sname);
        }
    }
    else if (! strcmp (action, "newrr"))
    {
        char            name[128];
        const char *    sname     = HTTP::parameter ("name");
        uint_fast16_t   rrgidx    = HTTP::parameter_number ("rrgidx");
        uint_fast8_t    rridx;

        strncpy (name, sname, 127);
        name[127] = '\0';

        rridx = RailroadGroups::railroad_groups[rrgidx].add ({});

        if (rridx != 0xFF)
        {
            RailroadGroups::railroad_groups[rrgidx].railroads[rridx].set_name (name);
        }
    }
    else if (! strcmp (action, "changerr"))  // see handle_rr_edit())
    {
        const char *    sname           = HTTP::parameter ("name");
        uint_fast16_t   rrgidx          = HTTP::parameter_number ("rrgidx");
        uint_fast16_t   rridx           = HTTP::parameter_number ("rridx");
        uint_fast16_t   nrridx          = HTTP::parameter_number ("nrridx");
        uint_fast16_t   ll              = HTTP::parameter_number ("ll");
        Railroad *      rr              = &RailroadGroups::railroad_groups[rrgidx].railroads[rridx];
        uint_fast8_t    n_rr_switches   = rr->get_n_switches ();
        uint_fast8_t    real_sub_idx    = 0;
        Railroad *      nrr;
        uint_fast8_t    sub_idx;

        if (rridx != nrridx)
        {
            nrridx = RailroadGroups::railroad_groups[rrgidx].set_new_id(rridx, nrridx);
        }

        nrr = &RailroadGroups::railroad_groups[rrgidx].railroads[nrridx];

        nrr->set_name (sname);
        nrr->set_link_loco (ll);

        for (sub_idx = 0; sub_idx < MAX_SWITCHES_PER_RAILROAD; sub_idx++)
        {
            char            buf[16];
            uint_fast16_t   swidx;
            uint_fast8_t    state;

            sprintf (buf, "rr%d", sub_idx);
            swidx = HTTP::parameter_number (buf);

            if (swidx > 0)  // begins with 1, 0 means: switch unused
            {
                swidx--;

                sprintf (buf, "state%d", sub_idx);
                state = HTTP::parameter_number (buf);

                uint_fast8_t    flags   = Switches::switches[swidx].get_flags();

                if (state == DCC_SWITCH_STATE_BRANCH2 && ! (flags & SWITCH_FLAG_3WAY))
                {
                    std::string swname  = Switches::switches[swidx].get_name();

                    state = DCC_SWITCH_STATE_BRANCH;
                    HTTP::response += (String) "<font color='red'>Warnung: " + swname + " ist keine 3-Wege-Weiche. Daten wurden ge&auml;ndert auf 'Abzweig'.</font><P>";
                }

                bool its_ok = false;

                if (real_sub_idx >= n_rr_switches)
                {
                    uint_fast8_t new_idx = 0xFF;

                    new_idx = nrr->add_switch();

                    if (new_idx != 0xFF)
                    {
                        n_rr_switches++;
                        its_ok = true;
                    }
                    else
                    {
                        HTTP::response += (String) "<font color='red'>Fehler #1: n_rr_switches = " + std::to_string(n_rr_switches) + " new_idx = " + std::to_string(new_idx) + "</font><P>\r\n";
                    }
                }
                else
                {
                    its_ok = true;
                }

                if (its_ok)
                {
                    nrr->set_switch_idx(real_sub_idx, swidx);
                    nrr->set_switch_state(real_sub_idx, state);
                    real_sub_idx++;
                }
            }
        }

        while (real_sub_idx < n_rr_switches)
        {
            if (nrr->del_switch (n_rr_switches - 1))
            {
                n_rr_switches--;
            }
        }
    }
    else if (! strcmp (action, "delrr"))
    {
        uint_fast16_t   rrgidx    = HTTP::parameter_number ("rrgidx");
        uint_fast16_t   rridx     = HTTP::parameter_number ("rridx");

        RailroadGroups::railroad_groups[rrgidx].del(rridx);
    }
    else if (! strcmp (action, "delrrg"))
    {
        uint_fast16_t   rrgidx    = HTTP::parameter_number ("rrgidx");

        RailroadGroups::del(rrgidx);
    }

    uint_fast8_t    rrgidx   = 0;
    uint_fast8_t    rridx    = 0;
    const char *    bg;

    HTTP::response += (String)
        "<script>\r\n"
        "function rrset(rrgidx, rridx) { var http = new XMLHttpRequest(); http.open ('GET', '/action?action=rrset&rrgidx=' + rrgidx + '&rridx=' + rridx);"
        "http.addEventListener('load',"
        "function(event) { var text = http.responseText; if (http.status >= 200 && http.status < 300) { if (text !== '') alert (text); }});"
        "http.send (null);}\r\n"
        "function changerrg(rrgidx, name)\r\n"
        "{\r\n"
        "  document.getElementById('action').value = 'changerrg';\r\n"
        "  document.getElementById('rrgidx').value = rrgidx;\r\n"
        "  document.getElementById('nrrgidx').value = rrgidx;\r\n"
        "  document.getElementById('name').value = name;\r\n"
        "  document.getElementById('newid').style.display='';\r\n"
        "  document.getElementById('formrrg').style.display='';\r\n"
        "}\r\n"
        "function newrrg()\r\n"
        "{\r\n"
        "  document.getElementById('action').value = 'newrrg';\r\n"
        "  document.getElementById('name').value = name;\r\n"
        "  document.getElementById('newid').style.display='none';\r\n"
        "  document.getElementById('formrrg').style.display='';\r\n"
        "}\r\n"
        "function newrr(rrgidx) { document.getElementById('formrr' + rrgidx).style.display=''; }\r\n";

    HTTP::flush ();

    HTTP_Common::add_action_handler ("rr", "", 200);

    HTTP::response += (String) "</script>\r\n";

    HTTP::flush ();

    uint_fast8_t    n_railroad_groups = RailroadGroups::get_n_railroad_groups ();

    for (rrgidx = 0; rrgidx < n_railroad_groups; rrgidx++)
    {
        std::string rrg_name = RailroadGroups::railroad_groups[rrgidx].get_name();

        HTTP::response += (String)
            "<P><table style='border:1px lightgray solid;'>\r\n"
            "<tr bgcolor='#e0e0e0'><th>ID</th><th style='width:170px'>" + String (rrg_name) + "</th>"
            "<th style='width:170px' nowrap>Verkn&uuml;pfte Lok</th>"
            "<th class='hide650' style='width:170px' nowrap>Aktivierung durch</th>"
            "<th class='hide650' style='width:170px' nowrap>Erkannte Lok</th>"
            "<th>Aktion</th>";

        if (HTTP_Common::edit_mode)
        {
            HTTP::response += (String) "<th><button onclick=\"changerrg(" + std::to_string(rrgidx) + ",'" + String (rrg_name) + "')\">Bearbeiten</th>";
            HTTP::response += (String) "<th><button onclick=\"window.location.href='" + url + "?action=delrrg&rrgidx=" + std::to_string(rrgidx) + "'; \">L&ouml;schen</button></th>";
        }

        HTTP::response += (String) "</tr>\r\n";

        uint_fast8_t    n_railroads             = RailroadGroups::railroad_groups[rrgidx].get_n_railroads();

        for (rridx = 0; rridx < n_railroads; rridx++)
        {
            Railroad *      rr                  = &RailroadGroups::railroad_groups[rrgidx].railroads[rridx];
            uint_fast16_t   linked_loco_idx     = rr->get_link_loco();
            uint_fast16_t   located_loco_idx    = rr->get_located_loco();
            uint_fast16_t   active_loco_idx     = rr->get_active_loco();
            String          rr_name             = rr->get_name();
            String          linked_loco_name;
            String          located_loco_name;
            String          active_loco_name;

            if (linked_loco_idx != 0xFFFF)
            {
                linked_loco_name = Locos::locos[linked_loco_idx].get_name();
            }
            else
            {
                linked_loco_name = "";
            }

            if (located_loco_idx != 0xFFFF)
            {
                located_loco_name = Locos::locos[located_loco_idx].get_name();
            }
            else
            {
                located_loco_name = "";
            }

            if (active_loco_idx != 0xFFFF)
            {
                active_loco_name = Locos::locos[active_loco_idx].get_name();
            }
            else
            {
                active_loco_name = "";
            }

            if (rridx % 2)
            {
                bg = "bgcolor='#e0e0e0'";
            }
            else
            {
                bg = "";
            }

            HTTP::response += (String) "<tr " + bg + "><td style='text-align:right' id='id_" + std::to_string(rrgidx) + "_" + std::to_string(rridx) + "'>" + std::to_string(rridx) + "&nbsp;</td>"
                        + "<td>" + rr_name + "</td>"
                        + "<td style='width:170px;overflow:hidden' nowrap><a href='/loco?action=loco&lidx=" + std::to_string(linked_loco_idx) + "'>" + linked_loco_name + "</a></td>"
                        + "<td class='hide650' id='act_" + std::to_string(rrgidx) + "_" + std::to_string(rridx) + "' style='width:170px;overflow:hidden' nowrap><a href='/loco?action=loco&lidx=" + std::to_string(active_loco_idx) + "'>" + active_loco_name + "</a></td>"
                        + "<td class='hide650' id='loc_" + std::to_string(rrgidx) + "_" + std::to_string(rridx) + "' style='width:170px;overflow:hidden' nowrap><a href='/loco?action=loco&lidx=" + std::to_string(located_loco_idx) + "'>" + located_loco_name + "</a></td>"
                        + "<td><button onclick=rrset(" + std::to_string(rrgidx) + "," + std::to_string(rridx) + ")>Setzen</button></td>";

            if (HTTP_Common::edit_mode)
            {
                HTTP::response += (String)
                          "<td><button onclick=\"window.location.href='" + urledit + "?rrgidx=" + std::to_string(rrgidx) + "&rridx=" + std::to_string(rridx) + "'; "
                        + "\">Bearbeiten</button></td>"
                        + "<td><button onclick=\"window.location.href='" + url + "?action=delrr&rrgidx=" + std::to_string(rrgidx) + "&rridx=" + std::to_string(rridx) + "'; \">L&ouml;schen</button></td>";
            }

            HTTP::response += (String) "</tr>\r\n";
        }

        HTTP::response += (String) "</table>\r\n";

        if (HTTP_Common::edit_mode)
        {
            HTTP::response += (String)
                "<button onclick='newrr(" + std::to_string(rrgidx) + ")'>Neues Gleis</button>\r\n"
                "<form id='formrr" + std::to_string(rrgidx) + "' style='display:none'>\r\n"
                "<table>\r\n"
                "<tr><td>Name:</td><td><input type='text' style='width:200px' name='name' value=''></td></tr>\r\n"
                "<tr><td></td><td align='right'>"
                "<input type='hidden' name='action' value='newrr'>\r\n"
                "<input type='hidden' name='rrgidx' value='" + std::to_string(rrgidx) + "'>\r\n"
                "<input type='submit' value='Speichern'></td></tr>\r\n"
                "</table>\r\n"
                "</form><BR>\r\n";
        }

        HTTP::flush ();
    }

    if (HTTP_Common::edit_mode)
    {
        HTTP::response += (String)
            "<P><button onclick='newrrg()'>Neue Fahrstra&szlig;e</button>\r\n"
            "<form id='formrrg' style='display:none'>\r\n"
            "<table>\r\n"
            "<tr id='newid'>\r\n";

        HTTP_Common::print_id_select_list ("nrrgidx", n_railroad_groups);

        HTTP::response += (String)
            "</tr>\r\n"
            "<tr><td>Name der Fahrstra&szlig;e:</td><td><input type='text' style='width:200px' id='name' name='name'></td></tr>\r\n"
            "<tr><td></td><td align='right'>"
            "<input type='hidden' name='action' id='action' value='newrrg'>\r\n"
            "<input type='hidden' name='rrgidx' id='rrgidx'>\r\n"
            "<input type='submit' value='Speichern'></td></tr>\r\n"
            "</table>\r\n"
            "</form>\r\n";

        HTTP::flush ();
    }

    if (RailroadGroups::data_changed)
    {
        HTTP::response += (String)
            "<BR><form method='get' action='" + url + "'>\r\n"
            "<input type='hidden' name='action' value='saverrg'>\r\n"
            "<input type='submit' value='Ge&auml;nderte Konfiguration auf Datentr&auml;ger speichern'>\r\n";
    }

    HTTP::response += (String) "</div>\r\n";
    HTTP_Common::html_trailer ();
}

/*----------------------------------------------------------------------------------------------------------------------------------------
 * handle_rr_edit ()
 *----------------------------------------------------------------------------------------------------------------------------------------
 */
void
HTTP_Railroad::handle_rr_edit (void)
{
    String          url             = "/rr";
    String          browsertitle    = (String) "Fahrstra&szlig;e bearbeiten";
    String          title           = (String) "<a href='" + url + "'>Fahrstra&szlig;en</a> &rarr; " + "Fahrstra&szlig;e bearbeiten";
    uint_fast8_t    rrgidx          = HTTP::parameter_number ("rrgidx");
    uint_fast8_t    rridx           = HTTP::parameter_number ("rridx");
    RailroadGroup * rrg             = &RailroadGroups::railroad_groups[rrgidx];
    Railroad *      rr              = &(rrg->railroads[rridx]);
    uint_fast8_t    n_railroads     = rrg->get_n_railroads();
    std::string     rr_name         = rr->get_name();
    uint_fast16_t   linked_loco     = rr->get_link_loco();
    uint_fast8_t    n_rr_switches   = rr->get_n_switches ();
    uint_fast8_t    sub_idx;

    HTTP_Common::html_header (browsertitle, title, url, true);
    HTTP::response += (String) "<div style='margin-left:20px;'>\r\n";
    HTTP_Common::add_action_handler ("head", "", 200, true);
    
    HTTP::response += (String) "<form method='get' action='" + url + "'>"
                + "<table style='border:1px lightgray solid;'>\r\n";

    HTTP_Common::print_id_select_list ("nrridx", n_railroads, rridx);

    HTTP::response += (String) "<tr bgcolor='#e0e0e0'><td>Name:</td><td><input type='text' style='width:200px' name='name' value='"
                + rr_name + "'></td></tr>\r\n";

    HTTP::response += "<tr><td>Verkn&uuml;pfte Lok:</td><td>";

    HTTP_Common::print_loco_select_list ((String) "ll", linked_loco, LOCO_LIST_DO_DISPLAY);

    HTTP::response += (String) "</td></tr>\r\n";

    const char * bg = "bgcolor='#e0e0e0'";

    for (sub_idx = 0; sub_idx < MAX_SWITCHES_PER_RAILROAD; sub_idx++)
    {
        uint_fast16_t swidx;
        uint_fast8_t  state;

        if (sub_idx % 2)
        {
            bg = "bgcolor='#e0e0e0'";
        }
        else
        {
            bg = "";
        }

        HTTP::response += (String) "<tr " + bg + ">\r\n";

        if (sub_idx < n_rr_switches)
        {
            uint_fast16_t switch_idx = rr->get_switch_idx(sub_idx);

            swidx = switch_idx + 1; // swidx: 1 - n_switches
            state = rr->get_switch_state (sub_idx);
        }
        else
        {
            swidx = 0;                                                                  // swidx: 0: not used yet
            state = 0xFF;
        }

        uint_fast16_t n_switches = Switches::get_n_switches ();

        if (swidx <= n_switches)                                                // swidx ends with n_switches!
        {
            HTTP::response += (String) "<td>\r\n";
            print_select_switch (sub_idx, swidx);
            HTTP::response += (String) "</td>\r\n";
            HTTP::response += (String) "<td>";
            print_switch_state (sub_idx, state);
            HTTP::response += (String) "</td>\r\n";
        }
        else
        {
            HTTP::response += (String) "<td>Internal</td><td>Error!</td>\r\n";
        }

        HTTP::response += (String) "</tr>\r\n";
    }

    HTTP::response += (String)
        "<tr><td><input type='button'  onclick=\"window.location.href='" + url + "';\" value='Zur&uuml;ck'></td>\r\n"
        "<td align='right'><input type='hidden' name='action' id='action' value='changerr'>\r\n"
        "<input type='hidden' name='rrgidx' value='" + std::to_string(rrgidx) + "'>\r\n"
        "<input type='hidden' name='rridx' value='" + std::to_string(rridx) + "'>\r\n"
        "<input type='submit' value='Speichern'></td></tr>\r\n"
        "</table>\r\n"
        "</form>\r\n"
        "</div>\r\n";
    HTTP_Common::html_trailer ();
}

void
HTTP_Railroad::action_rr (void)
{
    uint_fast8_t    rrgidx;
    uint_fast8_t    rridx;
    uint_fast8_t    n_railroad_groups = RailroadGroups::get_n_railroad_groups ();

    HTTP_Common::head_action ();

    for (rrgidx = 0; rrgidx < n_railroad_groups; rrgidx++)
    {
        RailroadGroup * rrg                     = &RailroadGroups::railroad_groups[rrgidx];
        uint_fast8_t    n_railroads             = rrg->get_n_railroads();
        uint_fast8_t    active_railroad_idx     = rrg->get_active_railroad();

        for (rridx = 0; rridx < n_railroads; rridx++)
        {
            Railroad *      rr                  = &(rrg->railroads[rridx]);
            uint_fast16_t   active_loco_idx     = rr->get_active_loco();
            uint_fast16_t   located_loco_idx    = rr->get_located_loco();
            String          active_loco_name;
            String          located_loco_name;
            String          id;

            id = (String) "id_" + std::to_string(rrgidx) + "_" + std::to_string(rridx);

            if (rridx == active_railroad_idx)
            {
                HTTP_Common::add_action_content (id, "bgcolor", "green");
                HTTP_Common::add_action_content (id, "color", "white");
            }
            else
            {
                HTTP_Common::add_action_content (id, "bgcolor", "");
                HTTP_Common::add_action_content (id, "color", "");
            }

            if (active_loco_idx != 0xFFFF)
            {
                active_loco_name = Locos::locos[active_loco_idx].get_name();
            }
            else
            {
                active_loco_name = "";
            }

            id = (String) "act_" + std::to_string(rrgidx) + "_" + std::to_string(rridx);
            HTTP_Common::add_action_content (id, "text", active_loco_name);

            if (located_loco_idx != 0xFFFF)
            {
                located_loco_name = Locos::locos[located_loco_idx].get_name();
            }
            else
            {
                located_loco_name = "";
            }

            id = (String) "loc_" + std::to_string(rrgidx) + "_" + std::to_string(rridx);
            HTTP_Common::add_action_content (id, "text", located_loco_name);
        }
    }
}

void
HTTP_Railroad::action_rrset (void)
{
    uint_fast16_t   rrgidx      = HTTP::parameter_number ("rrgidx");
    uint_fast16_t   rridx       = HTTP::parameter_number ("rridx");

    RailroadGroups::railroad_groups[rrgidx].set_active_railroad(rridx);
}
