/*------------------------------------------------------------------------------------------------------------------------
 * http-pom.cc - HTTP POM routines
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
#include "dcc.h"
#include "base.h"
#include "http.h"
#include "http-common.h"
#include "http-pom.h"

/*----------------------------------------------------------------------------------------------------------------------------------------
 * handle_pominfo ()
 *----------------------------------------------------------------------------------------------------------------------------------------
 */
void
HTTP_POM::handle_pominfo (void)
{
    String          title       = "Decoder-Info (POM)";
    String          url         = "/pominfo";
    const char *    action      = HTTP::parameter ("action");
    uint_fast16_t   addr        = HTTP::parameter_number ("addr");

    if (addr == 0)
    {
        addr = 3;
    }

    HTTP_Common::html_header (title, title, url, true);
    HTTP::response += (String) "<div style='margin-left:20px;'>\r\n";
    HTTP_Common::add_action_handler ("head", "", 200, true);

    HTTP::response += (String) "<div id='isoff' style='display:none'><font color='red'>Booster ist abgeschaltet. Bitte einschalten.</font></div>\r\n";
    HTTP::response += (String) "<div id='ison' style='display:none'>\r\n";

    HTTP::response += (String) "Decoderinformationen k&ouml;nnen per POM nur abgerufen werden, wenn der Decoder r&uuml;ckmeldef&auml;hig ist.<P>\n";
    HTTP::response += (String) "<form method='GET' action='" + url + "'>\r\n";
    HTTP::response += (String) "Adresse: <input type='text' name='addr' value='" + std::to_string(addr) + "' style='width:50px;' maxlength='5'>\r\n";
    HTTP::response += (String) "<button type='submit' name='action' value='locoinfo'>Decoder-Info lesen</button></form>";

    HTTP::flush ();

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

        if (! POM::pom_write_cv (addr, 29, cv29, POM_WRITE_COMPARE_BEFORE_WRITE))  // don't compare after write, because addr could have been changed
        {
            HTTP::response += (String) "<font color='red'>Schreib-/Lesefehler</font><BR>";
        }
    }
    else if (strcmp (action, "locoinfo") == 0)
    {
        const char *  manu;
        bool          rtc;

        uint_fast8_t  cv1     = 0;    // Basisadresse
        uint_fast8_t  cv7     = 0;    // Hersteller Versionsnummer
        uint_fast8_t  cv8     = 0;    // Hersteller Identifikation
        uint_fast8_t  cv9     = 0;    // Hoeherwertige Adresse Zubehoerdecoder
        uint_fast8_t  cv17    = 0;    // Erweiterte Adresse (High)
        uint_fast8_t  cv18    = 0;    // Erweiterte Adresse (Low)
        uint_fast8_t  cv28    = 0;    // RailCom
        uint_fast8_t  cv29    = 0;    // Decoder Konfiguration
        uint16_t      cv17_18 = 0;    // Erweiterte Adresse (High + Low)

        if (addr > 0)
        {
            if (POM::pom_read_cv (&cv29, addr, 29))
            {
                if (cv29 & 0x80)      // Zubehoerdecoder
                {
                    rtc = POM::pom_read_cv (&cv1, addr, 1) &&
                          POM::pom_read_cv (&cv7, addr, 7) &&
                          POM::pom_read_cv (&cv8, addr, 8) &&
                          POM::pom_read_cv (&cv9, addr, 9);
                }
                else                  // Fahrzeugdecoder
                {
                    rtc = POM::pom_read_cv (&cv1, addr, 1) &&
                          POM::pom_read_cv (&cv7, addr, 7) &&
                          POM::pom_read_cv (&cv8, addr, 8) &&
                          POM::pom_read_cv (&cv28, addr, 28) &&
                          POM::pom_read_cv (&cv17, addr, 17) &&
                          POM::pom_read_cv (&cv18, addr, 18);

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
                    HTTP::response += (String) "<tr bgcolor='#E0E0E0'><th align='left'>CV</th><th align='left'>Bedeutung</th><th>Wert</th><th>Bemerkung</th></tr>\r\n";
                    HTTP::response += (String) "<tr><td align='right'>1</td><td>Basisadresse</td><td align='right'>" + std::to_string(cv1) +
                                          "</td><td>'Kurze Adresse'</td></tr>\r\n";
                    HTTP::response += (String) "<tr bgcolor='#E0E0E0'><td align='right'>7</td><td>Hersteller Versionsnummer</td><td align='right'>" + std::to_string(cv7) +
                                          "</td><td></td></tr>\r\n";
                    HTTP::response += (String) "<tr><td align='right'>8</td><td>Hersteller Identifikation</td><td align='right'>" + std::to_string(cv8) +
                                          "</td><td>" + manu + "</td><td></td></tr>\r\n";

                    if (cv29 & 0x80)      // Zubehoerdecoder
                    {
                        HTTP::response += (String) "<tr bgcolor='#E0E0E0'><td align='right'>9</td><td>H&ouml;herwertige Adresse</td><td align='right'>"
                                  + std::to_string(cv9) + "</td><td>'Obere 3 Bits'</td></tr>\r\n";
                    }
                    else                  // Fahrzeugdecoder
                    {
                        HTTP::response += (String) "<tr bgcolor='#E0E0E0'><td align='right'>17/18</td><td>Erweiterte Adresse</td><td align='right'>" + std::to_string(cv17_18) +
                                              "</td><td>'Lange Adresse'</td></tr>\r\n";
                        HTTP::response += (String) "<tr bgcolor='#E0E0E0'><td align='right'>28</td><td>RailCom</td><td align='right'>" + std::to_string(cv28) + "</td><td></td></tr>\r\n";
                    }

                    HTTP::response += (String) "<tr><td align='right'>29</td><td>Decoder Konfiguration</td><td align='right'>" + std::to_string(cv29) + "</td></tr>\r\n";
                    HTTP::response += (String) "</table>\r\n";
                    HTTP::flush ();

                    HTTP_Common::decoder_configuration (addr, cv29, cv1, cv9, cv17_18);
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
        }
        else
        {
            HTTP::response += (String) "<font color='red'>Ung&uuml;tige Adresse!</font>\r\n";
        }
    }

    HTTP::response += (String) "</div>\r\n";   // id='ison'
    HTTP::response += (String) "</div>\r\n";
    HTTP_Common::html_trailer ();
}

/*----------------------------------------------------------------------------------------------------------------------------------------
 * handle_pomaddr ()
 *----------------------------------------------------------------------------------------------------------------------------------------
 */
void
HTTP_POM::handle_pomaddr (void)
{
    String          title   = "Lokadresse (POM)";
    String          url     = "/pomaddr";
    const char *    action  = HTTP::parameter ("action");

    HTTP_Common::html_header (title, title, url, true);
    HTTP::response += (String) "<div style='margin-left:20px;'>\r\n";
    HTTP_Common::add_action_handler ("head", "", 200, true);

    HTTP::response += (String) "<div id='isoff' style='display:none'><font color='red'>Booster ist abgeschaltet. Bitte einschalten.</font></div>\r\n";
    HTTP::response += (String) "<div id='ison' style='display:none'>\r\n";

    HTTP::response += (String) "Die meisten Decoder erlauben keine &Auml;nderung der Adresse per POM.<BR>\n";
    HTTP::response += (String) "Einige Decoder erlauben jedoch die &Auml;nderung von kurzer Lokadresse zu einer anderen kurzen Lokadresse.<BR>\n";
    HTTP::response += (String) "Wenige Decoder erlauben zus&auml;tzlich auch die &Auml;nderung von einer langer Lokadresse zu einer kurzen Lokadresse.<BR>\n";
    HTTP::response += (String) "Sollte die &Auml;nderung hier fehlschlagen, &auml;ndere die Adresse &uuml;ber das Programmiergleis.<BR><P>\n";

    if (strcmp (action, "setaddr") == 0)
    {
        uint16_t      oldaddr;
        uint16_t      newaddr;

        oldaddr = HTTP::parameter_number ("oldaddr");
        newaddr = HTTP::parameter_number ("newaddr");

        if ((newaddr >= 1 && newaddr <= 99) || (newaddr >= 1000 && newaddr <= 9999))
        {
            DCC::pom_write_address (oldaddr, newaddr);
            HTTP::response += (String) "Neue Lokadresse " + std::to_string(newaddr) + " wurde gesendet.<BR>\n";
        }
        else
        {
            HTTP::response += (String) "Ung&uuml;tige Adresse: " + std::to_string(newaddr) + "<BR>\n";
        }
    }

    HTTP::response += (String) "<form method='GET' action='" + url + "'>\r\n";
    HTTP::response += (String) "<BR><table style='border:1px lightgray solid;'><tr bgcolor='#E0E0E0'><th colspan='2'>Lokadresse</th></tr>\r\n";
    HTTP::response += (String) "<tr><td>Lokadresse alt:</td><td><input type='text' style='width:50px;' maxlength='4' name='oldaddr' value=''></td></tr>\r\n";
    HTTP::response += (String) "<tr><td>Lokadresse neu:</td><td><input type='text' style='width:50px;' maxlength='4' name='newaddr' value=''></td></tr>\r\n";
    HTTP::response += (String) "<tr><td><input type='hidden' name='action' value='setaddr'></td><td><input type='submit' value='Adresse &auml;ndern'></td></tr>\r\n";
    HTTP::response += (String) "</table>\r\n";
    HTTP::response += (String) "</form>\r\n";
    HTTP::response += (String) "<div><BR>Empfehlung:<BR>\r\n";
    HTTP::response += (String) "Verwende die Adresse 1-99 f&uuml;r kurze Adressen (Basisadresse).<BR>\r\n";
    HTTP::response += (String) "Verwende die Adresse 1000-9999 f&uuml;r lange Adressen (Erweiterte Adresse).</div>\r\n";

    HTTP::response += (String) "</div>\r\n";   // id='ison'
    HTTP::response += (String) "</div>\r\n";
    HTTP_Common::html_trailer ();
}

/*----------------------------------------------------------------------------------------------------------------------------------------
 * handle_pomcv ()
 *----------------------------------------------------------------------------------------------------------------------------------------
 */
void
HTTP_POM::handle_pomcv (void)
{
    String          title   = "CV-Programmierung (POM)";
    String          url     = "/pomcv";
    const char *    action  = HTTP::parameter ("action");

    HTTP_Common::html_header (title, title, url, true);
    HTTP::response += (String) "<div style='margin-left:20px;'>\r\n";
    HTTP_Common::add_action_handler ("head", "", 200, true);

    HTTP::response += (String) "<div id='isoff' style='display:none'><font color='red'>Booster ist abgeschaltet. Bitte einschalten.</font></div>\r\n";
    HTTP::response += (String) "<div id='ison' style='display:none'>\r\n";

    const char *    saddr           = "";
    const char *    scvno           = "";
    String          scv_value_dec   = "";
    uint_fast8_t    cv_value_bin[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
    uint_fast8_t    i;

    if (! strcmp (action, "getcv") || ! strcmp (action, "setcvdec") || ! strcmp (action, "setcvbin"))
    {
        saddr   = HTTP::parameter ("addr");
        scvno   = HTTP::parameter ("cvno");
    }

    HTTP::response += (String) "<B>CV lesen</B><BR>\r\n";
    HTTP::response += (String) "<form method='GET' action='" + url + "'>\r\n";
    HTTP::response += (String) "<BR><table style='border:1px lightgray solid;margin-left:20px;'><tr bgcolor='#E0E0E0'><th colspan='2'>CV</th></tr>\r\n";
    HTTP::response += (String) "<tr><td>Adresse</td><td><input type='text' style='width:80px;' maxlength='4' name='addr' value='" + saddr + "'></td></tr>\r\n";
    HTTP::response += (String) "<tr><td>CV</td><td><input type='text' style='width:80px;' maxlength='4' name='cvno' value='" + scvno + "'></td></tr>\r\n";
    HTTP::response += (String) "<tr><td><input type='hidden' name='action' value='getcv'></td>\r\n";
    HTTP::response += (String) "<td><input type='submit' value='CV lesen'></td></tr>\r\n";
    HTTP::response += (String) "</table>\r\n";
    HTTP::response += (String) "</form>\r\n";

    HTTP::flush ();

    if (strcmp (action, "getcv") == 0)
    {
        uint_fast16_t addr = atoi (saddr);
        uint_fast16_t cvno = atoi (scvno);
        uint_fast8_t  cv_value = 0;

        if ((addr >= 1 && addr <= 99) || (addr >= 128 && addr <= 9999))
        {
            if (POM::pom_read_cv (&cv_value, addr, cvno))
            {
                char    tmpbuf[32];

                sprintf (tmpbuf, "%d", cv_value);
                scv_value_dec = tmpbuf;

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
                HTTP::response += (String) "<BR><font color='red'>Lesefehler</font><BR>\r\n";
            }
        }
        else
        {
            HTTP::response += (String) "Ung&uuml;tige Adresse<BR>\r\n";
        }
    }

    HTTP::response += (String) "<P><B>CV schreiben</B><P>\r\n";
    HTTP::response += (String) "<div>\r\n";
    HTTP::response += (String) "  <div><B>Hinweise</B></div>\r\n";
    HTTP::response += (String) "  <div>Manche Decoder schalten nach dem Schreiben von CV1 automatisch auf die Basis-Adresse um.</div>\r\n";
    HTTP::response += (String) "  <div>Benutze hier besser das Kommando 'Adresse', um die Lokadresse &auml;ndern.</div>\r\n";
    HTTP::response += (String) "  <P><div>CV17 und CV18 m&uuml;ssen immer paarweise programmiert werden.</div>\r\n";
    HTTP::response += (String) "  <div>Benutze daf&uuml;r bitte das Kommando 'Adresse', um die Lokadresse &auml;ndern.</div>\r\n";
    HTTP::response += (String) "</div>\r\n";

    HTTP::flush ();

    if (! strcmp (action, "setcvdec") || ! strcmp (action, "setcvbin"))
    {
        uint_fast16_t addr      = atoi (saddr);
        uint_fast16_t cvno      = atoi (scvno);
        uint_fast8_t  cv_value  = 0;

        if ((addr >= 1 && addr <= 99) || (addr >= 128 && addr <= 9999))
        {
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

            if (cvno == 17 || cvno == 18)
            {
                HTTP::response += (String) "<P><div style='color:red'>CV " + std::to_string(cvno) + "kann nur paarweise programmiert werden.</div>\r\n";
                HTTP::response += (String) "<div style='color:red'>Benutze bitte das Kommando 'Adresse', um die Lokadresse &auml;ndern.</div>\r\n";
            }
            else
            {
                if (POM::pom_write_cv (addr, cvno, cv_value, POM_WRITE_COMPARE_AFTER_WRITE))
                {
                    char          tmpbuf[32];
                    uint_fast8_t  i;

                    sprintf (tmpbuf, "%d", cv_value);
                    scv_value_dec = tmpbuf;

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

                    HTTP::response += (String) "<BR><div><font color='darkgreen'>CV " + std::to_string(cvno) + " wurde mit dem Wert " + std::to_string(cv_value) + " gespeichert.</font></div>\r\n";
                }
                else
                {
                    HTTP::response += (String) "<BR><div><font color='red'>Wert " + std::to_string(cv_value)
                              + " wurde gesendet. Das Ergebnis des Schreibvorgangs konnte nicht &uuml;berpr&uuml;ft werden.</font></div>\n";
                }
            }
        }
        else
        {
            HTTP::response += (String) "<BR><div><font color='red'>Ung&uuml;ltige Adresse: " + std::to_string(addr) + "</font></div>\n";
        }
    }

    HTTP::response += (String) "<table><tr><td>";

    HTTP::response += (String) "<form method='GET' action='" + url + "'>\r\n";
    HTTP::response += (String) "<BR><table style='border:1px lightgray solid;margin-left:20px;'><tr bgcolor='#E0E0E0'><th colspan='2'>CV-Wert (dez)</th></tr>\r\n";
    HTTP::response += (String) "<tr><td>Adresse</td><td><input type='text' style='width:80px;' maxlength='4' name='addr' value='" + saddr + "'></td></tr>\r\n";
    HTTP::response += (String) "<tr><td>CV</td><td><input type='text' style='width:80px;' maxlength='4' name='cvno' value='" + scvno + "'></td></tr>\r\n";
    HTTP::response += (String) "<tr><td>&nbsp;</td></tr>\r\n";
    HTTP::response += (String) "<tr><td>Wert</td><td><input type='text' style='width:80px;' maxlength='3' name='cvvalue' value='" + scv_value_dec + "'></td></tr>\r\n";
    HTTP::response += (String) "<tr><td colspan='2'><input type='hidden' name='action' value='setcvdec'><input type='submit' style='width:100%' value='CV speichern'></td></tr>\r\n";
    HTTP::response += (String) "</table>\r\n";
    HTTP::response += (String) "</form>\r\n";

    HTTP::response += (String) "</td><td>\r\n";
    HTTP::flush ();

    HTTP::response += (String) "<form method='GET' action='" + url + "'>\r\n";
    HTTP::response += (String) "<BR><table style='border:1px lightgray solid;'><tr bgcolor='#E0E0E0'><th colspan='9'>CV-Wert (bin)</th></tr>\r\n";
    HTTP::response += (String) "<tr><td>Adresse</td><td colspan='8'><input type='text' style='width:80px;' maxlength='4' name='addr' value='" + saddr + "'></td></tr>\r\n";
    HTTP::response += (String) "<tr><td>CV</td><td colspan='8'><input type='text' style='width:80px;' maxlength='4' name='cvno' value='" + scvno + "'></td></tr>\r\n";

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

    HTTP::response += (String) "<tr><td colspan='9'><input type='hidden' name='action' value='setcvbin'><input type='submit' style='width:100%' value='CV speichern'></td></tr>\r\n";
    HTTP::response += (String) "</table>\r\n";
    HTTP::response += (String) "</form>\r\n";

    HTTP::response += (String) "</td></tr></table>\r\n";

    HTTP::response += (String) "</div>\r\n";   // id='ison'
    HTTP::response += (String) "</div>\r\n";
    HTTP_Common::html_trailer ();
}

