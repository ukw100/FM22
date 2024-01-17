/*------------------------------------------------------------------------------------------------------------------------
 * http-pommot.cc - HTTP POM motor parameter routines
 *------------------------------------------------------------------------------------------------------------------------
 * Copyright (c) 2023-2024 Frank Meyer - frank(at)uclock.de
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
#include "http-pommot.h"

typedef struct
{
    String          motortype;
    uint_fast8_t    cv2;
    uint_fast8_t    cv9;
    uint_fast8_t    cv51;
    uint_fast8_t    cv52;
    uint_fast8_t    cv53;
    uint_fast8_t    cv54;
    uint_fast8_t    cv55;
    uint_fast8_t    cv56;
    uint_fast8_t    cv116;
    uint_fast8_t    cv117;
    uint_fast8_t    cv118;
    uint_fast8_t    cv119;
} ESU_MOTOR_PARAMETERS;

#define N_ESU_MOTOR_EXAMPLES    15

static ESU_MOTOR_PARAMETERS esu_motor_examples[N_ESU_MOTOR_EXAMPLES] =
{
    //                                                          CV CV  CV  CV   CV   CV   CV   CV   CV   CV  CV  CV
    //   Motortyp                                               2   9  51  52   53   54   55   56  116  117 118 119
    {    "Standardeinstellungen für ROCO, Liliput, Brawa",      3, 40, 10, 10, 130,  50, 100, 255,  50, 150, 15, 20 },
    {    "Fleischmann® Rundmotor",                              3, 20, 20, 20, 110,  50, 200, 255,  50, 150, 20, 25 },
    {    "Märklin® kleiner Scheibenkollektormotor",             4, 20, 15, 30,  80,  20, 200, 255,  50, 150, 20, 25 },
    {    "Märklin® gro&szlig;er Scheibenkollektor",             4, 20, 20, 20,  80,  25, 100, 255,  50, 150, 20, 25 },
    {    "Märklin® Trommelkollektormotor",                      3, 20, 20, 20, 110,  25, 200, 255, 100, 150, 20, 25 },
    {    "Märklin® 5*-Hochleistungsmotor",                      3, 20, 20, 20, 110,  25, 200, 255, 100, 150, 20, 25 },
    {    "Märklin® mit Maxon Motor",                            3, 50, 10,  0, 120, 115,  50, 255,  50, 150, 20, 20 },
    {    "HAG® Motoren",                                        3, 20, 20, 15, 100,  40, 150, 255,  50, 150, 20, 25 },
    {    "Trix® mit Maxon® Motor",                              3, 50, 10,  0, 120, 115,  50, 255,  50, 150, 20, 20 },
    {    "Faulhaber® Motoren",                                  3, 50,  0,  0, 100,  25,  50, 255, 100, 150, 20, 25 },
    {    "PIKO® 5-Pol Motor",                                   3, 25, 15, 20, 110,  30,  40, 220,  50, 100, 10, 25 },
    {    "PIKO® 3-Pol Motor",                                   4, 25, 15, 20, 120,  25,  50, 220,  50,  80, 10, 25 },
    {    "Märklin® SoftDrive Sinus",                            3, 40, 10, 10, 130,  50, 100, 255,  50, 150, 15, 20 },
    {    "Bühler® Motor, generisch",                            3, 40, 30, 10, 120,  50,  10, 255,  50,  80, 15, 20 },
    {    "Bühler® Motor, Kiss Loks",                            3, 40, 32,  0, 140,  30, 100, 240,  80, 120, 15, 20 },
};

static ESU_MOTOR_PARAMETERS esu_motor_parameters;

static bool
get_esu_motor_modes (uint_fast16_t addr)
{
    bool        rtc;

    rtc = POM::pom_read_cv (&esu_motor_parameters.cv2, addr, 2) &&
          POM::pom_read_cv (&esu_motor_parameters.cv9, addr, 9) &&
          POM::pom_read_cv (&esu_motor_parameters.cv51, addr, 51) &&
          POM::pom_read_cv (&esu_motor_parameters.cv52, addr, 52) &&
          POM::pom_read_cv (&esu_motor_parameters.cv53, addr, 53) &&
          POM::pom_read_cv (&esu_motor_parameters.cv54, addr, 54) &&
          POM::pom_read_cv (&esu_motor_parameters.cv55, addr, 55) &&
          POM::pom_read_cv (&esu_motor_parameters.cv56, addr, 56) &&
          POM::pom_read_cv (&esu_motor_parameters.cv116, addr, 116) &&
          POM::pom_read_cv (&esu_motor_parameters.cv117, addr, 117) &&
          POM::pom_read_cv (&esu_motor_parameters.cv118, addr, 118) &&
          POM::pom_read_cv (&esu_motor_parameters.cv119, addr, 119);

    return rtc;
}

static void
print_esu_motor_modes (String url, uint_fast8_t manu, uint_fast16_t addr)
{
    HTTP::response += (String)
        "<P>Untenstehende Tabelle entpricht den Empfehlungen von ESU f&uuml;r den LokPilot 5.<BR>\r\n"
        "Beachte dazu auch die Bemerkungen in der Lokpilot-Dokumentation im Kapitel \"Motorsteuerung\".\r\n";

    HTTP::response += (String)
        "<P>"
        "<form method='GET' action='" + url + "'>\r\n"
        "<table style='border:1px solid gray'>\r\n"
        "<tr style='padding:4px;text-align:center;background-color:#E0E0E0;'>"
        "<td style='padding:4px;text-align:left;'>Motortyp</td>"
        "<td style='padding:4px;text-align:center;'>CV 2</td>"
        "<td style='padding:4px;text-align:center;'>CV 9</td>"
        "<td style='padding:4px;text-align:center;'>CV 51</td>"
        "<td style='padding:4px;text-align:center;'>CV 52</td>"
        "<td style='padding:4px;text-align:center;'>CV 53</td>"
        "<td style='padding:4px;text-align:center;'>CV 54</td>"
        "<td style='padding:4px;text-align:center;'>CV 55</td>"
        "<td style='padding:4px;text-align:center;'>CV 56</td>"
        "<td style='padding:4px;text-align:center;'>CV 116</td>"
        "<td style='padding:4px;text-align:center;'>CV 117</td>"
        "<td style='padding:4px;text-align:center;'>CV 118</td>"
        "<td style='padding:4px;text-align:center;'>CV 119</td>"
        "<td style='padding:4px;text-align:center;'>Aktion</td>"
        "</tr>";

    HTTP::flush ();

    for (uint_fast8_t i = 0; i < N_ESU_MOTOR_EXAMPLES; i++)
    {
        String style;
        String color2;
        String color9;
        String color51;
        String color52;
        String color53;
        String color54;
        String color55;
        String color56;
        String color116;
        String color117;
        String color118;
        String color119;

        if (esu_motor_examples[i].cv2   == esu_motor_parameters.cv2 &&
            esu_motor_examples[i].cv9   == esu_motor_parameters.cv9 &&
            esu_motor_examples[i].cv51  == esu_motor_parameters.cv51 &&
            esu_motor_examples[i].cv52  == esu_motor_parameters.cv52 &&
            esu_motor_examples[i].cv53  == esu_motor_parameters.cv53 &&
            esu_motor_examples[i].cv54  == esu_motor_parameters.cv54 &&
            esu_motor_examples[i].cv55  == esu_motor_parameters.cv55 &&
            esu_motor_examples[i].cv56  == esu_motor_parameters.cv56 &&
            esu_motor_examples[i].cv116 == esu_motor_parameters.cv116 &&
            esu_motor_examples[i].cv117 == esu_motor_parameters.cv117 &&
            esu_motor_examples[i].cv118 == esu_motor_parameters.cv118 &&
            esu_motor_examples[i].cv119 == esu_motor_parameters.cv119)
        {
            style = "style='color:white;background-color:green'";
        }

        color2      = (esu_motor_examples[i].cv2   == esu_motor_parameters.cv2)     ? "green" : "black";
        color9      = (esu_motor_examples[i].cv9   == esu_motor_parameters.cv9)     ? "green" : "black";
        color51     = (esu_motor_examples[i].cv51  == esu_motor_parameters.cv51)    ? "green" : "black";
        color52     = (esu_motor_examples[i].cv52  == esu_motor_parameters.cv52)    ? "green" : "black";
        color53     = (esu_motor_examples[i].cv53  == esu_motor_parameters.cv53)    ? "green" : "black";
        color54     = (esu_motor_examples[i].cv54  == esu_motor_parameters.cv54)    ? "green" : "black";
        color55     = (esu_motor_examples[i].cv55  == esu_motor_parameters.cv55)    ? "green" : "black";
        color56     = (esu_motor_examples[i].cv56  == esu_motor_parameters.cv56)    ? "green" : "black";
        color116    = (esu_motor_examples[i].cv116 == esu_motor_parameters.cv116)   ? "green" : "black";
        color117    = (esu_motor_examples[i].cv117 == esu_motor_parameters.cv117)   ? "green" : "black";
        color118    = (esu_motor_examples[i].cv118 == esu_motor_parameters.cv118)   ? "green" : "black";
        color119    = (esu_motor_examples[i].cv119 == esu_motor_parameters.cv119)   ? "green" : "black";

        HTTP::response += (String)
            "<tr>"
            "<td " + style + ">" + esu_motor_examples[i].motortype + "</td>"
            "<td style='padding:4px;color:" + color2   + "'>" + std::to_string(esu_motor_examples[i].cv2)   + "</td>"
            "<td style='padding:4px;color:" + color9   + "'>" + std::to_string(esu_motor_examples[i].cv9)   + "</td>"
            "<td style='padding:4px;color:" + color51  + "'>" + std::to_string(esu_motor_examples[i].cv51)  + "</td>"
            "<td style='padding:4px;color:" + color52  + "'>" + std::to_string(esu_motor_examples[i].cv52)  + "</td>"
            "<td style='padding:4px;color:" + color53  + "'>" + std::to_string(esu_motor_examples[i].cv53)  + "</td>"
            "<td style='padding:4px;color:" + color54  + "'>" + std::to_string(esu_motor_examples[i].cv54)  + "</td>"
            "<td style='padding:4px;color:" + color55  + "'>" + std::to_string(esu_motor_examples[i].cv55)  + "</td>"
            "<td style='padding:4px;color:" + color56  + "'>" + std::to_string(esu_motor_examples[i].cv56)  + "</td>"
            "<td style='padding:4px;color:" + color116 + "'>" + std::to_string(esu_motor_examples[i].cv116) + "</td>"
            "<td style='padding:4px;color:" + color117 + "'>" + std::to_string(esu_motor_examples[i].cv117) + "</td>"
            "<td style='padding:4px;color:" + color118 + "'>" + std::to_string(esu_motor_examples[i].cv118) + "</td>"
            "<td style='padding:4px;color:" + color119 + "'>" + std::to_string(esu_motor_examples[i].cv119) + "</td>"
            "<td>"
            "  <input type='submit' name='savemotesuex" + std::to_string(i) + "' value='Speichern'>"
            "</td>"
            "</tr>";
        HTTP::flush ();
    }

    HTTP::response += (String)
        "<tr>"
        "  <td colspan='14'><HR></td>"
        "</tr>"
        "<tr>"
        "<td>Aktuelle Einstellungen</td>"
        "<td><input type='text' maxlength='3' style='width:30px' name='cv2'   value='" + std::to_string(esu_motor_parameters.cv2)   + "'></td>"
        "<td><input type='text' maxlength='3' style='width:30px' name='cv9'   value='" + std::to_string(esu_motor_parameters.cv9)   + "'></td>"
        "<td><input type='text' maxlength='3' style='width:30px' name='cv51'  value='" + std::to_string(esu_motor_parameters.cv51)  + "'></td>"
        "<td><input type='text' maxlength='3' style='width:30px' name='cv52'  value='" + std::to_string(esu_motor_parameters.cv52)  + "'></td>"
        "<td><input type='text' maxlength='3' style='width:30px' name='cv53'  value='" + std::to_string(esu_motor_parameters.cv53)  + "'></td>"
        "<td><input type='text' maxlength='3' style='width:30px' name='cv54'  value='" + std::to_string(esu_motor_parameters.cv54)  + "'></td>"
        "<td><input type='text' maxlength='3' style='width:30px' name='cv55'  value='" + std::to_string(esu_motor_parameters.cv55)  + "'></td>"
        "<td><input type='text' maxlength='3' style='width:30px' name='cv56'  value='" + std::to_string(esu_motor_parameters.cv56)  + "'></td>"
        "<td><input type='text' maxlength='3' style='width:30px' name='cv116' value='" + std::to_string(esu_motor_parameters.cv116) + "'></td>"
        "<td><input type='text' maxlength='3' style='width:30px' name='cv117' value='" + std::to_string(esu_motor_parameters.cv117) + "'></td>"
        "<td><input type='text' maxlength='3' style='width:30px' name='cv118' value='" + std::to_string(esu_motor_parameters.cv118) + "'></td>"
        "<td><input type='text' maxlength='3' style='width:30px' name='cv119' value='" + std::to_string(esu_motor_parameters.cv119) + "'></td>"
        "<td>"
        "  <input type='hidden' name='action' value='savemotesu'>"
        "  <input type='hidden' name='manu' value='" + std::to_string(manu) + "'>"
        "  <input type='hidden' name='addr' value='" + std::to_string(addr) + "'>"
        "  <input type='submit' name='savemotesu' value='Speichern'>"
        "</td>"
        "</tr>";

    HTTP::response += (String) "</table></form>\r\n";

    HTTP::flush ();
}

static bool
get_lenz_motor_modes (uint_fast16_t addr)
{
    (void) addr;
    return true;
}

static void
print_lenz_motor_modes (String url, uint_fast8_t manu, uint_fast16_t addr)
{
    (void) url;
    (void) manu;
    (void) addr;
}

static bool
get_tams_motor_modes (uint_fast16_t addr)
{
    (void) addr;
    return true;
}

static void
print_tams_motor_modes (String url, uint_fast8_t manu, uint_fast16_t addr)
{
    (void) url;
    (void) manu;
    (void) addr;
}

static bool
savemotesu (uint_fast16_t addr, ESU_MOTOR_PARAMETERS * params)
{
    bool rtc = false;

    if (POM::pom_write_cv (addr,   2, params->cv2,  POM_WRITE_COMPARE_BEFORE_AND_AFTER_WRITE) &&
        POM::pom_write_cv (addr,   9, params->cv9,  POM_WRITE_COMPARE_BEFORE_AND_AFTER_WRITE) &&
        POM::pom_write_cv (addr,  51, params->cv51, POM_WRITE_COMPARE_BEFORE_AND_AFTER_WRITE) &&
        POM::pom_write_cv (addr,  52, params->cv52, POM_WRITE_COMPARE_BEFORE_AND_AFTER_WRITE) &&
        POM::pom_write_cv (addr,  53, params->cv53, POM_WRITE_COMPARE_BEFORE_AND_AFTER_WRITE) &&
        POM::pom_write_cv (addr,  54, params->cv54, POM_WRITE_COMPARE_BEFORE_AND_AFTER_WRITE) &&
        POM::pom_write_cv (addr,  55, params->cv55, POM_WRITE_COMPARE_BEFORE_AND_AFTER_WRITE) &&
        POM::pom_write_cv (addr,  56, params->cv56, POM_WRITE_COMPARE_BEFORE_AND_AFTER_WRITE) &&
        POM::pom_write_cv (addr, 116, params->cv116, POM_WRITE_COMPARE_BEFORE_AND_AFTER_WRITE) &&
        POM::pom_write_cv (addr, 117, params->cv117, POM_WRITE_COMPARE_BEFORE_AND_AFTER_WRITE) &&
        POM::pom_write_cv (addr, 118, params->cv118, POM_WRITE_COMPARE_BEFORE_AND_AFTER_WRITE) &&
        POM::pom_write_cv (addr, 119, params->cv119, POM_WRITE_COMPARE_BEFORE_AND_AFTER_WRITE))
    {
        rtc = true;
    }

    return rtc;
}

/*----------------------------------------------------------------------------------------------------------------------------------------
 * handle_pommot ()
 *----------------------------------------------------------------------------------------------------------------------------------------
 */
void
HTTP_POMMOT::handle_pommot (void)
{
    String          title   = "Motorparameter (POM)";
    String          url     = "/pommot";
    const char *    action  = HTTP::parameter ("action");
    const char *    saddr   = "";
    uint_fast16_t   addr    = 0;
    uint_fast8_t    manu    = 0;
    bool            get_motors_successful = false;

    HTTP_Common::html_header (title, title, url, true);
    HTTP::response += (String) "<div style='margin-left:20px;'>\r\n";
    HTTP_Common::add_action_handler ("head", "", 200, true);

    if (strcmp (action, "savemotesu") == 0)
    {
        saddr   = HTTP::parameter ("addr");
        addr    = atoi (saddr);

        if (addr > 0)
        {
            String clicked_button;

            clicked_button = HTTP::parameter ("savemotesu");

            if (! clicked_button.empty())
            {
                ESU_MOTOR_PARAMETERS esu_params;

                esu_params.cv2      = HTTP::parameter_number ("cv2");
                esu_params.cv9      = HTTP::parameter_number ("cv9");
                esu_params.cv51     = HTTP::parameter_number ("cv51");
                esu_params.cv52     = HTTP::parameter_number ("cv52");
                esu_params.cv53     = HTTP::parameter_number ("cv53");
                esu_params.cv54     = HTTP::parameter_number ("cv54");
                esu_params.cv55     = HTTP::parameter_number ("cv55");
                esu_params.cv56     = HTTP::parameter_number ("cv56");
                esu_params.cv116    = HTTP::parameter_number ("cv116");
                esu_params.cv117    = HTTP::parameter_number ("cv117");
                esu_params.cv118    = HTTP::parameter_number ("cv118");
                esu_params.cv119    = HTTP::parameter_number ("cv119");

                if (savemotesu (addr, &esu_params))
                {
                    HTTP::response += (String) "<font color='green'>Speichern der Motorparameter erfolgreich.</font><BR>\r\n";
                }
                else
                {
                    HTTP::response += (String) "<font color='red'>Speichern der Motorparameter fehlgeschlagen.</font><BR>\r\n";
                }
            }
            else
            {
                uint_fast16_t   idx;

                for (idx = 0; idx < N_ESU_MOTOR_EXAMPLES; idx++)
                {
                    clicked_button = HTTP::parameter ((String) "savemotesuex" + std::to_string(idx));

                    if (! clicked_button.empty())
                    {
                        if (savemotesu (addr, esu_motor_examples + idx))
                        {
                            HTTP::response += (String) "<font color='green'>Speichern der Motorparameter erfolgreich.</font><BR>\r\n";
                        }
                        else
                        {
                            HTTP::response += (String) "<font color='red'>Speichern der Motorparameter fehlgeschlagen.</font><BR>\r\n";
                        }
                        break;
                    }
                }
            }
        }
        else
        {
            printf ("savemotesu: addr is 0\n");
        }

        action = "getmot";
    }

    // NOT else!

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
    else if (strcmp (action, "getmot") == 0)
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
                    get_motors_successful = get_esu_motor_modes (addr);
                    break;
                case MANUFACTURER_LENZ:
                    get_motors_successful = get_lenz_motor_modes (addr);
                    break;
                case MANUFACTURER_TAMS:
                    get_motors_successful = get_tams_motor_modes (addr);
                    break;
                default:
                    HTTP::response += (String) "<font color='red'><B>Keine Motorparameter-Infos f&uuml;r Hersteller '" + HTTP_Common::manufacturers[manu] + "' vorhanden.</B></font><BR>\r\n";
                    break;
            }

            if (! get_motors_successful)
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
            "<div style='padding:4px;text-align:center;background-color:#E0E0E0;'><B>Motorparameter</B></div>\r\n"
            "<form method='GET' action='" + url + "'>\r\n"
            "<div style='padding:2px;'><span style='width:120px;display:inline-block;'>Adresse: " + saddr + "</span>"
            "<span style='width:110px;display:inline-block;'><a href='" + url + "'>Andere Adresse</a></span></div>\r\n"
            "<div style='padding:2px;'><span style='width:100%'>"
            "<input type='hidden' name='action' value='getmot'>"
            "<input type='hidden' name='manu' value='" + std::to_string(manu) + "'>"
            "<input type='hidden' name='addr' value='" + saddr + "'>"
            "<input type='submit' style='width:100%;padding:2px;' value='Motorparameter lesen'></span></div>\r\n"
            "</form></div>\r\n";
    }
    else
    {   
        HTTP::response += (String)
            "<form method='GET' action='" + url + "'>\r\n"
            "<BR><table style='border:1px lightgray solid;margin-left:10px;'><tr bgcolor='#E0E0E0'><th colspan='2'>Motorparameter</th></tr>\r\n"
            "<tr><td>Adresse</td><td><input type='text' style='width:120px;' maxlength='4' name='addr' value='" + saddr + "'></td></tr>\r\n"
            "<tr><td colspan='2' width='100%'><input type='hidden' name='action' value='getmanu'>\r\n"
            "<input type='submit' style='width:100%' value='Hersteller ermitteln'></td></tr>\r\n"
            "</table>\r\n"
            "</form>\r\n";
    }

    HTTP::response += (String) "<P>\r\n";
    HTTP::flush ();

    if (get_motors_successful)
    {
        switch (manu)
        {
            case MANUFACTURER_ESU:
                print_esu_motor_modes (url, manu, addr);
                break;
            case MANUFACTURER_LENZ:
                print_lenz_motor_modes (url, manu, addr);
                break;
            case MANUFACTURER_TAMS:
                print_tams_motor_modes (url, manu, addr);
                break;
        }
    }

    HTTP::response += (String) "</div>\r\n";   // id='ison'
    HTTP::response += (String) "</div>\r\n";
    HTTP_Common::html_trailer ();
}
