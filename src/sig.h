/*------------------------------------------------------------------------------------------------------------------------
 * sig.h - signal management functions
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
#ifndef SIG_H
#define SIG_H

#include <stdint.h>
#include <string>
#include <vector>
#include <memory>

#define MAX_SIGNALS             1024

#define SIG_ACTIVE_MASK         0x80                                        // flag: activate sig per DCC

#define SIG_EVENT_LEN           64

typedef struct
{
    uint_fast8_t                sig_idx;                                    // index of signal
    uint_fast8_t                mask;
} SIG_EVENTS;

class Signal
{
    public:
                                        Signal ();
        void                            set_id (uint_fast16_t id);

        void                            set_name (std::string name);
        std::string                     get_name ();

        void                            set_addr (uint_fast16_t addr);
        uint_fast16_t                   get_addr ();

        void                            set_state (uint_fast8_t f);
        uint_fast8_t                    get_state ();
    private:
        uint16_t                        id;
        std::string                     name;
        uint16_t                        addr;
        uint8_t                         current_state;
};

class Signals
{
    public:
        static std::vector<Signal>      signals;
        static bool                     data_changed;
        static uint_fast16_t            add (const Signal& new_sig);
        static bool                     remove (uint_fast16_t swidx);
        static uint_fast16_t            get_n_signals (void);
        static uint_fast16_t            set_new_id (uint_fast16_t swidx, uint_fast16_t new_swidx);
        static uint_fast16_t            schedule (void);
        static void                     add_event (uint16_t sig_idx, uint_fast8_t mask);
        static uint_fast8_t             booster_on (void);
        static uint_fast8_t             booster_off (void);
    private:
        static uint_fast16_t            n_signals;                                  // number of signals
        static void                     renumber();
        static SIG_EVENTS               events[SIG_EVENT_LEN];                      // event ringbuffer
        static uint_fast16_t            event_size;                                 // current event size
        static uint_fast16_t            event_start;                                // current event start index
        static uint_fast16_t            event_stop;                                 // current event stop index
};

#endif
