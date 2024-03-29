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
#include <cstring>
#include <cctype>
#include <cassert>
#include <algorithm>
#include <iostream>

#ifdef PICO_BUILD
#include <pico/time.h>
#else
#include <sys/time.h>
#include <arpa/inet.h>
#endif

#include "kc1fsz-tools/Common.h"

namespace kc1fsz {

#ifndef PICO_BUILD
//void panic(const char* msg) {
//    std::cerr << "PANIC: " << msg << std::endl;
//    assert(false);
//}
#else 
void panic(const char* msg) {
    std::cerr << "PANIC: " << msg << std::endl;
    assert(false);
}
#endif


void prettyHexDump(const uint8_t* data, uint32_t len, std::ostream& out,
    bool color) {
    
    uint32_t lines = len / 16;
    if (len % 16 != 0) {
        lines++;
    }

    char buf[16];

    for (uint32_t line = 0; line < lines; line++) {

        // Position counter
        int l = snprintf(buf, 16, "%04X | ", (unsigned int)line * 16);
        if (l > 16) {
            panic("OVERFLOW");
        }
        out << buf;

        // Hex section
        for (uint16_t i = 0; i < 16; i++) {
            uint32_t k = line * 16 + i;
            if (k < len) {
                sprintf(buf, "%02x", (unsigned int)data[k]);
                out << buf << " ";
            } else {
                out << "   ";
            }
            if (i == 7) {
                out << " ";
            }
        }
        // Space between hex and ASCII section
        out << " ";

        if (color) {   
            out << "\u001b[36m";
        }

        // ASCII section
        for (uint16_t i = 0; i < 16; i++) {
            uint32_t k = line * 16 + i;
            if (k < len) {
                if (isprint((char)data[k]) && data[k] != 32) {
                    out << (char)data[k];
                } else {
                    //out << ".";
                    out << "\u00b7";
                }
            } else {
                out << " ";
            }
            if (i == 7) {
                out << " ";
            }
        }

        if (color) {   
            out << "\u001b[0m";
        }
        out << std::endl;
    }
}

void strcpyLimited(char* target, const char* source, uint32_t limit) {
    if (limit > 1) {
        uint32_t len = std::min(limit - 1, (uint32_t)std::strlen(source));
        memcpy(target, source, len);
        target[len] = 0;
    }
}

void strcatLimited(char* target, const char* source, uint32_t targetLimit) {
    uint32_t existingLen = std::min((uint32_t)std::strlen(target), targetLimit);
    if (existingLen < targetLimit) {
        strcpyLimited(target + existingLen, source, targetLimit - existingLen);
    }
}

void memcpyLimited(uint8_t* target, const uint8_t* source, 
    uint32_t sourceLen, uint32_t targetLimit) {
    uint32_t len = std::min(targetLimit, sourceLen);
    memcpy(target, source, len);
}

bool isNullTerminated(const uint8_t* source, uint32_t sourceLen) {
    for (uint32_t i = 0; i < sourceLen; i++) {
        if (source[i] == 0) {
            return true;
        }
    }
    return false;
}

static bool timeFixed = false;
static uint32_t fakeTime = 0;

uint32_t time_ms() {
    if (timeFixed) {
        return fakeTime;
    } else {
#ifdef PICO_BUILD
    absolute_time_t now = get_absolute_time();
        return to_ms_since_boot(now);
#else
        struct timeval tp;
        gettimeofday(&tp, NULL);
        long int ms = tp.tv_sec * 1000 + tp.tv_usec / 1000;
        return ms;
#endif
    }
}

void set_time_ms(uint32_t ms) {
    fakeTime = ms;
    timeFixed = true;
}

void advance_time_ms(uint32_t ms) {
    fakeTime = time_ms() + ms;
    timeFixed = true;
}

void formatIP4Address(uint32_t addr, char* dottedAddr, uint32_t dottedAddrSize) {
    // This is the high-order part of the address.
    uint32_t a = (addr & 0xff000000) >> 24;
    uint32_t b = (addr & 0x00ff0000) >> 16;
    uint32_t c = (addr & 0x0000ff00) >> 8;
    uint32_t d = (addr & 0x000000ff);
    snprintf(dottedAddr, dottedAddrSize, "%lu.%lu.%lu.%lu", a, b, c, d);
}

uint32_t parseIP4Address(const char* dottedAddr, uint32_t len) {
    uint32_t result = 0;
    char acc[8];
    uint32_t accLen = 0;
    const char *p = dottedAddr;
    uint32_t octets = 4;

    // If a limit has been specifed, calculate the stopping point
    const char* lastChar = 0;
    if (len != 0) {
        lastChar = dottedAddr + len - 1;
    }

    while (true) {
        if (*p == '.' || *p == 0 || p == lastChar) {
            acc[accLen] = 0;
            // Shift up
            result <<= 8;
            // Accumulate LSB
            result |= (uint8_t)atoi(acc);
            accLen = 0;
            // Count octets
            octets++;
            // Done yet?
            if (octets == 4 || *p == 0 || p == lastChar) {
                break;
            }
        }
        else {
            acc[accLen++] = *p;
        }
        p++;
    }
#ifdef PICO_BUILD
    return result;
#else
    return htonl(result);
#endif
}

}

