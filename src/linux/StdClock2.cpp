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
#include <ctime>
#include <iostream>

#include <kc1fsz-tools/linux/StdClock2.h>

namespace kc1fsz {

uint32_t StdClock2::time() const {
    uint64_t t = timeUs();
    t /= 1000;
    return t;
}

uint64_t StdClock2::timeUs() const {
    /*
        The monotonic clock is being used to avoid breaks in timing logic
        when the "realtime" clock is shifted by something like NTP. 

        CLOCK_MONOTONIC
        A nonsettable system-wide clock that represents monotonic time
        since—as described by POSIX—"some unspecified point in the
        past".  On Linux, that point corresponds to the number of sec‐
        onds that the system has been running since it was booted.

        The CLOCK_MONOTONIC clock is not affected by discontinuous
        jumps in the system time (e.g., if the system administrator
        manually changes the clock), but is affected by the incremen‐
        tal adjustments performed by adjtime(3) and NTP.  This clock
        does not count time that the system is suspended.  All
        CLOCK_MONOTONIC variants guarantee that the time returned by
        consecutive calls will not go backwards, but successive calls
        may—depending on the architecture—return identical (not-
        increased) time values.
    */
    timespec t1;
    clock_gettime(CLOCK_MONOTONIC, &t1);
    return (t1.tv_sec * 1000000 + t1.tv_nsec / 1000);
}

}
