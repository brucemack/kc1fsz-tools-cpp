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
#include "kc1fsz-tools/Clock.h"
#include "kc1fsz-tools/StdPollTimer.h"

#include <cstdint>
#include <ctime>
#include <cassert>
#include <iostream>

using namespace std;

namespace kc1fsz {

StdPollTimer::StdPollTimer(Clock& clock, uint64_t us) 
:   _clock(clock) {
    setIntervalUs(us);
}

void StdPollTimer::setIntervalUs(uint32_t us) {   
    _intervalUs = us;
    reset();
}

void StdPollTimer::reset() {    
    _lastPointUs = _clock.timeUs();
    // Round off the start time using the interval size. So if the 
    // interval is 20,000 uS all intervals will be on an even 
    // 20,000 uS boundary.
    _lastPointUs /= _intervalUs;
    _lastPointUs *= _intervalUs;
}

uint64_t StdPollTimer::usLeftInInterval() const {
    uint64_t now = _clock.timeUs();
    uint64_t nextPointUs = _lastPointUs + _intervalUs;
    if (now < nextPointUs) 
        return nextPointUs - now;
    else
        return 0;
}

bool StdPollTimer::poll(uint64_t* intervalStart) {  
    uint64_t now = _clock.timeUs();
    uint64_t nextPointUs = _lastPointUs + _intervalUs;
    // When we pass the point move forward EXACTLY ONE interval. Note
    // that there is no guarantee that the next point will be
    // now or in the future (particularly if we've fallen behind).
    // No ticks should be lost.
    if (now >= nextPointUs) {
        _lastPointUs += _intervalUs;
        if (intervalStart)
            *intervalStart = _lastPointUs;
        return true;
    } else {
        return false;  
    }
}

uint64_t StdPollTimer::getCurrentIntervalUs() const {
    return _lastPointUs;
}

uint64_t StdPollTimer::getLateUs() const {
    uint64_t now = _clock.timeUs();
    return now - _lastPointUs;
}

}
