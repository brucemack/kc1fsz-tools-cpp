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
#pragma once

#include <cstdint>

namespace kc1fsz {

class Clock;

class StdPollTimer {
public:

    StdPollTimer(Clock& clock, uint64_t intervalUs);

    virtual void setIntervalUs(uint32_t intervalUs);

    /**
     * Starts the interval again. This is used for 
     * synchronization - it doesn't need to be called
     * explicitly on each interval.
     */
    virtual void reset();

    /**
     * @returns The number of microseconds remaining in the interval.
     */
    virtual uint64_t usLeftInInterval() const;

    /**
     * @returns true if the current interval has expired.  Will only return 
     * true exactly once per interval. Note that this means that 
     * polls may "queue up" if you are slow calling this. For example,
     * if the interval is set to 10 us and you wait 20us before calling
     * poll() you will get a few expirations in quick succession while
     * the timer catches up.
     */
    virtual bool poll();

    /**
     * @returns The start time of the current interval in microseconds.
     * This will be updated AFTER the poll() call is made.
     */
    virtual uint64_t getCurrentIntervalUs() const;

    /**
     * @return The number of us past the last scheduled interval 
     * expirationation. In other words, how late are we?
     */
    virtual uint64_t getLateUs() const;

private:

    Clock& _clock;
    uint64_t _intervalUs = 0;
    uint64_t _lastPointUs = 0;
};

}
