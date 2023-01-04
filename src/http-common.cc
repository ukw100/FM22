/*------------------------------------------------------------------------------------------------------------------------
 * http-common.cc - HTTP addon routines
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
#include "event.h"
#include "userio.h"
#include "dcc.h"
#include "loco.h"
#include "rcl.h"
#include "railroad.h"
#include "s88.h"
#include "addon.h"
#include "func.h"
#include "base.h"
#include "http.h"
#include "version.h"
#include "http-common.h"

bool                    HTTP_Common::edit_mode  = false;
const char *            HTTP_Common::alert_msg  = (char *) 0;
static uint_fast8_t     todo_speed_deadtime     = 50;

/*----------------------------------------------------------------------------------------------------------------------------------------
 * global data:
 *----------------------------------------------------------------------------------------------------------------------------------------
 */
const char * HTTP_Common::manufacturers[256] =
{
    (const char *) NULL,                                            // 0
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
    String s88_color            = "blue";
    String rcl_color            = "blue";

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

    HTTP::response += (String) "<!DOCTYPE HTML>\r\n";
    HTTP::response += (String) "<html>\r\n";
    HTTP::response += (String) "<head>\r\n";

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
        mainbrowsertitle = (String) "DCC-Zentrale";
    }
    else
    {
        mainbrowsertitle = (String) "DCC-Zentrale - " + browsertitle;
    }

    HTTP::response += (String) "<title>";
    HTTP::response += (String) mainbrowsertitle;
    HTTP::response += (String) "</title>\r\n";
    HTTP::response += (String) "<meta name='viewport' content='width=device-width,initial-scale=1'/>\r\n";
    HTTP::response += (String) "<style>\r\n";
    HTTP::response += (String) "BODY { FONT-FAMILY: Helvetica,Arial; FONT-SIZE: 14px; }\r\n";
    HTTP::response += (String) "A, U { text-decoration: none; }\r\n";
    HTTP::response += (String) "button, input[type=button], input[type=submit], input[type=reset] { background-color: #EEEEEE; border: 1px solid #AAAAEE; }\r\n";
    HTTP::response += (String) "button:hover, input[type=button]:hover, input[type=submit]:hover, input[type=reset] { background-color: #DDDDDD; }\r\n";
    HTTP::response += (String) "select { background-color: #FFFFFF; border: 1px solid #AAAAEE; }\r\n";
    HTTP::response += (String) "select:hover { background-color: #EEEEEE; }\r\n";

    HTTP::flush ();

    HTTP::response += (String) "option.red {\r\n";
    HTTP::response += (String) "  /* background-color: #cc0000;*/\r\n";
    HTTP::response += (String) "  font-weight: bold;\r\n";
    HTTP::response += (String) "  font-size: 12px;\r\n";
    HTTP::response += (String) "  /* color: white; */\r\n";
    HTTP::response += (String) "}\r\n";

    /* On screens that are 650px or less, make content invisible */
    HTTP::response += (String) "@media screen and (max-width: 650px) { .hide650 { display:none; }}\r\n";
    HTTP::response += (String) "@media screen and (max-width: 600px) { .hide600 { display:none; }}\r\n";

    /* On screens that are 650px or less, make table width 100% */
//  HTTP::response += (String) ".table650 { width:650px; }\r\n";
//  HTTP::response += (String) "@media screen and (max-width: 650px) { .table650 { width:100%; }}\r\n";

    HTTP::response += (String) "</style>\r\n";
    HTTP::response += (String) "</head>\r\n";
    HTTP::response += (String) "<body>\r\n";

    HTTP::response += (String) "<script>\r\n";

    HTTP::response += (String) "function on() { var http = new XMLHttpRequest(); http.open ('GET', '/action?action=on'); http.send (null);}\r\n";
    HTTP::response += (String) "function off() { var http = new XMLHttpRequest(); http.open ('GET', '/action?action=off'); http.send (null);}\r\n";
    HTTP::response += (String) "</script>\r\n";

    if (DCC::booster_is_on)
    {
        display_off = " style='display:none'";
    }
    else
    {
        display_on = " style='display:none'";
    }

    HTTP::flush ();

    HTTP::response += (String) "<table>\r\n";
    HTTP::response += (String) "<tr>\r\n";
    HTTP::response += (String) "<td>";
    HTTP::response += (String) "<div id='onmenu'"  + display_on  + " style='display:inline-block'>" + "<button style='width:100px;' onclick='off()'><font color='red'>Ausschalten</font></button>"  + "</div>";
    HTTP::response += (String) "<div id='offmenu'" + display_off + " style='display:inline-block'>" + "<button style='width:100px;' onclick='on()'><font color='green'>Einschalten</font></button>" + "</div>\r\n";
    HTTP::response += (String) "<td>\r\n";
    HTTP::response += (String) "<td style='padding:2px;'>\r\n";
    HTTP::response += (String) "  <select style='color:" + control_color + "' id='m_steuerung' onchange=\"window.location.href=document.getElementById(id).value;\">\r\n";
    HTTP::response += (String) "      <option value='' class='red'>Steuerung</option>\r\n";
    HTTP::response += (String) "      <option style='color:" + list_color + "' value='/loco'>Lokliste</option>\r\n";
    HTTP::response += (String) "      <option style='color:" + addon_color + "' value='/addon'>Zusatzdecoder</option>\r\n";
    HTTP::response += (String) "      <option style='color:" + switch_color + "' value='/switch'>Weichen</option>\r\n";
    HTTP::response += (String) "      <option style='color:" + rr_color + "' value='/rr'>Fahrstra&szlig;en</option>\r\n";
    HTTP::response += (String) "      <option style='color:" + switch_test_color + "' value='/swtest'>Weichentest</option>\r\n";
    HTTP::response += (String) "      <option style='color:" + s88_color + "' value='/s88'>S88</option>\r\n";
    HTTP::response += (String) "      <option style='color:" + rcl_color + "' value='/rcl'>RC-Detektoren</option>\r\n";
    HTTP::response += (String) "  </select>\r\n";
    HTTP::response += (String) "</td>\r\n";
    HTTP::flush ();

    if (HTTP_Common::edit_mode)
    {
        HTTP::response += (String) "<td style='padding:2px;'>\r\n";
        HTTP::response += (String) "  <select style='color:" + pgm_color + "' id='m_programmierung' onchange=\"window.location.href=document.getElementById('m_programmierung').value;\">\r\n";
        HTTP::response += (String) "    <option value='' class='red'>Programmierung</option>\r\n";
        HTTP::response += (String) "    <option value='' disabled='disabled'>Programmiergleis:</option>\r\n";
        HTTP::response += (String) "    <option style='color:" + pgm_info_color + "' value='/pgminfo'>&nbsp;&nbsp;Decoder-Info</option>\r\n";
        HTTP::response += (String) "    <option style='color:" + pgm_addr_color + "' value='/pgmaddr'>&nbsp;&nbsp;Lokadresse</option>\r\n";
        HTTP::response += (String) "    <option style='color:" + pgm_cv_color + "' value='/pgmcv'>&nbsp;&nbsp;CV</option>\r\n";
        HTTP::response += (String) "    <option value='' disabled='disabled'>---------------------</option>\r\n";
        HTTP::response += (String) "    <option value='' disabled='disabled'>Hauptgleis:</option>\r\n";
        HTTP::response += (String) "    <option style='color:" + pom_info_color + ";margin-left:10px;' value='/pominfo'>&nbsp;&nbsp;Decoder-Info</option>\r\n";
        HTTP::response += (String) "    <option style='color:" + pom_addr_color + "' value='/pomaddr'>&nbsp;&nbsp;Lokadresse</option>\r\n";
        HTTP::response += (String) "    <option style='color:" + pom_cv_color + "' value='/pomcv'>&nbsp;&nbsp;CV</option>\r\n";
        HTTP::response += (String) "    <option style='color:" + pom_map_color + "' value='/pommap'>&nbsp;&nbsp;Mapping</option>\r\n";
        HTTP::response += (String) "    <option style='color:" + pom_out_color + "' value='/pomout'>&nbsp;&nbsp;Ausg&auml;nge</option>\r\n";
        HTTP::response += (String) "  </select>\r\n";
        HTTP::response += (String) "</td>\r\n";
        HTTP::flush ();
    }

    HTTP::response += (String) "<td style='padding:2px;'>\r\n";
    HTTP::response += (String) "  <select style='color:" + system_color + "' id='m_zentrale' onchange=\"window.location.href=document.getElementById('m_zentrale').value;\">\r\n";
    HTTP::response += (String) "    <option value='' class='red'>Zentrale</option>\r\n";

    HTTP::response += (String) "<option style='color:" + setup_color + "' value='/setup'>Einstellungen</option>\r\n";

    if (HTTP_Common::edit_mode)
    {
        HTTP::response += (String) "<option value='' disabled='disabled'>----------------</option>\r\n";
        HTTP::response += (String) "<option style='color:" + network_color + "' value='/net'>Netzwerk</option>\r\n";
        HTTP::response += (String) "<option style='color:" + upload_color + "' value='/upl'>Upload Hex</option>\r\n";
        HTTP::response += (String) "<option style='color:" + flash_color + "' value='/flash'>Flash STM32</option>\r\n";
    }

    HTTP::response += (String) "    <option style='color:" + info_color + "' value='/info'>Info</option>\r\n";
    HTTP::response += (String) "  </select>\r\n";
    HTTP::response += (String) "</td>\r\n";
    HTTP::response += (String) "</tr>\r\n";
    HTTP::response += (String) "</table>\r\n";

    HTTP::response += (String) "<script>";
    HTTP::response += (String) "function rstalert() { var http = new XMLHttpRequest(); http.open ('GET', '/action?action=rstalert');";
    HTTP::response += (String) "http.addEventListener('load',";
    HTTP::response += (String) "function(event) { if (http.status >= 200 && http.status < 300) { ; }});";
    HTTP::response += (String) "http.send (null);}\r\n";
    HTTP::response += (String) "</script>\r\n";

    HTTP::response += (String) "<div style='margin-left:20px'>Strom: <span style='display:inline-block; width:60px;' id='adc'>" + std::to_string(DCC::adc_value) + "</span>\r\n";
    HTTP::response += (String) "RC1: <span id='rc1'>";

    if (DCC::rc1_value == 0xFFFF)
    {
        HTTP::response += (String) "-----";
    }
    else
    {
        HTTP::response += std::to_string (DCC::rc1_value);
    }

    HTTP::response += (String) "</span><span style='margin-left:10px;color:red;font-weight:bold' id='alert'>";

    if (HTTP_Common::alert_msg && *HTTP_Common::alert_msg)
    {
        HTTP::response += (String) HTTP_Common::alert_msg;
    }

    HTTP::response += (String) "</span><span style='margin-left:10px;'><button id='rstalert' onclick='rstalert()' style='display:none'>Reset</button></span></div>\r\n";

    HTTP::response += (String) "<H3 style='margin-left:20px;'>";
    HTTP::response += (String) title;
    HTTP::response += (String) "</H3>\r\n";
    HTTP::flush ();
}

/*----------------------------------------------------------------------------------------------------------------------------------------
 * HTTP_Common::html_trailer () - send html trailer
 *----------------------------------------------------------------------------------------------------------------------------------------
 */
void
HTTP_Common::html_trailer (void)
{
    HTTP::response += (String) "</body>\r\n";
    HTTP::response += (String) "</html>\r\n";
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

    if (DCC::rc1_value == 0xFFFF)
    {
        HTTP_Common::add_action_content ("rc1",  "text", "-----");
    }
    else
    {
        HTTP_Common::add_action_content ("rc1",  "text", std::to_string (DCC::rc1_value));
    }

    if (alert_msg && *alert_msg)
    {
        HTTP_Common::add_action_content ("alert", "text", (alert_msg && *alert_msg) ? alert_msg : "");
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

    HTTP::response += (String) "Version DCC-Zentrale: " + VERSION + "<BR>\r\n";

    HTTP::response += (String) "</span>\r\n";
    HTTP::response += (String) "</div>\r\n";
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
    uint_fast8_t    deadtime;
    String          checked_edit = "";
    String          checked_railcom = "";

    action = HTTP::parameter ("action");

    if (strcmp (action, "setedit") == 0)                            // action "setedit" must be handled before html_header()!
    {
        HTTP_Common::edit_mode = HTTP::parameter_number ("edit");
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

    HTTP_Common::html_header (title, title, url, true);
    HTTP::response += (String) "<div style='margin-left:20px;'>\r\n";
    HTTP_Common::add_action_handler ("head", "", 200, true);

    if (HTTP_Common::edit_mode)
    {
        checked_edit = "checked";
    }

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

    HTTP::response += (String) "<form method='GET' action='" + url + "'>\r\n";
    HTTP::response += (String) " <div style='padding:10px;width:400px;border:1px lightgray solid'>\r\n";
    HTTP::response += (String) "  <B>Bearbeitung:</B><P>";
    HTTP::response += (String) "  Bearbeitung aktiv: <input type='checkbox' name='edit' value='1' " + checked_edit + ">\r\n";
    HTTP::response += (String) "  <input type='hidden' name='action' value='setedit'>\r\n";
    HTTP::response += (String) "  <div style='text-align:right;'><input type='submit' style='margin-left:40px' value='Speichern'></div>\r\n";
    HTTP::response += (String) "</div></form>\r\n";

    HTTP::response += (String) "<P>";
    HTTP::flush ();

    if (HTTP_Common::edit_mode)
    {
        HTTP::response += (String) "<form method='GET' action='" + url + "'>\r\n";
        HTTP::response += (String) " <div style='padding:10px;width:400px;border:1px lightgray solid'>\r\n";
        HTTP::response += (String) "  <B>RailCom:</B><P>";
        HTTP::response += (String) "  RailCom: <input type='checkbox' name='railcom' value='1' " + checked_railcom + ">\r\n";
        HTTP::response += (String) "  <input type='hidden' name='action' value='setrailcom'>\r\n";
        HTTP::response += (String) "  <div style='text-align:right;'><input type='submit' style='margin-left:40px' value='Speichern'></div>\r\n";
        HTTP::response += (String) "</div></form>\r\n";

        HTTP::response += (String) "<P>";
        HTTP::flush ();

        HTTP::response += (String) "<script>function upd_slider() { var slider = document.getElementById('deadtime'); var out = document.getElementById('out'); ";
        HTTP::response += (String) "var value = parseInt(slider.value); out.textContent = value; }</script>\r\n";

        HTTP::response += (String) "<form method='GET' action='" + url + "'>\r\n";
        HTTP::response += (String) " <div style='padding:10px;width:400px;border:1px lightgray solid'>\r\n";
        HTTP::response += (String) "  <B>Geschwindigkeitssteuerung Wii-Gamepad:</B><P>";
        HTTP::response += (String) "  <div id='out' style='width:350px;text-align:center;'>" + std::to_string(deadtime) + "</div>\r\n";
        HTTP::response += (String) "  Direkt\r\n";
        HTTP::response += (String) "  <input type='range' min='0' max='255' value='" + std::to_string(deadtime) + "' name='deadtime' id='deadtime' style='width:256px' onchange='upd_slider()'>\r\n";
        HTTP::response += (String) "  Sanft\r\n";
        HTTP::response += (String) "  <input type='hidden' name='action' value='setdeadtime'>\r\n";
        HTTP::response += (String) "  <div style='text-align:right;'><input type='submit' style='margin-left:40px' value='Speichern'></div>\r\n";
        HTTP::response += (String) "</div></form>\r\n";

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

    HTTP::response += (String) "function action_handler_" + action + "() {\r\n";
    HTTP::response += (String) "  var http = new XMLHttpRequest(); http.open ('GET', '/action?action=" + action + sparam + "');\r\n";
    HTTP::response += (String) "  http.addEventListener('load',\r\n";
    HTTP::response += (String) "    function(event) {\r\n";
    HTTP::response += (String) "      var i; var j;\r\n";
    HTTP::response += (String) "      var text = http.responseText;\r\n";
    HTTP::response += (String) "      if (http.status >= 200 && http.status < 300) {\r\n";
    HTTP::response += (String) "        const a = text.split('\t');\r\n";
    HTTP::response += (String) "        var l = a.length - 1;\r\n";              // -1: last element empty (trailing '\t')
    HTTP::response += (String) "        for (i = 0; i < l; i++) {\r\n";
    HTTP::response += (String) "          const b = a[i].split('\b');\r\n";
    HTTP::response += (String) "          var oo = document.getElementById(b[0]);\r\n";
    HTTP::response += (String) "          if (oo) {\r\n";
    HTTP::response += (String) "            switch (b[1])\r\n";
    HTTP::response += (String) "            {\r\n";
    HTTP::response += (String) "              case 'display':\r\n";
    HTTP::response += (String) "                oo.style.display = b[2];\r\n";
    HTTP::response += (String) "                break;\r\n";
    HTTP::response += (String) "              case 'value':\r\n";
    HTTP::response += (String) "                oo.value = b[2];\r\n";
    HTTP::response += (String) "                break;\r\n";
    HTTP::response += (String) "              case 'text':\r\n";
    HTTP::response += (String) "                oo.textContent = b[2];\r\n";
    HTTP::response += (String) "                break;\r\n";
    HTTP::response += (String) "              case 'html':\r\n";
    HTTP::response += (String) "                oo.innerHTML = b[2];\r\n";
    HTTP::response += (String) "                break;\r\n";
    HTTP::response += (String) "              case 'checked':\r\n";
    HTTP::response += (String) "                oo.checked = parseInt(b[2]);\r\n";
    HTTP::response += (String) "                break;\r\n";
    HTTP::response += (String) "              case 'color':\r\n";
    HTTP::response += (String) "                oo.style.color = b[2];\r\n";
    HTTP::response += (String) "                break;\r\n";
    HTTP::response += (String) "              case 'bgcolor':\r\n";
    HTTP::response += (String) "                oo.style.backgroundColor = b[2];\r\n";
    HTTP::response += (String) "                break;\r\n";
    HTTP::response += (String) "              default:\r\n";
    HTTP::response += (String) "                console.log ('invalid type: ' + b[1]);\r\n";
    HTTP::response += (String) "                break;\r\n";
    HTTP::response += (String) "            }\r\n";
    HTTP::response += (String) "          }\r\n";
    HTTP::response += (String) "        }\r\n";
    HTTP::response += (String) "      }\r\n";
    HTTP::response += (String) "    }\r\n";
    HTTP::response += (String) "  );\r\n";
    HTTP::response += (String) "  http.send (null);\r\n";
    HTTP::response += (String) "}\r\n";
    HTTP::response += (String) "var intervalId" + action + " = window.setInterval(function(){ action_handler_" + action + " (); }, " + std::to_string(msec) + ");\r\n";

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

    HTTP::response += (String) "<td>Neue ID:</td><td>";
    HTTP::response += (String) "<select id='" + name + "' name='" + name + "'>\r\n";

    for (i = 0; i < n_entries; i++)
    {
        HTTP::response += (String) "<option value='" + std::to_string(i) + "'>" + std::to_string(i) + "</option>\r\n";
    }

    HTTP::response += (String) "</select>\r\n";
    HTTP::response += (String) "</td>\r\n";
}

/*----------------------------------------------------------------------------------------------------------------------------------------
 * print_id_select_list ()
 *----------------------------------------------------------------------------------------------------------------------------------------
 */
void
HTTP_Common::print_id_select_list (String name, uint_fast16_t n_entries, uint_fast16_t selected_idx)
{
    uint_fast16_t i;

    HTTP::response += (String) "<td>Neue ID:</td><td>";
    HTTP::response += (String) "<select id='" + name + "' name='" + name + "'>\r\n";

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

    HTTP::response += (String) "</select>\r\n";
    HTTP::response += (String) "</td>\r\n";
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

            HTTP::response += (String) ">(Erkannte Lok '" + RCL::get_name (track_idx) + "')</option>\r\n";
        }
    }
    else
    {
        HTTP::response += (String) ">-- Keine --</option>\r\n";
    }

    uint_fast16_t   n_locos = Loco::get_n_locos ();
    uint_fast16_t   lidx;

    for (lidx = 0; lidx < n_locos; lidx++)
    {
        char * name = Loco::get_name (lidx);

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
    uint_fast16_t   n_addons = AddOn::get_n_addons ();
    uint_fast16_t   aidx;

    if (! (show_mask & ADDON_LIST_DO_DISPLAY))
    {
        style = "style='display:none'";
    }

    HTTP::response += (String) "<select " + style + " name='" + name + "' id='" + name + "'>\r\n";

    for (aidx = 0; aidx < n_addons; aidx++)
    {
        char * name = AddOn::get_name (aidx);

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

    uint_fast16_t   n_addons = AddOn::get_n_addons ();
    uint_fast16_t   lidx;

    for (lidx = 0; lidx < n_addons; lidx++)
    {
        char * name = AddOn::get_name (lidx);

        HTTP::response += (String) "<option value='" + std::to_string(lidx) + "'";

        if (lidx == selected_idx)
        {
            HTTP::response += (String) " selected";
        }

        HTTP::response += (String) ">" + name + "</option>\r\n";
    }

    HTTP::response += (String) "</select>\r\n";
}

#endif // 00

const char *
HTTP_Common::get_location (uint_fast16_t loco_idx)
{
    static char     location_buf[256];
    uint_fast8_t    rcllocation;
    uint_fast16_t   rrlocation;
    const char *    location_ptr;

    rcllocation = Loco::get_rcllocation (loco_idx);

    if (rcllocation != 0xFF)
    {
        location_ptr = RCL::get_name (rcllocation);

        if (! location_ptr)
        {
            location_ptr = "";
        }
    }
    else
    {
        rrlocation = Loco::get_rrlocation (loco_idx);

        if (rrlocation != 0xFFFF)
        {
            uint_fast8_t    rrgidx      = rrlocation >> 8;
            uint_fast8_t    rridx       = rrlocation & 0xFF;
            char *          s1          = Railroad::group_getname (rrgidx);
            char *          s2          = Railroad::get_name (rrgidx, rridx);

            if (s1 && s2 && *s1 && *s2)
            {
                sprintf (location_buf, "%s - %s", s1, s2);
                location_ptr = location_buf;
            }
            else
            {
                location_ptr = "";
            }
        }
        else
        {
            location_ptr = "";
        }
    }

    return location_ptr;
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
                sprintf (buf, "nach %0.1f sec f&uuml;r", (float) start / 10);
            }
            else if (start < 3000)
            {
                sprintf (buf, "nach %u sec f&uuml;r", (unsigned int) (start / 10));
            }
            else
            {
                sprintf (buf, "nach %u min f&uuml;r", (unsigned int) (start / 600));
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


    HTTP::response += (String) "<select " + style + " name='" + name + "' id='" + name + "'>\r\n";
    HTTP::response += (String) "<option value='0' " + selected0 + ">R&uuml;ckw&auml;rts</option>\r\n";
    HTTP::response += (String) "<option value='1' " + selected1 + ">Vorw&auml;rts</option>\r\n";
    HTTP::response += (String) "</select>\r\n";
}

void
HTTP_Common::print_function_list (String name, uint_fast16_t selected_f, bool do_display)
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
HTTP_Common::print_function_list (uint_fast16_t loco_or_addon_idx, bool is_loco, String name, uint_fast16_t selected_fidx, bool do_display)
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
            function_name_idx = Loco::get_function_name_idx (loco_or_addon_idx, fidx);
        }
        else
        {
            function_name_idx = AddOn::get_function_name_idx (loco_or_addon_idx, fidx);
        }

        if (function_name_idx != 0xFFFF)
        {
            if (is_loco)
            {
                funcname = Loco::get_function_name (loco_or_addon_idx, fidx);
            }
            else
            {
                funcname = AddOn::get_function_name (loco_or_addon_idx, fidx);
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
            HTTP::response += (String) ">M" + std::to_string(m) + "</option>\r\n";
        }
    }

    HTTP::response += (String) "</select>\r\n";
}

void
HTTP_Common::print_railroad_group_list (String name, uint_fast8_t rrgidx, bool do_display)
{
    String          style;
    uint_fast8_t    n_railroad_groups   = Railroad::get_n_railroad_groups ();
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

    for (idx = 0; idx < n_railroad_groups; idx++)
    {
        String          selected;

        if (idx == rrgidx)
        {
            selected = "selected";
        }
        else
        {
            selected = "";
        }

        HTTP::response += (String) "<option value='" + std::to_string(idx) + "' " + selected + ">" + Railroad::group_getname (idx) + "</option>\r\n";
    }

    HTTP::response += (String) "</select>\r\n";
}

void
HTTP_Common::print_railroad_list (String name, uint_fast16_t linkedrr, bool do_allow_none, bool do_display)
{
    String          style;
    uint_fast8_t    n_railroad_groups   = Railroad::get_n_railroad_groups ();
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
        uint_fast8_t    n_railroads = Railroad::get_n_railroads (rrgidx);

        for (rridx = 0; rridx < n_railroads; rridx++)
        {
            String          selected;
            String          sloco_name;
            uint_fast16_t   loco_idx    = Railroad::get_link_loco (rrgidx, rridx);
            uint_fast16_t   lrr         = (rrgidx << 8) | rridx;

            if (loco_idx != 0xFFFF)
            {
                sloco_name = std::to_string(loco_idx) + " " + Loco::get_name (loco_idx);
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

            HTTP::response += (String) "<option value='" + std::to_string(lrr) + "' " + selected + ">"
                        + Railroad::group_getname (rrgidx) + " - " + Railroad::get_name (rrgidx, rridx) + " (" + sloco_name + ")</option>\r\n";
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

        HTTP::response += (String) "<option value='" + std::to_string(idx) + "' " + selected + ">" + S88::get_name (idx) + "</option>\r\n";
    }

    HTTP::response += (String) "</select>\r\n";
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
            HTTP::response += (String) "Setze Geschwindigkeit von Lok";
            break;
        }

        case RCL_ACTION_SET_LOCO_MIN_SPEED:                                             // parameters: START, LOCO_IDX, SPEED, TENTHS
        {
            HTTP::response += (String) "Setze Mindestgeschwindigkeit von Lok";
            break;
        }

        case RCL_ACTION_SET_LOCO_MAX_SPEED:                                             // parameters: START, LOCO_IDX, SPEED, TENTHS
        {
            HTTP::response += (String) "Setze H&ouml;chstgeschwindigkeit von Lok";
            break;
        }

        case RCL_ACTION_SET_LOCO_FORWARD_DIRECTION:                                     // parameters: START, LOCO_IDX, FWD
        {
            HTTP::response += (String) "Setze Richtung von Lok";
            break;
        }

        case RCL_ACTION_SET_LOCO_ALL_FUNCTIONS_OFF:                                     // parameters: START, LOCO_IDX
        {
            HTTP::response += (String) "Schalte alle Funktionen aus von Lok";
            break;
        }

        case RCL_ACTION_SET_LOCO_FUNCTION_OFF:                                          // parameters: START, LOCO_IDX, FUNC_IDX
        {
            HTTP::response += (String) "Schalte Funktion aus von Lok";
            break;
        }

        case RCL_ACTION_SET_LOCO_FUNCTION_ON:                                           // parameters: START, LOCO_IDX, FUNC_IDX
        {
            HTTP::response += (String) "Schalte Funktion ein von Lok";
            break;
        }

        case RCL_ACTION_EXECUTE_LOCO_MACRO:                                             // parameters: START, LOCO_IDX, MACRO_IDX
        {
            HTTP::response += (String) "F&uuml;hre Macro aus f&uuml;r Lok";
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
 * decoder_configuration ()
 *
 * addr == 0 : PGM
 * addr >  0 : POM
 *----------------------------------------------------------------------------------------------------------------------------------------
 */
void
HTTP_Common::decoder_configuration (uint_fast16_t addr, uint_fast8_t cv29, uint_fast8_t cv1, uint_fast8_t cv9, uint_fast16_t cv17_18)
{
    uint_fast8_t    reserved;

    HTTP::response += (String) "<P><form><table style='border:1px lightgray solid;'>\r\n";
    HTTP::response += (String) "<tr bgcolor='#E0E0E0'><th colspan='2'>Decoder-Konfiguration (CV29)</th></tr>\r\n";

    if (cv29 & 0x80)      // Zubehoerdecoder
    {
        HTTP::response += (String) "<tr bgcolor='#E0E0E0'><td>Freigabe RailCom</td><td>" + ((cv29 & 0x08) ? "Ja" : "Nein") + "</td></tr>\r\n";
        HTTP::response += (String) "<tr><td>Decoder Typ</td><td>" + ((cv29 & 0x20) ? "Erweiteter" : "Einfacher") + " Zubeh&ouml;rdecoder</td></tr>\r\n";
        HTTP::response += (String) "<tr><td>Adressierung</td><td>" + ((cv29 & 0x40) ? "Ausgang" : "Decoder") + "</td></tr>\r\n";

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

        HTTP::response += (String) "<tr><td>Richtung</td><td>";

        if (cv29 & 0x01)
        {
            direction_invers_selected = "selected";
        }
        else
        {
            direction_normal_selected = "selected";
        }

        HTTP::response += (String) "<select name='direction' style='width:180px'><option value='0' " + direction_normal_selected + ">normal</option><option value='1' " + direction_invers_selected + ">invers</option></select>";
        HTTP::response += (String) "</td></tr>\r\n";

        HTTP::response += (String) "<tr bgcolor='#E0E0E0'><td>Fahrstufen</td><td>";

        if (cv29 & 0x02)
        {
            speed_steps128_selected = "selected";
        }
        else
        {
            speed_steps14_selected = "selected";
        }

        HTTP::response += (String) "<select name='steps' style='width:180px'><option value='0' " + speed_steps14_selected + ">14</option><option value='2' " + speed_steps128_selected + ">28/128</option></select>";
        HTTP::response += (String) "</td></tr>\r\n";

        HTTP::response += (String) "<tr><td>Freigabe Analogbetrieb</td><td>";

        if (cv29 & 0x04)
        {
            analog_yes_selected = "selected";
        }
        else
        {
            analog_no_selected = "selected";
        }

        HTTP::response += (String) "<select name='analog' style='width:180px'><option value='0' " + analog_no_selected + ">nein</option><option value='4' " + analog_yes_selected + ">ja</option></select>";
        HTTP::response += (String) "</td></tr>\r\n";

        HTTP::response += (String) "<tr bgcolor='#E0E0E0'><td>Freigabe RailCom</td><td>";

        if (cv29 & 0x08)
        {
            railcom_yes_selected = "selected";
        }
        else
        {
            railcom_no_selected = "selected";
        }

        HTTP::response += (String) "<select name='railcom' style='width:180px'><option value='0' " + railcom_no_selected + ">nein</option><option value='8' " + railcom_yes_selected + ">ja</option></select>";
        HTTP::response += (String) "</td></tr>\r\n";

        HTTP::response += (String) "<tr><td>Tabelle Geschwindigkeit</td><td>";

        if (cv29 & 0x10)
        {
            speedtable_yes_selected = "selected";
        }
        else
        {
            speedtable_no_selected = "selected";
        }

        HTTP::response += (String) "<select name='speedtable' style='width:180px'><option value='0' " + speedtable_no_selected + ">CV 2/5/6</option><option value='16' " + speedtable_yes_selected + ">CV 67-94</option></select>";
        HTTP::response += (String) "</td></tr>\r\n";

        HTTP::response += (String) "<tr bgcolor='#E0E0E0'><td>Aktive Adresse</td><td>";

        if (cv29 & 0x20)
        {
            extaddr_yes_selected = "selected";
        }
        else
        {
            extaddr_no_selected = "selected";
        }

        HTTP::response += (String) "<select name='extaddr' style='width:180px'><option value='0' " + extaddr_no_selected + ">Basisadresse " + std::to_string(cv1) + "</option>";
        HTTP::response += (String) "<option value='32' " + extaddr_yes_selected + ">Erweiterte Adresse " + std::to_string(cv17_18) + "</option></select>";
        HTTP::response += (String) "</td></tr>\r\n";

        if (cv29 & 0x40)
        {
            reserved = 64;
        }
        else
        {
            reserved = 0;
        }

        HTTP::response += (String) "<tr><td>Ansteuerung</td><td>";

        if (cv29 & 0x80)
        {
            zubehoer_yes_selected = "selected";
        }
        else
        {
            zubehoer_no_selected = "selected";
        }

        HTTP::response += (String) "<select name='zubehoer' style='width:180px'><option value='0' " + zubehoer_no_selected + ">Fahrzeugdecoder</option>";
        HTTP::response += (String) "<option value='128' " + zubehoer_yes_selected + " disabled>Zubeh&ouml;rdecoder</option></select>";
        HTTP::response += (String) "</td></tr>\r\n";
    }

    HTTP::response += (String) "<tr><td></td><td align='right'>";
    HTTP::response += (String) "<input type='hidden' name='reserved' value='" + std::to_string(reserved) + "'>";

    if (addr > 0) // pom
    {
        HTTP::response += (String) "<input type='hidden' name='addr' value='" + std::to_string(addr) + "'>";
    }

    HTTP::response += (String) "<input type='hidden' name='action' value='save_cv29'>";
    HTTP::response += (String) "<input type='submit' value='Speichern'></td></tr>\r\n";
    HTTP::response += (String) "</table></form>\r\n";
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
