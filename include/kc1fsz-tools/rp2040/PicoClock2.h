/**
 * Copyright (C) 2026, Bruce MacKinnon KC1FSZ
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
 */
#pragma once 

#include <pico/time.h>

#include "kc1fsz-tools/Clock.h"

namespace kc1fsz {

class PicoClock2 : public Clock {
public:

    uint32_t time() const {
        absolute_time_t now = get_absolute_time();
        return to_ms_since_boot(now);
    };

    uint64_t timeUs() const { 
        absolute_time_t now = get_absolute_time();
        return to_us_since_boot(now);
    }
};

}
