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
#ifndef _Win32PerfTimer_h
#define _Win32PerfTimer_h

#include <cstdint>
#include <string>
#include <iostream>

#ifdef __CYGWIN__
#include <Windows.h>
#endif 

namespace kc1fsz {

class Win32PerfTimer {
public:

    Win32PerfTimer();

    void reset();
    uint32_t elapsedUs() const;

private:
#ifdef __CYGWIN__
    LARGE_INTEGER _freq;
#endif
    uint32_t _startUs;
};

}

#endif
