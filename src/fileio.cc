/*-------------------------------------------------------------------------------------------------------------------------------------------
 * fileio.c - file I/O
 *-------------------------------------------------------------------------------------------------------------------------------------------
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
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <string>
#include "dcc.h"
#include "func.h"
#include "loco.h"
#include "addon.h"
#include "switch.h"
#include "railroad.h"
#include "s88.h"
#include "rcl.h"
#include "base.h"
#include "debug.h"
#include "fileio.h"

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
                loco_idx = Loco::add ();

                if (loco_idx == 0xFFFF)
                {
                    Debug::printf (DEBUG_LEVEL_NONE, "%s: error: maximum number of locos reached.\n", fname);
                    break;
                }

                Loco::activate (loco_idx);
                in_loco_section = true;
                in_addon_section = false;
            }
            else if (! strcmp (buf, "[ADDON]"))
            {
                addon_idx = AddOn::add ();

                if (addon_idx == 0xFFFF)
                {
                    Debug::printf (DEBUG_LEVEL_NONE, "%s: error: maximum number of addons reached.\n", fname);
                }

                AddOn::activate (addon_idx);
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
                            Loco::set_name (loco_idx, p);
                        }
                        else if (! strcmp (buf, "ADDR"))
                        {
                            Loco::set_addr (loco_idx, atoi(p));
                        }
                        else if (! strcmp (buf, "STEPS"))
                        {
                            Loco::set_speed_steps (loco_idx, atoi(p));
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
                            Loco::set_function_type (loco_idx, fidx, function_name_idx, pulse, sound);
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
                                        uint_fast8_t actionidx = Loco::add_macro_action (loco_idx, midx);

                                        if (actionidx == 0xFF)
                                        {
                                            Debug::printf (DEBUG_LEVEL_NONE, "%s: error: maximum number of loco macro actions reached.\n", fname);
                                            break;
                                        }

                                        Loco::set_macro_action (loco_idx, midx, actionidx, &la);
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
                            AddOn::set_name (addon_idx, p);
                        }
                        else if (! strcmp (buf, "ADDR"))
                        {
                            AddOn::set_addr (addon_idx, atoi(p));
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
                            AddOn::set_function_type (addon_idx, fidx, function_name_idx, pulse, sound);
                        }
                        else if (! strcmp (buf, "LOCO"))
                        {
                            uint_fast16_t addon_loco_idx = atoi(p);

                            Loco::set_addon (addon_loco_idx, addon_idx);
                            AddOn::set_loco (addon_idx, addon_loco_idx);
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
                                    Loco::set_coupled_function (addon_loco_idx, lfidx, afidx);
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

    actionidx = Loco::add_macro_action (loco_idx, macroidx);
    la.action = LOCO_ACTION_SET_LOCO_FORWARD_DIRECTION;
    la.n_parameters = 2;
    la.parameters[0] = 0;
    la.parameters[1] = 1;
    Loco::set_macro_action (loco_idx, macroidx, actionidx, &la);

    actionidx = Loco::add_macro_action (loco_idx, macroidx);
    la.action = LOCO_ACTION_SET_LOCO_FUNCTION_ON;
    la.n_parameters = 2;
    la.parameters[0] = 0;
    la.parameters[1] = LOCO_F0;
    Loco::set_macro_action (loco_idx, macroidx, actionidx, &la);

    actionidx = Loco::add_macro_action (loco_idx, macroidx);
    la.action = LOCO_ACTION_SET_LOCO_FUNCTION_ON;
    la.n_parameters = 2;
    la.parameters[0] = 0;
    la.parameters[1] = LOCO_F3;
    Loco::set_macro_action (loco_idx, macroidx, actionidx, &la);

    actionidx = Loco::add_macro_action (loco_idx, macroidx);
    la.action = LOCO_ACTION_SET_LOCO_FUNCTION_ON;
    la.n_parameters = 2;
    la.parameters[0] = 20;
    la.parameters[1] = LOCO_F1;
    Loco::set_macro_action (loco_idx, macroidx, actionidx, &la);

    actionidx = Loco::add_macro_action (loco_idx, macroidx);
    la.action = LOCO_ACTION_SET_LOCO_SPEED;
    la.n_parameters = 3;
    la.parameters[0] = 50;
    la.parameters[1] = 30;
    la.parameters[2] = 50;
    Loco::set_macro_action (loco_idx, macroidx, actionidx, &la);

    macroidx = 1;

    actionidx = Loco::add_macro_action (loco_idx, macroidx);
    la.action = LOCO_ACTION_SET_LOCO_SPEED;
    la.n_parameters = 3;
    la.parameters[0] = 0;
    la.parameters[1] = 0;
    la.parameters[2] = 0;
    Loco::set_macro_action (loco_idx, macroidx, actionidx, &la);

    actionidx = Loco::add_macro_action (loco_idx, macroidx);
    la.action = LOCO_ACTION_SET_LOCO_ALL_FUNCTIONS_OFF;
    la.n_parameters = 1;
    la.parameters[0] = 50;
    Loco::set_macro_action (loco_idx, macroidx, actionidx, &la);
#endif

    Loco::data_changed = false;
    AddOn::data_changed = false;
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
        n_locos = Loco::get_n_locos ();

        for (loco_idx = 0; loco_idx < n_locos; loco_idx++)
        {
            fprintf (fp, "[LOCO]\r\n");
            fprintf (fp, "NAME=%s\r\n", Loco::get_name (loco_idx));
            fprintf (fp, "ADDR=%u\r\n", (unsigned int) Loco::get_addr (loco_idx));
            fprintf (fp, "STEPS=%u\r\n", Loco::get_speed_steps (loco_idx));

            for (fidx = 0; fidx < MAX_LOCO_FUNCTIONS; fidx++)
            {
                uint16_t        function_name_idx   = Loco::get_function_name_idx (loco_idx, fidx);
                std::string     fnname              = Loco::get_function_name (loco_idx, fidx);

                bool pulse       = 0;
                bool sound       = 0;

                if (function_name_idx < MAX_FUNCTION_NAMES && fnname.length() > 0)
                {
                    pulse = Loco::get_function_pulse (loco_idx, fidx);
                    sound = Loco::get_function_sound (loco_idx, fidx);

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
                n_actions = Loco::get_n_macro_actions (loco_idx, midx);

                for (actionidx = 0; actionidx < n_actions; actionidx++)
                {
                    if (Loco::get_macro_action (loco_idx, midx, actionidx, &la))
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

        n_addons = AddOn::get_n_addons ();

        for (addon_idx = 0; addon_idx < n_addons; addon_idx++)
        {
            fprintf (fp, "[ADDON]\r\n");
            fprintf (fp, "NAME=%s\r\n", AddOn::get_name (addon_idx));
            fprintf (fp, "ADDR=%u\r\n", (unsigned int) AddOn::get_addr (addon_idx));

            for (fidx = 0; fidx < MAX_LOCO_FUNCTIONS; fidx++)
            {
                uint16_t        function_name_idx   = AddOn::get_function_name_idx (addon_idx, fidx);
                std::string     fnname              = AddOn::get_function_name (addon_idx, fidx);

                bool pulse       = 0;
                bool sound       = 0;

                if (function_name_idx < MAX_FUNCTION_NAMES && fnname.length() > 0)
                {
                    pulse = AddOn::get_function_pulse (addon_idx, fidx);
                    sound = AddOn::get_function_sound (addon_idx, fidx);

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

            uint16_t addon_loco_idx = AddOn::get_loco (addon_idx);
            fprintf (fp, "LOCO=%u\r\n", (unsigned int) addon_loco_idx);

            uint_fast8_t lfidx;

            for (lfidx = 0; lfidx < MAX_LOCO_FUNCTIONS; lfidx++)
            {
                uint_fast8_t afidx = Loco::get_coupled_function (addon_loco_idx, lfidx);

                if (afidx != 0xFF)
                {
                    fprintf (fp, "COUPLE=%u,%u,%u\r\n", addon_loco_idx, lfidx, afidx);
                }
            }
        }

        Loco::data_changed = false;
        AddOn::data_changed = false;

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


#define SWITCH_SECTION              1
#define RAILROAD_GROUP_SECTION      2
#define RAILROAD_SECTION            3
#define RAILROAD_SWITCH_SECTION     4

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
                if ((swidx = Switch::add ()) == 0xFFFF)
                {
                    Debug::printf (DEBUG_LEVEL_NONE, "%s: error: maximum number of switches reached.\n", fname);
                    break;
                }

                Switch::set_state (swidx, DCC_SWITCH_STATE_UNDEFINED);
                section = SWITCH_SECTION;
            }
            else if (! strcmp (buf, "[RAILROAD_GROUP]"))
            {
                if ((rrgidx = Railroad::group_add ()) == 0xFF)
                {
                    break;
                }

                section = RAILROAD_GROUP_SECTION;
            }
            else if (! strcmp (buf, "[RAILROAD]"))
            {
                if ((rridx = Railroad::add (rrgidx)) == 0xFF)
                {
                    Debug::printf (DEBUG_LEVEL_NONE, "%s: error: maximum number of railroads reached.\n", fname);
                    break;
                }

                section = RAILROAD_SECTION;
            }
            else if (! strcmp (buf, "[RAILROAD_SWITCH]"))
            {
                if ((subidx = Railroad::add_switch (rrgidx, rridx)) == 0xFF)
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
                        Switch::set_name (swidx, buf + 5);
                    }
                    else if (! strncmp (buf, "ADDR=", 5))
                    {
                        Switch::set_addr (swidx, atoi(buf + 5));
                    }
                    else if (! strncmp (buf, "FLAGS=", 6))
                    {
                        Switch::set_flags (swidx, atoi(buf + 6));
                    }
                }
                break;

                case RAILROAD_GROUP_SECTION:
                {
                    if (! strncmp (buf, "NAME=", 5))
                    {
                        Railroad::group_setname (rrgidx, buf + 5);
                    }
                }
                break;

                case RAILROAD_SECTION:
                {
                    if (! strncmp (buf, "NAME=", 5))
                    {
                        Railroad::set_name (rrgidx, rridx, buf + 5);
                    }
                    else if (! strncmp (buf, "LOCO=", 5))
                    {
                        uint_fast8_t loco_idx = atoi (buf + 5);

                        Railroad::set_link_loco (rrgidx, rridx, loco_idx);
                    }
                }
                break;

                case RAILROAD_SWITCH_SECTION:
                {
                    if (! strncmp (buf, "SWITCH=", 7))
                    {
                        Railroad::set_switch_idx (rrgidx, rridx, subidx, atoi(buf + 7));
                    }
                    else if (! strncmp (buf, "STATE=", 6))
                    {
                        Railroad::set_switch_state (rrgidx, rridx, subidx, atoi(buf + 6));
                    }
                }
                break;
            }
        }

        Switch::data_changed = false;
        Railroad::data_changed = false;
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
        n_switches = Switch::get_n_switches ();

        for (swidx = 0; swidx < n_switches; swidx++)
        {
            fprintf (fp, "[SWITCH]\r\n");
            fprintf (fp, "NAME=%s\r\n", Switch::get_name (swidx));
            fprintf (fp, "ADDR=%u\r\n", (unsigned int) Switch::get_addr (swidx));
            fprintf (fp, "FLAGS=%u\r\n", Switch::get_flags (swidx));
        }

        n_railroad_groups = Railroad::get_n_railroad_groups ();

        for (rrgidx = 0; rrgidx < n_railroad_groups; rrgidx++)
        {
            fprintf (fp, "[RAILROAD_GROUP]\r\n");
            fprintf (fp, "NAME=%s\r\n", Railroad::group_getname (rrgidx));

            n_railroads = Railroad::get_n_railroads (rrgidx);

            for (rridx = 0; rridx < n_railroads; rridx++)
            {
                uint_fast16_t   loco_idx = Railroad::get_link_loco (rrgidx, rridx);

                fprintf (fp, "[RAILROAD]\r\n");
                fprintf (fp, "NAME=%s\r\n", Railroad::get_name (rrgidx, rridx));
                fprintf (fp, "LOCO=%u\r\n", (unsigned int) loco_idx);

                n_switches = Railroad::get_n_switches (rrgidx, rridx);

                for (subidx = 0; subidx < n_switches; subidx++)
                {
                    fprintf (fp, "[RAILROAD_SWITCH]\r\n");
                    fprintf (fp, "SWITCH=%u\r\n", (unsigned int) Railroad::get_switch_idx(rrgidx, rridx, subidx));
                    fprintf (fp, "STATE=%u\r\n", Railroad::get_switch_state (rrgidx, rridx, subidx));
                }
            }
        }

        Switch::data_changed = false;
        Railroad::data_changed = false;
        rtc = true;
        fclose (fp);
    }

    return rtc;
}

#define CONTACT_SECTION             1

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
                if ((coidx = S88::add_contact ()) == 0xFFFF)
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
                        S88::set_name (coidx, buf + 5);
                    }
                    else if (! strncmp (buf, "RAILROAD=", 9))
                    {
                        uint_fast8_t    rrgidx;
                        uint_fast8_t    rridx;

                        rrgidx      = htoi (buf + 9, 2);
                        rridx       = htoi (buf + 11, 2);
                        S88::set_link_railroad (coidx, rrgidx, rridx);
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
                                uint_fast8_t caidx = S88::add_contact_action (true, coidx);

                                if (caidx == 0xFF)
                                {
                                    Debug::printf (DEBUG_LEVEL_NONE, "%s: error: maximum number of contact actions (in) reached.\n", fname);
                                    break;
                                }

                                S88::set_contact_action (true, coidx, caidx, &ca);
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
                                uint_fast8_t caidx = S88::add_contact_action (false, coidx);

                                if (caidx == 0xFF)
                                {
                                    Debug::printf (DEBUG_LEVEL_NONE, "%s: error: maximum number of contact actions (out) reached.\n", fname);
                                    break;
                                }

                                S88::set_contact_action (false, coidx, caidx, &ca);
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
            fprintf (fp, "NAME=%s\r\n", S88::get_name (coidx));

            rrg_rr_idx = S88::get_link_railroad (coidx);
            fprintf (fp, "RAILROAD=%04X\r\n", (unsigned int) rrg_rr_idx);

            n_contact_actions_in = S88::get_n_contact_actions (true, coidx);

            for (caidx = 0; caidx < n_contact_actions_in; caidx++)
            {
                if (S88::get_contact_action (true, coidx, caidx, &ca))
                {
                    fprintf (fp, "ACTION_IN=%d,%d", ca.action, ca.n_parameters);

                    for (paidx = 0; paidx < ca.n_parameters; paidx++)
                    {
                        fprintf (fp, ",%d", ca.parameters[paidx]);
                    }
                    fprintf (fp, "\r\n");
                }
            }

            n_contact_actions_out = S88::get_n_contact_actions (false, coidx);

            for (caidx = 0; caidx < n_contact_actions_out; caidx++)
            {
                if (S88::get_contact_action (false, coidx, caidx, &ca))
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

#define RCLTRACK_SECTION             1

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
                if ((trackidx = RCL::add_track ()) == 0xFF)
                {
                    Debug::printf (DEBUG_LEVEL_NONE, "%s: error: maximum number of tracks reached.\n", fname);
                    break;
                }

//              RCL::tracks[trackidx].flags |= RCL_TRACK_FLAG_BLOCK_PROTECTION; // hack

                section = RCLTRACK_SECTION;
            }

            switch (section)
            {
                case RCLTRACK_SECTION:
                {
                    if (! strncmp (buf, "NAME=", 5))
                    {
                        RCL::set_name (trackidx, buf + 5);
                    }
                    else if (! strncmp (buf, "FLAGS=", 6))
                    {
                        if (! strcmp (buf + 6, "BLOCK_PROTECTION"))
                        {
                            RCL::set_flags (trackidx, RCL_TRACK_FLAG_BLOCK_PROTECTION);
                        }
                    }
                    else if (! strncmp (buf, "ACTION_IN=", 10))
                    {
                        RCL_TRACK_ACTION    track_action;
                        uint_fast8_t        paidx;
                        char *              p;

                        track_action.action = atoi (buf + 10);

                        p = strchr (buf + 10, ',');

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
                                uint_fast8_t track_action_idx = RCL::add_track_action (true, trackidx);

                                if (track_action_idx == 0xFF)
                                {
                                    Debug::printf (DEBUG_LEVEL_NONE, "%s: error: maximum number of track actions (in) reached.\n", fname);
                                    break;
                                }

                                RCL::set_track_action (true, trackidx, track_action_idx, &track_action);
                            }
                        }
                    }
                    else if (! strncmp (buf, "ACTION_OUT=", 11))
                    {
                        RCL_TRACK_ACTION    track_action;
                        uint_fast8_t        paidx;
                        char *              p;

                        track_action.action = atoi (buf + 11);

                        p = strchr (buf + 11, ',');

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
                                uint_fast8_t track_action_idx = RCL::add_track_action (false, trackidx);

                                if (track_action_idx == 0xFF)
                                {
                                    Debug::printf (DEBUG_LEVEL_NONE, "%s: error: maximum number of track actions (out) reached.\n", fname);
                                    break;
                                }

                                RCL::set_track_action (false, trackidx, track_action_idx, &track_action);
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
            fprintf (fp, "NAME=%s\r\n", RCL::get_name (trackidx));

            flags = RCL::get_flags (trackidx);

            if (flags & RCL_TRACK_FLAG_BLOCK_PROTECTION)
            {
                fprintf (fp, "FLAGS=BLOCK_PROTECTION\r\n");
            }

            n_track_actions = RCL::get_n_track_actions (true, trackidx);

            for (track_action_idx = 0; track_action_idx < n_track_actions; track_action_idx++)
            {
                if (RCL::get_track_action (true, trackidx, track_action_idx, &track_action))
                {
                    fprintf (fp, "ACTION_IN=%d,%d", track_action.action, track_action.n_parameters);

                    for (paidx = 0; paidx < track_action.n_parameters; paidx++)
                    {
                        fprintf (fp, ",%d", track_action.parameters[paidx]);
                    }

                    fprintf (fp, "\r\n");
                }
            }

            n_track_actions = RCL::get_n_track_actions (false, trackidx);

            for (track_action_idx = 0; track_action_idx < n_track_actions; track_action_idx++)
            {
                if (RCL::get_track_action (false, trackidx, track_action_idx, &track_action))
                {
                    fprintf (fp, "ACTION_OUT=%d,%d", track_action.action, track_action.n_parameters);

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

bool
FileIO::write_all_ini_files (void)
{
    bool rtc = true;

    if (rtc && (Loco::data_changed || AddOn::data_changed))
    {
        rtc = write_loco_ini ();
    }

    if (rtc && (Switch::data_changed || Railroad::data_changed))
    {
        rtc = write_switch_ini ();
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

