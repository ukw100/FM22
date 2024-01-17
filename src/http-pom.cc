/*------------------------------------------------------------------------------------------------------------------------
 * http-pom.cc - HTTP POM routines
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
#include "dcc.h"
#include "base.h"
#include "http.h"
#include "http-common.h"
#include "http-pom.h"

#define SECONDS_AT_2000_01_01   (946684800-3600)                // time since 2000-01-01 00:00:00 UTC

static bool
pom_read_block (uint8_t * buffer, uint_fast8_t len, uint_fast16_t addr, uint_fast8_t cv31_value, uint_fast8_t cv32_value, uint_fast16_t start_cv)
{
    uint_fast8_t    i;
    bool            rtc = false;

    len *= 4;                                                   // in units of 4
    start_cv++;                                                 // param beginning with 0

    if (POM::pom_write_cv_index (addr, cv31_value, cv32_value))
    {
        rtc = true;

        for (i = 0; i < len; i++)
        {
            rtc = POM::pom_read_cv (buffer, addr, start_cv);

            if (! rtc)
            {
                break;
            }

            buffer++;
            start_cv++;
        }
    }

    return rtc;
}

static bool
extended_decoder_info (uint_fast16_t addr, uint_fast8_t cv8)
{
    bool    rtc = true;

    if (cv8 == MANUFACTURER_ESU || cv8 == MANUFACTURER_ZIMO)
    {
        uint8_t     info[24];
        uint8_t *   pmanuid         = info + 0;     // cvno = 256-259, but only 256 used
        uint8_t *   pprodid         = info + 4;     // cvno = 260-263
        uint8_t *   pserial         = info + 8;     // cvno = 264-267
        uint8_t *   pprdate         = info + 12;    // cvno = 268-271
        uint8_t *   pversion        = info + 16;    // cvno = 384-387, but only 384-385 used
        uint8_t *   pesu_fw         = info + 20;    // cvno = 284-287, only ESU
        uint8_t     zimo_cv250      = 0;
        char        smanuid[32]     = { 0 };
        char        sprodid[32]     = { 0 };
        char        sserial[32]     = { 0 };
        char        sprdate[64]     = { 0 };
        char        sversion[32]    = { 0 };
        char        sfirmware[32]   = { 0 };
        String      sname;
        uint32_t    lserial;
        uint32_t    lprodid;
        time_t      ldate;

        if (cv8 == MANUFACTURER_ESU)
        {
            rtc =   POM::xpom_read_cv (info, 4, addr, 0, 255, (uint_fast8_t) 256) &&
                    POM::xpom_read_cv (pversion, 1, addr, 0, 255, (uint_fast8_t) 384) &&
                    POM::xpom_read_cv (pesu_fw, 1, addr, 0, 255, (uint_fast8_t) 284);       // nur ESU

            if (rtc)
            {
                sprintf (sfirmware, "%u.%u Build %u", pesu_fw[3], pesu_fw[2], (pesu_fw[1] << 8) | pesu_fw[0]);
            }
        }
        else
        {
            rtc =   pom_read_block (info, 4, addr, 0, 255, 256) &&
                    pom_read_block (pversion, 1, addr, 0, 255, 384);

            if (rtc && cv8 == MANUFACTURER_ZIMO)
            {
                rtc = POM::pom_read_cv (&zimo_cv250, addr, 250);
            }
        }

        if (rtc)
        {
            sprintf (smanuid, "%d", pmanuid[0]);

            lprodid = (pprodid[3] << 24) | (pprodid[2] << 16) | (pprodid[1] << 8) | (pprodid[0] << 0);
            sprintf (sprodid, "%08X", lprodid);

            lserial = (pserial[3] << 24) | (pserial[2] << 16) | (pserial[1] << 8) | (pserial[0] << 0);

            if (lserial != (uint32_t) 0xFFFFFFFF)
            {
                sprintf (sserial, "%u", lserial);
            }
            else
            {
                strcpy (sserial, "n.a.");
            }

            ldate   = (pprdate[3] << 24) | (pprdate[2] << 16) | (pprdate[1] << 8) | (pprdate[0] << 0);

            if (ldate != -1L)
            {
                ldate   += SECONDS_AT_2000_01_01;

                struct tm * tmp = localtime (&ldate);
                sprintf (sprdate, "%02d.%02d.%4d %02d:%02d:%02d", tmp->tm_mday, tmp->tm_mon + 1, tmp->tm_year + 1900, tmp->tm_hour, tmp->tm_min, tmp->tm_sec);
            }
            else
            {
                strcpy (sprdate, "n.a.");
            }

            sprintf (sversion, "%d.%d", pversion[0], pversion[1]);

            switch (lprodid)
            {
                case 0x0200003D:
                case 0x02000047:
                case 0x0200006A:
                    sname = "LokSound V4.0";
                    break;
                case 0x0200003C:
                case 0x0200004F:
                case 0x0200005F:
                    sname = "LokSound Select";
                    break;
                case 0x02000041:
                    sname = "LokSound micro V4.0";
                    break;
                case 0x0200004A:
                case 0x0200006F:
                    sname = "LokSound Select direct/micro";
                    break;
                case 0x02000065:
                    sname = "LokSound Select OEM";
                    break;
                case 0x0200004B:
                case 0x0200006D:
                    sname = "LokSound XL V4.0";
                    break;
                case 0x02000044:
                case 0x02000068:
                    sname = "LokSound V4.0 M4";
                    break;
                case 0x02000059:
                case 0x02000073:
                    sname = "LokSound V4.0 M4 OEM";
                    break;
                case 0x02000070:
                    sname = "LokSound L V4.0";
                    break;
                case 0x02000079:
                    sname = "LokSound Select L";
                    break;
                case 0x0100000D:
                case 0x01000012:
                case 0x01000017:
                case 0x01000020:
                    sname = "LokSound V3.5" ;
                    break;
                case 0x0100000E:
                case 0x01000014:
                case 0x01000024:
                    sname = "LokSound XL V3.5" ;
                    break;
                case 0x0200000E:
                case 0x02000015:
                case 0x02000021:
                    sname = "LokSound V3.0 M4" ;
                    break;
                case 0x01000019:
                case 0x0100001E:
                    sname = "LokSound micro V3.5" ;
                    break;
                case 0x0200003F:
                    sname = "LokPilot V4.0";
                    break;
                case 0x02000042:
                    sname = "LokPilot V4.0 DCC";
                    break;
                case 0x0200005B:
                    sname = "LokPilot V4.0 DCC PX";
                    break;
                case 0x02000040:
                    sname = "LokPilot micro V4.0";
                    break;
                case 0x02000046:
                    sname = "LokPilot micro V4.0 DCC";
                    break;
                case 0x02000043:
                    sname = "LokPilot V4.0 M4";
                    break;
                case 0x02000055:
                    sname = "LokPilot XL V4.0";
                    break;
                case 0x02000056:
                    sname = "LokPilot Fx V4.0";
                    break;
                case 0x0100005C:
                    sname = "LokPilot Standard V1.0" ;
                    break;
                case 0x0200001C:
                case 0x02000026:
                case 0x02000029:
                    sname = "LokPilot V3.0" ;
                    break;
                case 0x0200001D:
                case 0x02000027:
                case 0x0200002A:
                    sname = "LokPilot V3.0 DCC" ;
                    break;
                case 0x0200001F:
                case 0x02000028:
                case 0x0200002B:
                    sname = "LokPilot V3.0 M4" ;
                    break;
                case 0x02000023:
                case 0x02000030:
                case 0x02000031:
                    sname = "LokPilot V3.0 OEM" ;
                    break;
                case 0x0200002E:
                    sname = "LokPilot micro V3.0" ;
                    break;
                case 0x0200002F:
                    sname = "LokPilot micro V3.0 DCC" ;
                    break;
                case 0x02000033:
                    sname = "LokPilot XL V3.0" ;
                    break;
                case 0x0200002C:
                    sname = "LokPilot Fx V3.0" ;
                    break;
                case 0x02000036:
                    sname = "LokPilot Fx micro V3.0" ;
                    break;
                case 0x02000058:
                    sname = "SwitchPilot V2.0";
                    break;
                case 0x0200005A:
                    sname = "SwitchPilot Servo V2.0";
                    break;
                case 0x02000062:
                    sname = "Messwagen EHG388" ;
                    break;
                case 0x02000076:
                    sname = "Messwagen EHG388 M4" ;
                    break;
                case 0x01000061:
                    sname = "Raucherzeuger (Spur0, G)" ;
                    break;
                case 0x02000045:
                    sname = "KMI Smoke Unit" ;
                    break;
                case 0x01000064:
                    sname = "Digital Innenbeleuchtung" ;
                    break;
                case 0x010000C7: // fm
                    sname = "LokPilot 5 Basic";
                    break;
                case 0x20000A8:
                case 0x20000CC:
                    sname = "LokPilot 5";
                    break;
                case 0x20000AA:
                case 0x20000CE:
                    sname = "LokPilot 5 DCC";
                    break;
                case 0x10000AE:
                    sname = "LokPilot 5 micro";
                    break;
                case 0x10000AF:
                    sname = "LokPilot 5 micro DCC";
                    break;
                case 0x20000AC:
                    sname = "LokPilot 5 micro Next18";
                    break;
                case 0x20000AD:
                    sname = "LokPilot 5 micro DCC Next18";
                    break;
                case 0x20000B1:
                case 0x20000CF:
                    sname = "LokPilot 5 L";
                    break;
                case 0x20000B2:
                case 0x20000D0:
                    sname = "LokPilot 5 L DCC";
                    break;
                case 0x10000BE:
                case 0x10000D1:
                    sname = "LokPilot 5 Fx";
                    break;
                case 0x10000BF:
                case 0x10000D2:
                    sname = "LokPilot 5 Fx DCC";
                    break;
                case 0x20000B3:
                    sname = "LokPilot 5 Fx micro";
                    break;
                case 0x20000B4:
                    sname = "LokPilot 5 Fx micro DCC";
                    break;
                case 0x20000B8:
                    sname = "LokPilot 5 Fx micro Next18";
                    break;
                case 0x20000B9:
                    sname = "LokPilot 5 Fx micro Next18 DCC";
                    break;
                case 0x20000A9:
                case 0x20000CD:
                    sname = "LokPilot 5 MKL";
                    break;
                case 0x20000B0:
                case 0x20000D3:
                    sname = "LokPilot 5 MKL DCC";
                    break;
                case 0x020000F9:
                    sname = "SoundPilot 5 21MTC MKL (58449)";
                    break;
            }

            if (cv8 == MANUFACTURER_ZIMO)
            {
                switch (zimo_cv250)
                {
                    case 131:   sname = "MX630";                break;  // fm
                    case 197:   sname = "MX617";                break;
                    case 199:   sname = "MX600";                break;
                    case 200:   sname = "MX82";                 break;
                    case 201:   sname = "MX620";                break;
                    case 202:   sname = "MX62";                 break;
                    case 203:   sname = "MX63";                 break;
                    case 204:   sname = "MX64";                 break;
                    case 205:   sname = "MX64H";                break;
                    case 206:   sname = "MX64D";                break;
                    case 207:   sname = "MX680";                break;
                    case 208:   sname = "MX690";                break;
                    case 209:   sname = "MX69";                 break;
                    case 210:   sname = "MX640";                break;
                    case 211:   sname = "MX630-P2520";          break;
                    case 212:   sname = "MX632";                break;
                    case 213:   sname = "MX631";                break;
                    case 214:   sname = "MX642";                break;
                    case 215:   sname = "MX643";                break;
                    case 216:   sname = "MX647";                break;
                    case 217:   sname = "MX646";                break;
                    case 218:   sname = "MX630-P25K22";         break;
                    case 219:   sname = "MX631-P25K22";         break;
                    case 220:   sname = "MX632-P25K22";         break;
                    case 221:   sname = "MX645";                break;
                    case 222:   sname = "MX644";                break;
                    case 223:   sname = "MX621";                break;
                    case 224:   sname = "MX695-RevB";           break;
                    case 225:   sname = "MX648";                break;
                    case 226:   sname = "MX685";                break;
                    case 227:   sname = "MX695-RevC";           break;
                    case 228:   sname = "MX681";                break;
                    case 229:   sname = "MX695N";               break;
                    case 230:   sname = "MX696";                break;
                    case 231:   sname = "MX696N";               break;
                    case 232:   sname = "MX686";                break;
                    case 233:   sname = "MX622";                break;
                    case 234:   sname = "MX623";                break;
                    case 235:   sname = "MX687";                break;
                    case 236:   sname = "MX621-Fleischmann";    break;
                    case 243:   sname = "MX618";                break;
                    case 245:   sname = "MX697";                break;
                    case 246:   sname = "MX658N18";             break;
                    case 248:   sname = "MX821";                break;
                    case 250:   sname = "MX699";                break;
                    case 253:   sname = "MX649";                break;
                    case 254:   sname = "MX697 RevB";           break;
                }
            }

            HTTP::response += (String)
                "<P><table style='border:1px lightgray solid;width:480px'>\r\n"
                "<tr bgcolor='#E0E0E0'><th colspan='2'>Erweiterte Decoder-Info</th></tr>\r\n"
                "<tr><td>Hersteller-ID:</td><td>"       + smanuid   + "</td></tr>"
                "<tr bgcolor='#E0E0E0'><td>Decodertyp:</td><td>"            + sname     + "</td></tr>"
                "<tr><td>Produktnummer:</td><td>0x"     + sprodid   + "</td></tr>"
                "<tr bgcolor='#E0E0E0'><td>Seriennummer:</td><td>"      + sserial   + "</td></tr>"
                "<tr><td>Produktionsdatum:</td><td>"    + sprdate   + "</td></tr>"
                "<tr bgcolor='#E0E0E0'><td>RailCom-Version:</td><td>"       + sversion  + "</td></tr>";

            if (*sfirmware)
            {
                HTTP::response += (String)
                    "<tr><td>Firmware-Version:</td><td>" + sfirmware + "</td></tr>";
            }

            HTTP::response += (String)
                "</table>\r\n";

#if 0
            uint8_t all_cvs[128];

            HTTP::response += (String)
                "<P><table>\r\n";

            for (uint_fast8_t i = 0; i < 128; i++)
            {
                rtc = POM::pom_read_cv (all_cvs + i, addr, i + 1);

                if (rtc)
                {
                    HTTP::response += (String)
                        "<tr><td align='right'>" + std::to_string(i + 1) + "</td><td align='right'>" + std::to_string(all_cvs[i]) + "</td></tr>\r\n";
                }
                else
                {
                    HTTP::response += (String)
                        "</table>\r\n"
                        "<BR>Lesefehler bei Index " + std::to_string(i) + ".<BR>\r\n";
                    break;
                }
            }
#endif
        }
        else
        {
            HTTP::response += (String)
                "<BR>Keine erweiterten Decoder-Infos verfügbar.<BR>\r\n";
        }
    }

    return rtc;
}

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

    HTTP::response += (String)
        "<div id='isoff' style='display:none'><font color='red'>Booster ist abgeschaltet. Bitte einschalten.</font></div>\r\n"
        "<div id='ison' style='display:none'>\r\n"
        "Decoderinformationen k&ouml;nnen per POM nur abgerufen werden, wenn der Decoder r&uuml;ckmeldef&auml;hig ist.<P>\n"
        "<form method='GET' action='" + url + "'>\r\n"
        "Adresse: <input type='text' name='addr' value='" + std::to_string(addr) + "' style='width:50px;' maxlength='5'>\r\n"
        "<button type='submit' name='action' value='locoinfo'>Decoder-Info lesen</button></form>";

    HTTP::flush ();

    if (strcmp (action, "save_cv28") == 0)
    {
        uint_fast8_t    rc1             = HTTP::parameter_number ("rc1");
        uint_fast8_t    rc2             = HTTP::parameter_number ("rc2");
        uint_fast8_t    rc1_auto        = HTTP::parameter_number ("rc1_auto");
        uint_fast8_t    reserved_bit3   = HTTP::parameter_number ("reserved_bit3");
        uint_fast8_t    long_addr_3     = HTTP::parameter_number ("long_addr_3");
        uint_fast8_t    reserved_bit5   = HTTP::parameter_number ("reserved_bit5");
        uint_fast8_t    high_current    = HTTP::parameter_number ("high_current");
        uint_fast8_t    logon           = HTTP::parameter_number ("logon");

        uint_fast8_t    cv28            = rc1 | rc2 | rc1_auto | reserved_bit3 | long_addr_3 | reserved_bit5 | high_current | logon;

        if (! POM::pom_write_cv (addr, 28, cv28, POM_WRITE_COMPARE_AFTER_WRITE))
        {
            HTTP::response += (String) "<font color='red'>Schreib-/Lesefehler</font><BR>";
        }
    }
    else if (strcmp (action, "save_cv29") == 0)
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
                          POM::pom_read_cv (&cv9, addr, 9) &&
                          POM::pom_read_cv (&cv28, addr, 28);
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

                    HTTP::response += (String)
                        "<BR><table style='border:1px lightgray solid;width:480px'>\r\n"
                        "<tr bgcolor='#E0E0E0'><th align='left'>CV</th><th align='left'>Bedeutung</th><th>Wert</th><th>Bemerkung</th></tr>\r\n"
                        "<tr><td align='right'>1</td><td>Basisadresse</td><td align='right'>" + std::to_string(cv1) + "</td><td></td></tr>\r\n"
                        "<tr bgcolor='#E0E0E0'><td align='right'>7</td><td>Hersteller Versionsnummer</td><td align='right'>" + std::to_string(cv7) + "</td><td></td></tr>\r\n"
                        "<tr><td align='right'>8</td><td>Hersteller Identifikation</td><td align='right'>" + std::to_string(cv8) + "</td><td>" + manu + "</td><td></td></tr>\r\n";

                    if (cv29 & 0x80)      // Zubehoerdecoder
                    {
                        HTTP::response += (String)
                            "<tr bgcolor='#E0E0E0'><td align='right'>9</td><td>H&ouml;herwertige Adresse</td><td align='right'>"   + std::to_string(cv9) + "</td><td>'Obere 3 Bits'</td></tr>\r\n"
                            "<tr><td align='right'>29</td><td>Decoder Konfiguration</td><td align='right'>" + std::to_string(cv29) + "</td><td></td></tr>\r\n";
                    }
                    else                  // Fahrzeugdecoder
                    {
                        HTTP::response += (String)
                            "<tr bgcolor='#E0E0E0'><td align='right'>17/18</td><td>Erweiterte Adresse</td><td align='right'>" + std::to_string(cv17_18) + "</td><td></td></tr>\r\n"
                            "<tr><td align='right'>28</td><td>RailCom</td><td align='right'>" + std::to_string(cv28) + "</td><td></td></tr>\r\n"
                            "<tr bgcolor='#E0E0E0'><td align='right'>29</td><td>Decoder Konfiguration</td><td align='right'>" + std::to_string(cv29) + "</td><td></td></tr>\r\n";
                    }

                    HTTP::response += (String) "</table>\r\n";
                    HTTP::flush ();

                    HTTP_Common::decoder_railcom (addr, cv28, cv29);
                    HTTP::response += (String) "<P>\r\n";
                    HTTP_Common::decoder_configuration (addr, cv29, cv1, cv9, cv17_18);
                    HTTP::flush ();
                    rtc = extended_decoder_info (addr, cv8);
                    HTTP::flush ();
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
    String          title   = "Decoder-Adresse (POM)";
    String          url     = "/pomaddr";
    const char *    action  = HTTP::parameter ("action");

    HTTP_Common::html_header (title, title, url, true);
    HTTP::response += (String) "<div style='margin-left:20px;'>\r\n";
    HTTP_Common::add_action_handler ("head", "", 200, true);

    HTTP::response += (String)
        "<div id='isoff' style='display:none'><font color='red'>Booster ist abgeschaltet. Bitte einschalten.</font></div>\r\n"
        "<div id='ison' style='display:none'>\r\n"
        "Die meisten Decoder erlauben keine &Auml;nderung der Adresse per POM.<BR>\n"
        "Einige Decoder erlauben jedoch die &Auml;nderung von kurzer Decoder-Adresse zu einer anderen kurzen Decoder-Adresse.<BR>\n"
        "Wenige Decoder erlauben zus&auml;tzlich auch die &Auml;nderung von einer langer Decoder-Adresse zu einer kurzen Decoder-Adresse.<BR>\n"
        "Sollte die &Auml;nderung hier fehlschlagen, &auml;ndere die Adresse &uuml;ber das Programmiergleis.<BR><P>\n";

    if (strcmp (action, "setaddr") == 0)
    {
        uint16_t      oldaddr;
        uint16_t      newaddr;

        oldaddr = HTTP::parameter_number ("oldaddr");
        newaddr = HTTP::parameter_number ("newaddr");

        if ((newaddr >= 1 && newaddr <= 99) || (newaddr >= 1000 && newaddr <= 9999))
        {
            DCC::pom_write_address (oldaddr, newaddr);
            HTTP::response += (String) "Neue Decoder-Adresse " + std::to_string(newaddr) + " wurde gesendet.<BR>\n";
        }
        else
        {
            HTTP::response += (String) "Ung&uuml;tige Adresse: " + std::to_string(newaddr) + "<BR>\n";
        }
    }

    HTTP::response += (String)
        "<form method='GET' action='" + url + "'>\r\n"
        "<BR><table style='border:1px lightgray solid;'><tr bgcolor='#E0E0E0'><th colspan='2'>Decoder-Adresse</th></tr>\r\n"
        "<tr><td>Decoder-Adresse alt:</td><td><input type='text' style='width:50px;' maxlength='4' name='oldaddr' value=''></td></tr>\r\n"
        "<tr><td>Decoder-Adresse neu:</td><td><input type='text' style='width:50px;' maxlength='4' name='newaddr' value=''></td></tr>\r\n"
        "<tr><td><input type='hidden' name='action' value='setaddr'></td><td><input type='submit' value='Adresse &auml;ndern'></td></tr>\r\n"
        "</table>\r\n"
        "</form>\r\n"
        "<div><BR>Empfehlung:<BR>\r\n"
        "Verwende die Adresse 1-99 f&uuml;r kurze Adressen (Basisadresse).<BR>\r\n"
        "Verwende die Adresse 1000-9999 f&uuml;r lange Adressen (Erweiterte Adresse).</div>\r\n"
        "</div>\r\n"
        "</div>\r\n";
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

    HTTP::response += (String)
        "<div id='isoff' style='display:none'><font color='red'>Booster ist abgeschaltet. Bitte einschalten.</font></div>\r\n"
        "<div id='ison' style='display:none'>\r\n";

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

    HTTP::response += (String)
        "<B>CV lesen</B><BR>\r\n"
        "<form method='GET' action='" + url + "'>\r\n"
        "<BR><table style='border:1px lightgray solid;margin-left:20px;'><tr bgcolor='#E0E0E0'><th colspan='2'>CV</th></tr>\r\n"
        "<tr><td>Adresse</td><td><input type='text' style='width:80px;' maxlength='4' name='addr' value='" + saddr + "'></td></tr>\r\n"
        "<tr><td>CV</td><td><input type='text' style='width:80px;' maxlength='4' name='cvno' value='" + scvno + "'></td></tr>\r\n"
        "<tr><td><input type='hidden' name='action' value='getcv'></td>\r\n"
        "<td><input type='submit' value='CV lesen'></td></tr>\r\n"
        "</table>\r\n"
        "</form>\r\n";

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

    HTTP::response += (String)
        "<P><B>CV schreiben</B><P>\r\n"
        "<div>\r\n"
        "  <div><B>Hinweise</B></div>\r\n"
        "  <div>Manche Decoder schalten nach dem Schreiben von CV1 automatisch auf die Basis-Adresse um.</div>\r\n"
        "  <div>Benutze hier besser das Kommando 'Adresse', um die Decoder-Adresse &auml;ndern.</div>\r\n"
        "  <P><div>CV17 und CV18 m&uuml;ssen immer paarweise programmiert werden.</div>\r\n"
        "  <div>Benutze daf&uuml;r bitte das Kommando 'Adresse', um die Decoder-Adresse &auml;ndern.</div>\r\n"
        "</div>\r\n";

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
                HTTP::response += (String)
                    "<P><div style='color:red'>CV " + std::to_string(cvno) + "kann nur paarweise programmiert werden.</div>\r\n"
                    "<div style='color:red'>Benutze bitte das Kommando 'Adresse', um die Decoder-Adresse &auml;ndern.</div>\r\n";
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

    HTTP::response += (String)
        "<table><tr><td>"
        "<form method='GET' action='" + url + "'>\r\n"
        "<BR><table style='border:1px lightgray solid;margin-left:20px;'><tr bgcolor='#E0E0E0'><th colspan='2'>CV-Wert (dez)</th></tr>\r\n"
        "<tr><td>Adresse</td><td><input type='text' style='width:80px;' maxlength='4' name='addr' value='" + saddr + "'></td></tr>\r\n"
        "<tr><td>CV</td><td><input type='text' style='width:80px;' maxlength='4' name='cvno' value='" + scvno + "'></td></tr>\r\n"
        "<tr><td>&nbsp;</td></tr>\r\n"
        "<tr><td>Wert</td><td><input type='text' style='width:80px;' maxlength='3' name='cvvalue' value='" + scv_value_dec + "'></td></tr>\r\n"
        "<tr><td colspan='2'><input type='hidden' name='action' value='setcvdec'><input type='submit' style='width:100%' value='CV speichern'></td></tr>\r\n"
        "</table>\r\n"
        "</form>\r\n"
        "</td><td>\r\n"
        "<form method='GET' action='" + url + "'>\r\n"
        "<BR><table style='border:1px lightgray solid;'><tr bgcolor='#E0E0E0'><th colspan='9'>CV-Wert (bin)</th></tr>\r\n"
        "<tr><td>Adresse</td><td colspan='8'><input type='text' style='width:80px;' maxlength='4' name='addr' value='" + saddr + "'></td></tr>\r\n"
        "<tr><td>CV</td><td colspan='8'><input type='text' style='width:80px;' maxlength='4' name='cvno' value='" + scvno + "'></td></tr>\r\n"
        "<tr><td></td>";

    HTTP::flush ();

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
    
    HTTP::response += (String)
        "</tr>\r\n"
        "<tr><td colspan='9'><input type='hidden' name='action' value='setcvbin'><input type='submit' style='width:100%' value='CV speichern'></td></tr>\r\n"
        "</table>\r\n"
        "</form>\r\n"
        "</td></tr></table>\r\n"
        "</div>\r\n"
        "</div>\r\n";
    HTTP_Common::html_trailer ();
}
