/*------------------------------------------------------------------------------------------------------------------------
 * http-common.h - HTTP common routines
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
#ifndef HTTP_COMMON_H
#define HTTP_COMMON_H

#include <stdint.h>

#define MANUFACTURER_LENZ                   99
#define MANUFACTURER_ZIMO                   145
#define MANUFACTURER_ESU                    151
#define MANUFACTURER_TAMS                   62

#define LOCO_LIST_SHOW_NONE_LOCO            0x0001
#define LOCO_LIST_SHOW_LINKED_LOCO          0x0002
#define LOCO_LIST_SHOW_THIS_DETECTED_LOCO   0x0004
#define LOCO_LIST_SHOW_DETECTED_LOCO        0x0008
#define LOCO_LIST_DO_DISPLAY                0x8000

#define ADDON_LIST_DO_DISPLAY               0x8000

class HTTP_Common
{
    public:
        static bool             edit_mode;
        static std::string      alert_msg;
        static const char *     manufacturers[256];

        static void             html_header (String browsertitle, String title, String url, bool use_utf8);
        static void             html_trailer (void);
        static void             head_action (void);
        static void             handle_info (void);
        static void             handle_setup (void);

        static void             add_action_content (String id, String type, String value);
        static void             print_start_list (String name, uint_fast16_t selected_start, bool do_display);
        static void             print_speed_list (String name, uint_fast16_t selected_speed, bool do_display);
        static void             print_tenths_list (String name, uint_fast16_t selected_tenths, bool do_display);
        static void             print_fwd_list (String name, uint_fast8_t selected_fwd, bool do_display);
        static void             print_loco_function_list (String name, uint_fast16_t selected_f, bool do_display);
        static void             print_loco_function_list (uint_fast16_t loco_or_common_idx, bool is_loco, String name, uint_fast16_t selected_fidx, bool do_display);
        static void             add_action_handler (const char * action, const char * parameters, int msec, bool $do_print_script_tag);
        static void             add_action_handler (const char * action, const char * parameters, int msec);
        static std::string      get_location (uint_fast16_t loco_idx);
        static void             print_loco_select_list (String name, uint_fast16_t selected_idx, uint_fast16_t show_mask);
        static void             print_addon_select_list (String name, uint_fast16_t selected_idx, uint_fast16_t show_mask);
        static void             print_id_select_list (String name, uint_fast16_t n_entries);
        static void             print_id_select_list (String name, uint_fast16_t n_entries, uint_fast16_t selected_idx);
        static void             print_loco_macro_list (String name, uint_fast16_t selected_m, bool do_display);
        static void             print_railroad_group_list (String name, uint_fast8_t rrgidx, bool do_display_norrg, bool do_display);
        static void             print_railroad_list (String name, uint_fast16_t linkedrr, bool do_allow_none, bool do_display);
        static void             print_s88_list (String name, uint_fast16_t coidx, bool do_display);
        static void             print_switch_list (String name, uint_fast16_t swidx, bool do_display);
        static void             print_switch_state_list (String name, uint_fast8_t swstate, bool do_display);
        static void             print_signal_list (String name, uint_fast16_t sigidx, bool do_display);
        static void             print_signal_state_list (String name, uint_fast8_t sigstate, bool do_display);
        static void             print_led_group_list (String name, uint_fast16_t ledidx, bool do_display);
        static void             print_script_led_mask_list (void);
        static void             print_led_mask_list (String name, uint_fast8_t ledstate, bool do_display);
        static void             print_ledon_list (String name, uint_fast8_t ledon, bool do_display);
        static void             print_rcl_condition_list (bool in, uint_fast8_t caidx, uint_fast8_t condition, uint_fast8_t destination);
        static void             print_rcl_action_list (bool in, uint_fast8_t caidx, uint_fast8_t action);
        static void             decoder_railcom (uint_fast16_t addr, uint_fast8_t cv28, uint_fast8_t cv29);
        static void             decoder_configuration (uint_fast16_t addr, uint_fast8_t cv29, uint_fast8_t cv1, uint_fast8_t cv9, uint_fast16_t cv17_18);

        static void             action_on (void);
        static void             action_off (void);
        static void             action_go (void);
    private:
};

#endif
