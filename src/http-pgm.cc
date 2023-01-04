/*------------------------------------------------------------------------------------------------------------------------
 * http-pgm.cc - HTTP PGM routines
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

#include "dcc.h"
#include "base.h"
#include "http.h"
#include "http-common.h"
#include "http-pgm.h"

/*----------------------------------------------------------------------------------------------------------------------------------------
 * pgm_statistics ()
 *----------------------------------------------------------------------------------------------------------------------------------------
 */
static void
pgm_statistics (void)
{
    char minbuf[32];
    char maxbuf[32];

    sprintf (minbuf, "%0.1f", (float) DCC::pgm_min_cnt / 10.0);
    sprintf (maxbuf, "%0.1f", (float) DCC::pgm_max_cnt / 10.0);

    HTTP::response += (String) "<BR><button onclick=\"document.getElementById('statistics').style.display=''\">Details</button>\r\n";
    HTTP::response += (String) "<table id='statistics' style='border:1px lightgray solid;display:none'>\r\n";
    HTTP::response += (String) "<tr><td>limit</td><td>" + std::to_string(DCC::pgm_limit) + "</td></tr>\r\n";
    HTTP::response += (String) "<tr><td>lower range</td><td>" + std::to_string(DCC::pgm_min_lower_value) + " - " + std::to_string(DCC::pgm_max_lower_value) + "</td></tr>\r\n";
    HTTP::response += (String) "<tr><td>upper range</td><td>" + std::to_string(DCC::pgm_min_upper_value) + " - " + std::to_string(DCC::pgm_max_upper_value) + "</td></tr>\r\n";

    if (DCC::pgm_min_cnt == DCC::pgm_max_cnt) // only 1 measurement
    {
        HTTP::response += (String) "<tr><td>pulse width</td><td>" + minbuf + " msec</td></tr>\r\n";
    }
    else
    {
        HTTP::response += (String) "<tr><td>pulse width</td><td>" + minbuf + " - " + maxbuf + " msec</td></tr>\r\n";
    }

    HTTP::response += (String) "</table><BR>\r\n";
}

/*----------------------------------------------------------------------------------------------------------------------------------------
 * handle_pgminfo ()
 *----------------------------------------------------------------------------------------------------------------------------------------
 */
void
HTTP_PGM::handle_pgminfo (void)
{
    String          title   = "Decoder-Info";
    String          url     = "/pgminfo";
    const char *    action  = HTTP::parameter ("action");

    HTTP_Common::html_header (title, title, url, true);
    HTTP::response += (String) "<div style='margin-left:20px;'>\r\n";
    HTTP_Common::add_action_handler ("head", "", 200, true);

    HTTP::response += (String) "<div id='isoff' style='display:none'><font color='red'>Booster ist abgeschaltet. Bitte einschalten.</font></div>\r\n";
    HTTP::response += (String) "<div id='ison' style='display:none'>\r\n";

    HTTP::response += (String) "<font color='red'><B>Es darf nur eine Lok mit einem Decoder auf dem Programmiergleis stehen!</B></font><P>\n";
    HTTP::response += (String) "<form method='GET' action='" + url + "'><button type='submit' name='action' value='locoinfo'>Decoder-Info lesen</button></form>";

    if (strcmp (action, "save_cv29") == 0)
    {
        uint_fast8_t    direction   = HTTP::parameter_number ("direction");
        uint_fast8_t    steps       = HTTP::parameter_number ("steps");
        uint_fast8_t    analog      = HTTP::parameter_number ("analog");
        uint_fast8_t    railcom     = HTTP::parameter_number ("railcom");
        uint_fast8_t    speedtable  = HTTP::parameter_number ("speedtable");
        uint_fast8_t    extaddr     = HTTP::parameter_number ("extaddr");
        uint_fast8_t    reserved    = HTTP::parameter_number ("reserved");
        uint_fast8_t    zubehoer    = HTTP::parameter_number ("zubehoer");

        uint_fast8_t    cv29        = direction | steps | analog | railcom | speedtable | extaddr | reserved | zubehoer;

        if (! DCC::pgm_write_cv (29, cv29))
        {
            HTTP::response += (String) "<font color='red'>Schreib-/Lesefehler</font><BR>";
        }
    }
    else if (strcmp (action, "locoinfo") == 0)
    {
        char          buf[8];
        const char *  manu;
        bool          rtc;

        uint_fast8_t  cv1 = 0;        // Basisadresse
        uint_fast8_t  cv7 = 0;        // Hersteller Versionsnummer
        uint_fast8_t  cv8 = 0;        // Hersteller Identifikation
        uint_fast8_t  cv9 = 0;        // Hoeherwertige Adresse Zubehoerdecoder
        uint_fast8_t  cv17 = 0;       // Erweiterte Adresse (High)
        uint_fast8_t  cv18 = 0;       // Erweiterte Adresse (Low)
        uint_fast8_t  cv28 = 0;       // Railcom
        uint_fast8_t  cv29 = 0;       // Decoder Konfiguration
        uint16_t      cv17_18 = 0;    // Erweiterte Adresse (High + Low)

        if (DCC::pgm_read_cv (&cv29, 29))
        {
            if (cv29 & 0x80)      // Zubehoerdecoder
            {
                rtc = DCC::pgm_read_cv (&cv1, 1) &&
                      DCC::pgm_read_cv (&cv7, 7) &&
                      DCC::pgm_read_cv (&cv8, 8) &&
                      DCC::pgm_read_cv (&cv9, 9);
            }
            else                  // Fahrzeugdecoder
            {
                rtc = DCC::pgm_read_cv (&cv1, 1) &&
                      DCC::pgm_read_cv (&cv7, 7) &&
                      DCC::pgm_read_cv (&cv8, 8) &&
                      DCC::pgm_read_cv (&cv28, 28) &&
                      DCC::pgm_read_cv (&cv17, 17) &&
                      DCC::pgm_read_cv (&cv18, 18);

                if (rtc)
                {
                    cv17_18  = ((cv17 << 8) | cv18) & 0x3FFF;
                }
            }

            if (rtc)
            {
                manu = HTTP_Common::manufacturers[cv8];

                if (cv8 == 238)
                {
                    manu = "Andere";
                }
                else if (! manu)
                {
                    manu = "Unbekannt";
                }

                HTTP::response += (String) "<BR><table style='border:1px lightgray solid;'>\r\n";
                HTTP::response += (String) "<tr bgcolor='#E0E0E0'><th align='left'>CV</th><th align='left'>Bedeutung</th><th>Wert</th><th>Wert (Hex)</th><th>Bemerkung</th></tr>\r\n";
                sprintf (buf, "%02X", cv1);
                HTTP::response += (String) "<tr><td align='right'>1</td><td>Basisadresse</td><td align='right'>" + std::to_string(cv1) + "</td><td align='right'>0x" + buf +
                                      "</td><td>'Kurze Adresse'</td></tr>\r\n";
                sprintf (buf, "%02X", cv7);
                HTTP::response += (String) "<tr bgcolor='#E0E0E0'><td align='right'>7</td><td>Hersteller Versionsnummer</td><td align='right'>" + std::to_string(cv7) +
                                      "</td><td align='right'>0x" + buf + "</td><td></td></tr>\r\n";
                sprintf (buf, "%02X", cv8);
                HTTP::response += (String) "<tr><td align='right'>8</td><td>Hersteller Identifikation</td><td align='right'>" + std::to_string(cv8) +
                                      "</td><td align='right'>0x" + buf + "</td><td>" + manu + "</td><td></td></tr>\r\n";
                sprintf (buf, "%04X", cv17_18);

                if (cv29 & 0x80)      // Zubehoerdecoder
                {
                    HTTP::response += (String) "<tr bgcolor='#E0E0E0'><td align='right'>9</td><td>H&ouml;herwertige Adresse</td><td align='right'>"
                              + std::to_string(cv9) + "</td><td align='right'>0x" + buf + "</td><td>'Obere 3 Bits'</td></tr>\r\n";
                }
                else                  // Fahrzeugdecoder
                {
                    HTTP::response += (String) "<tr bgcolor='#E0E0E0'><td align='right'>17/18</td><td>Erweiterte Adresse</td><td align='right'>"
                              + std::to_string(cv17_18) + "</td><td align='right'>0x" + buf + "</td><td>'Lange Adresse'</td></tr>\r\n";
                    sprintf (buf, "%02X", cv28);
                    HTTP::response += (String) "<tr bgcolor='#E0E0E0'><td align='right'>28</td><td>RailCom</td><td align='right'>" + std::to_string(cv28)
                              + "</td><td align='right'>0x" + buf + "</td><td></td></tr>\r\n";
                }

                sprintf (buf, "%02X", cv29);
                HTTP::response += (String) "<tr><td align='right'>29</td><td>Decoder Konfiguration</td><td align='right'>" + std::to_string(cv29) + "</td><td align='right'>0x" + buf + "</td></tr>\r\n";
                HTTP::response += (String) "</table>\r\n";
                HTTP::flush ();

                HTTP_Common::decoder_configuration (0, cv29, cv1, cv9, cv17_18);
            }
            else
            {
                HTTP::response += (String) "<font color='red'>Lesefehler!</font>\r\n";
            }
        }
        else
        {
            HTTP::response += (String) "<font color='red'>Lesefehler!</font>\r\n";
        }

        pgm_statistics ();
    }

    HTTP::response += (String) "</div>\r\n";   // id='ison'
    HTTP::response += (String) "</div>\r\n";
    HTTP_Common::html_trailer ();
}

/*----------------------------------------------------------------------------------------------------------------------------------------
 * handle_pgmaddr ()
 *----------------------------------------------------------------------------------------------------------------------------------------
 */
void
HTTP_PGM::handle_pgmaddr (void)
{
    String          title   = "Lokadresse";
    String          url     = "/pgmaddr";
    const char *    action  = HTTP::parameter ("action");

    HTTP_Common::html_header (title, title, url, true);
    HTTP::response += (String) "<div style='margin-left:20px;'>\r\n";
    HTTP_Common::add_action_handler ("head", "", 200, true);

    HTTP::response += (String) "<div id='isoff' style='display:none'><font color='red'>Booster ist abgeschaltet. Bitte einschalten.</font></div>\r\n";
    HTTP::response += (String) "<div id='ison' style='display:none'>\r\n";

    HTTP::response += (String) "<font color='red'><B>Es darf nur eine Lok mit einem Decoder auf dem Programmiergleis stehen!</B></font><P>\n";

    if (strcmp (action, "setaddr") == 0)
    {
        uint_fast16_t   addr = HTTP::parameter_number ("addr");

        if (DCC::pgm_write_address (addr))
        {
            HTTP::response += (String) "Neue Lokadresse " + std::to_string(addr) + " wurde geschrieben.<BR>\n";
        }
        else
        {
            HTTP::response += (String) "Schreibvorgang der Adresse " + std::to_string(addr) + " fehlgeschlagen.<BR>\n";
        }
    }

    HTTP::response += (String) "<form method='GET' action='" + url + "'><button type='submit' name='action' value='readaddr'>Adresse lesen</button></form>";

    if (strcmp (action, "readaddr") == 0)
    {
        uint_fast8_t    cv1;       // Basisadresse
        uint_fast8_t    cv17;      // Erweiterte Adresse (High)
        uint_fast8_t    cv18;      // Erweiterte Adresse (Low)
        uint_fast8_t    cv29;      // Decoder Konfiguration
        uint16_t        cv17_18;   // Erweiterte Adresse (High + Low)
        uint16_t        addr;      // Aktive Adresse
        char            addr_buf[16];

        if (DCC::pgm_read_cv (&cv1, 1) &&
            DCC::pgm_read_cv (&cv17, 17) &&
            DCC::pgm_read_cv (&cv18, 18) &&
            DCC::pgm_read_cv (&cv29, 29))
        {
            cv17_18  = ((cv17 << 8) | cv18) & 0x3FFF;
            addr = ((cv29 & 0x20) ? cv17_18 : cv1);
            sprintf (addr_buf, "%d", addr);
        }
        else
        {
            addr_buf[0] = '\0';
            HTTP::response += (String) "<font color='red'>Lesefehler!</font><P>\r\n";
        }

        HTTP::response += (String) "<form method='GET' action='" + url + "'>\r\n";
        HTTP::response += (String) "<BR><table style='border:1px lightgray solid;'><tr bgcolor='#E0E0E0'><th colspan='2'>Lokadresse</th></tr>\r\n";
        HTTP::response += (String) "<tr><td>Lokadresse alt:</td><td>" + addr_buf + "</td></tr>\r\n";
        HTTP::response += (String) "<tr><td>Lokadresse neu:</td><td><input type='text' style='width:50px;' maxlength='4' name='addr' value='" + addr_buf + "'></td></tr>\r\n";
        HTTP::response += (String) "<tr><td><input type='hidden' name='action' value='setaddr'></td><td><input type='submit' value='Adresse &auml;ndern'></td></tr>\r\n";
        HTTP::response += (String) "</table>\r\n";
        HTTP::response += (String) "</form>\r\n";
        HTTP::response += (String) "<div><BR>Empfehlung:<BR>\r\n";
        HTTP::response += (String) "Verwende die Adresse 1-99 f&uuml;r kurze Adressen (Basisadresse).<BR>\r\n";
        HTTP::response += (String) "Verwende die Adresse 1000-9999 f&uuml;r lange Adressen (Erweiterte Adresse).</div>\r\n";

        pgm_statistics ();
    }

    HTTP::response += (String) "</div>\r\n";   // id='ison'
    HTTP::response += (String) "</div>\r\n";
    HTTP_Common::html_trailer ();
}

/*----------------------------------------------------------------------------------------------------------------------------------------
 * handle_pgmcv ()
 *----------------------------------------------------------------------------------------------------------------------------------------
 */
void
HTTP_PGM::handle_pgmcv (void)
{
    String          title   = "CV-Programmierung";
    String          url     = "/pgmcv";
    const char *    action  = HTTP::parameter ("action");
    const char *    cvno_str = HTTP::parameter ("cv");
    uint_fast8_t    cv_value_bin[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
    uint_fast8_t    i;

    HTTP_Common::html_header (title, title, url, true);
    HTTP::response += (String) "<div style='margin-left:20px;'>\r\n";
    HTTP_Common::add_action_handler ("head", "", 200, true);

    HTTP::response += (String) "<div id='isoff' style='display:none'><font color='red'>Booster ist abgeschaltet. Bitte einschalten.</font></div>\r\n";
    HTTP::response += (String) "<div id='ison' style='display:none'>\r\n";

    HTTP::response += (String) "<font color='red'><B>Es darf nur eine Lok mit einem Decoder auf dem Programmiergleis stehen!</B></font><P>\n";

    if (! strcmp (action, "setcvdec") || ! strcmp (action, "setcvbin"))
    {
        uint_fast16_t cvno = atoi (cvno_str);
        uint_fast8_t  cv_value = 0;

        if (strcmp (action, "setcvdec") == 0)
        {
            cv_value = HTTP::parameter_number ("cvvalue");
        }
        else if (strcmp (action, "setcvbin") == 0)
        {
            cv_value = 0;

            for (i = 0; i < 8; i++)
            {
                cv_value += HTTP::parameter_number ((String) "cvvaluebin" + std::to_string(i));
            }
        }

        if (DCC::pgm_write_cv (cvno, cv_value))
        {
            HTTP::response += (String) "CV " + std::to_string(cvno) + " wurde mit dem Wert " + std::to_string(cv_value) + " gespeichert.<BR>\r\n";
        }
        else
        {
            HTTP::response += (String) "Schreibvorgang fehlgeschlagen.<BR>\n";
        }

        pgm_statistics ();
    }

    HTTP::response += (String) "<form method='GET' action='" + url + "'>\r\n";
    HTTP::response += (String) "CV: <input type='text' name='cv' value='" + cvno_str +
                          "' style='width:80px;' maxlength='4'><button type='submit' name='action' value='readcv'>Wert lesen</button></form>";

    if (strcmp (action, "readcv") == 0)
    {
        uint_fast16_t   cvno;       // CV number
        uint_fast8_t    cv_value;
        char            decbuf[8];

        decbuf[0] = '\0';

        cvno = HTTP::parameter_number ("cv");

        if (DCC::pgm_read_cv (&cv_value, cvno))
        {
            uint_fast8_t  i;

            sprintf (decbuf, "%u", cv_value);

            for (i = 0; i < 8; i++)
            {
                if (cv_value & (1 << i))
                {
                    cv_value_bin[7 - i] = 1;
                }
                else
                {
                    cv_value_bin[7 - i] = 0;
                }
            }
        }
        else
        {
            HTTP::response += (String) "</font color='red'>Lesefehler!</font><BR>\r\n";
        }

        HTTP::flush ();

        if (cvno == 1)
        {
            HTTP::response += (String) "<BR><div style='color:red'>Achtung: Manche Decoder schalten nach dem Schreiben von CV1 automatisch auf die Basis-Adresse um.</div>\r\n";
            HTTP::response += (String) "<div style='color:red'>Benutze besser das Kommando 'Adresse', um die Lokadresse &auml;ndern.</div>\r\n";
        }
        else if (cvno == 17 || cvno == 18)
        {
            HTTP::response += (String) "<BR><div style='color:red'>Achtung: CV17 und CV18 m&uuml;ssen immer paarweise programmiert werden.</div>\r\n";
            HTTP::response += (String) "<div style='color:red'>Benutze bitte das Kommando 'Adresse', um die Lokadresse &auml;ndern.</div>\r\n";
        }

        HTTP::response += (String) "<table><tr><td>";

        HTTP::response += (String) "<form method='GET' action='" + url + "'>\r\n";
        HTTP::response += (String) "<BR><table style='border:1px lightgray solid;'><tr bgcolor='#E0E0E0'><th colspan='2'>CV" + std::to_string(cvno) + " - Wert (dez)</th></tr>\r\n";
        HTTP::response += (String) "<tr><td>Wert alt:</td><td>" + decbuf + "</td></tr>\r\n";
        HTTP::response += (String) "<tr><td>Wert neu</td><td><input type='text' style='width:80px;' maxlength='3' name='cvvalue' value='" + decbuf + "'></td></tr>\r\n";
        HTTP::response += (String) "<tr><td><input type='hidden' name='action' value='setcvdec'><input type='hidden' name='cv' value='" + std::to_string(cvno) + "'></td>\r\n";
        HTTP::response += (String) "<td><input type='submit' value='CV speichern'></td></tr>\r\n";
        HTTP::response += (String) "</table>\r\n";
        HTTP::response += (String) "</form>\r\n";

        HTTP::response += (String) "</td><td>\r\n";
        HTTP::flush ();

        HTTP::response += (String) "<form method='GET' action='" + url + "'>\r\n";
        HTTP::response += (String) "<BR><table style='border:1px lightgray solid;'><tr bgcolor='#E0E0E0'><th colspan='9'>CV-Wert (bin)</th></tr>\r\n";
        HTTP::response += (String) "<tr><td></td>";

        for (i = 128; i > 0; i >>= 1)
        {
            HTTP::response += (String) "<td style='width:15px;text-align:center'>" + std::to_string(i) + "</td>";
        }

        HTTP::response += (String) "</tr><tr><td>Wert</td>";

        for (i = 0; i < 8; i++)
        {
            String checked = "";

            if (cv_value_bin[i] == 1)
            {
                checked = " checked";
            }

            HTTP::response += (String) "<td><input type='checkbox'  style='width:15px;text-align:center' name='cvvaluebin" + std::to_string(i)
                                + "' value='" + std::to_string(1 << (7-i)) + "'" + checked + "></td>";
        }
        
        HTTP::response += (String) "</tr>\r\n";

        HTTP::response += (String) "<tr><td colspan='9'><input type='hidden' name='action' value='setcvbin'>";
        HTTP::response += (String) "<input type='hidden' name='cv' value='" + std::to_string(cvno) + "'>";
        HTTP::response += (String) "<input type='submit' style='width:100%' value='CV speichern'></td></tr>\r\n";
        HTTP::response += (String) "</table>\r\n";
        HTTP::response += (String) "</form>\r\n";

        HTTP::response += (String) "</td></tr></table>\r\n";
        HTTP::flush ();

        pgm_statistics ();
    }

    HTTP::response += (String) "</div>\r\n";   // id='ison'
    HTTP::response += (String) "</div>\r\n";
    HTTP_Common::html_trailer ();
}

