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
 *
 * NOT FOR COMMERCIAL USE WITHOUT PERMISSION.
 */
#ifndef _TimeDebouncer_h
#define _TimeDebouncer_h

#include <cstdint>
#include "kc1fsz-tools/Clock.h"
#include "kc1fsz-tools/BinaryWrapper.h"

namespace kc1fsz {

/**
 * @brief A utility that takes a raw binary value and converts it 
 * into a debounced value based on configured transition time minimums.
 */
class TimeDebouncer : public BinaryWrapper {
public:

    TimeDebouncer(Clock& clock, const BinaryWrapper& rawValue)
    :   _clock(clock),
        _rawValue(rawValue),
        _debouncedState(rawValue.get()),
        _lastStableTime(clock.time()) { 
    }

    void setActiveTime(uint32_t ms) { _activeTimeMs = ms; }
    void setInactiveTime(uint32_t ms) { _inactiveTimeMs = ms; }

    bool get() const {
        const bool state = _rawValue.get();
        // Decide how long the transition window should be, depending
        // on whether we are going false->true (activeTime) or true->false
        // (inactiveTime).
        auto transitionTimeMs = 
            (_debouncedState == false) ? _activeTimeMs : _inactiveTimeMs;
        // If the raw state is the same as the debounced state then 
        // we don't have the start of a valid transition yet, keep
        // moving the start of the transition period forward.
        if (_debouncedState == state) {
           const_cast<TimeDebouncer*>(this)->_lastStableTime = _clock.time();
        }
        // If the raw state is different from the debounced state
        // then we are in a POTENTIAL transition. Keep watching 
        // the time and see if we make it through the transition window.
        else {
            if ((_clock.time() - _lastStableTime) > transitionTimeMs) {
                const_cast<TimeDebouncer*>(this)->_debouncedState = !_debouncedState;
            }
        }
        return _debouncedState;
    }

private:

    const Clock& _clock;
    const BinaryWrapper& _rawValue;
    uint32_t _activeTimeMs = 0;
    uint32_t _inactiveTimeMs = 0;

    // This is the official state
    bool _debouncedState = false;
    uint32_t _lastStableTime = 0;
};

}

#endif
