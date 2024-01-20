/*------------------------------------------------------------------------------------------------------------------------
 * http-sig.cc - HTTP sig routines
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
#include "sig.h"
#include "switch.h"
#include "led.h"
#include "func.h"
#include "base.h"
#include "http.h"
#include "http-common.h"
#include "http-test.h"

/*----------------------------------------------------------------------------------------------------------------------------------------
 * handle_test ()
 *----------------------------------------------------------------------------------------------------------------------------------------
 */
void
HTTP_Test::handle_test (void)
{
    String          title   = "Decodertest";
    String          url     = "/test";
    const char *    action  = HTTP::parameter ("action");
    const char *    saddr   = HTTP::parameter ("addr");
    uint_fast16_t   addr;

    HTTP_Common::html_header (title, title, url, true);
    HTTP::response += (String) "<div style='margin-left:20px;'>\r\n";
    HTTP_Common::add_action_handler ("head", "", 200, true);

    if (strcmp (action, "switch") == 0)
    {
        uint_fast16_t   nswitch = HTTP::parameter_number ("switch");

        addr = atoi (saddr);
        DCC::base_switch_set (addr, nswitch);
    }
    else if (strcmp (action, "sig") == 0)
    {
        uint_fast16_t   nsig = HTTP::parameter_number ("sig");

        addr = atoi (saddr);
        DCC::base_switch_set (addr, nsig);
    }
    else if (strcmp (action, "led") == 0)
    {
        uint_fast16_t   led0 = HTTP::parameter_number ("led0");
        uint_fast16_t   led1 = HTTP::parameter_number ("led1");
        uint_fast16_t   led2 = HTTP::parameter_number ("led2");
        uint_fast16_t   led3 = HTTP::parameter_number ("led3");
        uint_fast16_t   led4 = HTTP::parameter_number ("led4");
        uint_fast16_t   led5 = HTTP::parameter_number ("led5");
        uint_fast16_t   led6 = HTTP::parameter_number ("led6");
        uint_fast16_t   led7 = HTTP::parameter_number ("led7");

        uint_fast16_t   led = led0 | led1 | led2 | led3 | led4 | led5 | led6 | led7;

        addr = atoi (saddr);
        DCC::ext_accessory_set (addr, led);
    }

    HTTP::response += (String)
        "<form method='get' action='" + url + "'>\r\n"
        "<table style='border:1px lightgray solid;width:350px'>\r\n"
        "  <tr><th align='left' colspan='2'>Weichen-Decoder</th></tr>\r\n"
        "  <tr><td width='50'>Adresse:</td><td><input type=text name='addr' maxlength=4 value='" + saddr + "'></td></tr>\r\n"
        "  <tr><td>Stellung:</td><td>\r\n"
        "    <input type='radio' name='switch' value='1' checked><label for='go'>Gerade</label>\r\n"
        "    <input type='radio' name='switch' value='0'><label for='halt'>Abzweig</label>\r\n"
        "    <input type='radio' name='switch' value='2'><label for='halt'>Abzweig2</label>\r\n"
        "  </td></tr>\r\n"
        "  <tr><td colspan='2' align='right'><button type='submit' name='action' value='switch'>Schalten</button></td></tr>\r\n"
        "</table>\r\n"
        "</form>\r\n"
        "<P>";

    HTTP::flush ();

    HTTP::response += (String)
        "<form method='get' action='" + url + "'>\r\n"
        "<table style='border:1px lightgray solid;width:350px'>\r\n"
        "  <tr><th align='left' colspan='2'>Signal-Decoder</th></tr>\r\n"
        "  <tr><td width='50'>Adresse:</td><td><input type=text name='addr' maxlength=4 value='" + saddr + "'></td></tr>\r\n"
        "  <tr><td>Stellung:</td><td>\r\n"
        "    <input type='radio' name='sig' value='1' checked><label for='go'>Fahrt</label>\r\n"
        "    <input type='radio' name='sig' value='0'><label for='halt'>Halt</label>\r\n"
        "  </td></tr>\r\n"
        "  <tr><td colspan='2' align='right'><button type='submit' name='action' value='sig'>Schalten</button></td></tr>\r\n"
        "</table>\r\n"
        "</form>\r\n"
        "<P>";

    HTTP::flush ();

    HTTP::response += (String)
        "<form method='get' action='" + url + "'>\r\n"
        "<table style='border:1px lightgray solid;width:350px'>\r\n"
        "  <tr><th align='left' colspan='9'>LED-Decoder</th></tr>\r\n"
        "  <tr><td width='50'>Adresse:</td><td colspan='8'><input type=text name='addr' maxlength=4 value='" + saddr + "'></td></tr>\r\n"
        "  <tr>"
        "    <td>LEDs:</td>"
        "    <td align='center'>0</td>"
        "    <td align='center'>1</td>"
        "    <td align='center'>2</td>"
        "    <td align='center'>3</td>"
        "    <td align='center'>4</td>"
        "    <td align='center'>5</td>"
        "    <td align='center'>6</td>"
        "    <td align='center'>7</td>"
        "  </tr>\r\n"
        "  <tr>"
        "    <td></td>"
        "    <td align='center'><input type='checkbox' name='led0' value='1'></td>"
        "    <td align='center'><input type='checkbox' name='led1' value='2'></td>"
        "    <td align='center'><input type='checkbox' name='led2' value='4'></td>"
        "    <td align='center'><input type='checkbox' name='led3' value='8'></td>"
        "    <td align='center'><input type='checkbox' name='led4' value='16'></td>"
        "    <td align='center'><input type='checkbox' name='led5' value='32'></td>"
        "    <td align='center'><input type='checkbox' name='led6' value='64'></td>"
        "    <td align='center'><input type='checkbox' name='led7' value='128'></td>"
        "  </tr>\r\n"
        "  <tr><td colspan='9' align='right'><button type='submit' name='action' value='led'>Schalten</button></td></tr>\r\n"
        "</table>\r\n"
        "</form>\r\n"
        "<P>";

    HTTP::flush ();

    HTTP::response += (String)
        "</div>\r\n";

    HTTP_Common::html_trailer ();
}
