/*------------------------------------------------------------------------------------------------------------------------
 * sig.cc - signal management functions
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dcc.h"
#include "debug.h"
#include "sig.h"

#define SIG_SCHEDULE_DELAY      220                                         // schedule time for signals in msec

std::vector<Signal>             Signals::signals;                         // signals
uint_fast16_t                   Signals::n_signals  = 0;                  // number of signals
bool                            Signals::data_changed = false;

SIG_EVENTS                      Signals::events[SIG_EVENT_LEN];             // event ringbuffer
uint_fast16_t                   Signals::event_size      = 0;              // current event size
uint_fast16_t                   Signals::event_start     = 0;              // current event start index
uint_fast16_t                   Signals::event_stop      = 0;              // current event stop index

/*------------------------------------------------------------------------------------------------------------------------
 * Signal () - constructor
 *------------------------------------------------------------------------------------------------------------------------
 */
Signal::Signal ()
{
    this->name                  = "";
    this->addr                  = 0;
    this->current_state         = DCC_SIGNAL_STATE_UNDEFINED;
}

/*------------------------------------------------------------------------------------------------------------------------
 * set_id () - set id
 *------------------------------------------------------------------------------------------------------------------------
 */
void
Signal::set_id (uint_fast16_t id)
{
    this->id = id;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  Signal::set_name () - set name
 *------------------------------------------------------------------------------------------------------------------------
 */
void
Signal::set_name (std::string name)
{
    this->name = name;
    Signals::data_changed = true;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  Signal::get_name () - get name
 *------------------------------------------------------------------------------------------------------------------------
 */
std::string
Signal::get_name ()
{
    return this->name;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  Signal::set_addr () - set address
 *------------------------------------------------------------------------------------------------------------------------
 */
void
Signal::set_addr (uint_fast16_t addr)
{
    this->addr = addr;
    Signals::data_changed = true;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  Signal::get_addr () - get address
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast16_t
Signal::get_addr ()
{
    return this->addr;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  Signal::set_state () - set state
 *------------------------------------------------------------------------------------------------------------------------
 */
void
Signal::set_state (uint_fast8_t f)
{
    if (this->current_state != f)
    {
        this->current_state = f;

        if (f == DCC_SIGNAL_STATE_HALT || f == DCC_SIGNAL_STATE_GO)
        {
            Signals::add_event (this->id, f | SIG_ACTIVE_MASK);
            Signals::add_event (this->id, f);
        }
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 *  Signal::get_state ()
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
Signal::get_state ()
{
    return this->current_state;
}

/*------------------------------------------------------------------------------------------------------------------------
 * Signals::add_event() - add event
 *------------------------------------------------------------------------------------------------------------------------
 */
void
Signals::add_event (uint16_t sig_idx, uint_fast8_t mask)
{
    if (event_size < SIG_EVENT_LEN)                                     // buffer full?
    {
        Signals::events[event_stop].sig_idx   = sig_idx;
        Signals::events[event_stop].mask         = mask;

        event_stop++;

        if (event_stop >= SIG_EVENT_LEN)                                // at end of ringbuffer?
        {                                                               // yes
            event_stop = 0;                                             // reset to beginning
        }

        event_size++;                                                   // increment used size
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 * Signals::schedule () - schedule events
 *
 * Return value:
 *  time in msec when next call should be done
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast16_t
Signals::schedule (void)
{
    uint_fast16_t   rtc = SIG_SCHEDULE_DELAY;

    if (event_size != 0)
    {
        uint_fast8_t    active  = Signals::events[event_start].mask & SIG_ACTIVE_MASK;
        uint_fast8_t    f       = Signals::events[event_start].mask & DCC_SIGNAL_STATE_MASK;
        uint16_t        sigidx   = Signals::events[event_start].sig_idx;
        uint_fast16_t   addr    = Signals::signals[sigidx].get_addr();

        Debug::printf (DEBUG_LEVEL_VERBOSE, "Signals::schedule: active=%u f=%u sigidx=%u addr=%u\r\n", active, f, sigidx, addr);

        if (active)         // activate sig
        {
            DCC::base_switch_set (addr, f);
        }

        event_start++;

        if (event_start == SIG_EVENT_LEN)                                // at end of ringbuffer?
        {                                                                   // yes
            event_start = 0;                                                // reset to beginning
        }
        event_size--;
    }

    return rtc;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  Signals::add () - add a sig
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast16_t
Signals::add (const Signal& new_sig)
{
    if (Signals::n_signals < MAX_SIGNALS)
    {
        signals.push_back(new_sig);
        Signals::signals[n_signals].set_id(n_signals);
        Signals::n_signals++;
        return signals.size() - 1;
    }
    return 0xFFFF;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  Signals::remove ()
 *------------------------------------------------------------------------------------------------------------------------
 */
bool
Signals::remove (uint_fast16_t sigidx)
{
    uint_fast16_t   idx;
    uint_fast8_t    rtc = false;

    if (sigidx < Signals::n_signals)
    {
        Signals::n_signals--;

        for (idx = sigidx; idx < Signals::n_signals; idx++)
        {
            Signals::signals[idx] = Signals::signals[idx + 1];
        }

        Signals::data_changed = true;
        rtc = true;
    }

    return rtc;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  set_new_id () - set new id
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast16_t
Signals::set_new_id (uint_fast16_t sigidx, uint_fast16_t new_sigidx)
{
    uint_fast16_t   rtc = 0xFFFF;

    if (sigidx < Signals::n_signals && new_sigidx < Signals::n_signals)
    {
        if (sigidx != new_sigidx)
        {
            Signal          tmpsig;
            uint_fast16_t   sidx;

            uint16_t *      map_new_sig_idx = (uint16_t *) calloc (Signals::n_signals, sizeof (uint16_t));

            for (sidx = 0; sidx < Signals::n_signals; sidx++)
            {
                map_new_sig_idx[sidx] = sidx;
            }

            tmpsig = Signals::signals[sigidx];

            if (new_sigidx < sigidx)
            {
                for (sidx = sigidx; sidx > new_sigidx; sidx--)
                {
                    Signals::signals[sidx] = Signals::signals[sidx - 1];
                    map_new_sig_idx[sidx - 1] = sidx;
                }
            }
            else // if (new_sigidx > sigidx)
            {
                for (sidx = sigidx; sidx < new_sigidx; sidx++)
                {
                    Signals::signals[sidx] = Signals::signals[sidx + 1];
                    map_new_sig_idx[sidx + 1] = sidx;
                }
            }

            Signals::signals[sidx] = tmpsig;
            map_new_sig_idx[sigidx] = sidx;

            for (sidx = 0; sidx < Signals::n_signals; sidx++)
            {
                Debug::printf (DEBUG_LEVEL_VERBOSE, "%2d -> %2d\n", sidx, map_new_sig_idx[sidx]);
            }

            free (map_new_sig_idx);
            Signals::data_changed = true;
            rtc = new_sigidx;

            Signals::renumber ();
        }
        else
        {
            rtc = sigidx;
        }
    }

    return rtc;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  Signals::get_n_signals () - get number of signals
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast16_t
Signals::get_n_signals (void)
{
    return Signals::n_signals;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  renumber () - renumber ids
 *------------------------------------------------------------------------------------------------------------------------
 */
void
Signals::renumber ()
{
    uint_fast16_t sigidx;

    for (sigidx = 0; sigidx < Signals::n_signals; sigidx++)
    {
        Signals::signals[sigidx].set_id (sigidx);
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 *  Signals::booster_on ()
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
Signals::booster_on (void)
{
    uint_fast16_t sigidx;

    for (sigidx = 0; sigidx < Signals::n_signals; sigidx++)
    {
        Signals::signals[sigidx].set_state (DCC_SIGNAL_STATE_UNDEFINED);
    }

    return 0;
}

/*------------------------------------------------------------------------------------------------------------------------
 *  Signals::booster_off ()
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
Signals::booster_off (void)
{
    uint_fast16_t sigidx;

    for (sigidx = 0; sigidx < Signals::n_signals; sigidx++)
    {
        Signals::signals[sigidx].set_state (DCC_SIGNAL_STATE_UNDEFINED);
    }
    return 0;
}
