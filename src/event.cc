/*------------------------------------------------------------------------------------------------------------------------
 * event.cc - event management functions
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
#include <stdint.h>

#include "millis.h"
#include "event.h"
#include "loco.h"
#include "addon.h"
#include "s88.h"

EVENTS                  Event::event_buffer[EVENT_LEN];                             // event ringbuffer
uint_fast16_t           Event::event_size      = 0;                                 // current event size

uint_fast16_t
Event::get_free_slot_loco_function (uint16_t tenths, uint_fast16_t loco_idx, uint_fast8_t f, bool b)
{
    uint_fast16_t   idx;
    uint_fast16_t   empty_idx   = 0xFFFF;
    uint_fast16_t   len         = Event::event_size;
    uint_fast16_t   rtc         = 0xFFFF;

    for (idx = 0; len > 0 && idx < EVENT_LEN; idx++)
    {
        if (Event::event_buffer[idx].millis == 0)
        {
            if (empty_idx == 0xFFFF)
            {
                empty_idx = idx;
            }
        }
        else
        {
            if (Event::event_buffer[idx].tenths == tenths && Event::event_buffer[idx].type == EVENT_TYPE_LOCO_FUNCTION)                                  // find double entry...
            {
                if (Event::event_buffer[idx].loco_func.loco_idx == loco_idx &&
                    Event::event_buffer[idx].loco_func.f == f &&
                    Event::event_buffer[idx].loco_func.b == b)
                {
                    rtc = idx;
                    break;
                }
            }

            len--;
        }
    }

    if (rtc == 0xFFFF)
    {
        if (empty_idx != 0xFFFF)
        {
            rtc = empty_idx;
        }
        else if (idx < EVENT_LEN)
        {
            if (Event::event_buffer[idx].millis == 0)
            {
                rtc = idx;
            }
            else
            {
                printf ("Internal error in get_free_slot_loco_function(), idx=%u\n", (unsigned int) idx);
            }
        }
    }

    return rtc;
}

uint_fast16_t
Event::get_free_slot_loco_speed (uint16_t tenths, uint_fast16_t loco_idx, uint_fast8_t speed_type, uint_fast8_t speed, uint_fast16_t ramp)
{
    uint_fast16_t   idx;
    uint_fast16_t   empty_idx   = 0xFFFF;
    uint_fast16_t   len         = Event::event_size;
    uint_fast16_t   rtc         = 0xFFFF;

    for (idx = 0; len > 0 && idx < EVENT_LEN; idx++)
    {
        if (Event::event_buffer[idx].millis == 0)
        {
            if (empty_idx == 0xFFFF)
            {
                empty_idx = idx;
            }
        }
        else
        {
            if (Event::event_buffer[idx].tenths == tenths && Event::event_buffer[idx].type == EVENT_TYPE_LOCO_SPEED)                                 // find double entry...
            {
                if (Event::event_buffer[idx].loco_speed.loco_idx == loco_idx &&
                    Event::event_buffer[idx].loco_speed.type == speed_type &&
                    Event::event_buffer[idx].loco_speed.sp == speed &&
                    Event::event_buffer[idx].loco_speed.ramp == ramp)
                {
                    rtc = idx;
                    break;
                }
            }

            len--;
        }
    }

    if (rtc == 0xFFFF)
    {
        if (empty_idx != 0xFFFF)
        {
            rtc = empty_idx;
        }
        else if (idx < EVENT_LEN)
        {
            if (Event::event_buffer[idx].millis == 0)
            {
                rtc = idx;
            }
            else
            {
                printf ("Internal error in get_free_slot_loco_speed(), idx=%u\n", (unsigned int) idx);
            }
        }
    }

    return rtc;
}

uint_fast16_t
Event::get_free_slot_loco_dir (uint16_t tenths, uint_fast16_t loco_idx, uint_fast8_t fwd)
{
    uint_fast16_t   idx;
    uint_fast16_t   empty_idx   = 0xFFFF;
    uint_fast16_t   len         = Event::event_size;
    uint_fast16_t   rtc         = 0xFFFF;

    for (idx = 0; len > 0 && idx < EVENT_LEN; idx++)
    {
        if (Event::event_buffer[idx].millis == 0)
        {
            if (empty_idx == 0xFFFF)
            {
                empty_idx = idx;
            }
        }
        else
        {
            if (Event::event_buffer[idx].tenths == tenths && Event::event_buffer[idx].type == EVENT_TYPE_LOCO_DIR)                                   // find double entry...
            {
                if (Event::event_buffer[idx].loco_dir.loco_idx == loco_idx &&
                    Event::event_buffer[idx].loco_dir.fwd == fwd)
                {
                    rtc = idx;
                    break;
                }
            }

            len--;
        }
    }

    if (rtc == 0xFFFF)
    {
        if (empty_idx != 0xFFFF)
        {
            rtc = empty_idx;
        }
        else if (idx < EVENT_LEN)
        {
            if (Event::event_buffer[idx].millis == 0)
            {
                rtc = idx;
            }
            else
            {
                printf ("Internal error in get_free_slot_loco_dir(), idx=%u\n", (unsigned int) idx);
            }
        }
    }

    return rtc;
}

uint_fast16_t
Event::get_free_slot_addon_function (uint16_t tenths, uint_fast16_t addon_idx, uint_fast8_t f, bool b)
{
    uint_fast16_t   idx;
    uint_fast16_t   empty_idx   = 0xFFFF;
    uint_fast16_t   len         = Event::event_size;
    uint_fast16_t   rtc         = 0xFFFF;

    for (idx = 0; len > 0 && idx < EVENT_LEN; idx++)
    {
        if (Event::event_buffer[idx].millis == 0)
        {
            if (empty_idx == 0xFFFF)
            {
                empty_idx = idx;
            }
        }
        else
        {
            if (Event::event_buffer[idx].tenths == tenths && Event::event_buffer[idx].type == EVENT_TYPE_ADDON_FUNCTION)                                  // find double entry...
            {
                if (Event::event_buffer[idx].addon_func.addon_idx == addon_idx &&
                    Event::event_buffer[idx].addon_func.f == f &&
                    Event::event_buffer[idx].addon_func.b == b)
                {
                    rtc = idx;
                    break;
                }
            }

            len--;
        }
    }

    if (rtc == 0xFFFF)
    {
        if (empty_idx != 0xFFFF)
        {
            rtc = empty_idx;
        }
        else if (idx < EVENT_LEN)
        {
            if (Event::event_buffer[idx].millis == 0)
            {
                rtc = idx;
            }
            else
            {
                printf ("Internal error in get_free_slot_addon_function(), idx=%u\n", (unsigned int) idx);
            }
        }
    }

    return rtc;
}

uint_fast16_t
Event::get_free_slot_wait_s88 (uint16_t tenths, uint_fast16_t coidx, uint_fast16_t loco_idx, uint_fast8_t speed, uint_fast16_t ramp)
{
    uint_fast16_t   idx;
    uint_fast16_t   empty_idx   = 0xFFFF;
    uint_fast16_t   len         = Event::event_size;
    uint_fast16_t   rtc         = 0xFFFF;

    for (idx = 0; len > 0 && idx < EVENT_LEN; idx++)
    {
        if (Event::event_buffer[idx].millis == 0)
        {
            if (empty_idx == 0xFFFF)
            {
                empty_idx = idx;
            }
        }
        else
        {
            if (Event::event_buffer[idx].tenths == tenths && Event::event_buffer[idx].type == EVENT_TYPE_WAIT_S88)                              // find double entry...
            {
                if (Event::event_buffer[idx].wait_s88.loco_idx == loco_idx &&
                    Event::event_buffer[idx].wait_s88.sp == speed &&
                    Event::event_buffer[idx].wait_s88.ramp == ramp &&
                    Event::event_buffer[idx].wait_s88.coidx == coidx)
                {
                    rtc = idx;
                    break;
                }
            }

            len--;
        }
    }

    if (rtc == 0xFFFF)
    {
        if (empty_idx != 0xFFFF)
        {
            rtc = empty_idx;
        }
        else if (idx < EVENT_LEN)
        {
            if (Event::event_buffer[idx].millis == 0)
            {
                rtc = idx;
            }
            else
            {
                printf ("Internal error in get_free_slot_wait_s88(), idx=%u\n", (unsigned int) idx);
            }
        }
    }

    return rtc;
}

uint_fast16_t
Event::get_free_slot_execute_loco_macro (uint16_t tenths, uint_fast16_t loco_idx, uint_fast8_t macroidx)
{
    uint_fast16_t   idx;
    uint_fast16_t   empty_idx   = 0xFFFF;
    uint_fast16_t   len         = Event::event_size;
    uint_fast16_t   rtc         = 0xFFFF;

    for (idx = 0; len > 0 && idx < EVENT_LEN; idx++)
    {
        if (Event::event_buffer[idx].millis == 0)
        {
            if (empty_idx == 0xFFFF)
            {
                empty_idx = idx;
            }
        }
        else
        {
            if (Event::event_buffer[idx].tenths == tenths && Event::event_buffer[idx].type == EVENT_TYPE_EXECUTE_LOCO_MACRO)        // find double entry...
            {
                if (Event::event_buffer[idx].loco_macro.loco_idx == loco_idx &&
                    Event::event_buffer[idx].loco_macro.m == macroidx)
                {
                    rtc = idx;
                    break;
                }
            }

            len--;
        }
    }

    if (rtc == 0xFFFF)
    {
        if (empty_idx != 0xFFFF)
        {
            rtc = empty_idx;
        }
        else if (idx < EVENT_LEN)
        {
            if (Event::event_buffer[idx].millis == 0)
            {
                rtc = idx;
            }
            else
            {
                printf ("Internal error in get_free_slot_loco_macro(), idx=%u\n", (unsigned int) idx);
            }
        }
    }

    return rtc;
}

/*------------------------------------------------------------------------------------------------------------------------
 * add_event_loco_function () - add function event
 *------------------------------------------------------------------------------------------------------------------------
 */
void
Event::add_event_loco_function (uint16_t tenths, uint_fast16_t loco_idx, uint_fast8_t f, bool b)
{
    uint_fast16_t idx;

    if (Event::event_size < EVENT_LEN)                                              // buffer full?
    {                                                                               // no
        idx = Event::get_free_slot_loco_function (tenths, loco_idx, f, b);

        if (idx < EVENT_LEN)
        {
            if (Event::event_buffer[idx].millis == 0)
            {
                Event::event_size++;
            }

            Event::event_buffer[idx].millis         = Millis::elapsed () + 100 * tenths;
            Event::event_buffer[idx].tenths         = tenths;
            Event::event_buffer[idx].type           = EVENT_TYPE_LOCO_FUNCTION;
            Event::event_buffer[idx].loco_func.loco_idx  = loco_idx;
            Event::event_buffer[idx].loco_func.f         = f;
            Event::event_buffer[idx].loco_func.b         = b;
        }
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 * add_event_loco_speed () - add speed event
 *------------------------------------------------------------------------------------------------------------------------
 */
void
Event::add_event_loco_speed (uint16_t tenths, uint_fast16_t loco_idx, uint_fast8_t speed_type, uint_fast8_t speed, uint_fast16_t ramp)
{
    uint_fast16_t idx;

    if (Event::event_size < EVENT_LEN)                                              // buffer full?
    {                                                                               // no
        idx = Event::get_free_slot_loco_speed (tenths, loco_idx, speed_type, speed, ramp);

        if (idx < EVENT_LEN)
        {
            if (Event::event_buffer[idx].millis == 0)
            {
                Event::event_size++;
            }

            Event::event_buffer[idx].millis         = Millis::elapsed () + 100 * tenths;
            Event::event_buffer[idx].tenths         = tenths;
            Event::event_buffer[idx].type           = EVENT_TYPE_LOCO_SPEED;
            Event::event_buffer[idx].loco_speed.loco_idx = loco_idx;
            Event::event_buffer[idx].loco_speed.type     = speed_type;
            Event::event_buffer[idx].loco_speed.sp       = speed;
            Event::event_buffer[idx].loco_speed.ramp     = ramp;
        }
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 * add_event_addon_function () - add function event
 *------------------------------------------------------------------------------------------------------------------------
 */
void
Event::add_event_addon_function (uint16_t tenths, uint_fast16_t addon_idx, uint_fast8_t f, bool b)
{
    uint_fast16_t idx;

    if (Event::event_size < EVENT_LEN)                                              // buffer full?
    {                                                                               // no
        idx = Event::get_free_slot_addon_function (tenths, addon_idx, f, b);

        if (idx < EVENT_LEN)
        {
            if (Event::event_buffer[idx].millis == 0)
            {
                Event::event_size++;
            }

            Event::event_buffer[idx].millis                 = Millis::elapsed () + 100 * tenths;
            Event::event_buffer[idx].tenths                 = tenths;
            Event::event_buffer[idx].type                   = EVENT_TYPE_ADDON_FUNCTION;
            Event::event_buffer[idx].addon_func.addon_idx   = addon_idx;
            Event::event_buffer[idx].addon_func.f           = f;
            Event::event_buffer[idx].addon_func.b           = b;
        }
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 * add_event_wait_s88 () - add wait event for S88 contact
 *------------------------------------------------------------------------------------------------------------------------
 */
void
Event::add_event_wait_s88 (uint16_t tenths, uint_fast16_t coidx, uint_fast16_t loco_idx, uint_fast8_t speed, uint_fast16_t ramp)
{
    uint_fast16_t idx;

    if (Event::event_size < EVENT_LEN)                                              // buffer full?
    {                                                                               // no
        idx = Event::get_free_slot_wait_s88 (tenths, coidx, loco_idx, speed, ramp);

        if (idx < EVENT_LEN)
        {
            if (Event::event_buffer[idx].millis == 0)
            {
                Event::event_size++;
            }

            Event::event_buffer[idx].millis             = Millis::elapsed () + 100 * tenths;
            Event::event_buffer[idx].tenths             = tenths;
            Event::event_buffer[idx].type               = EVENT_TYPE_WAIT_S88;
            Event::event_buffer[idx].wait_s88.loco_idx  = loco_idx;
            Event::event_buffer[idx].wait_s88.sp        = speed;
            Event::event_buffer[idx].wait_s88.ramp      = ramp;
            Event::event_buffer[idx].wait_s88.coidx     = coidx;
        }
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 * delete_event_wait_s88 () - delete wait event
 *------------------------------------------------------------------------------------------------------------------------
 */
void
Event::delete_event_wait_s88 (uint_fast16_t loco_idx)
{
    uint_fast16_t   idx;
    uint_fast16_t   len = Event::event_size;

    for (idx = 0; len > 0 && idx < EVENT_LEN; idx++)
    {
        if (Event::event_buffer[idx].millis != 0 &&
            Event::event_buffer[idx].type == EVENT_TYPE_WAIT_S88 &&
            Event::event_buffer[idx].wait_s88.loco_idx == loco_idx)
        {
            Event::event_buffer[idx].millis = 0;
            Event::event_size--;
            Loco::reset_flag_halt (loco_idx);
            break;
        }

        len--;
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 * add_event_loco_dir () - add direction event
 *------------------------------------------------------------------------------------------------------------------------
 */
void
Event::add_event_loco_dir (uint16_t tenths, uint_fast16_t loco_idx, uint_fast8_t fwd)
{
    uint_fast16_t idx;

    if (Event::event_size < EVENT_LEN)                                              // buffer full?
    {                                                                               // no
        idx = Event::get_free_slot_loco_dir (tenths, loco_idx, fwd);

        if (idx < EVENT_LEN)
        {
            if (Event::event_buffer[idx].millis == 0)
            {
                Event::event_size++;
            }

            Event::event_buffer[idx].millis             = Millis::elapsed () + 100 * tenths;
            Event::event_buffer[idx].tenths             = tenths;
            Event::event_buffer[idx].type               = EVENT_TYPE_LOCO_DIR;
            Event::event_buffer[idx].loco_dir.loco_idx  = loco_idx;
            Event::event_buffer[idx].loco_dir.fwd       = fwd;
        }
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 * add_event_loco_macro () - add macro event
 *------------------------------------------------------------------------------------------------------------------------
 */
void
Event::add_event_execute_loco_macro (uint16_t tenths, uint_fast16_t loco_idx, uint_fast8_t macroidx)
{
    uint_fast16_t idx;

    if (Event::event_size < EVENT_LEN)                                              // buffer full?
    {                                                                               // no
        idx = Event::get_free_slot_execute_loco_macro (tenths, loco_idx, macroidx);

        if (idx < EVENT_LEN)
        {
            if (Event::event_buffer[idx].millis == 0)
            {
                Event::event_size++;
            }

            Event::event_buffer[idx].millis                 = Millis::elapsed () + 100 * tenths;
            Event::event_buffer[idx].tenths                 = tenths;
            Event::event_buffer[idx].type                   = EVENT_TYPE_EXECUTE_LOCO_MACRO;
            Event::event_buffer[idx].loco_macro.loco_idx    = loco_idx;
            Event::event_buffer[idx].loco_macro.m           = macroidx;
        }
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 * schedule_events() - schedule events
 *------------------------------------------------------------------------------------------------------------------------
 */
void
Event::schedule (void)
{
    uint_fast16_t   idx;
    uint_fast16_t   len = Event::event_size;
    uint32_t        current_millis = Millis::elapsed ();

    for (idx = 0; len > 0 && idx < EVENT_LEN; idx++)
    {
        if (Event::event_buffer[idx].millis > 0)
        {
            if (Event::event_buffer[idx].millis <= current_millis)
            {
                bool    do_delete_event = true;

                switch (Event::event_buffer[idx].type)
                {
                    case EVENT_TYPE_LOCO_FUNCTION:
                    {
                        if (Event::event_buffer[idx].loco_func.f == 0xFF)
                        {
                            Loco::reset_functions (Event::event_buffer[idx].loco_func.loco_idx);
                        }
                        else
                        {
                            Loco::set_function (Event::event_buffer[idx].loco_func.loco_idx, Event::event_buffer[idx].loco_func.f, Event::event_buffer[idx].loco_func.b);
                        }
                        break;
                    }

                    case EVENT_TYPE_LOCO_SPEED:
                    {
                        uint_fast16_t   loco_idx    = Event::event_buffer[idx].loco_speed.loco_idx;
                        uint_fast8_t    type        = Event::event_buffer[idx].loco_speed.type;
                        uint_fast8_t    speed       = Event::event_buffer[idx].loco_speed.sp;
                        uint_fast16_t   tenths      = Event::event_buffer[idx].loco_speed.ramp;

                        if (loco_idx != 0xFFFF)
                        {
                            switch (type)
                            {
                                case EVENT_SET_LOCO_SPEED:
                                {
                                    Loco::set_speed (loco_idx, speed, tenths);
                                    break;
                                }
                                case EVENT_SET_LOCO_MIN_SPEED:
                                {
                                    uint_fast8_t current_speed = Loco::get_speed (loco_idx);

                                    if (current_speed < speed)
                                    {
                                        Loco::set_speed (loco_idx, speed, tenths);
                                    }
                                    break;
                                }
                                case EVENT_SET_LOCO_MAX_SPEED:
                                {
                                    uint_fast8_t current_speed = Loco::get_speed (loco_idx);

                                    if (current_speed > speed)
                                    {
                                        Loco::set_speed (loco_idx, speed, tenths);
                                    }
                                    break;
                                }
                            }
                        }

                        break;
                    }

                    case EVENT_TYPE_LOCO_DIR:
                    {
                        uint_fast16_t   loco_idx    = Event::event_buffer[idx].loco_dir.loco_idx;
                        uint_fast8_t    fwd         = Event::event_buffer[idx].loco_dir.fwd;

                        if (loco_idx != 0xFFFF)
                        {
                            Loco::set_fwd (loco_idx, fwd);
                        }

                        break;
                    }

                    case EVENT_TYPE_ADDON_FUNCTION:
                    {
                        if (Event::event_buffer[idx].addon_func.f == 0xFF)
                        {
                            AddOn::reset_functions (Event::event_buffer[idx].addon_func.addon_idx);
                        }
                        else
                        {
                            AddOn::set_function (Event::event_buffer[idx].addon_func.addon_idx, Event::event_buffer[idx].addon_func.f, Event::event_buffer[idx].addon_func.b);
                        }
                        break;
                    }

                    case EVENT_TYPE_WAIT_S88:
                    {
                        uint_fast16_t   loco_idx    = Event::event_buffer[idx].wait_s88.loco_idx;
                        uint_fast8_t    speed       = Event::event_buffer[idx].wait_s88.sp;
                        uint_fast16_t   tenths      = Event::event_buffer[idx].wait_s88.ramp;
                        uint_fast16_t   coidx       = Event::event_buffer[idx].wait_s88.coidx;

                        if (loco_idx != 0xFFFF)
                        {
                            if (S88::get_state_bit (coidx) == S88_STATE_OCCUPIED)
                            {
                                Loco::set_flag_halt (loco_idx);
                                Event::event_buffer[idx].millis = current_millis + 100;                         // wait 100msec for next check
                                do_delete_event = false;
                            }
                            else
                            {
                                Loco::reset_flag_halt (loco_idx);
                                Loco::set_speed (loco_idx, speed, tenths);
                            }
                        }
                        break;
                    }

                    case EVENT_TYPE_EXECUTE_LOCO_MACRO:
                    {
                        uint_fast16_t   loco_idx    = Event::event_buffer[idx].loco_macro.loco_idx;
                        uint_fast8_t    macroidx    = Event::event_buffer[idx].loco_macro.m;

                        if (loco_idx != 0xFFFF)
                        {
                            Loco::execute_macro (loco_idx, macroidx);
                        }

                        break;
                    }
                }

                if (do_delete_event)
                {
                    Event::event_buffer[idx].millis = 0;
                    Event::event_size--;
                }
            }

            len--;
        }
    }
}
