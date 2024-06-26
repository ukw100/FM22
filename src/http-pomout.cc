/*------------------------------------------------------------------------------------------------------------------------
 * http-pomout.cc - HTTP POM function output routines
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

#include "pom.h"
#include "http.h"
#include "http-common.h"
#include "http-pomout.h"

#define ESU_OUTPUT_LINES        24
#define ESU_OUTPUT_COLS         4
#define ESU_OUTPUT_MODES        32

static const char * esu_outputs[ESU_OUTPUT_LINES] =
{
    "Lv (K1)",
    "Lh (K1)",
    "AUX1 (K1)",
    "AUX2 (K1)",
    "AUX3",
    "AUX4",
    "AUX5",
    "AUX6",
    "AUX7",
    "AUX8",
    "AUX9",
    "AUX10",
    "AUX11",
    "AUX12",
    "AUX13",
    "AUX14",
    "AUX15",
    "AUX16",
    "AUX17",
    "AUX18",
    "Lv (K2)",
    "Lh (K2)",
    "AUX1 (K2)",
    "AUX2 (K2)"
};

static const char * esu_output_modes[ESU_OUTPUT_MODES] =
{
    "(aus)",                                        // 0
    "Dimmbares Licht",                              // 1
    "Dimmbares Licht (auf/abblendbar)",             // 2
    "Feuerb&uuml;chse",                             // 3
    "Intelligente Feuerb&uuml;chse",                // 4
    "Single Strobe",                                // 5
    "Double Strobe",                                // 6
    "Rotary Beacon",                                // 7
    "Strato Light",                                 // 8
    "Ditch Light Type 1",                           // 9
    "Ditch Light Type 2",                           // 10
    "Oscillator",                                   // 11
    "Blinklicht",                                   // 12
    "Mars Light",                                   // 13
    "Gyra Light",                                   // 14
    "FRED",                                         // 15
    "Neonlampe",                                    // 16
    "Energiesparlampe",                             // 17
    "Single Strobe Zuf&auml;llig",                  // 18
    "(19 unbekannt)",                               // 19
    "(20 unbekannt)",                               // 20
    "ESU Kupplung 1+2 (Kompatibilit&auml;t)",       // 21
    "Raucherzeuger (Soundgesteuert)",               // 22
    "L&uuml;fterfunktion (Ventilator)",             // 23
    "Seuthe&#174; Rauchgenerator",                  // 24
    "Reserviert",                                   // 25
    "Reserviert",                                   // 26
    "Servo",                                        // 27
    "Konventionelle Kupplungsfunktion",             // 28
    "ROCO&#174; Kupplungsfunktion",                 // 29
    "Pantographensteuerung",                        // 30
    "PowerPack Control",                            // 31
};

static uint32_t                 read_retries;
static uint32_t                 num_reads;
static uint32_t                 time_reads;

static uint8_t                  esu_output_mode_values[ESU_OUTPUT_LINES][ESU_OUTPUT_COLS];
static uint8_t                  new_esu_output_mode_values[ESU_OUTPUT_LINES][ESU_OUTPUT_COLS];
static uint_fast16_t            esu_output_changes = 0;

static void
print_select_cv_values (const char * prefix, uint_fast8_t line, const char * onchange, uint_fast8_t value, uint_fast8_t min_value, uint_fast8_t max_value, float factor, const char * unit)
{
    uint_fast16_t       idx;                // uint_fast8_t is too small: 0-256
    String              svalue;
    String              selected;
    String              onchangeattr;

    if (onchange)
    {
        onchangeattr = (String) "onchange=\"" + onchange + "('" + prefix + "'," + std::to_string(line) + ")\"";
    }
    else
    {
        onchangeattr = "";
    }

    HTTP::response += (String) "<select id='" + (String) prefix + std::to_string(line) + "' " + onchangeattr + ">\r\n";

    for (idx = min_value; idx <= max_value; idx++)
    {
        if (idx == value)
        {
            selected = "selected";
        }
        else
        {
            selected = "";
        }

        if (factor >= 1.0)
        {
            svalue = std::to_string ((int) (idx * factor));
        }
        else
        {
            char buf[32];

            sprintf (buf, "%0.1f", idx * factor);
            svalue = buf;
        }

        if (unit && *unit)
        {
            svalue += (String) " " + unit;
        }

        HTTP::response += (String) "<option value='" + std::to_string(idx) + "' " + selected + ">" + svalue + "</option>\r\n";
    }

    HTTP::response += (String) "</select>\r\n";
}

static void
print_select_esu_output_modes (const char * prefix, uint_fast8_t line, const char * onchange, uint_fast8_t value)
{
    uint_fast8_t        idx;
    String              selected;
    String              onchangeattr;

    if (onchange)
    {
        onchangeattr = (String) "onchange=\"" + onchange + "('" + prefix + "'," + std::to_string(line) + ")\"";
    }
    else
    {
        onchangeattr = "";
    }

    HTTP::response += (String) "<select id='" + (String) prefix + std::to_string(line) + "' " + onchangeattr + ">\r\n";

    for (idx = 0; idx < ESU_OUTPUT_MODES; idx++)
    {
        if (idx == value)
        {
            selected = "selected";
        }
        else
        {
            selected = "";
        }

        HTTP::response += (String) "<option value='" + std::to_string(idx) + "' " + selected + ">" + esu_output_modes[idx] + "</option>\r\n";
    }

    HTTP::response += (String) "</select>\r\n";
}

static bool
get_esu_output_modes (uint_fast16_t addr)
{
    uint_fast8_t    line;
    uint_fast8_t    col;
    uint_fast8_t    cv;
    uint_fast8_t    cv31_value = 16;
    uint_fast8_t    cv32_value = 0;
    uint_fast8_t    values[4];
    bool            rtc = false;

    POM::pom_reset_num_reads ();
    time_reads = time ((time_t *) NULL);

    HTTP::response += (String) "<progress id='progress' max='" + std::to_string(ESU_OUTPUT_LINES) + "' value='0'></progress>\r\n";

    for (line = 0; line < ESU_OUTPUT_LINES; line++)
    {
        HTTP::response += (String) "<script>document.getElementById('progress').value = " + std::to_string(line + 1) + ";</script>";
        HTTP::flush ();

        cv = 8 * line + 259 - 257;

        if (! POM::xpom_read_cv (values, 1, addr, cv31_value, cv32_value, cv))
        {
            fprintf (stderr, "read error: cv=%u\n", cv);
            break;
        }

        for (col = 0; col < 4; col++)
        {
            esu_output_mode_values[line][col] = values[col];
        }
    }

    HTTP::response += (String) "<script>document.getElementById('progress').style.display = 'none';</script>";
    HTTP::flush ();

    if (line == ESU_OUTPUT_LINES)
    {
        memcpy (new_esu_output_mode_values, esu_output_mode_values, ESU_OUTPUT_LINES * ESU_OUTPUT_COLS);
        esu_output_changes = 0;
        rtc = true;
    }

    read_retries = POM::pom_get_read_retries ();
    num_reads = POM::pom_get_num_reads ();
    time_reads = time ((time_t *) NULL) - time_reads;
    return rtc;
}

static void
print_esu_output_modes (void)
{
    uint_fast8_t    line;
    uint_fast8_t    mode;
    uint_fast8_t    av;
    uint_fast8_t    ev;
    uint_fast8_t    auto_off;
    uint_fast8_t    brightness;

    HTTP::response += (String) std::to_string (num_reads) + " CV-Werte gelesen per XPOM, dabei mussten " + std::to_string(read_retries) + " Lesevorg&auml;nge wiederholt werden. \r\n";
    HTTP::response += (String) "Ben&ouml;tigte Zeit: " + std::to_string(time_reads) + " sec<BR>\r\n";;

    HTTP::response += (String)
        "<table style='border:1px solid gray'>\r\n"
        "<tr><th align='right'>Ausgang</th><th>Mode</th><th>EV</th><th>AV</th><th>AutoAb</th><th>Hell</th></tr>\r\n";

    for (line = 0; line < ESU_OUTPUT_LINES; line++)
    {
        mode        = esu_output_mode_values[line][0];
        av          = (esu_output_mode_values[line][1] & 0xF0) >> 4;
        ev          = (esu_output_mode_values[line][1] & 0x0F);
        auto_off    = esu_output_mode_values[line][2];
        brightness  = esu_output_mode_values[line][3];

        HTTP::response += (String) "<tr><td align='right' nowrap>" + esu_outputs[line] + "</td><td>";
        print_select_esu_output_modes ("om", line, "chout", mode);

        HTTP::response += (String) "</td><td>";
        print_select_cv_values ("ev", line, "chout", ev, 0, 15, 0.4096, "sec");

        HTTP::response += (String) "</td><td>";
        print_select_cv_values ("av", line, "chout", av, 0, 15, 0.4096, "sec");

        HTTP::response += (String) "</td><td>";
        print_select_cv_values ("ao", line, "chout", auto_off, 0, 255, 0.4, "sec");

        HTTP::response += (String) "</td><td>";
        print_select_cv_values ("br", line, "chout", brightness, 0, 31, 1, "");

        HTTP::response += (String) "</td></tr>\r\n";
    }

    HTTP::response += (String)
        "</table><P>\r\n"
        "<table>\r\n"
        "<tr><td>EV</td><td>Einschaltverz&ouml;gerung</td>\r\n"
        "<tr><td>AV</td><td>Ausschaltverz&ouml;gerung</td>\r\n"
        "<tr><td>AutoAb</td><td>Automatische Abschaltung</td>\r\n"
        "<tr><td>Hell</td><td>Helligkeit</td>\r\n"
        "</table>\r\n";
}

static bool
get_lenz_output_modes (uint_fast16_t addr)
{
    (void) addr;

    POM::pom_reset_num_reads ();
    time_reads = time ((time_t *) NULL);

    read_retries = POM::pom_get_read_retries ();
    num_reads = POM::pom_get_num_reads ();
    time_reads = time ((time_t *) NULL) - time_reads;
    return true;
}

static void
print_lenz_output_modes (void)
{
}

static bool
get_tams_output_modes (uint_fast16_t addr)
{
    (void) addr;

    POM::pom_reset_num_reads ();
    time_reads = time ((time_t *) NULL);

    read_retries = POM::pom_get_read_retries ();
    num_reads = POM::pom_get_num_reads ();
    time_reads = time ((time_t *) NULL) - time_reads;
    return true;
}

static void
print_tams_output_modes (void)
{
}

/*----------------------------------------------------------------------------------------------------------------------------------------
 * handle_pomout ()
 *----------------------------------------------------------------------------------------------------------------------------------------
 */
void
HTTP_POMOUT::handle_pomout (void)
{
    String          title   = "Funktionsausg&auml;nge (POM)";
    String          url     = "/pomout";
    const char *    action  = HTTP::parameter ("action");
    const char *    saddr   = "";
    uint_fast16_t   addr    = 0;
    uint_fast8_t    manu    = 0;
    bool            get_outputs_successful = false;

    HTTP_Common::html_header (title, title, url, true);
    HTTP::response += (String) "<div style='margin-left:20px;'>\r\n";
    HTTP_Common::add_action_handler ("head", "", 200, true);

    if (strcmp (action, "getmanu") == 0)
    {
        saddr   = HTTP::parameter ("addr");
        addr    = atoi (saddr);

        if (addr > 0)
        {
            if (! POM::pom_read_cv (&manu, addr, 8))
            {
                HTTP::response += (String) "<font color='red'><B>Lesefehler</B></font><BR>\r\n";
            }
            else if (manu)
            {
                HTTP::response += (String) "Hersteller: " + HTTP_Common::manufacturers[manu] + "<BR>\r\n";
            }
        }
        else
        {
            HTTP::response += (String) "<font color='red'><B>Ung&uuml;ltige Adresse</B></font><BR>\r\n";
        }
    }
    else if (strcmp (action, "getout") == 0)
    {
        manu    = HTTP::parameter_number ("manu");
        saddr   = HTTP::parameter ("addr");
        addr    = atoi (saddr);

        if (addr > 0)
        {
            switch (manu)
            {
                case 0:
                    HTTP::response += (String) "<font color='red'><B>Unbekannter Hersteller.</B></font><BR>\r\n";
                    break;
                case MANUFACTURER_ESU:
                    get_outputs_successful = get_esu_output_modes (addr);
                    break;
                case MANUFACTURER_LENZ:
                    get_outputs_successful = get_lenz_output_modes (addr);
                    break;
                case MANUFACTURER_TAMS:
                    get_outputs_successful = get_tams_output_modes (addr);
                    break;
                default:
                    HTTP::response += (String) "<font color='red'><B>Keine Ausg&auml;nge-Infos f&uuml;r Hersteller '" + HTTP_Common::manufacturers[manu] + "' vorhanden.</B></font><BR>\r\n";
                    break;
            }

            if (! get_outputs_successful)
            {
                HTTP::response += (String) "<font color='red'><B>Lesefehler</B></font><BR>\r\n";
            }
        }
        else
        {
            HTTP::response += (String) "<font color='red'><B>Ung&uuml;ltige Adresse</B></font><BR>\r\n";
        }
    }

    HTTP::response += (String)
        "<div id='isoff' style='display:none'><font color='red'>Booster ist abgeschaltet. Bitte einschalten.</font></div>\r\n"
        "<div id='ison' style='display:none'>\r\n";

    if (manu)
    {
        HTTP::response += (String)
            "<BR><div style='border:1px lightgray solid;margin-left:10px;display:inline-block;'>"
            "<div style='padding:4px;text-align:center;background-color:#E0E0E0;'><B>Ausg&auml;nge</B></div>\r\n"
            "<form method='GET' action='" + url + "'>\r\n"
            "<div style='padding:2px;'><span style='width:120px;display:inline-block;'>Adresse: " + saddr + "</span>"
            "<span style='width:110px;display:inline-block;'><a href='" + url + "'>Andere Adresse</a></span></div>\r\n"
            "<div style='padding:2px;'><span style='width:100%'>"
            "<input type='hidden' name='action' value='getout'>"
            "<input type='hidden' name='manu' value='" + std::to_string(manu) + "'>"
            "<input type='hidden' name='addr' value='" + saddr + "'>"
            "<input type='submit' style='width:100%;padding:2px;' value='Ausg&auml;nge lesen'></span></div>\r\n"
            "</form></div>\r\n";
    }
    else
    {   
        HTTP::response += (String)
            "<form method='GET' action='" + url + "'>\r\n"
            "<BR><table style='border:1px lightgray solid;margin-left:10px;'><tr bgcolor='#E0E0E0'><th colspan='2'>Ausg&auml;nge</th></tr>\r\n"
            "<tr><td>Adresse</td><td><input type='text' style='width:120px;' maxlength='4' name='addr' value='" + saddr + "'></td></tr>\r\n"
            "<tr><td colspan='2' width='100%'><input type='hidden' name='action' value='getmanu'>\r\n"
            "<input type='submit' style='width:100%' value='Hersteller ermitteln'></td></tr>\r\n"
            "</table>\r\n"
            "</form>\r\n";
    }

    HTTP::response += (String) "<P>\r\n";
    HTTP::flush ();

    HTTP::response += (String) "<script>";

    if (manu == MANUFACTURER_ESU)
    {
        HTTP::response += (String)
            "function chout(prefix,line) {\r\n"
            "  var id = prefix + line;\r\n"
            "  var value = document.getElementById(id).value;\r\n"
            "  var http = new XMLHttpRequest(); http.open ('GET', '/action?action=setoutputesu&prefix=' + prefix + '&line=' + line + '&value=' + value);"
            "  http.addEventListener('load',"
            "    function(event) {\r\nvar text = http.responseText;\r\nif (http.status >= 200 && http.status < 300) {\r\n"
            "      if (text == '0') {\r\ndocument.getElementById('saveesu').style.display = 'none'; } else {\r\ndocument.getElementById('saveesu').style.display = ''; }"
            "  }});"
            "  http.send (null);\r\n"
            "}\r\n"
            "function saveesu() {\r\n"
            "  document.body.style.cursor = 'wait';"
            "  var http = new XMLHttpRequest(); http.open ('GET', '/action?action=saveoutputesu&addr=" + saddr + "');\r\n"
            "  http.addEventListener('load',"
            "    function(event) {\r\n"
            "      document.body.style.cursor = 'auto';\r\n"
            "      var text = http.responseText; if (http.status >= 200 && http.status < 300) { "
            "      if (text == '0') { document.getElementById('saveesu').style.display = 'none'; } "
            "      else { document.getElementById('saveesu').style.display = ''; alert ('Schreibfehler') }"
            "  }});"
            "  http.send (null);\r\n"
            "}\r\n";
    }
    else if (manu == MANUFACTURER_LENZ)
    {
    }
    else if (manu == MANUFACTURER_TAMS)
    {
    }

    HTTP::response += (String) "</script>\r\n";
    HTTP::flush ();

    if (get_outputs_successful)
    {
        switch (manu)
        {
            case MANUFACTURER_ESU:
                print_esu_output_modes ();
                HTTP::response += (String) "<BR><button id='saveesu' style='display:none' onclick='saveesu()'>&Auml;nderungen senden</button>\r\n";
                break;
            case MANUFACTURER_LENZ:
                print_lenz_output_modes ();
                HTTP::response += (String) "<BR><button id='savelenz' style='display:none' onclick='savelenz()'>&Auml;nderungen senden</button>\r\n";
                break;
            case MANUFACTURER_TAMS:
                print_tams_output_modes ();
                HTTP::response += (String) "<BR><button id='savetams' style='display:none' onclick='savetams()'>&Auml;nderungen senden</button>\r\n";
                break;
        }
    }

    HTTP::response += (String) "</div>\r\n";   // id='ison'
    HTTP::response += (String) "</div>\r\n";
    HTTP_Common::html_trailer ();
}

void
HTTP_POMOUT::action_setoutputesu (void)
{
    const char *    prefix      = HTTP::parameter ("prefix");                   // "om", "av", "ev", "ao" "br"
    uint_fast16_t   line        = HTTP::parameter_number ("line");              // 0 ... 24
    uint_fast8_t    value       = HTTP::parameter_number ("value");             // 0 ... 1
    uint_fast8_t    col         = 0xFF;

    if (line < ESU_OUTPUT_LINES)
    {
        if (! strcmp (prefix, "om"))
        {
            col = 0;
        }
        else if (! strcmp (prefix, "av"))
        {
            col = 1;
            value = (new_esu_output_mode_values[line][col] & 0x0F) | (value << 4);
        }
        else if (! strcmp (prefix, "ev"))
        {
            col = 1;
            value = (new_esu_output_mode_values[line][col] & 0xF0) | value;
        }
        else if (! strcmp (prefix, "ao"))
        {
            col = 2;
        }
        else if (! strcmp (prefix, "br"))
        {
            col = 3;
        }

        if (col != 0xFF)
        {
            uint8_t new_value           = value;
            uint8_t old_value           = esu_output_mode_values[line][col];
            uint8_t new_value_before    = new_esu_output_mode_values[line][col];

            new_esu_output_mode_values[line][col] = value;

            if (old_value == new_value_before)
            {
                if (new_value != old_value)
                {
                    esu_output_changes++;
                }
            }
            else
            {
                if (new_value == old_value)
                {
                    esu_output_changes--;
                }
            }

            printf ("setoutputesu: line=%u col=%u old_value=%u new_value=%u changes=%u\n", (unsigned int) line, col, old_value, new_value, (unsigned int) esu_output_changes);
        }
        else
        {
            printf ("setoutputesu: illegal prefix: '%s'\n", prefix);
        }

        HTTP::response = std::to_string (esu_output_changes);
    }
    else
    {
        HTTP::response = "setoutputesu: values out of range";
    }
}

void
HTTP_POMOUT::action_saveoutputesu (void)
{
    uint_fast16_t   addr        = HTTP::parameter_number ("addr");

    if (addr > 0)
    {
        uint_fast8_t    cv31_value  = 16;
        uint_fast8_t    cv32_value  = 0;

        if (POM::pom_write_cv_index (addr, cv31_value, cv32_value))
        {
            uint_fast16_t line;

            for (line = 0; line < ESU_OUTPUT_LINES; line++)
            {
                uint_fast16_t col;

                for (col = 0; col < ESU_OUTPUT_COLS; col++)
                {
                    uint_fast8_t    old_value   = esu_output_mode_values[line][col];
                    uint_fast8_t    new_value   = new_esu_output_mode_values[line][col];

                    if (old_value != new_value)
                    {
                        uint_fast16_t cv = 8 * line + col + 259;

                        if (POM::pom_write_cv (addr, cv, new_value, POM_WRITE_COMPARE_AFTER_WRITE))
                        {
                            esu_output_mode_values[line][col] = new_value;

                            if (esu_output_changes)
                            {
                                esu_output_changes--;
                            }

                            printf ("saveoutputesu: write successful, cv=%u value=%u changes=%u\n", (unsigned int) cv, new_value, (unsigned int) esu_output_changes);
                        }
                        else
                        {
                            printf ("saveoutputesu: write error, addr=%u, cv=%u, value=%u\n", (unsigned int) addr, (unsigned int) cv, new_value);
                        }
                    }
                }
            }
        }
        else
        {
            printf ("saveoutputesu: CV index (CV31=%u, CV32=%u) kann nicht gespeichert werden.\n", cv31_value, cv32_value);
        }
    }
    else
    {
        printf ("saveoutputesu: addr is 0\n");
    }

    HTTP::response = std::to_string (esu_output_changes);
}
