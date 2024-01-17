/*------------------------------------------------------------------------------------------------------------------------
 * http-common.cc - HTTP addon routines
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

#include "fm22.h"
#include "debug.h"
#include "fileio.h"
#include "event.h"
#include "userio.h"
#include "dcc.h"
#include "loco.h"
#include "led.h"
#include "rcl.h"
#include "railroad.h"
#include "s88.h"
#include "switch.h"
#include "sig.h"
#include "addon.h"
#include "func.h"
#include "base.h"
#include "http.h"
#include "version.h"
#include "http-common.h"

bool                    HTTP_Common::edit_mode  = false;
std::string             HTTP_Common::alert_msg  = "";
static uint_fast8_t     todo_speed_deadtime     = 50;

/*----------------------------------------------------------------------------------------------------------------------------------------
 * global data:
 *----------------------------------------------------------------------------------------------------------------------------------------
 */
const char * HTTP_Common::manufacturers[256] =
{
    "FM22",                                                         // 0
    "CML Electronics Limited",                                      // 1
    "Train Technology",                                             // 2
    (const char *) NULL,                                            // 3
    (const char *) NULL,                                            // 4
    (const char *) NULL,                                            // 5
    (const char *) NULL,                                            // 6
    (const char *) NULL,                                            // 7
    (const char *) NULL,                                            // 8
    (const char *) NULL,                                            // 9
    (const char *) NULL,                                            // 10
    "NCE Corporation (formerly North Coast Engineering)",           // 11
    "Wangrow Electronics",                                          // 12
    "Public Domain & Do-It-Yourself Decoders",                      // 13
    "PSI Dynatrol",                                                 // 14
    "Ramfixx Technologies (Wangrow)",                               // 15
    (const char *) NULL,                                            // 16
    "Advance IC Engineering",                                       // 17
    "JMRI",                                                         // 18
    "AMW",                                                          // 19
    "T4T - Technology for Trains GmbH",                             // 20
    "Kreischer Datentechnik",                                       // 21
    "KAM Industries",                                               // 22
    "S Helper Service",                                             // 23
    "MoBaTron.de",                                                  // 24
    "Team Digital, LLC",                                            // 25
    "MBTronik PiN GITmBH",                                          // 26
    "MTH Electric Trains, Inc.",                                    // 27
    "Heljan A/S",                                                   // 28
    "Mistral Train Models",                                         // 29
    "Digsight",                                                     // 30
    "Brelec",                                                       // 31
    "Regal Way Co. Ltd",                                            // 32
    "Praecipuus",                                                   // 33
    "Aristo-Craft Trains",                                          // 34
    "Electronik & Model Produktion",                                // 35
    "DCCconcepts",                                                  // 36
    "NAC Services, Inc",                                            // 37
    "Broadway Limited Imports, LLC",                                // 38
    "Educational Computer, Inc.",                                   // 39
    "KATO Precision Models",                                        // 40
    "Passmann",                                                     // 41
    "Digikeijs",                                                    // 42
    "Ngineering",                                                   // 43
    "SPROG-DCC",                                                    // 44
    "ANE Model Co, Ltd",                                            // 45
    "GFB Designs",                                                  // 46
    "Capecom",                                                      // 47
    "Hornby Hobbies Ltd",                                           // 48
    "Joka Electronic",                                              // 49
    "N&Q Electronics",                                              // 50
    "DCC Supplies, Ltd",                                            // 51
    "Krois-Modell",                                                 // 52
    "Rautenhaus Digital Vertrieb",                                  // 53
    "TCH Technology",                                               // 54
    "QElectronics GmbH",                                            // 55
    "LDH",                                                          // 56
    "Rampino Elektronik",                                           // 57
    "KRES GmbH",                                                    // 58
    "Tam Valley Depot",                                             // 59
    "Bluecher-Electronic",                                          // 60
    "TrainModules",                                                 // 61
    "Tams Elektronik GmbH",                                         // 62
    "Noarail",                                                      // 63
    "Digital Bahn",                                                 // 64
    "Gaugemaster",                                                  // 65
    "Railnet Solutions, LLC",                                       // 66
    "Heller Modenlbahn",                                            // 67
    "MAWE Elektronik",                                              // 68
    "E-Modell",                                                     // 69
    "Rocrail",                                                      // 70
    "New York Byano Limited",                                       // 71
    "MTB Model",                                                    // 72
    "The Electric Railroad Company",                                // 73
    "PpP Digital",                                                  // 74
    "Digitools Elektronika, Kft",                                   // 75
    "Auvidel",                                                      // 76
    "LS Models Sprl",                                               // 77
    "Tehnologistic (train-O-matic)",                                // 78
    "Hattons Model Railways",                                       // 79
    "Spectrum Engineering",                                         // 80
    "GooVerModels",                                                 // 81
    "HAG Modelleisenbahn AG",                                       // 82
    "JSS-Elektronic",                                               // 83
    "Railflyer Model Prototypes, Inc.",                             // 84
    "Uhlenbrock GmbH",                                              // 85
    "Wekomm Engineering, GmbH",                                     // 86
    "RR-Cirkits",                                                   // 87
    "HONS Model",                                                   // 88
    "Pojezdy.EU",                                                   // 89
    "Shourt Line",                                                  // 90
    "Railstars Limited",                                            // 91
    "Tawcrafts",                                                    // 92
    "Kevtronics cc",                                                // 93
    "Electroniscript, Inc.",                                        // 94
    "Sanda Kan Industrial, Ltd.",                                   // 95
    "PRICOM Design",                                                // 96
    "Doehler & Haas",                                               // 97
    "Harman DCC",                                                   // 98
    "Lenz Elektronik GmbH",                                         // 99
    "Trenes Digitales",                                             // 100
    "Bachmann Trains",                                              // 101
    "Integrated Signal Systems",                                    // 102
    "Nagasue System Design Office",                                 // 103
    "TrainTech",                                                    // 104
    "Computer Dialysis France",                                     // 105
    "Opherline1",                                                   // 106
    "Phoenix Sound Systems, Inc.",                                  // 107
    "Nagoden",                                                      // 108
    "Viessmann Modellspielwaren GmbH",                              // 109
    "AXJ Electronics",                                              // 110
    "Haber & Koenig Electronics GmbH (HKE)",                        // 111
    "LSdigital",                                                    // 112
    "QS Industries (QSI)",                                          // 113
    "Benezan Electronics",                                          // 114
    "Dietz Modellbahntechnik",                                      // 115
    "MyLocoSound",                                                  // 116
    "cT Elektronik",                                                // 117
    "MT GmbH",                                                      // 118
    "W. S. Ataras Engineering",                                     // 119
    "csikos-muhely",                                                // 120
    (const char *) NULL,                                            // 121
    "Berros",                                                       // 122
    "Massoth Elektronik, GmbH",                                     // 123
    "DCC-Gaspar-Electronic",                                        // 124
    "ProfiLok Modellbahntechnik GmbH",                              // 125
    "Moellehem Gardsproduktion",                                    // 126
    "Atlas Model Railroad Products",                                // 127
    "Frateschi Model Trains",                                       // 128
    "Digitrax",                                                     // 129
    "cmOS Engineering",                                             // 130
    "Trix Modelleisenbahn",                                         // 131
    "ZTC",                                                          // 132
    "Intelligent Command Control",                                  // 133
    "LaisDCC",                                                      // 134
    "CVP Products",                                                 // 135
    "NYRS",                                                         // 136
    (const char *) NULL,                                            // 137
    "Train ID Systems",                                             // 138
    "RealRail Effects",                                             // 139
    "Desktop Station",                                              // 140
    "Throttle-Up (Soundtraxx)",                                     // 141
    "SLOMO Railroad Models",                                        // 142
    "Model Rectifier Corp.",                                        // 143
    "DCC Train Automation",                                         // 144
    "Zimo Elektronik",                                              // 145
    "Rails Europ Express",                                          // 146
    "Umelec Ing. Buero",                                            // 147
    "BLOCKsignalling",                                              // 148
    "Rock Junction Controls",                                       // 149
    "Wm. K. Walthers, Inc.",                                        // 150
    "Electronic Solutions Ulm GmbH",                                // 151
    "Digi-CZ",                                                      // 152
    "Train Control Systems",                                        // 153
    "Dapol Limited",                                                // 154
    "Gebr. Fleischmann GmbH & Co.",                                 // 155
    "Nucky",                                                        // 156
    "Kuehn Ing.",                                                   // 157
    "Fucik",                                                        // 158
    "LGB (Ernst Paul Lehmann Patentwerk)",                          // 159
    "MD Electronics",                                               // 160
    "Modelleisenbahn GmbH (formerly Roco)",                         // 161
    "PIKO",                                                         // 162
    "WP Railshops",                                                 // 163
    "drM",                                                          // 164
    "Model Electronic Railway Group",                               // 165
    "Maison de DCC",                                                // 166
    "Helvest Systems GmbH",                                         // 167
    "Model Train Technology",                                       // 168
    "AE Electronic Ltd.",                                           // 169
    "AuroTrains",                                                   // 170
    (const char *) NULL,                                            // 171
    (const char *) NULL,                                            // 172
    "Arnold - Rivarossi",                                           // 173
    (const char *) NULL,                                            // 174
    (const char *) NULL,                                            // 175
    (const char *) NULL,                                            // 176
    (const char *) NULL,                                            // 177
    (const char *) NULL,                                            // 178
    (const char *) NULL,                                            // 179
    (const char *) NULL,                                            // 180
    (const char *) NULL,                                            // 181
    (const char *) NULL,                                            // 182
    (const char *) NULL,                                            // 183
    (const char *) NULL,                                            // 185
    (const char *) NULL,                                            // 185
    "BRAWA Modellspielwaren GmbH & Co.",                            // 186
    (const char *) NULL,                                            // 187
    (const char *) NULL,                                            // 188
    (const char *) NULL,                                            // 189
    (const char *) NULL,                                            // 190
    (const char *) NULL,                                            // 191
    (const char *) NULL,                                            // 192
    (const char *) NULL,                                            // 193
    (const char *) NULL,                                            // 194
    (const char *) NULL,                                            // 195
    (const char *) NULL,                                            // 196
    (const char *) NULL,                                            // 197
    (const char *) NULL,                                            // 198
    (const char *) NULL,                                            // 199
    (const char *) NULL,                                            // 200
    (const char *) NULL,                                            // 201
    (const char *) NULL,                                            // 202
    (const char *) NULL,                                            // 203
    "Con-Com GmbH",                                                 // 204
    (const char *) NULL,                                            // 205
    (const char *) NULL,                                            // 206
    (const char *) NULL,                                            // 207
    (const char *) NULL,                                            // 208
    (const char *) NULL,                                            // 209
    (const char *) NULL,                                            // 210
    (const char *) NULL,                                            // 211
    (const char *) NULL,                                            // 212
    (const char *) NULL,                                            // 213
    (const char *) NULL,                                            // 214
    (const char *) NULL,                                            // 215
    (const char *) NULL,                                            // 216
    (const char *) NULL,                                            // 217
    (const char *) NULL,                                            // 218
    (const char *) NULL,                                            // 219
    (const char *) NULL,                                            // 220
    (const char *) NULL,                                            // 221
    (const char *) NULL,                                            // 222
    (const char *) NULL,                                            // 223
    (const char *) NULL,                                            // 224
    "Blue Digital",                                                 // 225
};

/*----------------------------------------------------------------------------------------------------------------------------------------
 * HTTP_Common::html_header () - send html header
 *----------------------------------------------------------------------------------------------------------------------------------------
 */
void
HTTP_Common::html_header (String browsertitle, String title, String url, bool use_utf8)
{
    String        mainbrowsertitle;
    String        maintitle;
    String        display_on;
    String        display_off;

    HTTP::response = "";

    String control_color        = "blue";
    String list_color           = "blue";
    String addon_color          = "blue";
    String switch_color         = "blue";
    String rr_color             = "blue";
    String switch_test_color    = "blue";
    String sig_color            = "blue";
    String sig_test_color       = "blue";
    String s88_color            = "blue";
    String rcl_color            = "blue";
    String led_color           = "blue";

    String pgm_color            = "blue";
    String pgm_info_color       = "blue";
    String pgm_addr_color       = "blue";
    String pgm_cv_color         = "blue";
    String pom_info_color       = "blue";
    String pom_addr_color       = "blue";
    String pom_cv_color         = "blue";
    String pom_map_color        = "blue";
    String pom_out_color        = "blue";

    String setup_color          = "blue";
    String system_color         = "blue";
    String network_color        = "blue";
    String upload_color         = "blue";
    String flash_color          = "blue";
    String info_color           = "blue";

    if (url.compare ("/") == 0 || url.compare ("/loco") == 0)
    {
        list_color = "red";
        control_color = "red";
    }
    else if (url.compare ("/addon") == 0)
    {
        addon_color = "red";
        control_color = "red";
    }
    else if (url.compare ("/switch") == 0)
    {
        switch_color = "red";
        control_color = "red";
    }
    else if (url.compare ("/rr") == 0)
    {
        rr_color = "red";
        control_color = "red";
    }
    else if (url.compare ("/swtest") == 0)
    {
        switch_test_color = "red";
        control_color = "red";
    }
    else if (url.compare ("/sig") == 0)
    {
        sig_color = "red";
        control_color = "red";
    }
    else if (url.compare ("/sigtest") == 0)
    {
        sig_test_color = "red";
        control_color = "red";
    }
    else if (url.compare ("/s88") == 0)
    {
        s88_color = "red";
        control_color = "red";
    }
    else if (url.compare ("/rcl") == 0)
    {
        rcl_color = "red";
        control_color = "red";
    }
    else if (url.compare ("/led") == 0)
    {
        led_color = "red";
        control_color = "red";
    }
    else if (url.compare ("/pgminfo") == 0)
    {
        pgm_info_color = "red";
        pgm_color = "red";
    }
    else if (url.compare ("/pgmaddr") == 0)
    {
        pgm_addr_color = "red";
        pgm_color = "red";
    }
    else if (url.compare ("/pgmcv") == 0)
    {
        pgm_cv_color = "red";
        pgm_color = "red";
    }
    else if (url.compare ("/pominfo") == 0)
    {
        pom_info_color = "red";
        pgm_color = "red";
    }
    else if (url.compare ("/pomaddr") == 0)
    {
        pom_addr_color = "red";
        pgm_color = "red";
    }
    else if (url.compare ("/pomcv") == 0)
    {
        pom_cv_color = "red";
        pgm_color = "red";
    }
    else if (url.compare ("/pommap") == 0)
    {
        pom_map_color = "red";
        pgm_color = "red";
    }
    else if (url.compare ("/pomout") == 0)
    {
        pom_out_color = "red";
        pgm_color = "red";
    }
    else if (url.compare ("/setup") == 0)
    {
        setup_color = "red";
        system_color = "red";
    }
    else if (url.compare ("/net") == 0)
    {
        network_color = "red";
        system_color = "red";
    }
    else if (url.compare ("/upl") == 0)
    {
        upload_color = "red";
        system_color = "red";
    }
    else if (url.compare ("/flash") == 0)
    {
        flash_color = "red";
        system_color = "red";
    }
    else if (url.compare ("/info") == 0)
    {
        info_color = "red";
        system_color = "red";
    }

    HTTP::response += (String)
        "<!DOCTYPE HTML>\r\n"
        "<html>\r\n"
        "<head>\r\n";

    if (use_utf8)
    {
        HTTP::response += (String) "<meta charset='UTF-8'>";
    }
    else
    {
        HTTP::response += (String) "<meta charset='ISO-8859-1'>";
    }

    if (browsertitle.length() == 0)
    {
        mainbrowsertitle = (String) "DCC FM22";
    }
    else
    {
        mainbrowsertitle = (String) "DCC FM22 - " + browsertitle;
    }

    HTTP::response += (String)
        "<title>" + mainbrowsertitle + "</title>\r\n"
        "<meta name='viewport' content='width=device-width,initial-scale=1'/>\r\n"
        "<style>\r\n"
        "BODY { FONT-FAMILY: Helvetica,Arial; FONT-SIZE: 14px; }\r\n"
        "A, U { text-decoration: none; }\r\n"
        "button, input[type=button], input[type=submit], input[type=reset] { background-color: #EEEEEE; border: 1px solid #AAAAEE; }\r\n"
        "button:hover, input[type=button]:hover, input[type=submit]:hover, input[type=reset] { background-color: #DDDDDD; }\r\n"
        "select { background-color: #FFFFFF; border: 1px solid #AAAAEE; }\r\n"
        "select:hover { background-color: #EEEEEE; }\r\n"
        ".estop { width:100%; height:32px; color:red; }\r\n"
        ".estop:hover { color:white; background-color:red; }\r\n";

    HTTP::flush ();

    HTTP::response += (String)
        "option.red {\r\n"
        "  /* background-color: #cc0000;*/\r\n"
        "  font-weight: bold;\r\n"
        "  font-size: 12px;\r\n"
        "  /* color: white; */\r\n"
        "}\r\n";

    /* On screens that are 650px or less, make content invisible */
    HTTP::response += (String)
        "@media screen and (max-width: 650px) { .hide650 { display:none; }}\r\n"
        "@media screen and (max-width: 600px) { .hide600 { display:none; }}\r\n"
        "@media screen and (max-width: 1000px) { .hide1000 { display:none; }}\r\n"
        "@media screen and (max-width: 1200px) { .hide1200 { display:none; }}\r\n"
        "@media screen and (max-width: 1800px) { .hide1800 { display:none; }}\r\n";

    HTTP::response += (String)
        "</style>\r\n"
        "</head>\r\n"
        "<body>\r\n"
        "<script>\r\n"
        "function estop()"
        "{"
        "  var http = new XMLHttpRequest(); http.open ('GET', '/action?action=estop'); http.send (null);"
        "  document.getElementById('estop').style.color = 'white';"
        "  document.getElementById('estop').style.backgroundColor = 'red';"
        "  setTimeout(function()"
        "  {"
        "    document.getElementById('estop').style.color = '';"
        "    document.getElementById('estop').style.backgroundColor = '';"
        "  }, 500);"
        "}\r\n"
        "window.onkeyup = function (event) { if (event.keyCode == 27) { estop(); }}\r\n"
        "function on() { var http = new XMLHttpRequest(); http.open ('GET', '/action?action=on'); http.send (null);}\r\n"
        "function off() { var http = new XMLHttpRequest(); http.open ('GET', '/action?action=off'); http.send (null);}\r\n"
        "</script>\r\n";

    if (DCC::booster_is_on)
    {
        display_off = " style='display:none'";
        display_on  = " style='display:inline-block'";
    }
    else
    {
        display_off = " style='display:inline-block'";
        display_on  = " style='display:none'";
    }

    HTTP::flush ();

    HTTP::response += (String)
        "<button class='estop' id='estop' onclick='estop()'>STOP</button><BR>"
        "<table>\r\n"
        "<tr>\r\n"
        "<td>"
        "<div id='onmenu'"  + display_on  + "><button style='width:100px;' onclick='off()'><font color='red'>Ausschalten</font></button></div>"
        "<div id='offmenu'" + display_off + "><button style='width:100px;' onclick='on()'><font color='green'>Einschalten</font></button></div>\r\n"
        "</td>\r\n"
        "<td style='padding:2px;'>\r\n"
        "  <select style='color:" + control_color + "' id='m_steuerung' onchange=\"window.location.href=document.getElementById(id).value;\">\r\n"
        "      <option value='' class='red'>Steuerung</option>\r\n"
        "      <option style='color:" + list_color + "' value='/loco'>Lokliste</option>\r\n"
        "      <option style='color:" + addon_color + "' value='/addon'>Zusatzdecoder</option>\r\n"
        "      <option style='color:" + switch_color + "' value='/switch'>Weichen</option>\r\n"
        "      <option style='color:" + switch_test_color + "' value='/swtest'>Weichentest</option>\r\n"
        "      <option style='color:" + rr_color + "' value='/rr'>Fahrstra&szlig;en</option>\r\n"
        "      <option style='color:" + sig_color + "' value='/sig'>Signale</option>\r\n"
        "      <option style='color:" + sig_test_color + "' value='/sigtest'>Signaltest</option>\r\n"
        "      <option style='color:" + led_color + "' value='/led'>LEDs</option>\r\n"
        "      <option style='color:" + s88_color + "' value='/s88'>S88</option>\r\n"
        "      <option style='color:" + rcl_color + "' value='/rcl'>RC-Detektoren</option>\r\n"
        "  </select>\r\n"
        "</td>\r\n";

    HTTP::flush ();

    if (HTTP_Common::edit_mode)
    {
        HTTP::response += (String)
            "<td style='padding:2px;'>\r\n"
            "  <select style='color:" + pgm_color + "' id='m_programmierung' onchange=\"window.location.href=document.getElementById('m_programmierung').value;\">\r\n"
            "    <option value='' class='red'>Programmierung</option>\r\n"
            "    <option value='' disabled='disabled'>Programmiergleis:</option>\r\n"
            "    <option style='color:" + pgm_info_color + "' value='/pgminfo'>&nbsp;&nbsp;Decoder-Info</option>\r\n"
            "    <option style='color:" + pgm_addr_color + "' value='/pgmaddr'>&nbsp;&nbsp;Decoder-Adresse</option>\r\n"
            "    <option style='color:" + pgm_cv_color + "' value='/pgmcv'>&nbsp;&nbsp;CV</option>\r\n"
            "    <option value='' disabled='disabled'>---------------------</option>\r\n"
            "    <option value='' disabled='disabled'>Hauptgleis:</option>\r\n"
            "    <option style='color:" + pom_info_color + ";margin-left:10px;' value='/pominfo'>&nbsp;&nbsp;Decoder-Info</option>\r\n"
            "    <option style='color:" + pom_addr_color + "' value='/pomaddr'>&nbsp;&nbsp;Decoder-Adresse</option>\r\n"
            "    <option style='color:" + pom_cv_color + "' value='/pomcv'>&nbsp;&nbsp;CV</option>\r\n"
            "    <option style='color:" + pom_map_color + "' value='/pommot'>&nbsp;&nbsp;Motorparameter</option>\r\n"
            "    <option style='color:" + pom_map_color + "' value='/pommap'>&nbsp;&nbsp;F-Mapping</option>\r\n"
            "    <option style='color:" + pom_out_color + "' value='/pomout'>&nbsp;&nbsp;F-Ausg&auml;nge</option>\r\n"
            "  </select>\r\n"
            "</td>\r\n";

        HTTP::flush ();
    }

    HTTP::response += (String)
        "<td style='padding:2px;'>\r\n"
        "  <select style='color:" + system_color + "' id='m_zentrale' onchange=\"window.location.href=document.getElementById('m_zentrale').value;\">\r\n"
        "    <option value='' class='red'>Zentrale</option>\r\n"
        "    <option style='color:" + setup_color + "' value='/setup'>Einstellungen</option>\r\n";

    if (HTTP_Common::edit_mode)
    {
        HTTP::response += (String)
            "<option value='' disabled='disabled'>----------------</option>\r\n"
            "<option style='color:" + network_color + "' value='/net'>Netzwerk</option>\r\n"
            "<option style='color:" + upload_color + "' value='/upl'>Upload Hex</option>\r\n"
            "<option style='color:" + flash_color + "' value='/flash'>Flash STM32</option>\r\n";
    }

    HTTP::response += (String)
        "    <option style='color:" + info_color + "' value='/info'>Info</option>\r\n"
        "  </select>\r\n"
        "</td>\r\n"
        "<td><button onclick=\"window.open('" + url + "', '_blank');\">+</button></td>\r\n"
        "<td id='iframe2' class='hide1200'><button onclick=\"window.location.href='/2';\">2</button></td>\r\n"
        "<td id='iframe3' class='hide1800'><button onclick=\"window.location.href='/3';\">3</button></td>\r\n"
        "<td id='iframe4' class='hide1200'><button onclick=\"window.location.href='/4';\">4</button></td>\r\n"
        "<td id='iframe6' class='hide1800'><button onclick=\"window.location.href='/6';\">6</button></td>\r\n"
        "</tr>\r\n"
        "</table>\r\n";

    HTTP::flush ();

    HTTP::response += (String)
        "<script>"
        "function isiFrame() { try { return window.self !== window.top; } catch (e) { return true; } }\r\n"
        "if (isiFrame()) {\r\n"
        "  document.getElementById('estop').style.display = 'none';\r\n"
        "  document.getElementById('iframe2').style.display = 'none';\r\n"
        "  document.getElementById('iframe3').style.display = 'none';\r\n"
        "  document.getElementById('iframe4').style.display = 'none';\r\n"
        "  document.getElementById('iframe6').style.display = 'none';\r\n"
        "}\r\n"
        "function rstalert() { var http = new XMLHttpRequest(); http.open ('GET', '/action?action=rstalert');"
        "http.addEventListener('load',"
        "function(event) { if (http.status >= 200 && http.status < 300) { ; }});"
        "http.send (null);}\r\n"
        "</script>\r\n";

    HTTP::flush ();

    HTTP::response += (String) "<div style='margin-left:20px'>RC1: <span id='rc1' style='margin-right:20px'>";

    if (DCC::rc1_value == 0xFFFF)
    {
        HTTP::response += (String) "-----";
    }
    else
    {
        HTTP::response += std::to_string (DCC::rc1_value);
    }

    HTTP::response += (String)
        "</span>"
        "Strom: <span style='display:inline-block; width:60px;' id='adc'>" + std::to_string(DCC::adc_value) + "</span>"
        "<progress id='adc2' max='4096' value='0'></progress></div>"
        "<div style='margin-top:10px;padding-top:2px;padding-bottom:2px;border:1px gray solid;width:400px;display:none' id='alert'>"
        "<span style='margin-left:10px;color:red;font-weight:bold;' id='alertmsg'>";

    HTTP::response += (String) HTTP_Common::alert_msg;

    HTTP::response += (String)
        "</span><span style='margin-left:10px;float:right'><button onclick='rstalert()'>L&ouml;schen</button></span></div>\r\n"
        "<H3 style='margin-left:20px;'>" + title + "</H3>\r\n";

    HTTP::flush ();
}

/*----------------------------------------------------------------------------------------------------------------------------------------
 * HTTP_Common::html_trailer () - send html trailer
 *----------------------------------------------------------------------------------------------------------------------------------------
 */
void
HTTP_Common::html_trailer (void)
{
    HTTP::response += (String)
        "</body>\r\n"
        "</html>\r\n";
    HTTP::flush ();
}

/*----------------------------------------------------------------------------------------------------------------------------------------
 * head_action ()
 *----------------------------------------------------------------------------------------------------------------------------------------
 */
void
HTTP_Common::head_action (void)
{
    if (DCC::booster_is_on)
    {
        HTTP_Common::add_action_content ("onmenu",  "display", "");
        HTTP_Common::add_action_content ("offmenu", "display", "none");
        HTTP_Common::add_action_content ("ison",    "display", "");
        HTTP_Common::add_action_content ("isoff",   "display", "none");
    }
    else
    {
        HTTP_Common::add_action_content ("onmenu",  "display", "none");
        HTTP_Common::add_action_content ("offmenu", "display", "");
        HTTP_Common::add_action_content ("ison",    "display", "none");
        HTTP_Common::add_action_content ("isoff",   "display", "");
    }

    HTTP_Common::add_action_content ("adc",  "text", std::to_string (DCC::adc_value));
    HTTP_Common::add_action_content ("adc2", "value", std::to_string (DCC::adc_value));

    if (DCC::rc1_value == 0xFFFF)
    {
        HTTP_Common::add_action_content ("rc1",  "text", "-----");
    }
    else
    {
        HTTP_Common::add_action_content ("rc1",  "text", std::to_string (DCC::rc1_value));
    }

    if (! HTTP_Common::alert_msg.empty())
    {
        HTTP_Common::add_action_content ("alertmsg", "text", HTTP_Common::alert_msg);
        HTTP_Common::add_action_content ("alert", "display", "");
        HTTP_Common::add_action_content ("rstalert", "display", "");
    }
    else
    {
        HTTP_Common::add_action_content ("alert", "display", "none");
        HTTP_Common::add_action_content ("rstalert", "display", "none");
    }
}

/*----------------------------------------------------------------------------------------------------------------------------------------
 * handle_info ()
 *----------------------------------------------------------------------------------------------------------------------------------------
 */
void
HTTP_Common::handle_info (void)
{
    String          title       = "";
    String          url     = "/info";

    HTTP_Common::html_header (title, title, url, true);
    HTTP::response += (String) "<div style='margin-left:20px;'>\r\n";
    HTTP_Common::add_action_handler ("head", "", 200, true);

    HTTP::response += (String)
        "Version DCC-Zentrale: " + VERSION + "<BR>\r\n"
        "</span>\r\n"
        "</div>\r\n";
    HTTP_Common::html_trailer ();
}

/*----------------------------------------------------------------------------------------------------------------------------------------
 * handle_setup ()
 *----------------------------------------------------------------------------------------------------------------------------------------
 */
void
HTTP_Common::handle_setup (void)
{
    String          title   = "Einstellungen";
    String          url     = "/setup";
    const char *    action;
    uint_fast8_t    railcom_mode;
    uint_fast16_t   shortcut_value;
    uint_fast8_t    deadtime;
    String          checked_edit = "";
    String          checked_railcom = "";

    action = HTTP::parameter ("action");

    HTTP_Common::html_header (title, title, url, true);
    HTTP::response += (String) "<div style='margin-left:20px;'>\r\n";

    if (! strcmp (action, "savesetup"))
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
    else if (strcmp (action, "setedit") == 0)                            // action "setedit" must be handled before html_header()!
    {
        HTTP_Common::edit_mode = HTTP::parameter_number ("edit");
    }
    else if (strcmp (action, "setshortcut") == 0)
    {
        shortcut_value = HTTP::parameter_number ("shortcut");

        FM22::set_shortcut_value (shortcut_value);
        DCC::set_shortcut_value (shortcut_value);
    }
    else if (strcmp (action, "setrailcom") == 0)
    {
        railcom_mode = HTTP::parameter_number ("railcom");

        if (railcom_mode)
        {
            DCC::set_mode (RAILCOM_MODE);
        }
        else
        {
            DCC::set_mode (NO_RAILCOM_MODE);
        }
    }
    else if (strcmp (action, "setdeadtime") == 0)
    {
        todo_speed_deadtime = HTTP::parameter_number ("deadtime");
    }

    HTTP_Common::add_action_handler ("head", "", 200, true);

    if (HTTP_Common::edit_mode)
    {
        checked_edit = "checked";
    }

    shortcut_value = FM22::get_shortcut_value ();

    if (DCC::get_mode () == RAILCOM_MODE)
    {
        railcom_mode = 1;
    }
    else
    {
        railcom_mode = 0;
    }

    deadtime        = todo_speed_deadtime;

    if (railcom_mode)
    {
        checked_railcom = "checked";
    }

    if (HTTP_Common::edit_mode && FM22::data_changed)
    {
        HTTP::response += (String)
            "<BR><form method='get' action='" + url + "'>\r\n"
            "<input type='hidden' name='action' value='savesetup'>\r\n"
            "<input type='submit' value='Ge&auml;nderte Konfiguration auf Datentr&auml;ger speichern'>"
            "</form>\r\n"
            "<P>";
    }

    HTTP::response += (String)
        "<form method='GET' action='" + url + "'>\r\n"
        " <div style='padding:10px;width:400px;border:1px lightgray solid'>\r\n"
        "  <B>Bearbeitung:</B><P>"
        "  Bearbeitung aktiv: <input type='checkbox' name='edit' value='1' " + checked_edit + ">\r\n"
        "  <input type='hidden' name='action' value='setedit'>\r\n"
        "  <div style='text-align:right;'><input type='submit' style='margin-left:40px' value='Speichern'></div>\r\n"
        "</div></form>\r\n"
        "<P>";

    HTTP::flush ();

    if (HTTP_Common::edit_mode)
    {
        HTTP::response += (String)
            "<script>"
            "function upd_shortcut()"
            "{"
            "  var slider = document.getElementById('shortcut');"
            "  var shortcut_out = document.getElementById('shortcut_out');"
            "  var value = parseInt(slider.value);"
            "  shortcut_out.textContent = value;"
            "}"
            "function set_shortcut(value)"
            "{"
            "  var slider = document.getElementById('shortcut');"
            "  var shortcut_out = document.getElementById('shortcut_out');"
            "  slider.value = value;"
            "  shortcut_out.textContent = value;"
            "}"
            "</script>\r\n"
            "<form method='GET' action='" + url + "'>\r\n"
            " <div style='padding:10px;width:400px;border:1px lightgray solid'>\r\n"
            "  <B>Kurzschlusswert (Standardwert: " + std::to_string(FM22_SHORTCUT_DEFAULT) + "):</B><P>"
            "  <div id='shortcut_out' style='width:350px;text-align:center;'>" + std::to_string(shortcut_value) + "</div>\r\n"
            "  128\r\n"
            "  <input type='range' min='128' max='2047' value='" + std::to_string(shortcut_value) + "' name='shortcut' id='shortcut' style='width:256px' onchange='upd_shortcut()'>\r\n"
            "  2047\r\n"
            "  <input type='hidden' name='action' value='setshortcut'>\r\n"
            "  <BR><BR>"
            "  <div>"
            "    <div style='float: left;'><button onclick='set_shortcut(1000)'>Standard</button></div>\r\n"
            "    <div style='float: right;'><input type='submit' style='margin-left:40px' value='Speichern'></div>\r\n"
            "    <BR>"
            "  </div>"
            "</div></form>\r\n"
            "<P>";

        HTTP::flush ();

        HTTP::response += (String)
            "<form method='GET' action='" + url + "'>\r\n"
            " <div style='padding:10px;width:400px;border:1px lightgray solid'>\r\n"
            "  <B>RailCom:</B><P>"
            "  RailCom: <input type='checkbox' name='railcom' value='1' " + checked_railcom + ">\r\n"
            "  <input type='hidden' name='action' value='setrailcom'>\r\n"
            "  <div style='text-align:right;'><input type='submit' style='margin-left:40px' value='Speichern'></div>\r\n"
            "</div></form>\r\n"
            "<P>";

        HTTP::flush ();

        HTTP::response += (String)
            "<script>function upd_slider() { var slider = document.getElementById('deadtime'); var deadtime_out = document.getElementById('deadtime_out'); "
            "var value = parseInt(slider.value); deadtime_out.textContent = value; }</script>\r\n"
            "<form method='GET' action='" + url + "'>\r\n"
            " <div style='padding:10px;width:400px;border:1px lightgray solid'>\r\n"
            "  <B>Geschwindigkeitssteuerung Wii-Gamepad:</B><P>"
            "  <div id='deadtime_out' style='width:350px;text-align:center;'>" + std::to_string(deadtime) + "</div>\r\n"
            "  Direkt\r\n"
            "  <input type='range' min='0' max='255' value='" + std::to_string(deadtime) + "' name='deadtime' id='deadtime' style='width:256px' onchange='upd_slider()'>\r\n"
            "  Sanft\r\n"
            "  <input type='hidden' name='action' value='setdeadtime'>\r\n"
            "  <BR><BR>"
            "  <div style='text-align:right;'><input type='submit' style='margin-left:40px' value='Speichern'></div>\r\n"
            "</div></form>\r\n";

        HTTP::flush ();
    }

    HTTP::response += (String) "</div>\r\n";
    HTTP_Common::html_trailer ();
}

/*----------------------------------------------------------------------------------------------------------------------------------------
 * add_action_content ()
 *----------------------------------------------------------------------------------------------------------------------------------------
 */
void
HTTP_Common::add_action_content (String id, String type, String value)
{
    HTTP::response += id + "\b" + type + "\b" + value + "\t";
}

void
HTTP_Common::add_action_handler (const char * action, const char * parameters, int msec, bool do_print_script_tag)
{
    String sparam;

    if (parameters && *parameters)
    {
        sparam = (String) "&" + parameters;
    }
    else
    {
        sparam = "";
    }

    HTTP::flush ();

    if (do_print_script_tag)
    {
        HTTP::response += (String) "<script>\r\n";
    }

    HTTP::response += (String)
        "function action_handler_" + action + "() {\r\n"
        "  var http = new XMLHttpRequest(); http.open ('GET', '/action?action=" + action + sparam + "');\r\n"
        "  http.addEventListener('load',\r\n"
        "    function(event) {\r\n"
        "      var i; var j;\r\n"
        "      var text = http.responseText;\r\n"
        "      if (http.status >= 200 && http.status < 300) {\r\n"
        "        const a = text.split('\t');\r\n"
        "        var l = a.length - 1;\r\n"
        "        for (i = 0; i < l; i++) {\r\n"
        "          const b = a[i].split('\b');\r\n"
        "          var oo = document.getElementById(b[0]);\r\n"
        "          if (oo) {\r\n"
        "            switch (b[1])\r\n"
        "            {\r\n"
        "              case 'display':\r\n"
        "                oo.style.display = b[2];\r\n"
        "                break;\r\n"
        "              case 'value':\r\n"
        "                oo.value = b[2];\r\n"
        "                break;\r\n"
        "              case 'text':\r\n"
        "                oo.textContent = b[2];\r\n"
        "                break;\r\n"
        "              case 'width':\r\n"
        "                oo.style.width = b[2];\r\n"
        "                break;\r\n"
        "              case 'html':\r\n"
        "                oo.innerHTML = b[2];\r\n"
        "                break;\r\n"
        "              case 'checked':\r\n"
        "                oo.checked = parseInt(b[2]);\r\n"
        "                break;\r\n"
        "              case 'color':\r\n"
        "                oo.style.color = b[2];\r\n"
        "                break;\r\n"
        "              case 'bgcolor':\r\n"
        "                oo.style.backgroundColor = b[2];\r\n"
        "                break;\r\n"
        "              default:\r\n"
        "                console.log ('invalid type: ' + b[1]);\r\n"
        "                break;\r\n"
        "            }\r\n"
        "          }\r\n"
        "        }\r\n"
        "      }\r\n"
        "    }\r\n"
        "  );\r\n"
        "  http.send (null);\r\n"
        "}\r\n"
        "var intervalId" + action + " = window.setInterval(function(){ action_handler_" + action + " (); }, " + std::to_string(msec) + ");\r\n";

    if (do_print_script_tag)
    {
        HTTP::response += (String) "</script>\r\n";
    }

    HTTP::flush ();
}

void
HTTP_Common::add_action_handler (const char * action, const char * parameters, int msec)
{
    HTTP_Common::add_action_handler (action, parameters, msec, false);
}

/*----------------------------------------------------------------------------------------------------------------------------------------
 * print_id_select_list ()
 *----------------------------------------------------------------------------------------------------------------------------------------
 */
void
HTTP_Common::print_id_select_list (String name, uint_fast16_t n_entries)
{
    uint_fast16_t i;

    HTTP::response += (String)
        "<td>Neue ID:</td><td>"
        "<select id='" + name + "' name='" + name + "'>\r\n";

    for (i = 0; i < n_entries; i++)
    {
        HTTP::response += (String) "<option value='" + std::to_string(i) + "'>" + std::to_string(i) + "</option>\r\n";
    }

    HTTP::response += (String)
        "</select>\r\n"
        "</td>\r\n";
}

/*----------------------------------------------------------------------------------------------------------------------------------------
 * print_id_select_list ()
 *----------------------------------------------------------------------------------------------------------------------------------------
 */
void
HTTP_Common::print_id_select_list (String name, uint_fast16_t n_entries, uint_fast16_t selected_idx)
{
    uint_fast16_t i;

    HTTP::response += (String)
        "<td>Neue ID:</td><td>"
        "<select id='" + name + "' name='" + name + "'>\r\n";

    for (i = 0; i < n_entries; i++)
    {
        if (i == selected_idx)
        {
            HTTP::response += (String) "<option value='" + std::to_string(i) + "' selected>" + std::to_string(i) + "</option>\r\n";
        }
        else
        {
            HTTP::response += (String) "<option value='" + std::to_string(i) + "'>" + std::to_string(i) + "</option>\r\n";
        }
    }

    HTTP::response += (String)
        "</select>\r\n"
        "</td>\r\n";
}

/*----------------------------------------------------------------------------------------------------------------------------------------
 * print_loco_select_list ()
 *----------------------------------------------------------------------------------------------------------------------------------------
 */
void
HTTP_Common::print_loco_select_list (String name, uint_fast16_t selected_idx, uint_fast16_t show_mask)
{
    String          style;

    if (! (show_mask & LOCO_LIST_DO_DISPLAY))
    {
        style = "style='display:none'";
    }

    HTTP::response += (String) "<select " + style + " name='" + name + "' id='" + name + "'>\r\n";

    if (show_mask & LOCO_LIST_SHOW_NONE_LOCO)
    {
        HTTP::response += (String) "<option value='65535'>--- Keine Lok ---</option>\r\n";
    }

    if (show_mask & LOCO_LIST_SHOW_LINKED_LOCO)
    {
        HTTP::response += (String) "<option value='65535'";

        if (selected_idx == 0xFFFF)
        {
            HTTP::response += (String) " selected";
        }

        HTTP::response += (String) ">(Verkn&uuml;pfte Lok)</option>\r\n";
    }
    else if (show_mask & LOCO_LIST_SHOW_THIS_DETECTED_LOCO)
    {
        HTTP::response += (String) "<option value='65535'";

        if (selected_idx == 0xFFFF)
        {
            HTTP::response += (String) " selected";
        }

        HTTP::response += (String) ">(Erkannte Lok)</option>\r\n";
    }

    if (show_mask & LOCO_LIST_SHOW_DETECTED_LOCO)
    {
        uint_fast8_t    n_tracks = RCL::get_n_tracks ();
        uint_fast8_t    track_idx;

        for (track_idx = 0; track_idx < n_tracks; track_idx++)
        {
            HTTP::response += (String) "<option value='" + std::to_string(S88_RCL_DETECTED_OFFSET + track_idx) + "'";

            if (selected_idx == S88_RCL_DETECTED_OFFSET + track_idx)
            {
                HTTP::response += (String) " selected";
            }

            HTTP::response += (String) ">(Erkannte Lok '" + RCL::tracks[track_idx].get_name() + "')</option>\r\n";
        }
    }
    else
    {
        HTTP::response += (String) ">-- Keine --</option>\r\n";
    }

    uint_fast16_t   n_locos = Locos::get_n_locos ();
    uint_fast16_t   lidx;

    for (lidx = 0; lidx < n_locos; lidx++)
    {
        std::string name = Locos::locos[lidx].get_name();

        HTTP::response += (String) "<option value='" + std::to_string(lidx) + "'";

        if (lidx == selected_idx)
        {
            HTTP::response += (String) " selected";
        }

        HTTP::response += (String) ">" + name + "</option>\r\n";
    }

    HTTP::response += (String) "</select>\r\n";
}

/*----------------------------------------------------------------------------------------------------------------------------------------
 * print_addon_select_list ()
 *----------------------------------------------------------------------------------------------------------------------------------------
 */
void
HTTP_Common::print_addon_select_list (String name, uint_fast16_t selected_idx, uint_fast16_t show_mask)
{
    String          style;
    uint_fast16_t   n_addons = AddOns::get_n_addons ();
    uint_fast16_t   aidx;

    if (! (show_mask & ADDON_LIST_DO_DISPLAY))
    {
        style = "style='display:none'";
    }

    HTTP::response += (String) "<select " + style + " name='" + name + "' id='" + name + "'>\r\n";

    for (aidx = 0; aidx < n_addons; aidx++)
    {
        std::string name = AddOns::addons[aidx].get_name ();

        HTTP::response += (String) "<option value='" + std::to_string(aidx) + "'";

        if (aidx == selected_idx)
        {
            HTTP::response += (String) " selected";
        }

        HTTP::response += (String) ">" + name + "</option>\r\n";
    }

    HTTP::response += (String) "</select>\r\n";
}

#if 00
/*----------------------------------------------------------------------------------------------------------------------------------------
 * print_addon_select_list ()
 *----------------------------------------------------------------------------------------------------------------------------------------
 */
static void
print_addon_select_list (String name, uint_fast16_t selected_idx, uint_fast16_t show_mask)
{
    String          style;

    if (! (show_mask & ADDON_LIST_DO_DISPLAY))
    {
        style = "style='display:none'";
    }

    HTTP::response += (String) "<select " + style + " name='" + name + "' id='" + name + "'>\r\n";

    uint_fast16_t   n_addons = AddOns::get_n_addons ();
    uint_fast16_t   aidx;

    for (aidx = 0; aidx < n_addons; aidx++)
    {
        char * name = AddOns::addons[aidx].get_name ();

        HTTP::response += (String) "<option value='" + std::to_string(aidx) + "'";

        if (aidx == selected_idx)
        {
            HTTP::response += (String) " selected";
        }

        HTTP::response += (String) ">" + name + "</option>\r\n";
    }

    HTTP::response += (String) "</select>\r\n";
}

#endif // 00

#if 000
/*----------------------------------------------------------------------------------------------------------------------------------------
 * print_led_group_select_list ()
 *----------------------------------------------------------------------------------------------------------------------------------------
 */
void
HTTP_Common::print_led_group_select_list (String name, uint_fast16_t selected_idx, bool do_display)
{
    String          style;

    if (! do_display)
    {
        style = "style='display:none'";
    }

    HTTP::response += (String) "<select " + style + " name='" + name + "' id='" + name + "'>\r\n";

    HTTP::response += (String) "<option value='255'>-- Keine --</option>\r\n";

    uint_fast16_t   n_led_groups = Leds::get_n_led_groups ();
    uint_fast16_t   led_group_idx;

    for (led_group_idx = 0; led_group_idx < n_led_groups; led_group_idx++)
    {
        std::string name = Leds::led_groups[led_group_idx].get_name();

        HTTP::response += (String) "<option value='" + std::to_string(led_group_idx) + "'";

        if (led_group_idx == selected_idx)
        {
            HTTP::response += (String) " selected";
        }

        HTTP::response += (String) ">" + name + "</option>\r\n";
    }

    HTTP::response += (String) "</select>\r\n";
}
#endif

std::string
HTTP_Common::get_location (uint_fast16_t loco_idx)
{
    uint_fast8_t    rcllocation;
    uint_fast16_t   rrlocation;
    std::string     slocation;

    rcllocation = Locos::locos[loco_idx].get_rcllocation ();

    if (rcllocation != 0xFF)
    {
        if (rcllocation != 0xFF)
        {
            slocation = RCL::tracks[rcllocation].get_name();
        }
        else
        {
            slocation = "";
        }
    }
    else
    {
        rrlocation = Locos::locos[loco_idx].get_rrlocation ();

        if (rrlocation != 0xFFFF)
        {
            uint_fast8_t    rrgidx      = rrlocation >> 8;
            uint_fast8_t    rridx       = rrlocation & 0xFF;
            String          s1          = RailroadGroups::railroad_groups[rrgidx].get_name();
            String          s2          = RailroadGroups::railroad_groups[rrgidx].railroads[rridx].get_name();

            slocation = s1 + " - " + s2;
        }
        else
        {
            slocation = "";
        }
    }

    return slocation;
}

void
HTTP_Common::print_speed_list (String name, uint_fast16_t selected_speed, bool do_display)
{
    String          style;
    uint_fast8_t    speed;

    if (! do_display)
    {
        style = "style='display:none'";
    }

    HTTP::response += (String) "<select " + style + " name='" + name + "' id='" + name + "'>\r\n";

    for (speed = 0; speed < 128; speed++)
    {
        HTTP::response += (String) "<option value='" + std::to_string(speed) + "'";

        if (speed == selected_speed)
        {
            HTTP::response += (String) " selected";
        }

        HTTP::response += (String) ">auf " + std::to_string(speed) + "</option>\r\n";
    }

    HTTP::response += (String) "</select>\r\n";
}

void
HTTP_Common::print_start_list (String name, uint_fast16_t selected_start, bool do_display)
{
    char            buf[32];
    String          style;
    uint_fast16_t   start;

    if (! do_display)
    {
        style = "style='display:none'";
    }

    HTTP::response += (String) "<select " + style + " name='" + name + "' id='" + name + "'>\r\n";

    start = 0;

    while (start <= 36000)
    {
        HTTP::response += (String) "<option value='" + std::to_string(start) + "'";

        if (start == selected_start)
        {
            HTTP::response += (String) " selected";
        }

        if (start == 0)
        {
            strcpy (buf, "--- sofort ---");
        }
        else
        {
            if (start < 1200)
            {
                sprintf (buf, "nach %0.1f sec", (float) start / 10);
            }
            else if (start < 3000)
            {
                sprintf (buf, "nach %u sec", (unsigned int) (start / 10));
            }
            else
            {
                sprintf (buf, "nach %u min", (unsigned int) (start / 600));
            }
        }

        HTTP::response += (String) ">" + buf + "</option>\r\n";

        if (start < 50)             // 0...5 sec: step = 0.1 sec
        {
            start++;
        }
        else if (start < 600)       // 5...60 sec: step = 0.5 sec
        {
            start += 5;
        }
        else if (start < 1200)      // 60...120 sec: step = 1 sec
        {
            start += 10;
        }
        else if (start < 3000)      // 120...300 sec: step = 10 sec
        {
            start += 100;
        }
        else                        // 300...3600 sec: step = 60 sec
        {
            start += 600;
        }
    }

    HTTP::response += (String) "</select>\r\n";
}

void
HTTP_Common::print_tenths_list (String name, uint_fast16_t selected_tenths, bool do_display)
{
    char            buf[32];
    String          style;
    uint_fast16_t   tenths;

    if (! do_display)
    {
        style = "style='display:none'";
    }

    HTTP::response += (String) "<select " + style + " name='" + name + "' id='" + name + "'>\r\n";

    tenths = 0;

    while (tenths <= 200)
    {
        HTTP::response += (String) "<option value='" + std::to_string(tenths) + "'";

        if (tenths == selected_tenths)
        {
            HTTP::response += (String) " selected";
        }

        sprintf (buf, "Rampe: %0.1f sec", (float) tenths / 10);

        HTTP::response += (String) ">" + buf + "</option>\r\n";

        if (tenths < 50)
        {
            tenths++;
        }
        else
        {
            tenths += 5;
        }

    }

    HTTP::response += (String) "</select>\r\n";
}

void
HTTP_Common::print_fwd_list (String name, uint_fast8_t selected_fwd, bool do_display)
{
    String          style;
    String          selected0;
    String          selected1;

    if (! do_display)
    {
        style = "style='display:none'";
    }

    if (selected_fwd)
    {
        selected0 = "";
        selected1 = "selected";
    }
    else
    {
        selected0 = "selected";
        selected1 = "";
    }


    HTTP::response += (String)
        "<select " + style + " name='" + name + "' id='" + name + "'>\r\n"
        "<option value='0' " + selected0 + ">R&uuml;ckw&auml;rts</option>\r\n"
        "<option value='1' " + selected1 + ">Vorw&auml;rts</option>\r\n"
        "</select>\r\n";
}

void
HTTP_Common::print_loco_function_list (String name, uint_fast16_t selected_f, bool do_display)
{
    String          style;
    uint_fast8_t    f;

    if (! do_display)
    {
        style = "style='display:none'";
    }

    HTTP::response += (String) "<select " + style + " name='" + name + "' id='" + name + "'>\r\n";

    for (f = 0; f < MAX_LOCO_FUNCTIONS; f++)
    {
        HTTP::response += (String) "<option value='" + std::to_string(f) + "'";

        if (f == selected_f)
        {
            HTTP::response += (String) " selected";
        }

        HTTP::response += (String) ">F" + std::to_string(f) + "</option>\r\n";
    }

    HTTP::response += (String) "</select>\r\n";
}

void
HTTP_Common::print_loco_function_list (uint_fast16_t loco_or_addon_idx, bool is_loco, String name, uint_fast16_t selected_fidx, bool do_display)
{
    String          style;
    String          funcname;
    uint_fast8_t    fidx;

    if (! do_display)
    {
        style = "style='display:none'";
    }

    HTTP::response += (String) "<select " + style + " name='" + name + "' id='" + name + "'>\r\n";

    for (fidx = 0; fidx < MAX_LOCO_FUNCTIONS; fidx++)
    {
        uint_fast16_t  function_name_idx;

        if (is_loco)
        {
            function_name_idx = Locos::locos[loco_or_addon_idx].get_function_name_idx (fidx);
        }
        else
        {
            function_name_idx = AddOns::addons[loco_or_addon_idx].get_function_name_idx (fidx);
        }

        if (function_name_idx != 0xFFFF)
        {
            if (is_loco)
            {
                funcname = Locos::locos[loco_or_addon_idx].get_function_name (fidx);
            }
            else
            {
                funcname = AddOns::addons[loco_or_addon_idx].get_function_name (fidx);
            }

            HTTP::response += (String) "<option value='" + std::to_string(fidx) + "'";

            if (fidx == selected_fidx)
            {
                HTTP::response += (String) " selected";
            }

            HTTP::response += (String) ">F" + std::to_string(fidx) + " " + funcname + "</option>\r\n";
        }
    }

    HTTP::response += (String) "</select>\r\n";
}

void
HTTP_Common::print_loco_macro_list (String name, uint_fast16_t selected_m, bool do_display)
{
    String          style;
    uint_fast8_t    m;

    if (! do_display)
    {
        style = "style='display:none'";
    }

    HTTP::response += (String) "<select " + style + " name='" + name + "' id='" + name + "'>\r\n";

    for (m = 0; m < MAX_LOCO_MACROS_PER_LOCO; m++)
    {
        HTTP::response += (String) "<option value='" + std::to_string(m) + "'";

        if (m == selected_m)
        {
            HTTP::response += (String) " selected";
        }

        if (m == 0)
        {
            HTTP::response += (String) ">MF</option>\r\n";
        }
        else if (m == 1)
        {
            HTTP::response += (String) ">MH</option>\r\n";
        }
        else
        {
            HTTP::response += (String) ">M" + std::to_string(m + 1) + "</option>\r\n";
        }
    }

    HTTP::response += (String) "</select>\r\n";
}

void
HTTP_Common::print_railroad_group_list (String name, uint_fast8_t rrgidx, bool do_display_norrg, bool do_display)
{
    String          style;
    uint_fast8_t    n_railroad_groups   = RailroadGroups::get_n_railroad_groups ();
    uint_fast8_t    idx;

    if (! do_display)
    {
        style = "style='display:none'";
    }
    else
    {
        style = "";
    }

    HTTP::response += (String) "<select " + style + " name='" + name + "' id='" + name + "'>\r\n";

    if (do_display_norrg)
    {
        String  selected;

        if (rrgidx == 0xFF)
        {
            selected = "selected";
        }
        else
        {
            selected = "";
        }

        HTTP::response += (String) "<option value='255' " + selected + ">---</option>\r\n";
    }

    for (idx = 0; idx < n_railroad_groups; idx++)
    {
        RailroadGroup * rrg = &RailroadGroups::railroad_groups[idx];
        String          selected;

        if (idx == rrgidx)
        {
            selected = "selected";
        }
        else
        {
            selected = "";
        }

        HTTP::response += (String) "<option value='" + std::to_string(idx) + "' " + selected + ">" + rrg->get_name() + "</option>\r\n";
    }

    HTTP::response += (String) "</select>\r\n";
}

void
HTTP_Common::print_railroad_list (String name, uint_fast16_t linkedrr, bool do_allow_none, bool do_display)
{
    String          style;
    uint_fast8_t    n_railroad_groups   = RailroadGroups::get_n_railroad_groups ();
    uint_fast8_t    rrgidx;
    uint_fast8_t    rridx;

    if (! do_display)
    {
        style = "style='display:none'";
    }
    else
    {
        style = "";
    }

    HTTP::response += (String) "<select " + style + " name='" + name + "' id='" + name + "'>\r\n";

    if (do_allow_none)
    {
        HTTP::response += (String) "<option value='65535'>--- Kein Gleis ---</option>\r\n";
    }

    for (rrgidx = 0; rrgidx < n_railroad_groups; rrgidx++)
    {
        RailroadGroup * rrg         = &RailroadGroups::railroad_groups[rrgidx];
        uint_fast8_t    n_railroads = rrg->get_n_railroads();

        for (rridx = 0; rridx < n_railroads; rridx++)
        {
            String          selected;
            String          sloco_name;
            Railroad *      rr          = &(rrg->railroads[rridx]);
            uint_fast16_t   loco_idx    = rr->get_link_loco ();
            uint_fast16_t   lrr         = (rrgidx << 8) | rridx;

            if (loco_idx != 0xFFFF)
            {
                sloco_name = std::to_string(loco_idx) + " " + Locos::locos[loco_idx].get_name ();
            }
            else
            {
                sloco_name = "---";
            }

            if (lrr == linkedrr)
            {
                selected = "selected";
            }
            else
            {
                selected = "";
            }

            HTTP::response  += (String) "<option value='" + std::to_string(lrr) + "' " + selected + ">"
                            + rrg->get_name() + " - " + rr->get_name() + " (" + sloco_name + ")</option>\r\n";
        }
    }

    HTTP::response += (String) "</select>\r\n";
}

void
HTTP_Common::print_s88_list (String name, uint_fast16_t coidx, bool do_display)
{
    String          style;
    uint_fast16_t   n_contacts  = S88::get_n_contacts ();
    uint_fast16_t   idx;

    if (! do_display)
    {
        style = "style='display:none'";
    }
    else
    {
        style = "";
    }

    HTTP::response += (String) "<select " + style + " name='" + name + "' id='" + name + "'>\r\n";

    for (idx = 0; idx < n_contacts; idx++)
    {
        String          selected;

        if (idx == coidx)
        {
            selected = "selected";
        }
        else
        {
            selected = "";
        }

        HTTP::response += (String) "<option value='" + std::to_string(idx) + "' " + selected + ">" + S88::contacts[idx].get_name() + "</option>\r\n";
    }

    HTTP::response += (String) "</select>\r\n";
}

void
HTTP_Common::print_switch_list (String name, uint_fast16_t swidx, bool do_display)
{
    String          style;
    uint_fast16_t   n_switches = Switches::get_n_switches ();
    uint_fast16_t   idx;

    if (! do_display)
    {
        style = "style='display:none'";
    }
    else
    {
        style = "";
    }

    HTTP::response += (String) "<select " + style + " name='" + name + "' id='" + name + "'>\r\n";

    for (idx = 0; idx < n_switches; idx++)
    {
        String          selected;

        if (idx == swidx)
        {
            selected = "selected";
        }
        else
        {
            selected = "";
        }

        HTTP::response += (String) "<option value='" + std::to_string(idx) + "' " + selected + ">" + Switches::switches[idx].get_name() + "</option>\r\n";
    }

    HTTP::response += (String) "</select>\r\n";
}

void
HTTP_Common::print_switch_state_list (String name, uint_fast8_t swstate, bool do_display)
{
    String          style;
    String          selected_branch;
    String          selected_straight;
    String          selected_branch2;

    if (swstate == DCC_SWITCH_STATE_BRANCH)
    {
        selected_branch = "selected";
    }
    else
    {
        selected_branch = "";
    }

    if (swstate == DCC_SWITCH_STATE_BRANCH)
    {
        selected_straight = "selected";
    }
    else
    {
        selected_straight = "";
    }

    if (swstate == DCC_SWITCH_STATE_BRANCH2)
    {
        selected_branch2 = "selected";
    }
    else
    {
        selected_branch2 = "";
    }

    if (! do_display)
    {
        style = "style='display:none'";
    }
    else
    {
        style = "";
    }

    HTTP::response += (String) "<select " + style + " name='" + name + "' id='" + name + "'>\r\n";
    HTTP::response += (String) "<option value='" + std::to_string(DCC_SWITCH_STATE_STRAIGHT) + "' " + selected_straight + ">Gerade</option>\r\n";
    HTTP::response += (String) "<option value='" + std::to_string(DCC_SWITCH_STATE_BRANCH) + "' " + selected_branch + ">Abzweig</option>\r\n";
    HTTP::response += (String) "<option value='" + std::to_string(DCC_SWITCH_STATE_BRANCH2) + "' " + selected_branch2 + ">Abzweig2</option>\r\n";
    HTTP::response += (String) "</select>\r\n";
}

void
HTTP_Common::print_signal_list (String name, uint_fast16_t swidx, bool do_display)
{
    String          style;
    uint_fast16_t   n_signals = Signals::get_n_signals ();
    uint_fast16_t   idx;

    if (! do_display)
    {
        style = "style='display:none'";
    }
    else
    {
        style = "";
    }

    HTTP::response += (String) "<select " + style + " name='" + name + "' id='" + name + "'>\r\n";

    for (idx = 0; idx < n_signals; idx++)
    {
        String          selected;

        if (idx == swidx)
        {
            selected = "selected";
        }
        else
        {
            selected = "";
        }

        HTTP::response += (String) "<option value='" + std::to_string(idx) + "' " + selected + ">" + Signals::signals[idx].get_name() + "</option>\r\n";
    }

    HTTP::response += (String) "</select>\r\n";
}

void
HTTP_Common::print_signal_state_list (String name, uint_fast8_t sigstate, bool do_display)
{
    String          style;
    String          selected_halt;
    String          selected_go;

    if (sigstate == DCC_SIGNAL_STATE_HALT) 
    {
        selected_halt = "selected";
    }
    else
    {
        selected_halt = "";
    }

    if (sigstate == DCC_SIGNAL_STATE_GO)
    {
        selected_go = "selected";
    }
    else
    {
        selected_go = "";
    }

    if (! do_display)
    {
        style = "style='display:none'";
    }
    else
    {
        style = "";
    }

    HTTP::response += (String) "<select " + style + " name='" + name + "' id='" + name + "'>\r\n";
    HTTP::response += (String) "<option value='" + std::to_string(DCC_SIGNAL_STATE_HALT) + "' " + selected_halt + ">Halt</option>\r\n";
    HTTP::response += (String) "<option value='" + std::to_string(DCC_SIGNAL_STATE_GO) + "' " + selected_go + ">Fahrt</option>\r\n";
    HTTP::response += (String) "</select>\r\n";
}

void
HTTP_Common::print_led_group_list (String name, uint_fast16_t swidx, bool do_display)
{
    String          style;
    uint_fast16_t   n_led_groups = Leds::get_n_led_groups ();
    uint_fast16_t   idx;

    if (! do_display)
    {
        style = "style='display:none'";
    }
    else
    {
        style = "";
    }

    HTTP::response += (String) "<select " + style + " name='" + name + "' id='" + name + "'>\r\n";

    for (idx = 0; idx < n_led_groups; idx++)
    {
        String          selected;

        if (idx == swidx)
        {
            selected = "selected";
        }
        else
        {
            selected = "";
        }

        HTTP::response += (String) "<option value='" + std::to_string(idx) + "' " + selected + ">" + Leds::led_groups[idx].get_name() + "</option>\r\n";
    }

    HTTP::response += (String) "</select>\r\n";
}

void
HTTP_Common::print_script_led_mask_list (void)
{
    HTTP::response += (String)
        "<script>"
        "function toggle_led (id, led_idx)\r\n"
        "{\r\n"
        "  var obj = document.getElementById(id + 'v');\r\n"
        "  if (obj)\r\n"
        "  {\r\n"
        "    var idl = id + '_' + led_idx;\r\n"
        "    if (obj.value & (1 << led_idx))\r\n"
        "    {\r\n"
        "       obj.value &= ~(1<<led_idx);\r\n"
        "       document.getElementById(idl).style.color = '';\r\n"
        "       document.getElementById(idl).style.backgroundColor = '';\r\n"
        "    }\r\n"
        "    else\r\n"
        "    {\r\n"
        "       obj.value |= (1<<led_idx);\r\n"
        "       document.getElementById(idl).style.color = 'white';\r\n"
        "       document.getElementById(idl).style.backgroundColor = 'green';\r\n"
        "    }\r\n"
        "  }\r\n"
        "}\r\n"
        "</script>\r\n";
}

void
HTTP_Common::print_led_mask_list (String name, uint_fast8_t ledmask, bool do_display)
{
    String          style;
    uint_fast8_t    led_idx;

    if (! do_display)
    {
        style = "style='display:none'";
    }
    else
    {
        style = "";
    }

    HTTP::response += (String) "<input type='hidden'  id='" + name + "v' name='" + name + "' value='" + std::to_string(ledmask) + "'>\r\n";
    HTTP::response += (String) "<span " + style + " id='" + name + "'>\r\n";

    for (led_idx = 0; led_idx < 8; led_idx++)
    {
        uint_fast8_t mask = 1 << led_idx;

        if (ledmask & mask) 
        {
            style = "style='color:white;background-color:green'";
        }
        else
        {
            style = "";
        }

        String si = std::to_string(led_idx);
        String id = name + "_" + si;

        HTTP::response += (String)
            "<button type='button' id='" + id + "' " + style + " onclick=\"toggle_led('" + name + "'," + si + ")\">" + si + "</button>\r\n";
    }

    HTTP::response += (String) "</span>\r\n";
}

void
HTTP_Common::print_ledon_list (String name, uint_fast8_t ledon, bool do_display)
{
    String          style;
    String          selected_off;
    String          selected_on;

    if (ledon)
    {
        selected_on = "selected";
        selected_off = "";
    }
    else
    {
        selected_on = "";
        selected_off = "selected";
    }

    if (! do_display)
    {
        style = "style='display:none'";
    }
    else
    {
        style = "";
    }

    HTTP::response += (String) "<select " + style + " name='" + name + "' id='" + name + "'>\r\n";
    HTTP::response += (String) "<option value='0' " + selected_off + ">Aus</option>\r\n";
    HTTP::response += (String) "<option value='1' " + selected_on  + ">Ein</option>\r\n";
    HTTP::response += (String) "</select>\r\n";
}

void
HTTP_Common::print_rcl_condition_list (bool in, uint_fast8_t caidx, uint_fast8_t condition, uint_fast8_t destination)
{
    String  sprefix;
    String  sname;
    String  scaidx = std::to_string(caidx);
    bool    do_display_rrg = true;

    if (in)
    {
        sprefix = "i";
    }
    else
    {
        sprefix = "o";
    }

    sname = sprefix + "cc" + scaidx;

    HTTP::response += (String) "<select name='" + sname + "' id='" + sname + "' onchange=\"changecc('" + sprefix + "', " + scaidx + ")\">\r\n";

    String selected[4];

    if (condition < 4)
    {
        selected[condition] = "selected";
    }

    if (condition == RCL_CONDITION_NEVER || condition == RCL_CONDITION_ALWAYS)
    {
        do_display_rrg = false;
    }

    HTTP::response += (String) "<option value=0 " + selected[0] + ">---</option>\r\n";
    HTTP::response += (String) "<option value=1 " + selected[1] + ">Immer</option>\r\n";
    HTTP::response += (String) "<option value=2 " + selected[2] + ">Ziel ist</option>\r\n";
    HTTP::response += (String) "<option value=3 " + selected[3] + ">Ziel ist nicht</option>\r\n";
    HTTP::response += (String) "</select>\r\n";

    HTTP_Common::print_railroad_group_list (sprefix + "cd" + scaidx, destination, true, do_display_rrg);
}

static void
print_rcl_action (uint_fast8_t idx, uint_fast8_t action)
{
    if (idx == action)
    {
        HTTP::response += (String) "<option value='" + std::to_string(idx) + "' selected>";
    }
    else
    {
        HTTP::response += (String) "<option value='" + std::to_string(idx) + "'>";
    }

    switch (idx)
    {
        case RCL_ACTION_NONE:                                                           // no parameters
        {
            HTTP::response += (String) "&lt;keine&gt;";
            break;
        }

        case RCL_ACTION_SET_LOCO_SPEED:                                                 // parameters: START, LOCO_IDX, SPEED, TENTHS
        {
            HTTP::response += (String) "Setze Geschwindigkeit";
            break;
        }

        case RCL_ACTION_SET_LOCO_MIN_SPEED:                                             // parameters: START, LOCO_IDX, SPEED, TENTHS
        {
            HTTP::response += (String) "Setze Mindestgeschwindigkeit";
            break;
        }

        case RCL_ACTION_SET_LOCO_MAX_SPEED:                                             // parameters: START, LOCO_IDX, SPEED, TENTHS
        {
            HTTP::response += (String) "Setze H&ouml;chstgeschwindigkeit";
            break;
        }

        case RCL_ACTION_SET_LOCO_FORWARD_DIRECTION:                                     // parameters: START, LOCO_IDX, FWD
        {
            HTTP::response += (String) "Setze Richtung";
            break;
        }

        case RCL_ACTION_SET_LOCO_ALL_FUNCTIONS_OFF:                                     // parameters: START, LOCO_IDX
        {
            HTTP::response += (String) "Schalte alle Funktionen aus";
            break;
        }

        case RCL_ACTION_SET_LOCO_FUNCTION_OFF:                                          // parameters: START, LOCO_IDX, FUNC_IDX
        {
            HTTP::response += (String) "Schalte Funktion aus";
            break;
        }

        case RCL_ACTION_SET_LOCO_FUNCTION_ON:                                           // parameters: START, LOCO_IDX, FUNC_IDX
        {
            HTTP::response += (String) "Schalte Funktion ein";
            break;
        }

        case RCL_ACTION_EXECUTE_LOCO_MACRO:                                             // parameters: START, LOCO_IDX, MACRO_IDX
        {
            HTTP::response += (String) "F&uuml;hre Lok-Macro aus";
            break;
        }

        case RCL_ACTION_SET_ADDON_ALL_FUNCTIONS_OFF:                                    // parameters: START, ADDON_IDX
        {
            HTTP::response += (String) "Schalte alle Funktionen aus von Zusatzdecoder";
            break;
        }

        case RCL_ACTION_SET_ADDON_FUNCTION_OFF:                                         // parameters: START, ADDON_IDX, FUNC_IDX
        {
            HTTP::response += (String) "Schalte Funktion aus von Zusatzdecoder";
            break;
        }

        case RCL_ACTION_SET_ADDON_FUNCTION_ON:                                          // parameters: START, ADDON_IDX, FUNC_IDX
        {
            HTTP::response += (String) "Schalte Funktion ein von Zusatzdecoder";
            break;
        }

        case RCL_ACTION_SET_RAILROAD:                                                   // parameters: RRG_IDX, RR_IDX
        {
            HTTP::response += (String) "Schalte Gleis von Fahrstra&szlig;e";
            break;
        }

        case RCL_ACTION_SET_FREE_RAILROAD:                                              // parameters: RRG_IDX
        {
            HTTP::response += (String) "Schalte freies Gleis von Fahrstra&szlig;e";
            break;
        }

        case RCL_ACTION_SET_LINKED_RAILROAD:                                            // parameters: RRG_IDX
        {
            HTTP::response += (String) "Schalte verkn&uuml;pftes Gleis von Fahrstra&szlig;e";
            break;
        }

        case RCL_ACTION_WAIT_FOR_FREE_S88_CONTACT:                                      // parameters: START, LOCO_IDX, CO_IDX, TENTHS
        {
            HTTP::response += (String) "Warte auf freien S88-Kontakt, Halt";
            break;
        }

        case RCL_ACTION_SET_LOCO_DESTINATION:                                           // parameters: LOCO_IDX, RRG_IDX
        {
            HTTP::response += (String) "Setze Ziel";
            break;
        }

        case RCL_ACTION_SET_LED:                                                        // parameters: START, LG_IDX, LED_MASK, LED_ON
        {
            HTTP::response += (String) "Schalte LED";
            break;
        }

        case RCL_ACTION_SET_SWITCH:                                                     // parameters: START, SW_IDX, SW_STATE
        {
            HTTP::response += (String) "Schalte Weiche";
            break;
        }

        case RCL_ACTION_SET_SIGNAL:                                                     // parameters: START, SIG_IDX, SIG_STATE
        {
            HTTP::response += (String) "Schalte Signal";
            break;
        }
    }

    HTTP::response += (String) "</option>\r\n";
}

void
HTTP_Common::print_rcl_action_list (bool in, uint_fast8_t caidx, uint_fast8_t action)
{
    uint_fast8_t    i;

    if (in)
    {
        HTTP::response += (String) "<select name='iac" + std::to_string(caidx) + "' id='iac" + std::to_string(caidx) + "' onchange=\"changeca('i'," + std::to_string(caidx) + ")\">\r\n";
    }
    else
    {
        HTTP::response += (String) "<select name='oac" + std::to_string(caidx) + "' id='oac" + std::to_string(caidx) + "' onchange=\"changeca('o'," + std::to_string(caidx) + ")\">\r\n";
    }

    for (i = 0; i < RCL_ACTIONS; i++)
    {
        print_rcl_action (i, action);
    }

    HTTP::response += (String) "</select>\r\n";
}

/*----------------------------------------------------------------------------------------------------------------------------------------
 * decoder_railcom ()
 *
 * addr == 0 : PGM
 * addr >  0 : POM
 *----------------------------------------------------------------------------------------------------------------------------------------
 */
void
HTTP_Common::decoder_railcom (uint_fast16_t addr, uint_fast8_t cv28, uint_fast8_t cv29)
{
    HTTP::response += (String)
        "<P><form><table style='border:1px lightgray solid;width:480px'>\r\n"
        "<tr bgcolor='#E0E0E0'><th colspan='2'>RailCom-Konfiguration (CV28)</th></tr>\r\n";

    String          rc1_checked             = "";
    String          rc2_checked             = "";
    String          rc1_auto_checked        = "";
    String          reserved_bit3_checked   = "";
    String          long_addr_3_checked     = "";
    String          reserved_bit5_checked   = "";
    String          high_current_checked    = "";
    String          logon_checked           = "";

    if (cv28 & 0x01)
    {
        rc1_checked = "checked";
    }

    if (cv28 & 0x02)
    {
        rc2_checked = "checked";
    }

    if (cv28 & 0x04)
    {
        rc1_auto_checked = "checked";
    }

    if (cv28 & 0x08)
    {
        reserved_bit3_checked = "checked";
    }

    if (cv28 & 0x10)
    {
        long_addr_3_checked = "checked";
    }

    if (cv28 & 0x20)
    {
        reserved_bit5_checked = "checked";
    }

    if (cv28 & 0x40)
    {
        high_current_checked = "checked";
    }

    if (cv28 & 0x80)
    {
        logon_checked = "checked";
    }

    if (cv29 & 0x80)    // accessory decoder
    {
        HTTP::response += (String)
            "<tr><td>Reserviert</td><td align='center'><input type='checkbox' name='rc1' value='1' " + rc1_checked + "></td></tr>\r\n"
            "<tr bgcolor='#E0E0E0'><td>Freigabe Kanal 2</td><td align='center'><input type='checkbox' name='rc2' value='2' " + rc2_checked + "></td></tr>\r\n"
            "<tr><td>Reserviert</td><td align='center'><input type='checkbox' name='rc1_auto' value='4' " + rc1_auto_checked + "></td></tr>\r\n"
            "<tr bgcolor='#E0E0E0'><td>Reserviert</td><td align='center'><input type='checkbox' name='reserved_bit3' value='8' " + reserved_bit3_checked + "></td></tr>\r\n"
            "<tr><td>Reserviert</td><td align='center'><input type='checkbox' name='long_addr_3' value='16' " + long_addr_3_checked + "></td></tr>\r\n"
            "<tr bgcolor='#E0E0E0'><td>Reserviert</td><td align='center'><input type='checkbox' name='reserved_bit5' value='32' " + reserved_bit5_checked + "></td></tr>\r\n"
            "<tr><td>Freigabe Hochstrom-RailCom</td><td align='center'><input type='checkbox' name='high_current' value='64' " + high_current_checked + "></td></tr>\r\n"
            "<tr bgcolor='#E0E0E0'><td>Freigabe Automatische Anmeldung</td><td align='center'><input type='checkbox' name='logon' value='128' " + logon_checked + "></td></tr>\r\n"
            "<tr><td>";
    }
    else
    {
        HTTP::response += (String)
            "<tr><td>Freigabe Kanal 1</td><td align='center'><input type='checkbox' name='rc1' value='1' " + rc1_checked + "></td></tr>\r\n"
            "<tr bgcolor='#E0E0E0'><td>Freigabe Kanal 2</td><td align='center'><input type='checkbox' name='rc2' value='2' " + rc2_checked + "></td></tr>\r\n"
            "<tr><td>Kanal 1 automatisch abschalten</td><td align='center'><input type='checkbox' name='rc1_auto' value='4' " + rc1_auto_checked + "></td></tr>\r\n"
            "<tr bgcolor='#E0E0E0'><td>Reserviert</td><td align='center'><input type='checkbox' name='reserved_bit3' value='8' " + reserved_bit3_checked + "></td></tr>\r\n"
            "<tr><td>Programmieradresse 0003 (Lange Adresse 3)</td><td align='center'><input type='checkbox' name='long_addr_3' value='16' " + long_addr_3_checked + "></td></tr>\r\n"
            "<tr bgcolor='#E0E0E0'><td>Reserviert</td><td align='center'><input type='checkbox' name='reserved_bit5' value='32' " + reserved_bit5_checked + "></td></tr>\r\n"
            "<tr><td>Freigabe Hochstrom-RailCom</td><td align='center'><input type='checkbox' name='high_current' value='64' " + high_current_checked + "></td></tr>\r\n"
            "<tr bgcolor='#E0E0E0'><td>Freigabe Automatische Anmeldung</td><td align='center'><input type='checkbox' name='logon' value='128' " + logon_checked + "></td></tr>\r\n"
            "<tr><td>";
    }

    HTTP::flush ();

    if (addr > 0) // pom
    {
        HTTP::response += (String) "<input type='hidden' name='addr' value='" + std::to_string(addr) + "'>";
    }

    HTTP::response += (String)
        "<input type='hidden' name='action' value='save_cv28'>"
        "</td><td align='right'><input type='submit' value='Speichern'></td></tr>\r\n"
        "</table></form>\r\n";
}

/*----------------------------------------------------------------------------------------------------------------------------------------
 * decoder_configuration ()
 *
 * addr == 0 : PGM
 * addr >  0 : POM
 *----------------------------------------------------------------------------------------------------------------------------------------
 */
void
HTTP_Common::decoder_configuration (uint_fast16_t addr, uint_fast8_t cv29, uint_fast8_t cv1, uint_fast8_t cv9, uint_fast16_t cv17_18)
{
    uint_fast8_t    reserved = 0;

    HTTP::response += (String)
        "<P><form><table style='border:1px lightgray solid;width:480px'>\r\n"
        "<tr bgcolor='#E0E0E0'><th colspan='2'>Decoder-Konfiguration (CV29)</th></tr>\r\n";

    if (cv29 & 0x80)      // Zubehoerdecoder
    {
        HTTP::response += (String)
            "<tr bgcolor='#E0E0E0'><td>Freigabe RailCom</td><td>" + ((cv29 & 0x08) ? "Ja" : "Nein") + "</td></tr>\r\n"
            "<tr><td>Decoder Typ</td><td>" + ((cv29 & 0x20) ? "Erweiteter" : "Einfacher") + " Zubeh&ouml;rdecoder</td></tr>\r\n"
            "<tr><td>Adressierung</td><td>" + ((cv29 & 0x40) ? "Ausgang" : "Decoder") + "</td></tr>\r\n";

        HTTP::response += (String) "<tr bgcolor='#E0E0E0'><td>Aktive Adresse</td><td><font color='blue'>"
                + ((cv9 != 0) ? ((String) "Erweiterte Adresse " + std::to_string((cv9 << 8) + cv1)) : ((String) "Basisadresse " + std::to_string(cv1)))
                + "</font></td></tr>\r\n";
        HTTP::response += (String) "<tr><td>Ansteuerung</td><td>Zubeh&ouml;rdecoder</td></tr>\r\n";
    }
    else
    {
        String          direction_invers_selected   = "";
        String          direction_normal_selected   = "";
        String          speed_steps14_selected      = "";
        String          speed_steps128_selected     = "";
        String          analog_yes_selected         = "";
        String          analog_no_selected          = "";
        String          railcom_yes_selected        = "";
        String          railcom_no_selected         = "";
        String          speedtable_yes_selected     = "";
        String          speedtable_no_selected      = "";
        String          extaddr_yes_selected        = "";
        String          extaddr_no_selected         = "";
        String          zubehoer_no_selected        = "";
        String          zubehoer_yes_selected       = "";

        HTTP::response += (String) "<tr><td>Richtung</td><td align='right'>";

        if (cv29 & 0x01)
        {
            direction_invers_selected = "selected";
        }
        else
        {
            direction_normal_selected = "selected";
        }

        HTTP::response += (String)
            "<select name='direction' style='width:180px'><option value='0' " + direction_normal_selected + ">normal</option><option value='1' " + direction_invers_selected + ">invers</option></select>"
            "</td></tr>\r\n";

        HTTP::response += (String) "<tr bgcolor='#E0E0E0'><td>Fahrstufen</td><td align='right'>";

        if (cv29 & 0x02)
        {
            speed_steps128_selected = "selected";
        }
        else
        {
            speed_steps14_selected = "selected";
        }

        HTTP::response += (String)
            "<select name='steps' style='width:180px'><option value='0' " + speed_steps14_selected + ">14</option><option value='2' " + speed_steps128_selected + ">28/128</option></select>"
            "</td></tr>\r\n"
            "<tr><td>Freigabe Analogbetrieb</td><td align='right'>";

        if (cv29 & 0x04)
        {
            analog_yes_selected = "selected";
        }
        else
        {
            analog_no_selected = "selected";
        }

        HTTP::response += (String)
            "<select name='analog' style='width:180px'><option value='0' " + analog_no_selected + ">nein</option><option value='4' " + analog_yes_selected + ">ja</option></select>"
            "</td></tr>\r\n"
            "<tr bgcolor='#E0E0E0'><td>Freigabe RailCom</td><td align='right'>";

        if (cv29 & 0x08)
        {
            railcom_yes_selected = "selected";
        }
        else
        {
            railcom_no_selected = "selected";
        }

        HTTP::response += (String)
            "<select name='railcom' style='width:180px'><option value='0' " + railcom_no_selected + ">nein</option><option value='8' " + railcom_yes_selected + ">ja</option></select>"
            "</td></tr>\r\n"
            "<tr><td>Tabelle Geschwindigkeit</td><td align='right'>";

        if (cv29 & 0x10)
        {
            speedtable_yes_selected = "selected";
        }
        else
        {
            speedtable_no_selected = "selected";
        }

        HTTP::response += (String)
            "<select name='speedtable' style='width:180px'><option value='0' " + speedtable_no_selected + ">CV 2/5/6</option><option value='16' " + speedtable_yes_selected + ">CV 67-94</option></select>"
            "</td></tr>\r\n"
            "<tr bgcolor='#E0E0E0'><td>Aktive Adresse</td><td align='right'>";

        if (cv29 & 0x20)
        {
            extaddr_yes_selected = "selected";
        }
        else
        {
            extaddr_no_selected = "selected";
        }

        HTTP::response += (String)
            "<select name='extaddr' style='width:180px'><option value='0' " + extaddr_no_selected + ">Basisadresse " + std::to_string(cv1) + "</option>"
            "<option value='32' " + extaddr_yes_selected + ">Erweiterte Adresse " + std::to_string(cv17_18) + "</option></select>"
            "</td></tr>\r\n";

        if (cv29 & 0x40)
        {
            reserved = 64;
        }
        else
        {
            reserved = 0;
        }

        HTTP::response += (String) "<tr><td>Ansteuerung</td><td align='right'>";

        if (cv29 & 0x80)
        {
            zubehoer_yes_selected = "selected";
        }
        else
        {
            zubehoer_no_selected = "selected";
        }

        HTTP::response += (String)
            "<select name='zubehoer' style='width:180px'><option value='0' " + zubehoer_no_selected + ">Fahrzeugdecoder</option>"
            "<option value='128' " + zubehoer_yes_selected + " disabled>Zubeh&ouml;rdecoder</option></select>"
            "</td></tr>\r\n";
    }

    HTTP::response += (String)
        "<tr><td></td><td align='right'>"
        "<input type='hidden' name='reserved' value='" + std::to_string(reserved) + "'>";

    if (addr > 0) // pom
    {
        HTTP::response += (String) "<input type='hidden' name='addr' value='" + std::to_string(addr) + "'>";
    }

    HTTP::response += (String)
        "<input type='hidden' name='action' value='save_cv29'>"
        "<input type='submit' value='Speichern'></td></tr>\r\n"
        "</table></form>\r\n";
}

void
HTTP_Common::action_on (void)
{
    UserIO::booster_on (1);
    HTTP::response = (String) "1";
}

void
HTTP_Common::action_off (void)
{
    UserIO::booster_off (1);
    HTTP::response = (String) "0";
}

void
HTTP_Common::action_go (void)
{
    uint_fast16_t   loco_idx      = HTTP::parameter_number ("loco_idx");
    Event::delete_event_wait_s88 (loco_idx);
}
