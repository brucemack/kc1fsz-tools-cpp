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
#include "kc1fsz-tools/linux/LinuxPollTimer.h"

#include <cstdint>
#include <ctime>
#include <iostream>

using namespace std;

namespace kc1fsz {

LinuxPollTimer::LinuxPollTimer(uint32_t us)
:   _intervalUs(us) {   
    reset();
}

void LinuxPollTimer::setIntervalUs(uint32_t us) {   
    _intervalUs = us;
    reset();
}

static uint64_t getTimeUs() {    
    struct timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts) != -1) {
        return ts.tv_sec * 1000000 + (ts.tv_nsec / 1000);
    } else {
        return 0;
    }
}

void LinuxPollTimer::reset() {    
    _startPointUs = getTimeUs();
    _nextPointUs = _startPointUs + _intervalUs;    
}

bool LinuxPollTimer::poll() {  
    uint64_t now = getTimeUs();
    if (now >= _nextPointUs) {
        _nextPointUs += _intervalUs;
        return true;
    } else {
        return false;  
    }
}

}
