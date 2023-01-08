/*-------------------------------------------------------------------------------------------------------------------------------------------
 * main.cc - main module of dcc-central
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
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/time.h>

#include "userio.h"
#include "event.h"
#include "loco.h"
#include "http.h"
#include "dcc.h"
#include "switch.h"
#include "s88.h"
#include "rcl.h"
#include "fileio.h"
#include "serial.h"
#include "msg.h"
#include "gpio.h"
#include "stm32.h"
#include "millis.h"
#include "debug.h"

#define SWITCH_FIRST_PERIOD     500
#define SCHEDULE_PERIOD         5
#define RC2_RATE_PERIOD         1000

static void
usage (char * pgm)
{
    fprintf (stderr, "usage: %s [-e] [-d level]\n", pgm);
    exit (1);
}

#define MAX_ARGS                8
#define SHUTDOWN_TIME           1000
static uint32_t                 next_exit;
static char *                   pgm_argv[8];

static void
myalarm (int sig)
{
    Debug::printf (DEBUG_LEVEL_NORMAL, "got signal: %d\n", sig);

    if (sig == SIGINT)
    {
        Debug::printf (DEBUG_LEVEL_NORMAL, "shutting down in %0.2f sec...\n", (float) SHUTDOWN_TIME / 1000.0);
        UserIO::booster_off (true);
        next_exit = Millis::elapsed () + SHUTDOWN_TIME;
    }
    else if (sig == SIGHUP)
    {
        Debug::printf (DEBUG_LEVEL_NORMAL, "restarting\n");
        HTTP::deinit ();
        execv (pgm_argv[0], pgm_argv);
        exit (0);
    }
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * main () - main function
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
int
main (int argc, char ** argv)
{
    unsigned long   switch_millis;
    unsigned long   next_millis;
    unsigned long   current_millis;
    uint_fast16_t   n_contacts;
    bool            edit_mode = false;
    int             i;

    char * pgm = argv[0];

    for (i = 0; i < argc && i < MAX_ARGS - 1; i++)
    {
        pgm_argv[i] = argv[i];
    }

    pgm_argv[i] = (char *) NULL;

    while (argc > 1)
    {
        if (argc > 1 && ! strcmp (argv[1], "-e"))
        {
            edit_mode = true;
            argc--;
            argv++;
        }
        else if (argc > 2 && ! strcmp (argv[1], "-d"))
        {
            uint_fast8_t    level = atoi (argv[2]);
            Debug::set_level (level);
            argc -= 2;
            argv += 2;
        }
        else
        {
            usage (pgm);
        }
    }

    signal (SIGHUP, myalarm);
    signal (SIGINT, myalarm);

    FileIO::read_func_ini ();
    FileIO::read_loco_ini ();
    FileIO::read_switch_ini ();
    FileIO::read_s88_ini ();
    FileIO::read_rcl_ini ();

    Serial::init ();
    HTTP::init ();
    Millis::init ();
    DCC::init ();
    S88::init ();
    RCL::init ();
    STM32::init ();

    GPIO::activate_stm32_nrst ();                                           // reset STM32
    usleep (1000);                                                          // wait 1msec
    MSG::flush_msg ();                                                      // flush input
    GPIO::deactivate_stm32_nrst ();                                         // boot STM32
    usleep (200000);                                                        // wait 200msec for STM32 boot

    current_millis  = Millis::elapsed ();
    switch_millis   = current_millis + SWITCH_FIRST_PERIOD;                 // 1st switch scheduling in 500 msec
    next_millis     = current_millis + SCHEDULE_PERIOD;

    n_contacts = S88::get_n_contacts ();
    DCC::set_s88_n_contacts (n_contacts);
    (void) S88::get_n_contacts_changed ();

    while (1)
    {
        current_millis = Millis::elapsed ();

        if (next_exit && current_millis >= next_exit)
        {
            exit (0);
        }

        if (current_millis > switch_millis)
        {
            switch_millis = Switch::schedule () + Millis::elapsed ();
            current_millis = Millis::elapsed ();
        }

        if (current_millis >= next_millis)
        {
            bool loco_sched_rtc;

            next_millis = current_millis + SCHEDULE_PERIOD;
            Event::schedule ();

            do
            {
                loco_sched_rtc = Loco::schedule ();
                S88::schedule ();
                RCL::schedule ();
                MSG::read_msg ();
                edit_mode = HTTP::server (edit_mode);

                if (S88::get_n_contacts_changed ())                         // STM32 could have been resetted and forgot number of cntacts
                {
                    n_contacts = S88::get_n_contacts ();
                    DCC::set_s88_n_contacts (n_contacts);
                    Debug::printf (DEBUG_LEVEL_VERBOSE, "main: number of contacts changed, sending number of contacts: %d\n", n_contacts);
                }
            } while (loco_sched_rtc == 0);                                  // schedule all active locos
        }
    }

    return (0);
}
