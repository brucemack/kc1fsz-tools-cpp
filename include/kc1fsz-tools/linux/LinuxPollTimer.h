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
#ifndef _LinuxPollTimer_h
#define _LinuxPollTimer_h

#include <cstdint>

namespace kc1fsz {

class LinuxPollTimer {
public:

    LinuxPollTimer(uint64_t intervalUs);

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
     * @return true if the interval has expired.  Will only return 
     *  true once per interval.
    */
    virtual bool poll();

private:

    uint64_t _intervalUs = 0;
    uint64_t _startPointUs = 0;
    uint64_t _nextPointUs = 0;
};

}

#endif
