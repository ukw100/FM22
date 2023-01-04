/*------------------------------------------------------------------------------------------------------------------------
 * event.h - event management functions
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
#define EVENT_LEN                       256

#define EVENT_TYPE_LOCO_FUNCTION        1
#define EVENT_TYPE_LOCO_SPEED           2
#define EVENT_TYPE_LOCO_DIR             3
#define EVENT_TYPE_ADDON_FUNCTION       4
#define EVENT_TYPE_WAIT_S88             5
#define EVENT_TYPE_EXECUTE_LOCO_MACRO   6

#define EVENT_SET_LOCO_SPEED            1
#define EVENT_SET_LOCO_MIN_SPEED        2
#define EVENT_SET_LOCO_MAX_SPEED        3

typedef struct
{
    uint32_t            millis;                                                 // time to activate a loco function
    uint16_t            tenths;                                                 // tenths, only for identification of multiple events
    uint8_t             type;                                                   // type of event
    union
    {
        struct
        {
            uint16_t    loco_idx;                                               // index of loco
            uint8_t     f;                                                      // function as index: 0-31 or 0xFF for all
            bool        b;                                                      // false=reset true=set
        } loco_func;
        struct
        {
            uint16_t    loco_idx;                                               // index of loco
            uint8_t     type;                                                   // type of speed
            uint8_t     sp;                                                     // speed
            uint16_t    ramp;                                                   // ramp in tenths of a second
        } loco_speed;
        struct
        {
            uint16_t    loco_idx;                                               // index of loco
            uint8_t     fwd;                                                    // fwd direction
        } loco_dir;
        struct
        {
            uint16_t    addon_idx;                                              // index of loco
            uint8_t     f;                                                      // function as index: 0-31 or 0xFF for all
            bool        b;                                                      // false=reset true=set
        } addon_func;
        struct
        {
            uint16_t    loco_idx;                                               // index of loco
            uint8_t     sp;                                                     // speed
            uint16_t    ramp;                                                   // ramp in tenths of a second
            uint16_t    coidx;                                                  // S88 contact
        } wait_s88;
        struct
        {
            uint16_t    loco_idx;                                               // index of loco
            uint8_t     m;                                                      // macro index
        } loco_macro;
    };
} EVENTS;

class Event
{
    public:
        static void                     add_event_loco_function (uint16_t tenths, uint_fast16_t loco_idx, uint_fast8_t f, bool b);
        static void                     add_event_loco_speed (uint16_t tenths, uint_fast16_t loco_idx, uint_fast8_t speed_type, uint_fast8_t speed, uint_fast16_t ramp);
        static void                     add_event_loco_dir (uint16_t tenths, uint_fast16_t loco_idx, uint_fast8_t fwd);
        static void                     add_event_addon_function (uint16_t tenths, uint_fast16_t addon_idx, uint_fast8_t f, bool b);
        static void                     add_event_wait_s88 (uint16_t tenths, uint_fast16_t coidx, uint_fast16_t loco_idx, uint_fast8_t speed, uint_fast16_t ramp);
        static void                     add_event_execute_loco_macro (uint16_t tenths, uint_fast16_t loco_idx, uint_fast8_t macroidx);
        static void                     delete_event_wait_s88 (uint_fast16_t loco_idx);
        static void                     schedule (void);
    private:
        static uint_fast16_t            get_free_slot_loco_function (uint16_t tenths, uint_fast16_t loco_idx, uint_fast8_t f, bool b);
        static uint_fast16_t            get_free_slot_loco_speed (uint16_t tenths, uint_fast16_t loco_idx, uint_fast8_t speed_type, uint_fast8_t speed, uint_fast16_t ramp);
        static uint_fast16_t            get_free_slot_loco_dir (uint16_t tenths, uint_fast16_t loco_idx, uint_fast8_t fwd);
        static uint_fast16_t            get_free_slot_addon_function (uint16_t tenths, uint_fast16_t addon_idx, uint_fast8_t f, bool b);
        static uint_fast16_t            get_free_slot_wait_s88 (uint16_t tenths, uint_fast16_t coidx, uint_fast16_t loco_idx, uint_fast8_t speed, uint_fast16_t ramp);
        static uint_fast16_t            get_free_slot_execute_loco_macro (uint16_t tenths, uint_fast16_t loco_idx, uint_fast8_t macroidx);
        static EVENTS                   event_buffer[EVENT_LEN];
        static uint_fast16_t            event_size;
};
