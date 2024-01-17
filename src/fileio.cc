/*-------------------------------------------------------------------------------------------------------------------------------------------
 * fileio.cc - file I/O
 *-------------------------------------------------------------------------------------------------------------------------------------------
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
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <string>
#include "dcc.h"
#include "fm22.h"
#include "func.h"
#include "loco.h"
#include "addon.h"
#include "led.h"
#include "switch.h"
#include "sig.h"
#include "railroad.h"
#include "s88.h"
#include "rcl.h"
#include "base.h"
#include "debug.h"
#include "fileio.h"

#define SWITCH_SECTION              1
#define RAILROAD_GROUP_SECTION      2
#define RAILROAD_SECTION            3
#define RAILROAD_SWITCH_SECTION     4

#define SIGNAL_SECTION              1
#define LED_SECTION                 1
#define CONTACT_SECTION             1
#define RCLTRACK_SECTION             1

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * FileIO::read_fm22_ini () - read fm22.ini
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
bool
FileIO::read_fm22_ini (void)
{
    const char *    fname = "fm22.ini";
    FILE *          fp;
    char            buf[80];
    bool            in_fm22_section = false;

    fp = fopen (fname, "r");

    if (fp)
    {
        while (fgets (buf, 80, fp) != (char *) NULL)
        {
            trim (buf);

            if (! strcmp (buf, "[FM22]"))
            {
                in_fm22_section = true;
            }
            else
            {
                if (in_fm22_section)
                {
                    char *  p = strchr (buf, '=');

                    if (p)
                    {
                        *p++ = '\0';

                        if (! strcmp (buf, "SHORTCUT"))
                        {
                            FM22::set_shortcut_value (atoi(p));
                        }
                    }
                }
            }
        }

        fclose (fp);
    }
    else
    {
        perror (fname);
        return (0);
    }

    FM22::data_changed = false;
    return (1);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * FileIO::write_fm22_ini () - write fm22.ini
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
bool
FileIO::write_fm22_ini (void)
{
    FILE *          fp;
    const char *    fname = "fm22.ini";
    const char *    fname_bak = "fm22.bak";
    bool            rtc = false;

    unlink (fname_bak);
    rename (fname, fname_bak);

    fp = fopen (fname, "w");

    if (fp)
    {
        uint32_t    shortcut_value = FM22::get_shortcut_value ();

        fprintf (fp, "[FM22]\r\n");
        fprintf (fp, "SHORTCUT=%u\r\n", shortcut_value);

        FM22::data_changed = false;

        rtc = true;
        fclose (fp);
    }
    else
    {
        perror (fname);
    }

    return rtc;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * FileIO::read_loco_ini () - read loco.ini
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
bool
FileIO::read_loco_ini (void)
{
    const char *    fname = "loco.ini";
    FILE *          fp;
    char            buf[80];
    int             loco_idx = -1;
    int             addon_idx = -1;
    bool            in_loco_section = false;
    bool            in_addon_section = false;

    fp = fopen (fname, "r");

    if (fp)
    {
        while (fgets (buf, 80, fp) != (char *) NULL)
        {
            trim (buf);

            if (! strcmp (buf, "[LOCO]"))
            {
                loco_idx = Locos::add ({});

                if (loco_idx == 0xFFFF)
                {
                    Debug::printf (DEBUG_LEVEL_NONE, "%s: error: maximum number of locos reached.\n", fname);
                    break;
                }

                Locos::locos[loco_idx].activate ();
                in_loco_section = true;
                in_addon_section = false;
            }
            else if (! strcmp (buf, "[ADDON]"))
            {
                addon_idx = AddOns::add ({});

                if (addon_idx == 0xFFFF)
                {
                    Debug::printf (DEBUG_LEVEL_NONE, "%s: error: maximum number of addons reached.\n", fname);
                }

                AddOns::addons[addon_idx].activate ();
                in_loco_section = false;
                in_addon_section = true;
            }
            else
            {
                if (in_loco_section)
                {
                    char *  p = strchr (buf, '=');

                    if (p)
                    {
                        *p++ = '\0';

                        if (! strcmp (buf, "NAME"))
                        {
                            Locos::locos[loco_idx].set_name (p);
                        }
                        else if (! strcmp (buf, "ACTIVE"))
                        {
                            if (atoi(p) == 0)
                            {
                                Locos::locos[loco_idx].deactivate();
                            }
                            else
                            {
                                Locos::locos[loco_idx].activate();
                            }
                        }
                        else if (! strcmp (buf, "ADDR"))
                        {
                            Locos::locos[loco_idx].set_addr (atoi(p));
                        }
                        else if (! strcmp (buf, "STEPS"))
                        {
                            Locos::locos[loco_idx].set_speed_steps (atoi(p));
                        }
                        else if (*buf == 'F' && *(buf + 1) >= '0' && *(buf + 1) <= '9')
                        {
                            uint_fast8_t    sound = 0;
                            uint_fast8_t    pulse = 0;
                            int             fidx;
                            uint_fast16_t   function_name_idx;

                            fidx = atoi (buf + 1);

                            while (*p == '*' || *p == '+')
                            {
                                if (*p == '+')
                                {
                                    sound = 1;
                                }
                                else if (*p == '*')
                                {
                                    pulse = 1;
                                }
                                p++;
                            }

                            function_name_idx = Functions::search (p);
                            Locos::locos[loco_idx].set_function_type (fidx, function_name_idx, pulse, sound);
                        }
                        else if (! strcmp (buf, "MACRO_ACTION"))
                        {
                            LOCOACTION      la;
                            uint_fast8_t    midx;
                            uint_fast8_t    pidx;

                            midx = atoi (p);
                            p = strchr (p, ',');

                            if (p)
                            {
                                p++;
                                la.action = atoi (p);

                                p = strchr (p, ',');

                                if (p)
                                {
                                    p++;
                                    la.n_parameters = atoi (p);

                                    for (pidx = 0; pidx < la.n_parameters; pidx++)
                                    {
                                        p = strchr (p, ',');

                                        if (p)
                                        {
                                            p++;
                                            la.parameters[pidx] = atoi (p);
                                        }
                                        else
                                        {
                                            break;
                                        }
                                    }

                                    if (pidx == la.n_parameters)
                                    {
                                        uint_fast8_t actionidx = Locos::locos[loco_idx].add_macro_action (midx);

                                        if (actionidx == 0xFF)
                                        {
                                            Debug::printf (DEBUG_LEVEL_NONE, "%s: error: maximum number of loco macro actions reached.\n", fname);
                                            break;
                                        }

                                        Locos::locos[loco_idx].set_macro_action (midx, actionidx, &la);
                                    }
                                }
                            }
                        }
                    }
                }
                else if (in_addon_section)
                {
                    char *  p = strchr (buf, '=');

                    if (p)
                    {
                        *p++ = '\0';

                        if (! strcmp (buf, "NAME"))
                        {
                            AddOns::addons[addon_idx].set_name (p);
                        }
                        else if (! strcmp (buf, "ACTIVE"))
                        {
                            if (atoi(p) == 0)
                            {
                                AddOns::addons[addon_idx].deactivate();
                            }
                            else
                            {
                                AddOns::addons[addon_idx].activate();
                            }
                        }
                        else if (! strcmp (buf, "ADDR"))
                        {
                            AddOns::addons[addon_idx].set_addr (atoi(p));
                        }
                        else if (*buf == 'F' && *(buf + 1) >= '0' && *(buf + 1) <= '9')
                        {
                            uint_fast8_t    sound = 0;
                            uint_fast8_t    pulse = 0;
                            int             fidx;
                            uint_fast16_t   function_name_idx;

                            fidx = atoi (buf + 1);

                            while (*p == '*' || *p == '+')
                            {
                                if (*p == '+')
                                {
                                    sound = 1;
                                }
                                else if (*p == '*')
                                {
                                    pulse = 1;
                                }
                                p++;
                            }

                            function_name_idx = Functions::search (p);
                            AddOns::addons[addon_idx].set_function_type (fidx, function_name_idx, pulse, sound);
                        }
                        else if (! strcmp (buf, "LOCO"))
                        {
                            uint_fast16_t addon_loco_idx = atoi(p);

                            Locos::locos[addon_loco_idx].set_addon (addon_idx);
                            AddOns::addons[addon_idx].set_loco (addon_loco_idx);
                        }
                        else if (! strcmp (buf, "COUPLE"))
                        {
                            uint_fast16_t   addon_loco_idx;
                            uint_fast8_t    lfidx;
                            uint_fast8_t    afidx;

                            addon_loco_idx = atoi(p);

                            p = strchr (p, ',');

                            if (p)
                            {
                                p++;
                                lfidx = atoi(p);
                                p = strchr (p, ',');

                                if (p)
                                {
                                    p++;
                                    afidx = atoi(p);
                                    Locos::locos[addon_loco_idx].set_coupled_function (lfidx, afidx);
                                }
                            }
                        }
                    }
                }
            }
        }

        fclose (fp);
    }
    else
    {
        perror (fname);
        return (0);
    }

#if 0 // Test
    uint_fast8_t macroidx;
    uint_fast8_t actionidx;

    loco_idx = 0;
    macroidx = 0;

    static LOCOACTION la;

    actionidx = Locos::locos[loco_idx].add_macro_action (macroidx);
    la.action = LOCO_ACTION_SET_LOCO_FORWARD_DIRECTION;
    la.n_parameters = 2;
    la.parameters[0] = 0;
    la.parameters[1] = 1;
    Locos::locos[loco_idx].set_macro_action (macroidx, actionidx, &la);

    actionidx = Locos::locos[loco_idx].add_macro_action (macroidx);
    la.action = LOCO_ACTION_SET_LOCO_FUNCTION_ON;
    la.n_parameters = 2;
    la.parameters[0] = 0;
    la.parameters[1] = LOCO_F0;
    Locos::locos[loco_idx].set_macro_action (macroidx, actionidx, &la);

    actionidx = Locos::locos[loco_idx].add_macro_action (macroidx);
    la.action = LOCO_ACTION_SET_LOCO_FUNCTION_ON;
    la.n_parameters = 2;
    la.parameters[0] = 0;
    la.parameters[1] = LOCO_F3;
    Locos::locos[loco_idx].set_macro_action (macroidx, actionidx, &la);

    actionidx = Locos::locos[loco_idx].add_macro_action (macroidx);
    la.action = LOCO_ACTION_SET_LOCO_FUNCTION_ON;
    la.n_parameters = 2;
    la.parameters[0] = 20;
    la.parameters[1] = LOCO_F1;
    Locos::locos[loco_idx].set_macro_action (macroidx, actionidx, &la);

    actionidx = Locos::locos[loco_idx].add_macro_action (macroidx);
    la.action = LOCO_ACTION_SET_LOCO_SPEED;
    la.n_parameters = 3;
    la.parameters[0] = 50;
    la.parameters[1] = 30;
    la.parameters[2] = 50;
    Locos::locos[loco_idx].set_macro_action (macroidx, actionidx, &la);

    macroidx = 1;

    actionidx = Locos::locos[loco_idx].add_macro_action (macroidx);
    la.action = LOCO_ACTION_SET_LOCO_SPEED;
    la.n_parameters = 3;
    la.parameters[0] = 0;
    la.parameters[1] = 0;
    la.parameters[2] = 0;
    Locos::locos[loco_idx].set_macro_action (macroidx, actionidx, &la);

    actionidx = Locos::locos[loco_idx].add_macro_action (macroidx);
    la.action = LOCO_ACTION_SET_LOCO_ALL_FUNCTIONS_OFF;
    la.n_parameters = 1;
    la.parameters[0] = 50;
    Locos::locos[loco_idx].set_macro_action (macroidx, actionidx, &la);
#endif

    Locos::data_changed = false;
    AddOns::data_changed = false;
    return (1);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * FileIO::write_loco_ini () - write loco.ini
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
bool
FileIO::write_loco_ini (void)
{
    FILE *          fp;
    const char *    fname = "loco.ini";
    const char *    fname_bak = "loco.bak";
    uint_fast16_t   loco_idx;
    uint_fast16_t   addon_idx;
    uint_fast16_t   n_locos;
    uint_fast16_t   n_addons;
    uint_fast16_t   fidx;
    uint_fast8_t    midx;
    uint_fast8_t    n_actions;
    uint_fast8_t    actionidx;
    uint_fast8_t    pidx;
    LOCOACTION      la;
    bool            rtc = false;

    unlink (fname_bak);
    rename (fname, fname_bak);

    fp = fopen (fname, "w");

    if (fp)
    {
        n_locos = Locos::get_n_locos ();

        for (loco_idx = 0; loco_idx < n_locos; loco_idx++)
        {
            fprintf (fp, "[LOCO]\r\n");
            fprintf (fp, "NAME=%s\r\n", Locos::locos[loco_idx].get_name().c_str());
            fprintf (fp, "ACTIVE=%d\r\n", Locos::locos[loco_idx].is_active() ? 1 : 0);
            fprintf (fp, "ADDR=%u\r\n", (unsigned int) Locos::locos[loco_idx].get_addr ());
            fprintf (fp, "STEPS=%u\r\n", Locos::locos[loco_idx].get_speed_steps ());

            for (fidx = 0; fidx < MAX_LOCO_FUNCTIONS; fidx++)
            {
                uint16_t        function_name_idx   = Locos::locos[loco_idx].get_function_name_idx (fidx);
                std::string     fnname              = Locos::locos[loco_idx].get_function_name (fidx);

                bool pulse       = 0;
                bool sound       = 0;

                if (function_name_idx < MAX_FUNCTION_NAMES && fnname.length() > 0)
                {
                    pulse = Locos::locos[loco_idx].get_function_pulse (fidx);
                    sound = Locos::locos[loco_idx].get_function_sound (fidx);

                    fprintf (fp, "F%u=", (unsigned int) fidx);

                    if (pulse)
                    {
                        putc ('*', fp);
                    }

                    if (sound)
                    {
                        putc ('+', fp);
                    }

                    fprintf (fp, "%s\r\n", fnname.c_str());
                }
            }

            for (midx = 0; midx < MAX_LOCO_MACROS_PER_LOCO; midx++)
            {
                n_actions = Locos::locos[loco_idx].get_n_macro_actions (midx);

                for (actionidx = 0; actionidx < n_actions; actionidx++)
                {
                    if (Locos::locos[loco_idx].get_macro_action (midx, actionidx, &la))
                    {
                        fprintf (fp, "MACRO_ACTION=%d,%d,%d", midx, la.action, la.n_parameters);

                        for (pidx = 0; pidx < la.n_parameters; pidx++)
                        {
                            fprintf (fp, ",%d", la.parameters[pidx]);
                        }
                        fprintf (fp, "\r\n");
                    }
                }
            }
        }

        n_addons = AddOns::get_n_addons ();

        for (addon_idx = 0; addon_idx < n_addons; addon_idx++)
        {
            fprintf (fp, "[ADDON]\r\n");
            fprintf (fp, "NAME=%s\r\n", AddOns::addons[addon_idx].get_name().c_str());
            fprintf (fp, "ACTIVE=%d\r\n", AddOns::addons[addon_idx].is_active() ? 1 : 0);
            fprintf (fp, "ADDR=%u\r\n", (unsigned int) AddOns::addons[addon_idx].get_addr ());

            for (fidx = 0; fidx < MAX_LOCO_FUNCTIONS; fidx++)
            {
                uint16_t        function_name_idx   = AddOns::addons[addon_idx].get_function_name_idx (fidx);
                std::string     fnname              = AddOns::addons[addon_idx].get_function_name (fidx);

                bool pulse       = 0;
                bool sound       = 0;

                if (function_name_idx < MAX_FUNCTION_NAMES && fnname.length() > 0)
                {
                    pulse = AddOns::addons[addon_idx].get_function_pulse (fidx);
                    sound = AddOns::addons[addon_idx].get_function_sound (fidx);

                    fprintf (fp, "F%u=", (unsigned int) fidx);

                    if (pulse)
                    {
                        putc ('*', fp);
                    }

                    if (sound)
                    {
                        putc ('+', fp);
                    }

                    fprintf (fp, "%s\r\n", fnname.c_str());
                }
            }

            uint16_t addon_loco_idx = AddOns::addons[addon_idx].get_loco ();
            fprintf (fp, "LOCO=%u\r\n", (unsigned int) addon_loco_idx);

            uint_fast8_t lfidx;

            for (lfidx = 0; lfidx < MAX_LOCO_FUNCTIONS; lfidx++)
            {
                uint_fast8_t afidx = Locos::locos[addon_loco_idx].get_coupled_function (lfidx);

                if (afidx != 0xFF)
                {
                    fprintf (fp, "COUPLE=%u,%u,%u\r\n", addon_loco_idx, lfidx, afidx);
                }
            }
        }

        Locos::data_changed = false;
        AddOns::data_changed = false;

        rtc = true;
        fclose (fp);
    }
    else
    {
        perror (fname);
    }

    return rtc;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * FileIO::read_func_ini () - read central.ini, later loco.init
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
bool
FileIO::read_func_ini (void)
{
    const char *    fname = "func.ini";
    FILE *          fp;
    char            buf[80];

    fp = fopen (fname, "r");

    if (fp)
    {
        while (fgets (buf, 80, fp) != (char *) NULL)
        {
            trim (buf);

            if (*buf)
            {
                Functions::add (buf);
            }
        }

        fclose (fp);
    }
    else
    {
        perror (fname);
        return (false);
    }

    return (true);
}


bool
FileIO::read_switch_ini (void)
{
    const char *    fname = "switch.ini";
    FILE *          fp;
    char            buf[80];
    int             swidx = -1;
    int             rrgidx = -1;
    int             rridx = -1;
    int             subidx = -1;
    uint_fast8_t    section = 0;
    bool            rtc = false;

    fp = fopen (fname, "r");

    if (fp)
    {
        while (fgets (buf, 80, fp) != (char *) NULL)
        {
            trim (buf);

            if (! strcmp (buf, "[SWITCH]"))
            {
                if ((swidx = Switches::add ({})) == 0xFFFF)
                {
                    Debug::printf (DEBUG_LEVEL_NONE, "%s: error: maximum number of switches reached.\n", fname);
                    break;
                }

                Switches::switches[swidx].set_state (DCC_SWITCH_STATE_UNDEFINED);
                section = SWITCH_SECTION;
            }
            else if (! strcmp (buf, "[RAILROAD_GROUP]"))
            {
                if ((rrgidx = RailroadGroups::add ({})) == 0xFF)
                {
                    break;
                }

                section = RAILROAD_GROUP_SECTION;
            }
            else if (! strcmp (buf, "[RAILROAD]"))
            {
                if ((rridx = RailroadGroups::railroad_groups[rrgidx].add({})) == 0xFF)
                {
                    Debug::printf (DEBUG_LEVEL_NONE, "%s: error: maximum number of railroads reached.\n", fname);
                    break;
                }

                section = RAILROAD_SECTION;
            }
            else if (! strcmp (buf, "[RAILROAD_SWITCH]"))
            {
                if ((subidx = RailroadGroups::railroad_groups[rrgidx].railroads[rridx].add_switch()) == 0xFF)
                {
                    Debug::printf (DEBUG_LEVEL_NONE, "%s: error: maximum number of railroad switches reached.\n", fname);
                    break;
                }

                section = RAILROAD_SWITCH_SECTION;
            }

            switch (section)
            {
                case SWITCH_SECTION:
                {
                    if (! strncmp (buf, "NAME=", 5))
                    {
                        Switches::switches[swidx].set_name (buf + 5);
                    }
                    else if (! strncmp (buf, "ADDR=", 5))
                    {
                        Switches::switches[swidx].set_addr (atoi(buf + 5));
                    }
                    else if (! strncmp (buf, "FLAGS=", 6))
                    {
                        Switches::switches[swidx].set_flags (atoi(buf + 6));
                    }
                }
                break;

                case RAILROAD_GROUP_SECTION:
                {
                    if (! strncmp (buf, "NAME=", 5))
                    {
                        RailroadGroups::railroad_groups[rrgidx].set_name (buf + 5);
                    }
                }
                break;

                case RAILROAD_SECTION:
                {
                    if (! strncmp (buf, "NAME=", 5))
                    {
                        RailroadGroups::railroad_groups[rrgidx].railroads[rridx].set_name (buf + 5);
                    }
                    else if (! strncmp (buf, "LOCO=", 5))
                    {
                        uint_fast8_t loco_idx = atoi (buf + 5);

                        RailroadGroups::railroad_groups[rrgidx].railroads[rridx].set_link_loco (loco_idx);
                    }
                }
                break;

                case RAILROAD_SWITCH_SECTION:
                {
                    if (! strncmp (buf, "SWITCH=", 7))
                    {
                        RailroadGroups::railroad_groups[rrgidx].railroads[rridx].set_switch_idx (subidx, atoi(buf + 7));
                    }
                    else if (! strncmp (buf, "STATE=", 6))
                    {
                        RailroadGroups::railroad_groups[rrgidx].railroads[rridx].set_switch_state (subidx, atoi(buf + 6));
                    }
                }
                break;
            }
        }

        Switches::data_changed = false;
        RailroadGroups::data_changed = false;
        rtc = true;
        fclose (fp);
    }
    else
    {
        perror (fname);
    }

    return rtc;
}

bool
FileIO::write_switch_ini (void)
{
    const char *    fname = "switch.ini";
    const char *    fname_bak = "switch.bak";
    FILE *          fp;
    uint_fast16_t   n_switches;
    uint_fast16_t   swidx;
    uint_fast8_t    n_railroad_groups;
    uint_fast8_t    rrgidx;
    uint_fast8_t    n_railroads;
    uint_fast8_t    rridx;
    uint_fast8_t    subidx;
    bool            rtc = false;

    unlink (fname_bak);
    rename (fname, fname_bak);

    fp = fopen (fname, "w");

    if (fp)
    {
        n_switches = Switches::get_n_switches ();

        for (swidx = 0; swidx < n_switches; swidx++)
        {
            fprintf (fp, "[SWITCH]\r\n");
            fprintf (fp, "NAME=%s\r\n", Switches::switches[swidx].get_name().c_str());
            fprintf (fp, "ADDR=%u\r\n", (unsigned int) Switches::switches[swidx].get_addr());
            fprintf (fp, "FLAGS=%u\r\n", Switches::switches[swidx].get_flags());
        }

        n_railroad_groups = RailroadGroups::get_n_railroad_groups ();

        for (rrgidx = 0; rrgidx < n_railroad_groups; rrgidx++)
        {
            fprintf (fp, "[RAILROAD_GROUP]\r\n");
            fprintf (fp, "NAME=%s\r\n", RailroadGroups::railroad_groups[rrgidx].get_name().c_str());

            n_railroads = RailroadGroups::railroad_groups[rrgidx].get_n_railroads();

            for (rridx = 0; rridx < n_railroads; rridx++)
            {
                uint_fast16_t   loco_idx = RailroadGroups::railroad_groups[rrgidx].railroads[rridx].get_link_loco();

                fprintf (fp, "[RAILROAD]\r\n");
                fprintf (fp, "NAME=%s\r\n", RailroadGroups::railroad_groups[rrgidx].railroads[rridx].get_name().c_str());
                fprintf (fp, "LOCO=%u\r\n", (unsigned int) loco_idx);

                n_switches = RailroadGroups::railroad_groups[rrgidx].railroads[rridx].get_n_switches();

                for (subidx = 0; subidx < n_switches; subidx++)
                {
                    fprintf (fp, "[RAILROAD_SWITCH]\r\n");
                    fprintf (fp, "SWITCH=%u\r\n", (unsigned int) RailroadGroups::railroad_groups[rrgidx].railroads[rridx].get_switch_idx(subidx));
                    fprintf (fp, "STATE=%u\r\n", RailroadGroups::railroad_groups[rrgidx].railroads[rridx].get_switch_state(subidx));
                }
            }
        }

        Switches::data_changed = false;
        RailroadGroups::data_changed = false;
        rtc = true;
        fclose (fp);
    }

    return rtc;
}

bool
FileIO::read_signal_ini (void)
{
    const char *    fname = "signal.ini";
    FILE *          fp;
    char            buf[80];
    int             sigidx = -1;
    uint_fast8_t    section = 0;
    bool            rtc = false;

    fp = fopen (fname, "r");

    if (fp)
    {
        while (fgets (buf, 80, fp) != (char *) NULL)
        {
            trim (buf);

            if (! strcmp (buf, "[SIGNAL]"))
            {
                if ((sigidx = Signals::add ({})) == 0xFFFF)
                {
                    Debug::printf (DEBUG_LEVEL_NONE, "%s: error: maximum number of signals reached.\n", fname);
                    break;
                }

                Signals::signals[sigidx].set_state (DCC_SWITCH_STATE_UNDEFINED);
                section = SIGNAL_SECTION;
            }

            switch (section)
            {
                case SIGNAL_SECTION:
                {
                    if (! strncmp (buf, "NAME=", 5))
                    {
                        Signals::signals[sigidx].set_name (buf + 5);
                    }
                    else if (! strncmp (buf, "ADDR=", 5))
                    {
                        Signals::signals[sigidx].set_addr (atoi(buf + 5));
                    }
                }
                break;
            }
        }

        Signals::data_changed = false;
        rtc = true;
        fclose (fp);
    }
    else
    {
        perror (fname);
    }

    return rtc;
}

bool
FileIO::write_signal_ini (void)
{
    const char *    fname = "signal.ini";
    const char *    fname_bak = "signal.bak";
    FILE *          fp;
    uint_fast16_t   n_signals;
    uint_fast16_t   sigidx;
    bool            rtc = false;

    unlink (fname_bak);
    rename (fname, fname_bak);

    fp = fopen (fname, "w");

    if (fp)
    {
        n_signals = Signals::get_n_signals ();

        for (sigidx = 0; sigidx < n_signals; sigidx++)
        {
            fprintf (fp, "[SIGNAL]\r\n");
            fprintf (fp, "NAME=%s\r\n", Signals::signals[sigidx].get_name().c_str());
            fprintf (fp, "ADDR=%u\r\n", (unsigned int) Signals::signals[sigidx].get_addr());
        }

        Signals::data_changed = false;
        rtc = true;
        fclose (fp);
    }

    return rtc;
}

bool
FileIO::read_led_ini (void)
{
    const char *    fname = "led.ini";
    FILE *          fp;
    char            buf[80];
    int             sigidx = -1;
    uint_fast8_t    section = 0;
    bool            rtc = false;

    fp = fopen (fname, "r");

    if (fp)
    {
        while (fgets (buf, 80, fp) != (char *) NULL)
        {
            trim (buf);

            if (! strcmp (buf, "[LED]"))
            {
                if ((sigidx = Leds::add ({})) == 0xFFFF)
                {
                    Debug::printf (DEBUG_LEVEL_NONE, "%s: error: maximum number of leds reached.\n", fname);
                    break;
                }

                Leds::led_groups[sigidx].set_state (0x00);
                section = LED_SECTION;
            }

            switch (section)
            {
                case LED_SECTION:
                {
                    if (! strncmp (buf, "NAME=", 5))
                    {
                        Leds::led_groups[sigidx].set_name (buf + 5);
                    }
                    else if (! strncmp (buf, "ADDR=", 5))
                    {
                        Leds::led_groups[sigidx].set_addr (atoi(buf + 5));
                    }
                }
                break;
            }
        }

        Leds::data_changed = false;
        rtc = true;
        fclose (fp);
    }
    else
    {
        perror (fname);
    }

    return rtc;
}

bool
FileIO::write_led_ini (void)
{
    const char *    fname = "led.ini";
    const char *    fname_bak = "led.bak";
    FILE *          fp;
    uint_fast16_t   n_led_groups;
    uint_fast16_t   sigidx;
    bool            rtc = false;

    unlink (fname_bak);
    rename (fname, fname_bak);

    fp = fopen (fname, "w");

    if (fp)
    {
        n_led_groups = Leds::get_n_led_groups ();

        for (sigidx = 0; sigidx < n_led_groups; sigidx++)
        {
            fprintf (fp, "[LED]\r\n");
            fprintf (fp, "NAME=%s\r\n", Leds::led_groups[sigidx].get_name().c_str());
            fprintf (fp, "ADDR=%u\r\n", (unsigned int) Leds::led_groups[sigidx].get_addr());
        }

        Leds::data_changed = false;
        rtc = true;
        fclose (fp);
    }

    return rtc;
}

bool
FileIO::read_s88_ini (void)
{
    const char *    fname = "s88.ini";
    FILE *          fp;
    char            buf[80];
    uint_fast16_t   coidx = 0xFFFF;
    uint_fast8_t    section = 0;
    bool            rtc = false;

    fp = fopen (fname, "r");

    if (fp)
    {
        while (fgets (buf, 80, fp) != (char *) NULL)
        {
            trim (buf);

            if (! strcmp (buf, "[CONTACT]"))
            {
                if ((coidx = S88::add({})) == 0xFFFF)
                {
                    Debug::printf (DEBUG_LEVEL_NONE, "%s: error: maximum number of contacts reached.\n", fname);
                    break;
                }

                section = CONTACT_SECTION;
            }

            switch (section)
            {
                case CONTACT_SECTION:
                {
                    if (! strncmp (buf, "NAME=", 5))
                    {
                        S88::contacts[coidx].set_name (buf + 5);
                    }
                    else if (! strncmp (buf, "RAILROAD=", 9))
                    {
                        uint_fast8_t    rrgidx;
                        uint_fast8_t    rridx;

                        rrgidx      = htoi (buf + 9, 2);
                        rridx       = htoi (buf + 11, 2);
                        S88::contacts[coidx].set_link_railroad (rrgidx, rridx);
                    }
                    else if (! strncmp (buf, "ACTION_IN=", 10))
                    {
                        CONTACT_ACTION  ca;
                        uint_fast8_t    paidx;
                        char *          p;

                        ca.action = atoi (buf + 10);

                        p = strchr (buf + 10, ',');

                        if (p)
                        {
                            p++;

                            ca.n_parameters = atoi (p);

                            for (paidx = 0; paidx < ca.n_parameters; paidx++)
                            {
                                p = strchr (p, ',');

                                if (p)
                                {
                                    p++;
                                    ca.parameters[paidx] = atoi (p);
                                }
                                else
                                {
                                    break;
                                }
                            }

                            if (paidx == ca.n_parameters)
                            {
                                uint_fast8_t caidx = S88::contacts[coidx].add_contact_action (true);

                                if (caidx == 0xFF)
                                {
                                    Debug::printf (DEBUG_LEVEL_NONE, "%s: error: maximum number of contact actions (in) reached.\n", fname);
                                    break;
                                }

                                S88::contacts[coidx].set_contact_action (true, caidx, &ca);
                            }
                        }
                    }
                    else if (! strncmp (buf, "ACTION_OUT=", 11))
                    {
                        CONTACT_ACTION  ca;
                        uint_fast8_t    paidx;
                        char *          p;

                        ca.action = atoi (buf + 11);

                        p = strchr (buf + 11, ',');

                        if (p)
                        {
                            p++;

                            ca.n_parameters = atoi (p);

                            for (paidx = 0; paidx < ca.n_parameters; paidx++)
                            {
                                p = strchr (p, ',');

                                if (p)
                                {
                                    p++;
                                    ca.parameters[paidx] = atoi (p);
                                }
                                else
                                {
                                    break;
                                }
                            }

                            if (paidx == ca.n_parameters)
                            {
                                uint_fast8_t caidx = S88::contacts[coidx].add_contact_action (false);

                                if (caidx == 0xFF)
                                {
                                    Debug::printf (DEBUG_LEVEL_NONE, "%s: error: maximum number of contact actions (out) reached.\n", fname);
                                    break;
                                }

                                S88::contacts[coidx].set_contact_action (false, caidx, &ca);
                            }
                        }
                    }
                    break;
                }
            }
        }

        fclose (fp);
        S88::data_changed = false;
        rtc = true;
    }
    else
    {
        perror (fname);
    }

    return rtc;
}

bool
FileIO::write_s88_ini (void)
{
    const char *    fname = "s88.ini";
    const char *    fname_bak = "s88.bak";
    FILE *          fp;
    uint_fast16_t   n_contacts;
    uint_fast8_t    n_contact_actions_in;
    uint_fast8_t    n_contact_actions_out;
    uint_fast16_t   coidx;
    uint_fast8_t    caidx;
    uint_fast8_t    paidx;
    uint_fast16_t   rrg_rr_idx;
    CONTACT_ACTION  ca;
    bool            rtc = false;

    unlink (fname_bak);
    rename (fname, fname_bak);

    fp = fopen (fname, "w");

    if (fp)
    {
        n_contacts = S88::get_n_contacts ();

        for (coidx = 0; coidx < n_contacts; coidx++)
        {
            fprintf (fp, "[CONTACT]\r\n");
            fprintf (fp, "NAME=%s\r\n", S88::contacts[coidx].get_name().c_str());

            rrg_rr_idx = S88::contacts[coidx].get_link_railroad();
            fprintf (fp, "RAILROAD=%04X\r\n", (unsigned int) rrg_rr_idx);

            n_contact_actions_in = S88::contacts[coidx].get_n_contact_actions (true);

            for (caidx = 0; caidx < n_contact_actions_in; caidx++)
            {
                if (S88::contacts[coidx].get_contact_action (true, caidx, &ca))
                {
                    fprintf (fp, "ACTION_IN=%d,%d", ca.action, ca.n_parameters);

                    for (paidx = 0; paidx < ca.n_parameters; paidx++)
                    {
                        fprintf (fp, ",%d", ca.parameters[paidx]);
                    }
                    fprintf (fp, "\r\n");
                }
            }

            n_contact_actions_out = S88::contacts[coidx].get_n_contact_actions (false);

            for (caidx = 0; caidx < n_contact_actions_out; caidx++)
            {
                if (S88::contacts[coidx].get_contact_action (false, caidx, &ca))
                {
                    fprintf (fp, "ACTION_OUT=%d,%d", ca.action, ca.n_parameters);

                    for (paidx = 0; paidx < ca.n_parameters; paidx++)
                    {
                        fprintf (fp, ",%d", ca.parameters[paidx]);
                    }
                    fprintf (fp, "\r\n");
                }
            }
        }

        rtc = true;
        S88::data_changed = false;
        fclose (fp);
    }

    return rtc;
}

bool
FileIO::read_rcl_ini (void)
{
    const char *    fname = "rcl.ini";
    FILE *          fp;
    char            buf[80];
    uint_fast8_t    trackidx = 0xFF;
    uint_fast8_t    section = 0;
    bool            rtc = false;

    fp = fopen (fname, "r");

    if (fp)
    {
        while (fgets (buf, 80, fp) != (char *) NULL)
        {
            trim (buf);

            if (! strcmp (buf, "[RCLTRACK]"))
            {
                if ((trackidx = RCL::add({})) == 0xFF)
                {
                    Debug::printf (DEBUG_LEVEL_NONE, "%s: error: maximum number of tracks reached.\n", fname);
                    break;
                }

                section = RCLTRACK_SECTION;
            }

            switch (section)
            {
                case RCLTRACK_SECTION:
                {
                    if (! strncmp (buf, "NAME=", 5))
                    {
                        RCL::tracks[trackidx].set_name (buf + 5);
                    }
                    else if (! strncmp (buf, "FLAGS=", 6))
                    {
                        if (! strcmp (buf + 6, "BLOCK_PROTECTION"))
                        {
                            RCL::tracks[trackidx].set_flags (RCL_TRACK_FLAG_BLOCK_PROTECTION);
                        }
                    }
                    else if (! strncmp (buf, "ACTION_IN=", 10))
                    {
                        RCL_TRACK_ACTION    track_action;
                        uint_fast8_t        paidx;
                        char *              p;

                        track_action.condition = atoi (buf + 10);

                        p = strchr (buf + 10, ',');

                        if (p)
                        {
                            p++;

                            track_action.condition_destination = atoi (p);

                            p = strchr (p, ',');

                            if (p)
                            {
                                p++;

                                track_action.action = atoi (p);

                                p = strchr (p, ',');

                                if (p)
                                {
                                    p++;

                                    track_action.n_parameters = atoi (p);

                                    for (paidx = 0; paidx < track_action.n_parameters; paidx++)
                                    {
                                        p = strchr (p, ',');

                                        if (p)
                                        {
                                            p++;
                                            track_action.parameters[paidx] = atoi (p);
                                        }
                                        else
                                        {
                                            break;
                                        }
                                    }

                                    if (paidx == track_action.n_parameters)
                                    {
                                        uint_fast8_t track_action_idx = RCL::tracks[trackidx].add_track_action (true);

                                        if (track_action_idx == 0xFF)
                                        {
                                            Debug::printf (DEBUG_LEVEL_NONE, "%s: error: maximum number of track actions (in) reached.\n", fname);
                                            break;
                                        }

                                        RCL::tracks[trackidx].set_track_action (true, track_action_idx, &track_action);
                                    }
                                }
                            }
                        }
                    }
                    else if (! strncmp (buf, "ACTION_OUT=", 11))
                    {
                        RCL_TRACK_ACTION    track_action;
                        uint_fast8_t        paidx;
                        char *              p;

                        track_action.condition = atoi (buf + 11);

                        p = strchr (buf + 11, ',');

                        if (p)
                        {
                            p++;

                            track_action.condition_destination = atoi (p);

                            p = strchr (p, ',');

                            if (p)
                            {
                                p++;

                                track_action.action = atoi (p);

                                p = strchr (p, ',');

                                if (p)
                                {
                                    p++;

                                    track_action.n_parameters = atoi (p);

                                    for (paidx = 0; paidx < track_action.n_parameters; paidx++)
                                    {
                                        p = strchr (p, ',');

                                        if (p)
                                        {
                                            p++;
                                            track_action.parameters[paidx] = atoi (p);
                                        }
                                        else
                                        {
                                            break;
                                        }
                                    }

                                    if (paidx == track_action.n_parameters)
                                    {
                                        uint_fast8_t track_action_idx = RCL::tracks[trackidx].add_track_action (false);

                                        if (track_action_idx == 0xFF)
                                        {
                                            Debug::printf (DEBUG_LEVEL_NONE, "%s: error: maximum number of track actions (out) reached.\n", fname);
                                            break;
                                        }

                                        RCL::tracks[trackidx].set_track_action (false, track_action_idx, &track_action);
                                    }
                                }
                            }
                        }
                    }
                    break;
                }
            }
        }

        fclose (fp);
        RCL::data_changed = false;
        rtc = true;
    }
    else
    {
        perror (fname);
    }

    return rtc;
}

bool
FileIO::write_rcl_ini (void)
{
    const char *        fname = "rcl.ini";
    const char *        fname_bak = "rcl.bak";
    FILE *              fp;
    uint_fast8_t        trackidx;
    uint_fast8_t        paidx;
    RCL_TRACK_ACTION    track_action;
    bool                rtc = false;

    unlink (fname_bak);
    rename (fname, fname_bak);

    fp = fopen (fname, "w");

    if (fp)
    {
        uint_fast8_t    n_tracks = RCL::get_n_tracks ();
        uint_fast8_t    n_track_actions;
        uint_fast8_t    track_action_idx;

        for (trackidx = 0; trackidx < n_tracks; trackidx++)
        {
            uint32_t    flags;

            fprintf (fp, "[RCLTRACK]\r\n");
            fprintf (fp, "NAME=%s\r\n", RCL::tracks[trackidx].get_name().c_str());

            flags = RCL::tracks[trackidx].get_flags();

            if (flags & RCL_TRACK_FLAG_BLOCK_PROTECTION)
            {
                fprintf (fp, "FLAGS=BLOCK_PROTECTION\r\n");
            }

            n_track_actions = RCL::tracks[trackidx].get_n_track_actions (true);

            for (track_action_idx = 0; track_action_idx < n_track_actions; track_action_idx++)
            {
                if (RCL::tracks[trackidx].get_track_action (true, track_action_idx, &track_action))
                {
                    fprintf (fp, "ACTION_IN=%d,%d,%d,%d", track_action.condition, track_action.condition_destination, track_action.action, track_action.n_parameters);

                    for (paidx = 0; paidx < track_action.n_parameters; paidx++)
                    {
                        fprintf (fp, ",%d", track_action.parameters[paidx]);
                    }

                    fprintf (fp, "\r\n");
                }
            }

            n_track_actions = RCL::tracks[trackidx].get_n_track_actions (false);

            for (track_action_idx = 0; track_action_idx < n_track_actions; track_action_idx++)
            {
                if (RCL::tracks[trackidx].get_track_action (false, track_action_idx, &track_action))
                {
                    fprintf (fp, "ACTION_OUT=%d,%d,%d,%d", track_action.condition, track_action.condition_destination, track_action.action, track_action.n_parameters);

                    for (paidx = 0; paidx < track_action.n_parameters; paidx++)
                    {
                        fprintf (fp, ",%d", track_action.parameters[paidx]);
                    }
                    fprintf (fp, "\r\n");
                }
            }
        }

        rtc = true;
        RCL::data_changed = false;
        fclose (fp);
    }

    return rtc;
}

void
FileIO::read_all_ini_files (void)
{
    (void) FileIO::read_fm22_ini ();
    (void) FileIO::read_func_ini ();
    (void) FileIO::read_loco_ini ();
    (void) FileIO::read_switch_ini ();
    (void) FileIO::read_signal_ini ();
    (void) FileIO::read_led_ini ();
    (void) FileIO::read_s88_ini ();
    (void) FileIO::read_rcl_ini ();
}

bool
FileIO::write_all_ini_files (void)
{
    bool rtc = true;

    if (rtc && (FM22::data_changed))
    {
        rtc = write_fm22_ini ();
    }

    if (rtc && (Locos::data_changed || AddOns::data_changed))
    {
        rtc = write_loco_ini ();
    }

    if (rtc && (Switches::data_changed || RailroadGroups::data_changed))
    {
        rtc = write_switch_ini ();
    }

    if (rtc && (Signals::data_changed))
    {
        rtc = write_signal_ini ();
    }

    if (rtc && (Leds::data_changed))
    {
        rtc = write_led_ini ();
    }

    if (rtc && S88::data_changed)
    {
        rtc = write_s88_ini ();
    }

    if (rtc && RCL::data_changed)
    {
        rtc = write_rcl_ini ();
    }

    return rtc;
}
