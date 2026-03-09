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
#include <cmath>

#include "kc1fsz-tools/Common.h"
#include "kc1fsz-tools/NTPUtils.h"

using namespace std;

namespace kc1fsz {
    namespace NTPUtils {

// The number of seconds between the NTP epoch (1900) and the UNIX epoch (1970)
static uint32_t EpochAdjustment = 2208988800;

uint64_t usToNtpTime(uint64_t us) {
    // Change epochs
    uint64_t adjustedUs = us + (EpochAdjustment * 1000000LL);
    // Number of seconds
    uint64_t s = adjustedUs / 1000000LL;
    uint64_t remainderUs = adjustedUs % 1000000;
    uint64_t remainderS = (remainderUs << 32) / 1000000LL;
    return (s << 32) | remainderS;
}

uint64_t ntpTimeToUs(uint64_t ntp) {
    // Change to microseconds
    uint64_t s = ntp >> 32;
    uint64_t us2 = ((uint64_t)(ntp & 0xffffffff) * 1000000LL) >> 32;
    uint64_t adjustedUs = s * 1000000 + us2;
    // Change epochs
    return adjustedUs - (EpochAdjustment * 1000000LL);
}

static const double SHIFT_FACTOR = std::pow(2.0, 32);

/**
 * @returns the clock offset using the NTP time format. A positive number
 * means that the reference time is ahead. The result is signed.
 */
double calcOffset(const uint8_t* packet, uint64_t nowNtp) {
    double t1 = (double)unpack_uint64_be(packet + 24) / SHIFT_FACTOR;
    double t2 = (double)unpack_uint64_be(packet + 32) / SHIFT_FACTOR;
    double t3 = (double)unpack_uint64_be(packet + 40) / SHIFT_FACTOR;
    double t4 = (double)nowNtp / SHIFT_FACTOR;
    return ((t2 - t1) + (t3 - t4)) / 2;
}

/**
 * @returns The round-trip of the NTP query using the NTP time format.
 */
double calcDelay(const uint8_t* packet, uint64_t nowNtp) {
    double t1 = (double)unpack_uint64_be(packet + 24) / SHIFT_FACTOR;
    double t2 = (double)unpack_uint64_be(packet + 32) / SHIFT_FACTOR;
    double t3 = (double)unpack_uint64_be(packet + 40) / SHIFT_FACTOR;
    double t4 = (double)nowNtp / SHIFT_FACTOR;
    return (t4 - t1) - (t3 - t2);
}

    }
}
