/**
 * Copyright (C) 2024, Bruce MacKinnon KC1FSZ
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
 *
 * NOT FOR COMMERCIAL USE WITHOUT PERMISSION.
 */
 #include <pico/time.h>
#include "kc1fsz-tools/rp2040/PicoPollTimer.h"

namespace kc1fsz {

PicoPollTimer::PicoPollTimer()
:   _intervalUs(0) { 
}

void PicoPollTimer::setIntervalUs(uint32_t i) { 
    _intervalUs = i;
    reset(); 
}

void PicoPollTimer::reset() {
    _last = get_absolute_time();
}

bool PicoPollTimer::poll() {
    if (_intervalUs != 0) {
        absolute_time_t now = get_absolute_time();
        if (absolute_time_diff_us(_last, now) > _intervalUs) {
            // We update the last from the previous to maintain 
            // an even cadence.
            _last = delayed_by_us(_last, _intervalUs);
            return true;
        } else {
            return false;
        }
    } else {
        return false;
    }
}

}

