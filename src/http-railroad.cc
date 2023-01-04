/*------------------------------------------------------------------------------------------------------------------------
 * http-railroad.cc - HTTP railroad routines
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

    HTTP::response += (String) "<select class='select' name='rr" + std::to_string(subidx) + "'>\r\n";
    HTTP::response += (String) "<option value='0'>---</option>\r\n";

    uint_fast16_t n_switches = Switch::get_n_switches ();

    for (idx = 0; idx < n_switches; idx++)
    {
        char *  name    = Switch::get_name (idx);

        HTTP::response += (String) "<option value='" + std::to_string(idx + 1) + "'";

        if ((idx + 1) == swidx)
        {
            HTTP::response += (String) " selected";
        }

        HTTP::response += (String) ">";
        HTTP::response += (String) name;
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

    HTTP::response += (String) "<input type='radio' id='gerade" + std::to_string(subidx) + "' name='state" + std::to_string(subidx) + "' value='1' " + checked_straight + ">";
    HTTP::response += (String) "<label for='gerade" + std::to_string(subidx) + "'>Gerade</label>\r\n";
    HTTP::response += (String) "<input type='radio' id='abzweig" + std::to_string(subidx) + "' name='state" + std::to_string(subidx) + "' value='0' " + checked_branch + ">";
    HTTP::response += (String) "<label for='abzweig" + std::to_string(subidx) + "'>Abzweig</label>\r\n";
    HTTP::response += (String) "<input type='radio' id='abzweig2" + std::to_string(subidx) + "' name='state" + std::to_string(subidx) + "' value='2' " + checked_branch2 + ">";
    HTTP::response += (String) "<label for='abzweig2" + std::to_string(subidx) + "'>Abzweig2</label>\r\n";
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
    uint_fast8_t    rrg_idx   = 0;
    uint_fast8_t    rr_idx    = 0;

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

        rrgidx = Railroad::group_add ();

        if (rrgidx != 0xFF)
        {
            Railroad::group_setname (rrgidx, name);
        }
    }
    else if (! strcmp (action, "changerrg"))
    {
        const char *    sname       = HTTP::parameter ("name");
        uint_fast8_t   rrgidx       = HTTP::parameter_number ("rrgidx");
        uint_fast8_t   nrrgidx      = HTTP::parameter_number ("nrrgidx");

        if (rrgidx != nrrgidx)
        {
            nrrgidx = Railroad::group_setid (rrgidx, nrrgidx);
        }

        if (nrrgidx != 0xFF)
        {
            Railroad::group_setname (nrrgidx, sname);
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

        rridx = Railroad::add (rrgidx);

        if (rridx != 0xFF)
        {
            Railroad::set_name (rrgidx, rridx, name);
        }
    }
    else if (! strcmp (action, "changerr"))  // see handle_rr_edit())
    {
        const char *    sname           = HTTP::parameter ("name");
        uint_fast16_t   rrgidx          = HTTP::parameter_number ("rrgidx");
        uint_fast16_t   rridx           = HTTP::parameter_number ("rridx");
        uint_fast16_t   nrridx          = HTTP::parameter_number ("nrridx");
        uint_fast16_t   ll              = HTTP::parameter_number ("ll");
        uint_fast8_t    n_rr_switches   = Railroad::get_n_switches (rrgidx, rridx);
        uint_fast8_t    real_sub_idx    = 0;
        uint_fast8_t    sub_idx;

        if (rridx != nrridx)
        {
            nrridx = Railroad::setid (rrgidx, rridx, nrridx);
        }

        Railroad::set_name (rrgidx, nrridx, sname);
        Railroad::set_link_loco (rrgidx, nrridx, ll);

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

                uint_fast8_t    flags   = Switch::get_flags (swidx);

                if (state == DCC_SWITCH_STATE_BRANCH2 && ! (flags & SWITCH_FLAG_3WAY))
                {
                    char *  swname  = Switch::get_name (swidx);

                    state = DCC_SWITCH_STATE_BRANCH;
                    HTTP::response += (String) "<font color='red'>Warnung: " + swname + " ist keine 3-Wege-Weiche. Daten wurden ge&auml;ndert auf 'Abzweig'.</font><P>";
                }

                bool its_ok = false;

                if (real_sub_idx >= n_rr_switches)
                {
                    uint_fast8_t new_idx = 0xFF;

                    new_idx = Railroad::add_switch (rrgidx, nrridx);

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
                    Railroad::set_switch_idx (rrgidx, nrridx, real_sub_idx, swidx);
                    Railroad::set_switch_state (rrgidx, nrridx, real_sub_idx, state);
                    real_sub_idx++;
                }
            }
        }

        while (real_sub_idx < n_rr_switches)
        {
            if (Railroad::del_switch (rrgidx, nrridx, n_rr_switches - 1))
            {
                n_rr_switches--;
            }
        }
    }
    else if (! strcmp (action, "delrr"))
    {
        uint_fast16_t   rrgidx    = HTTP::parameter_number ("rrgidx");
        uint_fast16_t   rridx     = HTTP::parameter_number ("rridx");

        Railroad::del (rrgidx, rridx);
    }
    else if (! strcmp (action, "delrrg"))
    {
        uint_fast16_t   rrgidx    = HTTP::parameter_number ("rrgidx");

        Railroad::group_del (rrgidx);
    }

    const char *  bg;

    HTTP::response += (String) "<script>\r\n";
    HTTP::response += (String) "function rrset(rrgidx, rridx) { var http = new XMLHttpRequest(); http.open ('GET', '/action?action=rrset&rrgidx=' + rrgidx + '&rridx=' + rridx);";
    HTTP::response += (String) "http.addEventListener('load',";
    HTTP::response += (String) "function(event) { var text = http.responseText; if (http.status >= 200 && http.status < 300) { if (text !== '') alert (text); }});";
    HTTP::response += (String) "http.send (null);}\r\n";
    HTTP::response += (String) "function changerrg(rrgidx, name)\r\n";
    HTTP::response += (String) "{\r\n";
    HTTP::response += (String) "  document.getElementById('action').value = 'changerrg';\r\n";
    HTTP::response += (String) "  document.getElementById('rrgidx').value = rrgidx;\r\n";
    HTTP::response += (String) "  document.getElementById('nrrgidx').value = rrgidx;\r\n";
    HTTP::response += (String) "  document.getElementById('name').value = name;\r\n";
    HTTP::response += (String) "  document.getElementById('newid').style.display='';\r\n";
    HTTP::response += (String) "  document.getElementById('formrrg').style.display='';\r\n";
    HTTP::response += (String) "}\r\n";
    HTTP::response += (String) "function newrrg()\r\n";
    HTTP::response += (String) "{\r\n";
    HTTP::response += (String) "  document.getElementById('action').value = 'newrrg';\r\n";
    HTTP::response += (String) "  document.getElementById('name').value = name;\r\n";
    HTTP::response += (String) "  document.getElementById('newid').style.display='none';\r\n";
    HTTP::response += (String) "  document.getElementById('formrrg').style.display='';\r\n";
    HTTP::response += (String) "}\r\n";

    HTTP::response += (String) "function newrr(rrgidx) { document.getElementById('formrr' + rrgidx).style.display=''; }\r\n";

    HTTP::flush ();

    HTTP_Common::add_action_handler ("rr", "", 200);

    HTTP::response += (String) "</script>\r\n";

    HTTP::flush ();

    uint_fast8_t    n_railroad_groups = Railroad::get_n_railroad_groups ();

    for (rrg_idx = 0; rrg_idx < n_railroad_groups; rrg_idx++)
    {
        char * rrg_name = Railroad::group_getname (rrg_idx);

        HTTP::response += (String) "<P><table style='border:1px lightgray solid;'>\r\n";
        HTTP::response += (String) "<tr bgcolor='#e0e0e0'><th>ID</th><th style='width:170px'>" + String (rrg_name) + "</th>";
        HTTP::response += (String) "<th style='width:170px' nowrap>Verkn&uuml;pfte Lok</th>";
        HTTP::response += (String) "<th class='hide650' style='width:170px' nowrap>Aktivierung durch</th>";
        HTTP::response += (String) "<th class='hide650' style='width:170px' nowrap>Erkannte Lok</th>";
        HTTP::response += (String) "<th>Aktion</th>";

        if (HTTP_Common::edit_mode)
        {
            HTTP::response += (String) "<th><button onclick=\"changerrg(" + std::to_string(rrg_idx) + ",'" + String (rrg_name) + "')\">Bearbeiten</th>";
            HTTP::response += (String) "<th><button onclick=\"window.location.href='" + url + "?action=delrrg&rrgidx=" + std::to_string(rrg_idx) + "'; \">L&ouml;schen</button></th>";
        }

        HTTP::response += (String) "</tr>\r\n";

        uint_fast8_t    n_railroads             = Railroad::get_n_railroads (rrg_idx);

        for (rr_idx = 0; rr_idx < n_railroads; rr_idx++)
        {
            uint_fast16_t   linked_loco_idx     = Railroad::get_link_loco (rrg_idx, rr_idx);
            uint_fast16_t   located_loco_idx    = Railroad::get_located_loco (rrg_idx, rr_idx);
            uint_fast16_t   active_loco_idx     = Railroad::get_active_loco (rrg_idx, rr_idx);
            const char *    linked_loco_name;
            const char *    located_loco_name;
            const char *    active_loco_name;

            linked_loco_name = Loco::get_name (linked_loco_idx);

            if (! linked_loco_name)
            {
                linked_loco_name = "";
            }

            active_loco_name = Loco::get_name (active_loco_idx);

            if (! active_loco_name)
            {
                active_loco_name = "";
            }

            located_loco_name = Loco::get_name (located_loco_idx);

            if (! located_loco_name)
            {
                located_loco_name = "";
            }

            if (rr_idx % 2)
            {
                bg = "bgcolor='#e0e0e0'";
            }
            else
            {
                bg = "";
            }

            char * rr_name = Railroad::get_name (rrg_idx, rr_idx);

            HTTP::response += (String) "<tr " + bg + "><td style='text-align:right' id='id_" + std::to_string(rrg_idx) + "_" + std::to_string(rr_idx) + "'>" + std::to_string(rr_idx) + "&nbsp;</td>"
                        + "<td>" + rr_name + "</td>"
                        + "<td style='width:170px;overflow:hidden' nowrap><a href='/loco?action=loco&lidx=" + std::to_string(linked_loco_idx) + "'>" + linked_loco_name + "</a></td>"
                        + "<td class='hide650' id='act_" + std::to_string(rrg_idx) + "_" + std::to_string(rr_idx) + "' style='width:170px;overflow:hidden' nowrap><a href='/loco?action=loco&lidx=" + std::to_string(active_loco_idx) + "'>" + active_loco_name + "</a></td>"
                        + "<td class='hide650' id='loc_" + std::to_string(rrg_idx) + "_" + std::to_string(rr_idx) + "' style='width:170px;overflow:hidden' nowrap><a href='/loco?action=loco&lidx=" + std::to_string(located_loco_idx) + "'>" + located_loco_name + "</a></td>"
                        + "<td><button onclick=rrset(" + std::to_string(rrg_idx) + "," + std::to_string(rr_idx) + ")>Setzen</button></td>";

            if (HTTP_Common::edit_mode)
            {
                HTTP::response += (String)
                          "<td><button onclick=\"window.location.href='" + urledit + "?rrgidx=" + std::to_string(rrg_idx) + "&rridx=" + std::to_string(rr_idx) + "'; "
                        + "\">Bearbeiten</button></td>"
                        + "<td><button onclick=\"window.location.href='" + url + "?action=delrr&rrgidx=" + std::to_string(rrg_idx) + "&rridx=" + std::to_string(rr_idx) + "'; \">L&ouml;schen</button></td>";
            }

            HTTP::response += (String) "</tr>\r\n";
        }

        HTTP::response += (String) "</table>\r\n";

        if (HTTP_Common::edit_mode)
        {
            HTTP::response += (String) "<button onclick='newrr(" + std::to_string(rrg_idx) + ")'>Neues Gleis</button>\r\n";
            HTTP::response += (String) "<form id='formrr" + std::to_string(rrg_idx) + "' style='display:none'>\r\n";
            HTTP::response += (String) "<table>\r\n";
            HTTP::response += (String) "<tr><td>Name:</td><td><input type='text' style='width:200px' name='name' value=''></td></tr>\r\n";
            HTTP::response += (String) "<tr><td></td><td align='right'>";
            HTTP::response += (String) "<input type='hidden' name='action' value='newrr'>\r\n";
            HTTP::response += (String) "<input type='hidden' name='rrgidx' value='" + std::to_string(rrg_idx) + "'>\r\n";
            HTTP::response += (String) "<input type='submit' value='Speichern'></td></tr>\r\n";
            HTTP::response += (String) "</table>\r\n";
            HTTP::response += (String) "</form><BR>\r\n";
        }

        HTTP::flush ();
    }

    if (HTTP_Common::edit_mode)
    {
        HTTP::response += (String) "<P><button onclick='newrrg()'>Neue Fahrstra&szlig;e</button>\r\n";
        HTTP::response += (String) "<form id='formrrg' style='display:none'>\r\n";
        HTTP::response += (String) "<table>\r\n";
        HTTP::response += (String) "<tr id='newid'>\r\n";
        HTTP_Common::print_id_select_list ("nrrgidx", n_railroad_groups);
        HTTP::response += (String) "</tr>\r\n";
        HTTP::response += (String) "<tr><td>Name der Fahrstra&szlig;e:</td><td><input type='text' style='width:200px' id='name' name='name'></td></tr>\r\n";
        HTTP::response += (String) "<tr><td></td><td align='right'>";
        HTTP::response += (String) "<input type='hidden' name='action' id='action' value='newrrg'>\r\n";
        HTTP::response += (String) "<input type='hidden' name='rrgidx' id='rrgidx'>\r\n";
        HTTP::response += (String) "<input type='submit' value='Speichern'></td></tr>\r\n";
        HTTP::response += (String) "</table>\r\n";
        HTTP::response += (String) "</form>\r\n";

        HTTP::flush ();
    }

    if (Railroad::data_changed)
    {
        HTTP::response += (String) "<BR><form method='get' action='" + url + "'>\r\n";
        HTTP::response += (String) "<input type='hidden' name='action' value='saverrg'>\r\n";
        HTTP::response += (String) "<input type='submit' value='Ge&auml;nderte Konfiguration auf Datentr&auml;ger speichern'>\r\n";
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
    uint_fast8_t    n_rr_switches   = Railroad::get_n_switches (rrgidx, rridx);
    uint_fast8_t    sub_idx;

    HTTP_Common::html_header (browsertitle, title, url, true);
    HTTP::response += (String) "<div style='margin-left:20px;'>\r\n";
    HTTP_Common::add_action_handler ("head", "", 200, true);
    
    char *          rr_name     = Railroad::get_name (rrgidx, rridx);
    uint_fast8_t    n_railroads = Railroad::get_n_railroads (rrgidx);
    uint_fast16_t   linked_loco = Railroad::get_link_loco (rrgidx, rridx);

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
            uint_fast16_t switch_idx = Railroad::get_switch_idx (rrgidx, rridx, sub_idx);

            swidx = switch_idx + 1; // swidx: 1 - n_switches
            state = Railroad::get_switch_state (rrgidx, rridx, sub_idx);
        }
        else
        {
            swidx = 0;                                                                  // swidx: 0: not used yet
            state = 0xFF;
        }

        uint_fast16_t n_switches = Switch::get_n_switches ();

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

    HTTP::response += (String) "<tr><td><input type='button'  onclick=\"window.location.href='" + url + "';\" value='Zur&uuml;ck'></td>\r\n";
    HTTP::response += (String) "<td align='right'><input type='hidden' name='action' id='action' value='changerr'>\r\n";
    HTTP::response += (String) "<input type='hidden' name='rrgidx' value='" + std::to_string(rrgidx) + "'>\r\n";
    HTTP::response += (String) "<input type='hidden' name='rridx' value='" + std::to_string(rridx) + "'>\r\n";
    HTTP::response += (String) "<input type='submit' value='Speichern'></td></tr>\r\n";
    HTTP::response += (String) "</table>\r\n";

    HTTP::response += (String) "</form>\r\n";
    HTTP::response += (String) "</div>\r\n";
    HTTP_Common::html_trailer ();
}

void
HTTP_Railroad::action_rr (void)
{
    uint_fast8_t    rrg_idx;
    uint_fast8_t    rr_idx;
    uint_fast8_t    n_railroad_groups = Railroad::get_n_railroad_groups ();

    HTTP_Common::head_action ();

    for (rrg_idx = 0; rrg_idx < n_railroad_groups; rrg_idx++)
    {
        uint_fast8_t    n_railroads             = Railroad::get_n_railroads (rrg_idx);
        uint_fast8_t    active_railroad_idx     = Railroad::get (rrg_idx);

        for (rr_idx = 0; rr_idx < n_railroads; rr_idx++)
        {
            uint_fast16_t   active_loco_idx     = Railroad::get_active_loco (rrg_idx, rr_idx);
            uint_fast16_t   located_loco_idx    = Railroad::get_located_loco (rrg_idx, rr_idx);
            const char *    active_loco_name;
            const char *    located_loco_name;
            String          id;

            id = (String) "id_" + std::to_string(rrg_idx) + "_" + std::to_string(rr_idx);

            if (rr_idx == active_railroad_idx)
            {
                HTTP_Common::add_action_content (id, "bgcolor", "green");
                HTTP_Common::add_action_content (id, "color", "white");
            }
            else
            {
                HTTP_Common::add_action_content (id, "bgcolor", "");
                HTTP_Common::add_action_content (id, "color", "");
            }

            active_loco_name = Loco::get_name (active_loco_idx);

            if (! active_loco_name)
            {
                active_loco_name = "";
            }

            id = (String) "act_" + std::to_string(rrg_idx) + "_" + std::to_string(rr_idx);
            HTTP_Common::add_action_content (id, "text", active_loco_name);

            located_loco_name = Loco::get_name (located_loco_idx);

            if (! located_loco_name)
            {
                located_loco_name = "";
            }

            id = (String) "loc_" + std::to_string(rrg_idx) + "_" + std::to_string(rr_idx);
            HTTP_Common::add_action_content (id, "text", located_loco_name);
        }
    }
}

void
HTTP_Railroad::action_rrset (void)
{
    uint_fast16_t   rrgidx      = HTTP::parameter_number ("rrgidx");
    uint_fast16_t   rridx       = HTTP::parameter_number ("rridx");

    Railroad::set (rrgidx, rridx);
}
