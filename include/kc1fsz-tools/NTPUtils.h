/**
 * Copyright (C) 2026, Bruce MacKinnon KC1FSZ
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
 */
#pragma once

#include <cstdint>

namespace kc1fsz {
    namespace NTPUtils {

/**
 * Converts microseconds since the epoch (i.e. standard UNIX time in us)
 * to the NTP time format. NTP time is in seconds since 1900-01-01. The 
 * representation is 64-bits with 32 bits of integer seconds and 32 bits 
 * of fractional seconds. 
 *
 * One second is      00000001 00000000
 * One half second is 00000000 10000000
 * One microsecond is 00000000 000010c6
 */
uint64_t usToNtpTime(uint64_t us);

/**
 * Converts the standard NTP time format to microseconds since the epoch
 * (i.e. standard UNIX time in us).
 */
uint64_t ntpTimeToUs(uint64_t ntp);

double calcOffset(const uint8_t* packet, uint64_t nowNtp);

double calcDelay(const uint8_t* packet, uint64_t nowNtp);

    }
}
