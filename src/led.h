/*------------------------------------------------------------------------------------------------------------------------
 * led.h - led management functions
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
#ifndef LED_H
#define LED_H

#include <stdint.h>
#include <string>
#include <vector>
#include <memory>

#define MAX_LED_GROUPS                  256
#define MAX_LEDS_PER_GROUP              8

class LedGroup
{
    public:
                                        LedGroup ();
        void                            set_id (uint_fast16_t id);

        void                            set_name (std::string name);
        std::string                     get_name ();

        void                            set_addr (uint_fast16_t addr);
        uint_fast16_t                   get_addr ();

        void                            set_state (uint_fast8_t led_mask);
        void                            set_state (uint_fast8_t led_mask, uint_fast8_t on);
        uint_fast8_t                    get_state ();
    private:
        uint16_t                        id;
        std::string                     name;
        uint16_t                        addr;
        uint8_t                        current_state_mask;
};

class Leds
{
    public:
        static std::vector<LedGroup>    led_groups;
        static bool                     data_changed;
        static uint_fast16_t            add (const LedGroup& new_led_group);
        static bool                     remove (uint_fast16_t led_group_idx);
        static uint_fast16_t            get_n_led_groups (void);
        static uint_fast16_t            set_new_id (uint_fast16_t led_group_idx, uint_fast16_t new_led_group_idx);
        static uint_fast8_t             booster_on (void);
        static uint_fast8_t             booster_off (void);
    private:
        static uint_fast16_t            n_led_groups;                           // number of led groups
        static void                     renumber();
};

#endif
