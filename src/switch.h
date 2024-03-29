/*------------------------------------------------------------------------------------------------------------------------
 * switch.h - switch machine management functions
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
#ifndef SWITCH_H
#define SWITCH_H

#include <stdint.h>
#include <string>
#include <vector>
#include <memory>

#define MAX_SWITCHES            1024

#define SWITCH_ACTIVE_MASK      0x80                                        // flag: activate switch per DCC
#define SWITCH_FLAG_3WAY        0x01

#define SWITCH_EVENT_LEN        64

typedef struct
{
    uint_fast16_t               switch_idx;                                 // index of switch machine
    uint_fast8_t                mask;
} SWITCH_EVENTS;

class Switch
{
    public:
                                        Switch ();
        void                            set_id (uint_fast16_t id);

        void                            set_name (std::string name);
        std::string                     get_name ();

        void                            set_addr (uint_fast16_t addr);
        uint_fast16_t                   get_addr ();

        void                            set_state (uint_fast8_t f);
        uint_fast8_t                    get_state ();
        void                            set_flags (uint_fast8_t f);
        uint_fast8_t                    get_flags ();
    private:
        uint16_t                        id;
        std::string                     name;
        uint16_t                        addr;
        uint8_t                         current_state;
        uint8_t                         flags;
};

class Switches
{
    public:
        static std::vector<Switch>      switches;
        static bool                     data_changed;
        static uint_fast16_t            add (const Switch& new_switch);
        static bool                     remove (uint_fast16_t swidx);
        static uint_fast16_t            get_n_switches (void);
        static uint_fast16_t            set_new_id (uint_fast16_t swidx, uint_fast16_t new_swidx);
        static uint_fast16_t            schedule (void);
        static void                     add_event (uint16_t switch_idx, uint_fast8_t mask);
        static uint_fast8_t             booster_on (void);
        static uint_fast8_t             booster_off (void);
    private:
        static uint_fast16_t            n_switches;                                 // number of switches
        static void                     renumber();
        static SWITCH_EVENTS            events[SWITCH_EVENT_LEN];                   // event ringbuffer
        static uint_fast16_t            event_size;                                 // current event size
        static uint_fast16_t            event_start;                                // current event start index
        static uint_fast16_t            event_stop;                                 // current event stop index
};

#endif
