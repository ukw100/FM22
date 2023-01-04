/*------------------------------------------------------------------------------------------------------------------------
 * userio.cc - user i/o functions
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
#include "loco.h"
#include "switch.h"
#include "railroad.h"
#include "s88.h"
#include "rcl.h"
#include "userio.h"

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * UserIO::booster_off () - disable booster
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
UserIO::booster_off (bool do_send_booster_cmd)
{
    RCL::booster_off ();
    S88::booster_off ();
    Railroad::booster_off ();
    Switch::booster_off ();
    Loco::booster_off (do_send_booster_cmd);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * UserIO::booster_on () - enable booster
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
UserIO::booster_on (bool do_send_booster_cmd)
{
    Loco::booster_on (do_send_booster_cmd);
    Switch::booster_on ();
    Railroad::booster_on ();
    S88::booster_on ();
    RCL::booster_on ();
}
