/*------------------------------------------------------------------------------------------------------------------------
 * http-pommap.cc - HTTP POM function mapping routines
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

#include "pom.h"
#include "http.h"
#include "http-common.h"
#include "http-pommap.h"

#define MAX_ESU_MAP_LINES       72
#define MAX_ESU_CONDITION_COLS  10
#define MAX_ESU_OUTPUT_COLS     3
#define MAX_ESU_LOGIC_COLS      2
#define MAX_ESU_COLS            (MAX_ESU_OUTPUT_COLS + MAX_ESU_LOGIC_COLS)

static uint8_t                  esu_condition_map[MAX_ESU_MAP_LINES][MAX_ESU_CONDITION_COLS];
static uint8_t                  new_esu_condition_map[MAX_ESU_MAP_LINES][MAX_ESU_CONDITION_COLS];
static uint8_t                  esu_output_map[MAX_ESU_MAP_LINES][MAX_ESU_COLS];
static uint8_t                  new_esu_output_map[MAX_ESU_MAP_LINES][MAX_ESU_COLS];
static uint_fast16_t            esu_condition_mapping_changes;
static uint_fast16_t            esu_output_mapping_changes;

#define MAX_LENZ_MAP_LINES1     15          //  33 ..  47
#define MAX_LENZ_MAP_LINES2     16          // 129 .. 144
#define MAX_LENZ_MAP_LINES      (MAX_LENZ_MAP_LINES1 + MAX_LENZ_MAP_LINES2)

static uint8_t                  lenz_cv_map[MAX_LENZ_MAP_LINES];
static uint8_t                  lenz_output_map[MAX_LENZ_MAP_LINES];
static uint8_t                  new_lenz_output_map[MAX_LENZ_MAP_LINES];
static const char *             lenz_cv_desc[MAX_LENZ_MAP_LINES] =
{
    "F0 vorwärts",
    "F0 r&uuml;ckwärts",
    "F1 vorwärts",
    "F1 r&uuml;ckwärts",
    "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", "F11", "F12",
    "F13", "F14", "F15", "F16", "F17", "F18", "F19", "F20",
    "F21", "F22", "F23", "F24", "F25", "F26", "F27", "F28"
};
static uint_fast16_t            lenz_mapping_changes;

#define MAX_TAMS_MAP_LINES      (29 * 2)

static uint8_t                  tams_output_map[MAX_TAMS_MAP_LINES][4];
static uint8_t                  new_tams_output_map[MAX_TAMS_MAP_LINES][4];
static uint_fast16_t            tams_mapping_changes;

static bool
get_esu_conditions (uint_fast16_t addr, uint_fast16_t lines)
{
    uint16_t        cv;
    uint16_t        cv32;
    uint16_t        last_cv32 = 0xFFFF;
    uint16_t        line;
    uint16_t        bytepos;
    uint_fast8_t    value;
    String          color;
    bool            rtc = false;

    memset (esu_condition_map, 0, MAX_ESU_MAP_LINES * MAX_ESU_CONDITION_COLS);

    for (line = 0; line < lines; line++)
    {
        HTTP::response += (String) "<script>document.getElementById('progress').value = " + std::to_string(line + 1) + ";</script>";
        HTTP::flush ();

        cv32 = (line / 16) + 3;

        if (last_cv32 != cv32)
        {
            if (! POM::pom_write_cv (addr, 32, cv32, POM_WRITE_COMPARE_BEFORE_AND_AFTER_WRITE))
            {
                break;
            }
            last_cv32 = cv32;
        }

        for (bytepos = 0; bytepos < MAX_ESU_CONDITION_COLS; bytepos++)
        {
            cv = line * 16 + bytepos + 257;

            if (! POM::pom_read_cv (&value, addr, cv))
            {
                fprintf (stderr, "read error: cv=%u\n", cv);
                return false;
            }

            esu_condition_map[line][bytepos] = value;
        }       
    }

    if (line == lines)
    {
        memcpy (new_esu_condition_map, esu_condition_map, MAX_ESU_MAP_LINES * MAX_ESU_CONDITION_COLS);
        esu_condition_mapping_changes = 0;
        rtc = true;
    }

    return rtc;
}

static bool
get_esu_outputs (uint_fast16_t addr, uint_fast16_t lines)
{
    uint16_t        cv;
    uint16_t        cv32;
    uint16_t        last_cv32 = 0xFFFF;
    uint16_t        line;
    uint16_t        bytepos;
    uint_fast8_t    value;
    String          color;
    bool            rtc = false;

    memset (esu_output_map, 0, MAX_ESU_MAP_LINES * MAX_ESU_COLS);

    for (line = 0; line < lines; line++)
    {
        HTTP::response += (String) "<script>document.getElementById('progress').value = " + std::to_string(lines + line + 1) + ";</script>";
        HTTP::flush ();

        cv32 = (line / 16) + 8;

        if (last_cv32 != cv32)
        {
            if (! POM::pom_write_cv (addr, 32, cv32, POM_WRITE_COMPARE_BEFORE_AND_AFTER_WRITE))
            {
                break;
            }
            last_cv32 = cv32;
        }

        for (bytepos = 0; bytepos < MAX_ESU_COLS; bytepos++)
        {
            cv = line * 16 + bytepos + 257;

            if (! POM::pom_read_cv (&value, addr, cv))
            {
                fprintf (stderr, "read error: cv=%u\n", cv);
                return false;
            }

            esu_output_map[line][bytepos] = value;

            // printf ("%3d ", value);
        }       
    }

    if (line == lines)
    {
        memcpy (new_esu_output_map, esu_output_map, MAX_ESU_MAP_LINES * MAX_ESU_COLS);
        esu_output_mapping_changes = 0;
        rtc = true;
    }

    return rtc;
}

static void
print_esu_mapping (uint_fast16_t lines)
{
    char            buf[32];
    uint16_t        line;
    uint16_t        bytepos;
    uint16_t        bitpos;
    uint_fast8_t    value;
    uint_fast8_t    idx;
    uint_fast8_t    condition;
    String          id;
    String          color;
    String          linebg;

    HTTP::response += (String) "<div class='div'>";

    HTTP::response += (String) "<div style='display:inline-block; white-space:nowrap'>";
    HTTP::response += (String) "<span class='span1'>Bedingungen</span><span class='espan'></span>";
    HTTP::response += (String) "<span class='span2'>Ausg&auml;nge</span><span class='espan'></span>";
    HTTP::response += (String) "<span class='span3'>Logik</span></div><BR>";

    HTTP::response += (String) "<div style='display:inline-block; white-space:nowrap'><span class='span'><BR>Z</span><span class='span'>Fa<BR>hrt</span><span class='span'>Vo<BR>rw</span>";

    for (idx = 0; idx < 32; idx++)
    {
        sprintf (buf, "F<BR>%u", idx);
        HTTP::response += (String) "<span class='span'>" + buf + "</span>";
    }

    HTTP::response += (String) "<span class='span'><BR>RS</span><span class='span'><BR>-</span><span class='span'>S<BR>1</span>";
    HTTP::response += (String) "<span class='span'>S<BR>2</span><span class='span'>S<BR>3</span><span class='span'>S<BR>4</span>";
    HTTP::response += (String) "<span class='espan'>&nbsp;</span><span class='span'><BR>Z</span><span class='span'>L<BR>v</span><span class='span'>L<BR>h</span>";

    for (idx = 1; idx <= 18; idx++)
    {
        sprintf (buf, "AX<BR>%u", idx);
        HTTP::response += (String) "<span class='span'>" + buf + "</span>";
    }

    HTTP::response += (String) "<span class='span'>L<BR>v</span><span class='span'>L<BR>h</span><span class='span'>AX<BR>1</span><span class='span'>AX<BR>2</span>";
    HTTP::response += (String) "<span class='espan'>&nbsp;</span><span class='span'><BR>Z</span><span class='span'>Al<BR>La</span><span class='span'>Ra<BR>ng</span>";
    HTTP::response += (String) "<span class='span'>Br<BR>1</span><span class='span'>Br<BR>2</span><span class='span'>Br<BR>3</span><span class='span'>S<BR>L</span>";
    HTTP::response += (String) "<span class='span'>Ku<BR>Wa</span><span class='span'>D<BR>H</span><span class='span'>F<BR>B</span><span class='span'>Di<BR>m</span>";
    HTTP::response += (String) "<span class='span'>G<BR>C</span><span class='span'>AB<BR>V</span><span class='span'><BR>--</span>";
    HTTP::response += (String) "<span class='span'><BR>--</span><span class='span'><BR>--</span><span class='span'><BR>--</span>";
    HTTP::response += (String) "</div>\r\n";
    HTTP::flush ();

    for (line = 0; line < lines; line++)
    {
        if (line % 2)
        {
            linebg = "white";
        }
        else
        {
            linebg = "lightgray";
        }

        HTTP::response += (String) "<div style='display:inline-block; white-space:nowrap; background-color:" + linebg + "'><span class='span'>" + std::to_string(line + 1) + "</span>";

        for (bytepos = 0; bytepos < MAX_ESU_CONDITION_COLS; bytepos++)
        {
            value = esu_condition_map[line][bytepos];

            for (bitpos = 0; bitpos < 8; bitpos++)
            {
                condition = 0;

                if (value & (1 << bitpos))
                {
                    condition += 1;
                }

                bitpos++;

                if (value & (1 << bitpos))
                {
                    condition += 2;
                }

                switch (condition)
                {
                    case 0:     color = linebg;     break;
                    case 1:     color = "green";    break;
                    case 2:     color = "red";      break;
                    default:    color = "magenta";  break;  // 3 is invalid
                }

                id = (String) "coid" + std::to_string(line) + "_" + std::to_string(bytepos) + "_" + std::to_string(bitpos - 1);

                HTTP::response   += (String) "<span id='" + id + "' class='span' style='background-color:" + color + "' "
                                + "onclick=\"cond('" + id + "'," + std::to_string(line) + "," + std::to_string(bytepos) + "," + std::to_string(bitpos - 1) + ")\">&nbsp;</span>";
            }
        }       

        HTTP::response += (String) "<span class='espan'>&nbsp;</span>";

        for (bytepos = 0; bytepos < MAX_ESU_COLS; bytepos++)
        {
            if (bytepos == 0)
            {
                HTTP::response += (String) "<span class='span'>" + std::to_string(line + 1) + "</span>";
            }
            else if (bytepos == MAX_ESU_OUTPUT_COLS)
            {
                HTTP::response += (String) "<span class='espan'>&nbsp;</span>";
                HTTP::response += (String) "<span class='span'>" + std::to_string(line + 1) + "</span>";
            }

            value = esu_output_map[line][bytepos];

            for (bitpos = 0; bitpos < 8; bitpos++)
            {
                condition = 0;

                if (value & (1 << bitpos))
                {
                    condition += 1;
                }

                switch (condition)
                {
                    case 0:     color = linebg;     break;
                    case 1:     color = "green";    break;
                }

                id = (String) "ouid" + std::to_string(line) + "_" + std::to_string(bytepos) + "_" + std::to_string(bitpos);

                HTTP::response   += (String) "<span id='" + id + "' class='span' style='background-color:" + color + "' "
                                + "onclick=\"outp('" + id + "'," + std::to_string(line) + "," + std::to_string(bytepos) + "," + std::to_string(bitpos) + ")\">&nbsp;</span>";

            }
        }       

        HTTP::response += "</div>\r\n";
        HTTP::flush ();

    }

    HTTP::response += "<div style='display:inline-block; margin-top:10px; white-space:nowrap'>";

    HTTP::response += "<span class='span1a' style='vertical-align:top; border:0px'>";
    HTTP::response += "<table style='border:1px lightgray;'>";
    HTTP::response += "<tr><td><span class='span' style='background-color:green'>&nbsp;</span></td><td>Funktion ist an</td></tr>\r\n";
    HTTP::response += "<tr><td><span class='span' style='background-color:red'>&nbsp;</span></td><td>Funktion ist aus</td></tr>\r\n";
    HTTP::response += "<tr><td>Z</td><td>Zeile</td></tr>\r\n";
    HTTP::response += "<tr><td>Fahrt</td><td>Fahrt</td></tr>\r\n";
    HTTP::response += "<tr><td>Vorw</td><td>Vorw&auml;rts</td></tr>\r\n";
    HTTP::response += "<tr><td>F0-F31</td><td>Funktion F0-F31</td></tr>\r\n";
    HTTP::response += "<tr><td>RS</td><td>Radsensor</td></tr>\r\n";
    HTTP::response += "<tr><td>-</td><td>Reserviert</td></tr>\r\n";
    HTTP::response += "<tr><td>S1-S4</td><td>Sensor 1-4</td></tr>\r\n";
    HTTP::response += "</table>\r\n";
    HTTP::response += "</span><span class='espan'></span>";

    HTTP::response += "<span class='span2a' style='vertical-align:top; border:0px'>";
    HTTP::response += "<table style='border:1px lightgray;'>";
    HTTP::response += "<tr><td><span class='span' style='background-color:green'>&nbsp;</span></td><td>Ausgang ein</td></tr>\r\n";
    HTTP::response += "<tr><td>Z</td><td>Zeile</td></tr>\r\n";
    HTTP::response += "<tr><td>Lv</td><td>Licht vorne (Konf. 1)</td></tr>\r\n";
    HTTP::response += "<tr><td>Lh</td><td>Licht hinten (Konf. 1)</td></tr>\r\n";
    HTTP::response += "<tr><td>AX1-AX2</td><td>Ausg&auml;nge AUX1-AUX2 (Konf. 1)</td></tr>\r\n";
    HTTP::response += "<tr><td>AX3-AX18</td><td>Ausg&auml;nge AUX1-AUX18</td></tr>\r\n";
    HTTP::response += "<tr><td>Lv</td><td>Licht vorne (Konf. 2)</td></tr>\r\n";
    HTTP::response += "<tr><td>Lh</td><td>Licht hinten (Konf. 2)</td></tr>\r\n";
    HTTP::response += "<tr><td>AX1-AX2</td><td>Ausg&auml;nge AUX1-AUX2 (Konf. 2)</td></tr>\r\n";
    HTTP::response += "</table>\r\n";
    HTTP::response += "</span><span class='espan'></span>";

    HTTP::response += "<span class='span3a' style='vertical-align:top; border:0px'>";
    HTTP::response += "<span>";
    HTTP::response += "<table style='float:left;border:1px lightgray;'>";
    HTTP::response += "<tr><td><span class='span' style='background-color:green'>&nbsp;</span></td><td>Logik ein</td></tr>\r\n";
    HTTP::response += "<tr><td>Z</td><td>Zeile</td></tr>\r\n";
    HTTP::response += "<tr><td>Al La</td><td>Alternative Last</td></tr>\r\n";
    HTTP::response += "<tr><td>Rang</td><td>Rangierbetrieb</td></tr>\r\n";
    HTTP::response += "<tr><td>Br1-Br3</td><td>Bremsfunktion 1-3</td></tr>\r\n";
    HTTP::response += "<tr><td>SL</td><td>Schwere Last</td></tr>\r\n";
    HTTP::response += "<tr><td>KuWa</td><td>Kupplungswalzer</td></tr>\r\n";
    HTTP::response += "</table>\r\n";
    HTTP::response += "<table style='margin-left:40px;float:left;border:1px lightgray;'>\r\n";
    HTTP::response += "<tr><td>DH</td><td>Drivehold</td></tr>\r\n";
    HTTP::response += "<tr><td>FB</td><td>Feuerb&uuml;chse</td></tr>\r\n";
    HTTP::response += "<tr><td>Dim</td><td>Dimmer</td></tr>\r\n";
    HTTP::response += "<tr><td>GC</td><td>Grade-Crossing</td></tr>\r\n";
    HTTP::response += "<tr><td>ABV</td><td>ABV aus</td></tr>\r\n";
    HTTP::response += "<tr><td>-</td><td>Reserviert</td></tr>\r\n";
    HTTP::response += "</table>\r\n";
    HTTP::response += "</span>";
    HTTP::response += "</span></div>";

    HTTP::response += (String) "</div>\r\n";
    HTTP::flush ();
}

static bool
get_esu_mapping (uint_fast16_t addr, uint_fast16_t lines)
{
    bool rtc;

    HTTP::response += (String) "<progress id='progress' max='" + std::to_string(2 * lines) + "' value='0'></progress>\r\n";

    if (get_esu_conditions (addr, lines) && get_esu_outputs (addr, lines))
    {
        rtc = true;
    }
    else
    {
        rtc = false;
    }

    HTTP::response += (String) "<script>document.getElementById('progress').style.display = 'none';</script>";
    HTTP::flush ();

    return rtc;
}

static bool
get_lenz_mapping (uint_fast16_t addr)
{
    uint_fast8_t    line;
    uint_fast8_t    start;
    uint_fast16_t   cv;
    uint_fast8_t    value;
    bool            rtc = false;

    memset (lenz_output_map, 0, MAX_LENZ_MAP_LINES);

    lenz_cv_map[0] = 33;    // F0f
    lenz_cv_map[1] = 34;    // F0r
    lenz_cv_map[2] = 35;    // F1f
    lenz_cv_map[3] = 47;    // F1r

    start = 33;

    for (line = 4; line < MAX_LENZ_MAP_LINES1; line++)
    {
        lenz_cv_map[line] = (line - 1) + start;
    }

    start = 129;

    for (line = MAX_LENZ_MAP_LINES1; line < MAX_LENZ_MAP_LINES; line++)
    {
        lenz_cv_map[line] = (line - MAX_LENZ_MAP_LINES1) + start;
    }

    HTTP::response += (String) "<progress id='progress' max='" + std::to_string(MAX_LENZ_MAP_LINES) + "' value='0'></progress>\r\n";

    for (line = 0; line < MAX_LENZ_MAP_LINES; line++)
    {
        HTTP::response += (String) "<script>document.getElementById('progress').value = " + std::to_string(line + 1) + ";</script>";
        HTTP::flush ();

        cv = lenz_cv_map[line];

        if (! POM::pom_read_cv (&value, addr, cv))
        {
            fprintf (stderr, "read error: cv=%u\n", cv);
            break;
        }

        lenz_output_map[line] = value;
    }

    HTTP::response += (String) "<script>document.getElementById('progress').style.display = 'none';</script>";
    HTTP::flush ();

    if (line == MAX_LENZ_MAP_LINES)
    {
        memcpy (new_lenz_output_map, lenz_output_map, MAX_LENZ_MAP_LINES);
        lenz_mapping_changes = 0;
        rtc = true;
    }

    return rtc;
}

static void
print_lenz_mapping (uint_fast16_t outputs)
{
    uint_fast8_t    line;
    uint_fast16_t   cv;
    uint_fast8_t    value;
    uint16_t        bitpos;
    uint_fast8_t    condition;
    String          id;
    String          color;
    String          linebg;

    HTTP::response += (String) "<div class='div'><div style='display:inline-block; white-space:nowrap'><span class='span'>CV</span>";
    HTTP::response += (String) "<span class='span2'>Beschreibung</span>";
    HTTP::response += (String) "<span class='span'>A</span><span class='span'>B</span><span class='span'>C</span><span class='span'>D</span>";

    if (outputs == 8)
    {
        HTTP::response += (String) "<span class='span'>E</span><span class='span'>F</span><span class='span'>G</span><span class='span'>H</span>";
    }

    HTTP::response += (String) "</div>\r\n";

    HTTP::flush ();

    for (line = 0; line < MAX_LENZ_MAP_LINES; line++)
    {
        cv      = lenz_cv_map[line];
        value   = lenz_output_map[line];

        if (line % 2)
        {
            linebg = "white";
        }
        else
        {
            linebg = "lightgray";
        }

        HTTP::response += (String) "<div style='white-space:nowrap; background-color:" + linebg + "'><span class='span'>" + std::to_string(cv) + "</span>";
        HTTP::response += (String) "<span class='span2'>" + lenz_cv_desc[line] + "</span>";

        for (bitpos = 0; bitpos < outputs; bitpos++)
        {
            condition = 0;

            if (value & (1 << bitpos))
            {
                condition = 1;
            }

            switch (condition)
            {
                case 0:     color = linebg;     break;
                case 1:     color = "green";    break;
            }

            id = (String) "ouid" + std::to_string(line) + "_" + std::to_string(bitpos);

            HTTP::response   += (String) "<span id='" + id + "' class='span' style='background-color:" + color + "' "
                            + "onclick=\"outpl('" + id + "'," + std::to_string(line) + "," + std::to_string(bitpos) + ")\">&nbsp;</span>";
        }

        HTTP::response += "</div>\r\n";
        HTTP::flush ();
    }

    HTTP::response += "</div></div>\r\n";
    HTTP::flush ();
}

static bool
get_tams_mapping (uint_fast16_t addr, uint_fast16_t lines)
{
    uint_fast8_t    line;
    uint_fast8_t    col;
    uint_fast16_t   cv;
    uint_fast8_t    cv31_value = 0;
    uint_fast8_t    cv32_value = 255;
    uint_fast8_t    value;
    bool            rtc = false;

    if (POM::pom_read_cv (&value, addr, 96))                     // see RCN-225 CV96
    {
        if (value == 2)                                     // TAMS documentation says cv32=42, but CV96 and RCN-225 says cv32=40
        {                                                   // both cv32 values get same mapping values for TAMS decoder
            cv31_value  = 0;
            cv32_value  = 40;
        }
        else if (value == 4)
        {
            cv31_value  = 0;
            cv32_value  = 42;
        }

        if (cv32_value != 255)
        {
            if (POM::pom_write_cv_index (addr, cv31_value, cv32_value))
            {
                HTTP::response += (String) "<progress id='progress' max='" + std::to_string(lines) + "' value='0'></progress>\r\n";

                for (line = 0; line < lines; line++)
                {
                    HTTP::response += (String) "<script>document.getElementById('progress').value = " + std::to_string(line + 1) + ";</script>";
                    HTTP::flush ();

                    cv = (line * 4) + 257;

                    for (col = 0; col < 4; col++)
                    {
                        if (col != 1)       // skip col #2
                        {
                            if (! POM::pom_read_cv (&value, addr, cv + col))
                            {
                                fprintf (stderr, "read error: cv=%u\n", cv + col);
                                break;
                            }

                            tams_output_map[line][col] = value;
                        }
                    }

                    if (col != 4)
                    {
                        break;
                    }
                }

                HTTP::response += (String) "<script>document.getElementById('progress').style.display = 'none';</script>";
                HTTP::flush ();

                if (line == lines)
                {
                    memcpy (new_tams_output_map, tams_output_map, MAX_TAMS_MAP_LINES * 4);
                    tams_mapping_changes = 0;
                    rtc = true;
                }
            }
            else
            {
                HTTP::response += (String) "CV index (CV31=0, CV32=42) kann nicht gespeichert werden.";
            }
        }
        else
        {
            HTTP::response += (String) "Nicht unterst&uuml;tzte Mapping Methode: CV96=" + std::to_string(value);
        }
    }
    else
    {
        HTTP::response += (String) "CV 96 kann nicht gelesen werden.";
    }

    return rtc;
}

static void
print_tams_mapping (uint_fast16_t lines)
{
    uint_fast8_t    line;
    uint_fast8_t    col;
    uint_fast16_t   cv;
    uint_fast8_t    value;
    uint16_t        bitpos;
    String          id;
    String          color;
    String          linebg;
    uint_fast8_t    idx;

    HTTP::response += (String) "<div class='div'><div style='display:inline-block; white-space:nowrap'><span class='span'>CV</span>";
    HTTP::response += (String) "<span class='span2'>Funktion</span>";
    HTTP::response += (String) "<span class='span'>F0f</span><span class='span'>F0r</span>";

    for (idx = 1; idx <= 6; idx++)
    {
        HTTP::response += (String) "<span class='span'>AUX" + std::to_string (idx) + "</span>";
    }

    HTTP::response += (String) "<span class='espan'>&nbsp;</span>";
    HTTP::response += (String) "<span class='span'>RB</span>";
    HTTP::response += (String) "<span class='span'>AB</span>";
    HTTP::response += (String) "<span class='espan'>&nbsp;</span>";
    HTTP::response += (String) "<span class='span2'>Aus</span>";
    HTTP::response += (String) "</div>\r\n";

    HTTP::flush ();

    for (line = 0; line < lines; line++)
    {
        cv      = (line * 4) + 257;

        if (line % 2)
        {
            linebg = "white";
        }
        else
        {
            linebg = "lightgray";
        }

        HTTP::response += (String) "<div style='white-space:nowrap; background-color:" + linebg + "'><span class='span'>" + std::to_string(cv) + "</span>";
        HTTP::response += (String) "<span class='span2'>";

        HTTP::response += (String) "F" + std::to_string(line / 2) + ((line % 2) ? " r" : " f");
        HTTP::response += (String) "</span>";

        for (col = 0; col < 4; col++)
        {
            if (col == 0)
            {
                value = tams_output_map[line][col];

                for (bitpos = 0; bitpos < 8; bitpos++)
                {
                    color = (value & (1 << bitpos)) ? "green" : linebg;
                    id = (String) "col0_" + std::to_string(line) + "_" + std::to_string(bitpos);
                    HTTP::response   += (String) "<span id='" + id + "' class='span' style='background-color:" + color + "' "
                                    + "onclick=\"outt('" + id + "'," + std::to_string(line) + ",0," + std::to_string(bitpos) + ")\">&nbsp;</span>";
                }
            }
            else if (col == 1) // no value here
            {
                HTTP::response   += (String) "<span class='espan'>&nbsp;</span>";
            }
            else if (col == 2)  // skip col == 1
            {
                value = tams_output_map[line][col];

                for (bitpos = 2; bitpos < 4; bitpos++)
                {
                    color = (value & (1 << bitpos)) ? "green" : linebg;
                    id = (String) "col2_" + std::to_string(line) + "_" + std::to_string(bitpos);
                    HTTP::response   += (String) "<span id='" + id + "' class='span' style='background-color:" + color + "' "
                                    + "onclick=\"outt('" + id + "'," + std::to_string(line) + ",2," + std::to_string(bitpos) + ")\">&nbsp;</span>";
                }
            }
            else if (col == 3)
            {
                uint_fast8_t    fidx;

                value = tams_output_map[line][col];

                HTTP::response += (String) "<span class='espan'>&nbsp;</span><span class='span2'>";
                id = (String) "col3_" + std::to_string(line);
                HTTP::response += (String) "<select id='" + id + "' onchange=\"offt('" + id + "'," + std::to_string(line) + ",3);\" style='width:100%;height:100%'>\r\n";

                if (value == 0xFF)
                {
                    HTTP::response += (String) "<option value='255' selected>---</option>";
                }
                else
                {
                    HTTP::response += (String) "<option value='255'>---</option>";
                }

                for (fidx = 0; fidx <= 28; fidx++)
                {
                    String s = std::to_string(fidx);

                    if (value == fidx)
                    {
                        HTTP::response += (String) "<option value='" + s + "' selected>F" + s + "</option>";
                    }
                    else
                    {
                        HTTP::response += (String) "<option value='" + s + "'>F" + s + "</option>";
                    }
                }
                HTTP::response += (String) "</select></span>";
            }
        }

        HTTP::response += "</div>\r\n";
        HTTP::flush ();
    }

    HTTP::response += "</div></div>\r\n";
    HTTP::flush ();
}

/*----------------------------------------------------------------------------------------------------------------------------------------
 * handle_pommap ()
 *----------------------------------------------------------------------------------------------------------------------------------------
 */
void
HTTP_POMMAP::handle_pommap (void)
{
    String          title   = "Mapping (POM)";
    String          url     = "/pommap";
    const char *    action  = HTTP::parameter ("action");
    const char *    saddr   = "";
    uint_fast16_t   addr    = 0;
    uint_fast16_t   lines   = 0;
    uint_fast16_t   outputs = 0;
    uint_fast8_t    manu    = 0;
    bool            get_mapping_successful = false;

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
    else if (strcmp (action, "getmap") == 0)
    {
        manu    = HTTP::parameter_number ("manu");
        saddr   = HTTP::parameter ("addr");
        addr    = atoi (saddr);

        if (manu == MANUFACTURER_ESU)
        {
            lines   = HTTP::parameter_number ("lines");
        }
        else if (manu == MANUFACTURER_LENZ)
        {
            outputs = HTTP::parameter_number ("outputs");
        }
        else // if (manu == MANUFACTURER_TAMS)
        {
            lines   = HTTP::parameter_number ("lines");
        }

        if (addr > 0)
        {
            switch (manu)
            {
                case 0:
                    HTTP::response += (String) "<font color='red'><B>Unbekannter Hersteller.</B></font><BR>\r\n";
                    break;
                case MANUFACTURER_ESU:
                    get_mapping_successful = get_esu_mapping (addr, lines);
                    break;
                case MANUFACTURER_LENZ:
                    get_mapping_successful = get_lenz_mapping (addr);
                    break;
                case MANUFACTURER_TAMS:
                    get_mapping_successful = get_tams_mapping (addr, lines);
                    break;
                default:
                    HTTP::response += (String) "<font color='red'><B>Keine Mapping-Infos f&uuml;r Hersteller '" + HTTP_Common::manufacturers[manu] + "' vorhanden.</B></font><BR>\r\n";
                    break;
            }

            if (! get_mapping_successful)
            {
                HTTP::response += (String) "<font color='red'><B>Lesefehler</B></font><BR>\r\n";
            }
        }
        else
        {
            HTTP::response += (String) "<font color='red'><B>Ung&uuml;ltige Adresse</B></font><BR>\r\n";
        }
    }

    HTTP::response += (String) "<div id='isoff' style='display:none'><font color='red'>Booster ist abgeschaltet. Bitte einschalten.</font></div>\r\n";
    HTTP::response += (String) "<div id='ison' style='display:none'>\r\n";

    if (manu)
    {
        HTTP::response += (String) "<BR><div style='border:1px lightgray solid;margin-left:10px;display:inline-block;'>";
        HTTP::response += (String) "<div style='padding:4px;text-align:center;background-color:#E0E0E0;'><B>Mapping</B></div>\r\n";
        HTTP::response += (String) "<form method='GET' action='" + url + "'>\r\n";
        HTTP::response += (String) "<div style='padding:2px;'><span style='width:120px;display:inline-block;'>Adresse: " + saddr + "</span>";
        HTTP::response += (String) "<span style='width:110px;display:inline-block;'><a href='" + url + "'>Andere Adresse</a></span></div>\r\n";

        if (manu == MANUFACTURER_ESU)
        {
            HTTP::response += (String) "<div style='padding:2px;'><span style='width:120px;display:inline-block;'>Anzahl Zeilen</span><span style='width:110px;display:inline-block;'><select name='lines' style='width:100%'>";
            HTTP::response += (String) "<option value='16'>16</option><option value='32'>32</option><option value='72'>72</option></select></span></div>\r\n";
        }
        else if (manu == MANUFACTURER_LENZ)
        {
            HTTP::response += (String) "<div style='padding:2px;'><span style='width:120px;display:inline-block;'>Anzahl Ausg&auml;nge</span><span style='width:110px;display:inline-block;'><select name='outputs' style='width:100%'><option value='4'>4</option><option value='8'>8</option></select></span></div>\r\n";
        }
        else if (manu == MANUFACTURER_TAMS)
        {
            HTTP::response += (String) "<div style='padding:2px;'><span style='width:120px;display:inline-block;'>Bereich</span><span style='width:110px;display:inline-block;'><select name='lines' style='width:100%'>";
            HTTP::response += (String) "<option value='18'>F0 - F8</option><option value='38'>F0 - F18</option><option value='58'>F0 - F28</option></select></span></div>\r\n";
        }

        HTTP::response += (String) "<div style='padding:2px;'><span style='width:100%'>";
        HTTP::response += (String) "<input type='hidden' name='action' value='getmap'>";
        HTTP::response += (String) "<input type='hidden' name='manu' value='" + std::to_string(manu) + "'>";
        HTTP::response += (String) "<input type='hidden' name='addr' value='" + saddr + "'>";
        HTTP::response += (String) "<input type='submit' style='width:100%;padding:2px;' value='Mapping lesen'></span></div>\r\n";
        HTTP::response += (String) "</form></div>\r\n";
    }
    else
    {   
        HTTP::response += (String) "<form method='GET' action='" + url + "'>\r\n";
        HTTP::response += (String) "<BR><table style='border:1px lightgray solid;margin-left:10px;'><tr bgcolor='#E0E0E0'><th colspan='2'>Mapping</th></tr>\r\n";
        HTTP::response += (String) "<tr><td>Adresse</td><td><input type='text' style='width:120px;' maxlength='4' name='addr' value='" + saddr + "'></td></tr>\r\n";
        HTTP::response += (String) "<tr><td colspan='2' width='100%'><input type='hidden' name='action' value='getmanu'>\r\n";
        HTTP::response += (String) "<input type='submit' style='width:100%' value='Hersteller ermitteln'></td></tr>\r\n";
        HTTP::response += (String) "</table>\r\n";
        HTTP::response += (String) "</form>\r\n";
    }

    HTTP::response += (String) "<P>\r\n";
    HTTP::flush ();

    HTTP::response += "<style>";

    if (manu == MANUFACTURER_ESU)
    {
        HTTP::response += ".span1 { display:inline-block; border:1px solid black; min-width:736px;max-width:736px;text-align:center; }";
        HTTP::response += ".span2 { display:inline-block; border:1px solid black; min-width:448px;max-width:448px;text-align:center; }";
        HTTP::response += ".span3 { display:inline-block; border:1px solid black; min-width:304px;max-width:304px;text-align:center; }";
        HTTP::response += ".span1a { display:inline-block; min-width:736px;max-width:736px }";
        HTTP::response += ".span2a { display:inline-block; min-width:448px;max-width:448px; }";
        HTTP::response += ".span3a { display:inline-block; min-width:304px;max-width:304px; }";
        HTTP::response += ".span { display:inline-block; border:1px solid black; min-width:16px;max-width:16px;text-align:center; }";
        HTTP::response += ".espan { display:inline-block; min-width: 16px; max-width:16px; text-align:center; background-color:white; }";
        HTTP::response += ".div { font-family:Helvetica,Arial; font-size:10px; cursor:pointer; }";
    }
    else if (manu == MANUFACTURER_LENZ)
    {
        HTTP::response += ".span { display:inline-block; border:1px solid black; min-width:32px;max-width:32px;text-align:center; }";
        HTTP::response += ".span2 { display:inline-block; border:1px solid black; min-width:120px;max-width:120px;text-align:center; }";
        HTTP::response += ".espan { display:inline-block; min-width: 32px; max-width:32px; text-align:center; background-color:white; }";
        HTTP::response += ".div { display:inline-block; cursor:pointer; }";
    }
    else // if (manu == MANUFACTURER_TAMS)
    {
        HTTP::response += ".span { display:inline-block; border:1px solid black; min-width:36px;max-width:36px; line-height:1.5; text-align:center; }";
        HTTP::response += ".span2 { display:inline-block; border:1px solid black; min-width:60px;max-width:60px; line-height:1.5; text-align:center; }";
        HTTP::response += ".espan { display:inline-block; min-width: 28px; max-width:28px; text-align:center; line-height:1.5; background-color:white; }";
        HTTP::response += ".div { display:inline-block; cursor:pointer; }";
    }

    HTTP::response += "</style>\r\n";
    HTTP::response += (String) "<script>";

    if (manu == MANUFACTURER_ESU)
    {
        HTTP::response += (String) "function cond(id,line,bytepos,bitpos) {\r\n";
        HTTP::response += (String) "  var condition;\r\n";
        HTTP::response += (String) "  var color = document.getElementById(id).style.backgroundColor;\r\n";
        HTTP::response += (String) "  switch (color) {\r\n";
        HTTP::response += (String) "    case 'green': color = 'red';     condition = 2; break;\r\n";
        HTTP::response += (String) "    case 'red':   color = 'inherit'; condition = 0; break;\r\n";
        HTTP::response += (String) "    default:      color = 'green';   condition = 1;break;\r\n";
        HTTP::response += (String) "  }\r\n";
        HTTP::response += (String) "  document.getElementById(id).style.backgroundColor = color;\r\n";
        HTTP::response += (String) "  var http = new XMLHttpRequest(); http.open ('GET', '/action?action=setcondmapesu&line=' + line + '&bytepos=' + bytepos + '&bitpos=' + bitpos + '&condition=' + condition);";
        HTTP::response += (String) "  http.addEventListener('load',";
        HTTP::response += (String) "    function(event) { var text = http.responseText; if (http.status >= 200 && http.status < 300) { ";
        HTTP::response += (String) "      if (text == '0') { document.getElementById('saveesu').style.display = 'none'; } else { document.getElementById('saveesu').style.display = ''; }";
        HTTP::response += (String) "  }});";
        HTTP::response += (String) "  http.send (null);\r\n";
        HTTP::response += (String) "}\r\n";
        HTTP::response += (String) "function outp(id,line,bytepos,bitpos) {\r\n";
        HTTP::response += (String) "  var value;\r\n";
        HTTP::response += (String) "  var color = document.getElementById(id).style.backgroundColor;\r\n";
        HTTP::response += (String) "  switch (color) {\r\n";
        HTTP::response += (String) "    case 'green': color = 'inherit'; value = 0; break;\r\n";
        HTTP::response += (String) "    default:      color = 'green';   value = 1;break;\r\n";
        HTTP::response += (String) "  }\r\n";
        HTTP::response += (String) "  document.getElementById(id).style.backgroundColor = color;\r\n";
        HTTP::response += (String) "  var http = new XMLHttpRequest(); http.open ('GET', '/action?action=setoutputmapesu&line=' + line + '&bytepos=' + bytepos + '&bitpos=' + bitpos + '&value=' + value);";
        HTTP::response += (String) "  http.addEventListener('load',";
        HTTP::response += (String) "    function(event) { var text = http.responseText; if (http.status >= 200 && http.status < 300) { ";
        HTTP::response += (String) "      if (text == '0') { document.getElementById('saveesu').style.display = 'none'; } else { document.getElementById('saveesu').style.display = ''; }";
        HTTP::response += (String) "  }});";
        HTTP::response += (String) "  http.send (null);\r\n";
        HTTP::response += (String) "}\r\n";
        HTTP::response += (String) "function saveesu() {\r\n";
        HTTP::response += (String) "  var http = new XMLHttpRequest(); http.open ('GET', '/action?action=savemapesu&addr=" + saddr + "&lines=" + std::to_string(lines) + "');";
        HTTP::response += (String) "  http.addEventListener('load',";
        HTTP::response += (String) "    function(event) { var text = http.responseText; if (http.status >= 200 && http.status < 300) { ";
        HTTP::response += (String) "      if (text == '0') { document.getElementById('saveesu').style.display = 'none'; } ";
        HTTP::response += (String) "      else { document.getElementById('saveesu').style.display = ''; alert ('Schreibfehler') }";
        HTTP::response += (String) "  }});";
        HTTP::response += (String) "  http.send (null);\r\n";
        HTTP::response += (String) "}\r\n";
    }
    else if (manu == MANUFACTURER_LENZ)
    {
        HTTP::response += (String) "function outpl(id,line,bitpos) {\r\n";
        HTTP::response += (String) "  var value;\r\n";
        HTTP::response += (String) "  var color = document.getElementById(id).style.backgroundColor;\r\n";
        HTTP::response += (String) "  switch (color) {\r\n";
        HTTP::response += (String) "    case 'green': color = 'inherit'; value = 0; break;\r\n";
        HTTP::response += (String) "    default:      color = 'green';   value = 1;break;\r\n";
        HTTP::response += (String) "  }\r\n";
        HTTP::response += (String) "  document.getElementById(id).style.backgroundColor = color;\r\n";
        HTTP::response += (String) "  var http = new XMLHttpRequest(); http.open ('GET', '/action?action=setoutputmaplenz&line=' + line + '&bitpos=' + bitpos + '&value=' + value);";
        HTTP::response += (String) "  http.addEventListener('load',";
        HTTP::response += (String) "    function(event) { var text = http.responseText; if (http.status >= 200 && http.status < 300) { ";
        HTTP::response += (String) "      if (text == '0') { document.getElementById('savelenz').style.display = 'none'; } else { document.getElementById('savelenz').style.display = ''; }";
        HTTP::response += (String) "  }});";
        HTTP::response += (String) "  http.send (null);\r\n";
        HTTP::response += (String) "}\r\n";
        HTTP::response += (String) "function savelenz() {\r\n";
        HTTP::response += (String) "  var http = new XMLHttpRequest(); http.open ('GET', '/action?action=savemaplenz&addr=" + saddr + "');";
        HTTP::response += (String) "  http.addEventListener('load',";
        HTTP::response += (String) "    function(event) { var text = http.responseText; if (http.status >= 200 && http.status < 300) { ";
        HTTP::response += (String) "      if (text == '0') { document.getElementById('savelenz').style.display = 'none'; } ";
        HTTP::response += (String) "      else { document.getElementById('savelenz').style.display = ''; alert ('Schreibfehler: Decoder hat vielleicht nur 4 Ausgänge?') }";
        HTTP::response += (String) "  }});";
        HTTP::response += (String) "  http.send (null);\r\n";
        HTTP::response += (String) "}\r\n";
    }
    else if (manu == MANUFACTURER_TAMS)
    {
        HTTP::response += (String) "function outt(id,line,col,bitpos) {\r\n";
        HTTP::response += (String) "  var value;\r\n";
        HTTP::response += (String) "  var color = document.getElementById(id).style.backgroundColor;\r\n";
        HTTP::response += (String) "  switch (color) {\r\n";
        HTTP::response += (String) "    case 'green': color = 'inherit'; value = 0; break;\r\n";
        HTTP::response += (String) "    default:      color = 'green';   value = 1;break;\r\n";
        HTTP::response += (String) "  }\r\n";
        HTTP::response += (String) "  document.getElementById(id).style.backgroundColor = color;\r\n";
        HTTP::response += (String) "  var http = new XMLHttpRequest(); http.open ('GET', '/action?action=setoutputmaptams&line=' + line + '&col=' + col + '&bitpos=' + bitpos + '&value=' + value);";
        HTTP::response += (String) "  http.addEventListener('load',";
        HTTP::response += (String) "    function(event) { var text = http.responseText; if (http.status >= 200 && http.status < 300) { ";
        HTTP::response += (String) "      if (text == '0') { document.getElementById('savetams').style.display = 'none'; } else { document.getElementById('savetams').style.display = ''; }";
        HTTP::response += (String) "  }});";
        HTTP::response += (String) "  http.send (null);\r\n";
        HTTP::response += (String) "}\r\n";

        HTTP::response += (String) "function offt(id,line,col) {\r\n";
        HTTP::response += (String) "  var value = document.getElementById(id).value;\r\n";
        HTTP::response += (String) "  var http = new XMLHttpRequest(); http.open ('GET', '/action?action=setoutputmaptams&line=' + line + '&col=' + col + '&bitpos=0&value=' + value);\r\n";
        HTTP::response += (String) "  http.addEventListener('load',\r\n";
        HTTP::response += (String) "    function(event) { var text = http.responseText; if (http.status >= 200 && http.status < 300) {\r\n";
        HTTP::response += (String) "      if (text == '0') { document.getElementById('savetams').style.display = 'none'; } else { document.getElementById('savetams').style.display = ''; }\r\n";
        HTTP::response += (String) "  }});\r\n";
        HTTP::response += (String) "  http.send (null);\r\n";
        HTTP::response += (String) "}\r\n";

        HTTP::response += (String) "function savetams() {\r\n";
        HTTP::response += (String) "  var http = new XMLHttpRequest(); http.open ('GET', '/action?action=savemaptams&addr=" + saddr + "&lines=" + std::to_string(lines) + "');";
        HTTP::response += (String) "  http.addEventListener('load',";
        HTTP::response += (String) "    function(event) { var text = http.responseText; if (http.status >= 200 && http.status < 300) { ";
        HTTP::response += (String) "      if (text == '0') { document.getElementById('savetams').style.display = 'none'; } ";
        HTTP::response += (String) "      else { document.getElementById('savetams').style.display = ''; alert ('Schreibfehler: Decoder hat vielleicht nur 4 Ausgänge?') }";
        HTTP::response += (String) "  }});";
        HTTP::response += (String) "  http.send (null);\r\n";
        HTTP::response += (String) "}\r\n";
    }

    HTTP::response += (String) "</script>\r\n";
    HTTP::flush ();

    if (get_mapping_successful)
    {
        switch (manu)
        {
            case MANUFACTURER_ESU:
                print_esu_mapping (lines);
                HTTP::response += (String) "<BR><button id='saveesu' style='display:none' onclick='saveesu()'>&Auml;nderungen senden</button>\r\n";
                break;
            case MANUFACTURER_LENZ:
                print_lenz_mapping (outputs);
                HTTP::response += (String) "<BR><button id='savelenz' style='display:none' onclick='savelenz()'>&Auml;nderungen senden</button>\r\n";
                break;
            case MANUFACTURER_TAMS:
                print_tams_mapping (lines);
                HTTP::response += (String) "<BR><button id='savetams' style='display:none' onclick='savetams()'>&Auml;nderungen senden</button>\r\n";
                break;
        }
    }

    HTTP::response += (String) "</div>\r\n";   // id='ison'
    HTTP::response += (String) "</div>\r\n";
    HTTP_Common::html_trailer ();
}

void
HTTP_POMMAP::action_setcondmapesu (void)
{
    uint_fast16_t   line        = HTTP::parameter_number ("line");           // 0 ... 72
    uint_fast16_t   bytepos     = HTTP::parameter_number ("bytepos");        // 0 ... 10
    uint_fast8_t    bitpos      = HTTP::parameter_number ("bitpos");         // 0 ... 6 (double bits)
    uint_fast8_t    condition   = HTTP::parameter_number ("condition");      // 0 ... 2

    if (line < MAX_ESU_MAP_LINES && bytepos < MAX_ESU_CONDITION_COLS && bitpos < 7 && condition < 3)
    {
        uint8_t new_value       = condition << bitpos;
        uint8_t old_byte        = esu_condition_map[line][bytepos];
        uint8_t new_byte_before = new_esu_condition_map[line][bytepos];
        uint8_t new_byte_now    = (new_esu_condition_map[line][bytepos] & ~(0x03 << bitpos)) | new_value;

        new_esu_condition_map[line][bytepos] = new_byte_now;

        if (old_byte == new_byte_before)
        {
            if (new_byte_now != old_byte)
            {
                esu_condition_mapping_changes++;
            }
        }
        else
        {
            if (new_byte_now == old_byte)
            {
                esu_condition_mapping_changes--;
            }
        }

        printf ("setcondmapesu: line=%u bytepos=%u bitpos=%u value=%u new_value=%u changes=%u\n",
                line, bytepos, bitpos, esu_condition_map[line][bytepos], new_esu_condition_map[line][bytepos], esu_condition_mapping_changes);
        HTTP::response = std::to_string (esu_condition_mapping_changes + esu_output_mapping_changes);
    }
}

void
HTTP_POMMAP::action_setoutputmapesu (void)
{
    uint_fast16_t   line        = HTTP::parameter_number ("line");           // 0 ... 72
    uint_fast16_t   bytepos     = HTTP::parameter_number ("bytepos");        // 0 ... 5
    uint_fast8_t    bitpos      = HTTP::parameter_number ("bitpos");         // 0 ... 7 (single bits)
    uint_fast8_t    value       = HTTP::parameter_number ("value");          // 0 ... 1

    if (line < MAX_ESU_MAP_LINES && bytepos < MAX_ESU_COLS && bitpos < 8 && value < 2)
    {
        uint8_t new_value       = value << bitpos;
        uint8_t old_byte        = esu_output_map[line][bytepos];
        uint8_t new_byte_before = new_esu_output_map[line][bytepos];
        uint8_t new_byte_now    = (new_esu_output_map[line][bytepos] & ~(0x01 << bitpos)) | new_value;

        new_esu_output_map[line][bytepos] = new_byte_now;

        if (old_byte == new_byte_before)
        {
            if (new_byte_now != old_byte)
            {
                esu_output_mapping_changes++;
            }
        }
        else
        {
            if (new_byte_now == old_byte)
            {
                esu_output_mapping_changes--;
            }
        }

        printf ("setoutputmapesu: line=%u bytepos=%u bitpos=%u value=%u new_value=%u changes=%u\n",
                line, bytepos, bitpos, esu_output_map[line][bytepos], new_esu_output_map[line][bytepos], esu_output_mapping_changes);
        HTTP::response = std::to_string (esu_condition_mapping_changes + esu_output_mapping_changes);
    }
    else
    {
        HTTP::response = "setoutputmap: values out of range";
    }
}

void
HTTP_POMMAP::action_setoutputmaplenz (void)
{
    uint_fast16_t   line        = HTTP::parameter_number ("line");           // 0 ... 31
    uint_fast8_t    bitpos      = HTTP::parameter_number ("bitpos");         // 0 ... 7 (single bits)
    uint_fast8_t    value       = HTTP::parameter_number ("value");          // 0 ... 1

    if (line < MAX_LENZ_MAP_LINES && bitpos < 8 && value < 2)
    {
        uint8_t new_value       = value << bitpos;
        uint8_t old_byte        = lenz_output_map[line];
        uint8_t new_byte_before = new_lenz_output_map[line];
        uint8_t new_byte_now    = (new_lenz_output_map[line] & ~(0x01 << bitpos)) | new_value;

        new_lenz_output_map[line] = new_byte_now;

        if (old_byte == new_byte_before)
        {
            if (new_byte_now != old_byte)
            {
                lenz_mapping_changes++;
            }
        }
        else
        {
            if (new_byte_now == old_byte)
            {
                lenz_mapping_changes--;
            }
        }

        printf ("setoutputmaplenz: line=%u bitpos=%u value=%u new_value=%u changes=%u\n",
                line, bitpos, lenz_output_map[line], new_lenz_output_map[line], lenz_mapping_changes);
        HTTP::response = std::to_string (lenz_mapping_changes);
    }
    else
    {
        HTTP::response = "setoutputmaplenz: values out of range";
    }
}

void
HTTP_POMMAP::action_setoutputmaptams (void)
{
    uint_fast16_t   line        = HTTP::parameter_number ("line");           // 0 ... 31
    uint_fast16_t   col         = HTTP::parameter_number ("col");            // 0, 2, or 3
    uint_fast8_t    bitpos      = HTTP::parameter_number ("bitpos");         // 0 ... 7 (single bits)
    uint_fast8_t    value       = HTTP::parameter_number ("value");          // 0 ... 1

    if (line < MAX_TAMS_MAP_LINES && (col == 0 || col == 2) && bitpos < 8 && value < 2)
    {
        uint8_t new_value       = value << bitpos;
        uint8_t old_byte        = tams_output_map[line][col];
        uint8_t new_byte_before = new_tams_output_map[line][col];
        uint8_t new_byte_now    = (new_tams_output_map[line][col] & ~(0x01 << bitpos)) | new_value;

        new_tams_output_map[line][col] = new_byte_now;

        if (old_byte == new_byte_before)
        {
            if (new_byte_now != old_byte)
            {
                tams_mapping_changes++;
            }
        }
        else
        {
            if (new_byte_now == old_byte)
            {
                tams_mapping_changes--;
            }
        }

        printf ("setoutputmaptams: line=%u col=%u bitpos=%u value=%u new_value=%u changes=%u\n",
                line, col, bitpos, tams_output_map[line][col], new_tams_output_map[line][col], tams_mapping_changes);
        HTTP::response = std::to_string (tams_mapping_changes);
    }
    else if (line < MAX_TAMS_MAP_LINES && col == 3)
    {
        uint8_t new_value       = value;
        uint8_t old_byte        = tams_output_map[line][col];
        uint8_t new_byte_before = new_tams_output_map[line][col];
        uint8_t new_byte_now    = new_value;

        new_tams_output_map[line][col] = new_byte_now;

        if (old_byte == new_byte_before)
        {
            if (new_byte_now != old_byte)
            {
                tams_mapping_changes++;
            }
        }
        else
        {
            if (new_byte_now == old_byte)
            {
                tams_mapping_changes--;
            }
        }

        printf ("setoutputmaptams: line=%u col=%u bitpos=%u value=%u new_value=%u changes=%u\n",
                line, col, bitpos, tams_output_map[line][col], new_tams_output_map[line][col], tams_mapping_changes);
        HTTP::response = std::to_string (tams_mapping_changes);
    }
    else
    {
        HTTP::response = "setoutputmap: values out of range";
    }
}

void
HTTP_POMMAP::action_savemapesu (void)
{
    uint_fast16_t   addr        = HTTP::parameter_number ("addr");
    uint_fast16_t   lines       = HTTP::parameter_number ("lines");
    uint16_t        last_cv32   = 0xFFFF;

    printf ("enter savemapesu, addr=%u lines=%u\n", addr, lines);

    if (addr > 0)
    {
        uint_fast16_t   line;
        uint16_t        cv32;
        uint16_t        bytepos;
        bool            error_occured       = 0;

        for (line = 0; line < lines && ! error_occured; line++)
        {
            for (bytepos = 0; bytepos < MAX_ESU_CONDITION_COLS; bytepos++)
            {
                uint_fast8_t    old_byte    = esu_condition_map[line][bytepos];
                uint_fast8_t    new_byte    = new_esu_condition_map[line][bytepos];
                uint_fast16_t   cv;

                if (old_byte != new_byte)
                {
                    cv32 = (line / 16) + 3;

                    if (last_cv32 != cv32)
                    {
                        if (! POM::pom_write_cv (addr, 32, cv32, POM_WRITE_COMPARE_BEFORE_AND_AFTER_WRITE))
                        {
                            error_occured = true;
                            break;
                        }
                        last_cv32 = cv32;
                    }

                    cv = line * 16 + bytepos + 257;

                    if (POM::pom_write_cv (addr, cv, new_byte, POM_WRITE_COMPARE_AFTER_WRITE))
                    {
                        esu_condition_map[line][bytepos] = new_byte;

                        if (esu_condition_mapping_changes)
                        {
                            esu_condition_mapping_changes--;
                        }
                        printf ("savemapesu: write successful, changes=%u\n", esu_condition_mapping_changes);
                    }
                    else
                    {
                        printf ("savemapesu: write error, addr=%u, cv32=%u cv=%u, value=%u\n", addr, cv32, cv, new_byte);
                        error_occured = true;
                        break;
                    }
                }
            }
        }

        if (! error_occured)
        {
            for (line = 0; line < lines && ! error_occured; line++)
            {
                for (bytepos = 0; bytepos < MAX_ESU_COLS; bytepos++)
                {
                    uint_fast8_t    old_byte    = esu_output_map[line][bytepos];
                    uint_fast8_t    new_byte    = new_esu_output_map[line][bytepos];
                    uint_fast16_t   cv;

                    if (old_byte != new_byte)
                    {
                        cv32 = (line / 16) + 8;

                        if (last_cv32 != cv32)
                        {
                            if (! POM::pom_write_cv (addr, 32, cv32, POM_WRITE_COMPARE_BEFORE_AND_AFTER_WRITE))
                            {
                                error_occured = true;
                                break;
                            }
                            last_cv32 = cv32;
                        }

                        cv = line * 16 + bytepos + 257;

                        if (POM::pom_write_cv (addr, cv, new_byte, POM_WRITE_COMPARE_AFTER_WRITE))
                        {
                            esu_output_map[line][bytepos] = new_byte;

                            if (esu_output_mapping_changes)
                            {
                                esu_output_mapping_changes--;
                            }

                            printf ("savemapesu: write successful, cv32=%u, cv=%u, value=%u changes=%u\n", cv32, cv, new_byte, esu_output_mapping_changes);
                        }
                        else
                        {
                            printf ("savemapesu: write error, addr=%u, cv32=%u cv=%u, value=%u\n", addr, cv32, cv, new_byte);
                            error_occured = true;
                            break;
                        }
                    }
                }
            }
        }

        HTTP::response = std::to_string (esu_condition_mapping_changes + esu_output_mapping_changes);
    }
}

void
HTTP_POMMAP::action_savemaplenz (void)
{
    uint_fast16_t   addr = HTTP::parameter_number ("addr");

    if (addr > 0)
    {
        uint_fast16_t   line;

        for (line = 0; line < MAX_LENZ_MAP_LINES; line++)
        {
            uint_fast8_t    old_byte    = lenz_output_map[line];
            uint_fast8_t    new_byte    = new_lenz_output_map[line];
            uint_fast16_t   cv          = lenz_cv_map[line];

            if (old_byte != new_byte)
            {
                if (POM::pom_write_cv (addr, cv, new_byte, POM_WRITE_COMPARE_AFTER_WRITE))
                {
                    lenz_output_map[line] = new_byte;

                    if (lenz_mapping_changes)
                    {
                        lenz_mapping_changes--;
                    }
                    printf ("savemaplenz: write successful, changes=%u\n", lenz_mapping_changes);
                }
                else
                {
                    printf ("savemaplenz: write error, addr=%u, cv=%u, value=%u\n", addr, cv, new_byte);
                }
            }
        }
        HTTP::response = std::to_string (lenz_mapping_changes);
    }
}

void
HTTP_POMMAP::action_savemaptams (void)
{
    uint_fast16_t   addr    = HTTP::parameter_number ("addr");
    uint_fast16_t   lines   = HTTP::parameter_number ("lines");

    if (addr > 0)
    {
        uint_fast16_t   line;
        uint_fast16_t   col;

        if (POM::pom_write_cv_index (addr, 0, 42))
        {
            for (line = 0; line < lines; line++)
            {
                for (col = 0; col < 4; col++)
                {
                    uint_fast8_t    old_byte    = tams_output_map[line][col];
                    uint_fast8_t    new_byte    = new_tams_output_map[line][col];
                    uint_fast16_t   cv          = (line * 4) + col + 257;

                    if (old_byte != new_byte)
                    {
                        if (POM::pom_write_cv (addr, cv, new_byte, POM_WRITE_COMPARE_AFTER_WRITE))
                        {
                            tams_output_map[line][col] = new_byte;

                            if (tams_mapping_changes)
                            {
                                tams_mapping_changes--;
                            }
                            printf ("savemaptams: write successful, cv=%u value=%u changes=%u\n", cv, new_byte, tams_mapping_changes);
                        }
                        else
                        {
                            printf ("savemaptams: write error, addr=%u, cv=%u, value=%u\n", addr, cv, new_byte);
                        }
                    }
                }
            }
        }
        else
        {
            printf ("savemaptams: CV index (CV31=0, CV32=42) kann nicht gespeichert werden.\n");
        }

        HTTP::response = std::to_string (tams_mapping_changes);
    }
}
