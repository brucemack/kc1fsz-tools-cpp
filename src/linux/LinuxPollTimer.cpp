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
#include <cassert>
#include <iostream>

using namespace std;

namespace kc1fsz {

static uint64_t getTimeUs() {    
    struct timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts) != -1) {
        return (ts.tv_sec * 1000000L) + (ts.tv_nsec / 1000L);
    } else {
        assert(false);
        return 0;
    }
}

LinuxPollTimer::LinuxPollTimer(uint64_t us) {
    setIntervalUs(us);
}

void LinuxPollTimer::setIntervalUs(uint32_t us) {   
    _intervalUs = us;
    reset();
}

void LinuxPollTimer::reset() {    
    _lastPointUs = getTimeUs();
    // Round off the start time using the interval size. So if the 
    // interval is 20,000 uS all intervals will be on an even 
    // 20,000 uS boundary.
    _lastPointUs /= _intervalUs;
    _lastPointUs *= _intervalUs;
}

uint64_t LinuxPollTimer::usLeftInInterval() const {
    uint64_t now = getTimeUs();
    uint64_t nextPointUs = _lastPointUs + _intervalUs;
    if (now < nextPointUs) 
        return nextPointUs - now;
    else
        return 0;
}

bool LinuxPollTimer::poll() {  
    uint64_t now = getTimeUs();
    uint64_t nextPointUs = _lastPointUs + _intervalUs;
    // When we pass the point move forward ONE interval. Note
    // that there is no guarantee that the next point will be
    // in the future (particularly if we've fallen behind).
    if (now >= nextPointUs) {
        _lastPointUs += _intervalUs;
        return true;
    } else {
        return false;  
    }
}

uint64_t LinuxPollTimer::getCurrentIntervalUs() const {
    return _lastPointUs;
}

}
