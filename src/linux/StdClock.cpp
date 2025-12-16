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

#include <kc1fsz-tools/linux/StdClock.h>

namespace kc1fsz {

// TODO: CLEAN UP WRAP PROBLEMS

uint32_t StdClock::time() const {
    uint64_t t = timeUs();
    t /= 1000;
    return t;
}

uint64_t StdClock::timeUs() const {
    timespec t1;
    clock_gettime(CLOCK_REALTIME, &t1);
    return t1.tv_sec * 1000000 + t1.tv_nsec / 1000;
}

}
