/**
 * Copyright (C) 2025, Bruce MacKinnon KC1FSZ
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

#include <cassert>

#include "pico/time.h"

#include "kc1fsz-tools/PollTimer.h"

namespace kc1fsz {

class PicoPollTimer : public PollTimer {
public:

    PicoPollTimer();

    void setIntervalUs(uint32_t i);
    void reset();
    bool poll(); 

    // #### TODO NOT IMPLEMENTED
    uint64_t getCurrentIntervalUs() const { assert(false); return 0; }
    uint32_t usLeftInInterval() const { assert(false); return 0; }

private:

    uint32_t _intervalUs;
    absolute_time_t _last;
};

}
