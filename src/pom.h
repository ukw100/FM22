/*------------------------------------------------------------------------------------------------------------------------
 * pom.h - POM CV routines
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
#ifndef POM_H
#define POM_H

#include <stdint.h>

#define POM_WRITE_COMPARE_NONE                      0x00
#define POM_WRITE_COMPARE_BEFORE_WRITE              0x01
#define POM_WRITE_COMPARE_AFTER_WRITE               0x02
#define POM_WRITE_COMPARE_BEFORE_AND_AFTER_WRITE    (POM_WRITE_COMPARE_BEFORE_WRITE | POM_WRITE_COMPARE_AFTER_WRITE)

class POM
{
    public:
        static bool     pom_read_cv (uint_fast8_t * valuep, uint_fast16_t addr, uint16_t cv);
        static bool     xpom_read_cv (uint8_t * valuep, uint_fast8_t n, uint_fast16_t addr, uint_fast8_t cv31, uint_fast8_t cv32, uint_fast8_t cv);
        static uint32_t pom_get_read_retries (void);
        static uint32_t pom_get_num_reads (void);
        static void     pom_reset_num_reads (void);
        static bool     pom_write_cv (uint_fast16_t addr, uint16_t cv, uint_fast8_t value, uint_fast8_t compare);
        static bool     pom_write_cv_index (uint_fast16_t addr, uint_fast8_t cv31, uint_fast8_t cv32);
    private:
        static uint32_t sum_retries;
        static uint32_t sum_reads;
};

#endif
