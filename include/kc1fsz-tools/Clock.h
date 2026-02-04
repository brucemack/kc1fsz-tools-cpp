/* 
 * Copyright (C) 2025 Bruce MacKinnon
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
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
#ifndef _Clock_h
#define _Clock_h

#include <stdint.h>

namespace kc1fsz {

class Clock {
public:

    /**
     * @returns The canonical milliseconds since the epoch.
     * Use caution because of 32-bit wrap-around issues.
     */
    virtual uint32_t time() const = 0;

    /**
     * @returns true when the current time is past the reference time.
     * Use caution because of 32-bit wrap-around issues.
     */
    bool isPast(uint32_t refMs) const { return time() > refMs; }

    /**
     * @returns The canonical microseconds since the epoch.
     */
    virtual uint64_t timeUs() const { return time() * 1000; }

    /**
     * @returns The canonical milliseconds since the epoch.
     */
    virtual uint64_t timeMs() const { return timeUs() / 1000; }

    bool isPastWindow(uint64_t refMs, uint64_t windowMs) const { 
        return timeMs() > refMs + windowMs; 
    }

    bool isInWindow(uint64_t refMs, uint64_t windowMs) const { 
        uint64_t nowMs = timeMs();
        return refMs <= nowMs && nowMs < (refMs + windowMs); 
    }
};

}

#endif
