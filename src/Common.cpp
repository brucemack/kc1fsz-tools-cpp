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
#include <ctime>
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

#include "kc1fsz-tools/fixedqueue.h"
#include "kc1fsz-tools/fixedstring.h"
#include "kc1fsz-tools/Common.h"

namespace kc1fsz {

#ifndef PICO_BUILD
//void panic(const char* msg) {
//    std::cerr << "PANIC: " << msg << std::endl;
//    assert(false);
//}
void panic(const char* msg) {
}
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
                snprintf(buf, 16, "%02x", (unsigned int)data[k]);
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
static timestamp fakeTime;

timestamp time_ms() {
    if (timeFixed) {
        return fakeTime;
    } 
    else {
#ifdef PICO_BUILD
        absolute_time_t now = get_absolute_time();
        return timestamp(to_ms_since_boot(now));
#else
        struct timeval tp;
        gettimeofday(&tp, NULL);
        long int ms = tp.tv_sec * 1000 + tp.tv_usec / 1000;
        return timestamp(ms);
#endif
    }
}

void set_time(timestamp t) {
    timeFixed = true;
    fakeTime = t;
}

void advance_time_ms(int32_t ms) {
    timeFixed = true;
    fakeTime.advanceMs(ms);
}

int32_t ms_between(timestamp first, timestamp second) {
    int64_t b = second.ms - first.ms;
    return b;
}

/**
 * @returns Milliseconds since specified point, based on current time.
 */
int32_t ms_since(timestamp point) {
    int64_t b = time_ms().ms - point.ms;
    return b;
}

#ifdef PICO_BUILD
// TODO: 
static uint32_t lastMsSinceBoot = 0;
static uint32_t lastEpochTime = 0;
uint32_t get_epoch_time() {
    uint32_t diff = to_ms_since_boot(get_absolute_time()) - lastMsSinceBoot;
    return lastEpochTime + (uint32_t)(diff / 1000);
}

void set_epoch_time(uint32_t t) {
    lastEpochTime = t;
    lastMsSinceBoot = to_ms_since_boot(get_absolute_time());
}
#else
uint32_t get_epoch_time() {
    return 0;
}
#endif

void format_iso_time(char* buf, uint32_t bufLen) {
    time_t ut2 = get_epoch_time();
    int ms = ut2 % 1000;
    struct tm* tm_info = localtime(&ut2);
    // The YYYY-MM-DD:HH:MM:SS part takes up 19 characters
    strftime(buf, bufLen, "%Y-%m-%dT%H:%M:%S.000", tm_info);
    // If there's room, overwrite the MS
    if (bufLen >= 24)
        snprintf(buf + 19, bufLen - 19, ".%03d", ms);
}

void formatIP4Address(uint32_t addr, char* dottedAddr, uint32_t dottedAddrSize) {
    // This is the high-order part of the address.
    uint32_t a = (addr & 0xff000000) >> 24;
    uint32_t b = (addr & 0x00ff0000) >> 16;
    uint32_t c = (addr & 0x0000ff00) >> 8;
    uint32_t d = (addr & 0x000000ff);
    // BRM 2025-02-01 changed from %lu to resolve warning
    snprintf(dottedAddr, dottedAddrSize, "%u.%u.%u.%u", a, b, c, d);
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

int tokenize(const char* data, char delim, fixedqueue<fixedstring>& result) {
    int state = 0;
    fixedstring accumulator;
    for (const char* r = data; *r != 0; r++) {
        // Pre first token
        if (state == 0) {
            if (*r == delim) {
                // Ignore
            }
            else {
                state = 1;
                accumulator.append(*r);
            }
        }
        else if (state == 1) {
            if (*r == delim) {
                if (!result.hasCapacity())
                    return -1;
                result.push(accumulator);
                accumulator.clear();
                state = 0;
            }
            else {
                accumulator.append(*r);
            }
        }
    }
    if (!accumulator.empty()) {
        if (!result.hasCapacity())
            return -1;
        result.push(accumulator);
    }
    return 0;
}

void pack_uint32_be(uint32_t v, uint8_t* out) {
    out[0] = (v >> 24) & 0xff;
    out[1] = (v >> 16) & 0xff;
    out[2] = (v >>  8) & 0xff;
    out[3] = (v >>  0) & 0xff;
}

void pack_uint16_be(uint16_t v, uint8_t* out) {
    out[0] = (v >>  8) & 0xff;
    out[1] = (v >>  0) & 0xff;
}

void pack_int16_le(int16_t v, uint8_t* out) {
    out[0] = (v >>  0) & 0xff;
    out[1] = (v >>  8) & 0xff;
}

uint32_t unpack_uint32_be(const uint8_t* in) {
    return (in[0] << 24) | (in[1] << 16) | (in[2] << 8) | in[3];
}

uint16_t unpack_uint16_be(const uint8_t* in) {
    return (in[0] << 8) | in[1];
}

int16_t unpack_int16_le(const uint8_t* in) {
    return (in[1] << 8) | in[0];
}

void asciiHexToBin(const char* hex, unsigned hexLen, uint8_t* bin, unsigned binLen) {
    assert(hexLen == 2 * binLen);
    const char* p = hex;
    for (unsigned i = 0; i < binLen; i++, p += 2) {
        char buf[3] = { *p, *(p + 1), 0 };
        bin[i] = strtol(buf, 0, 16);
    }
}

void binToAsciiHex(const uint8_t* bin, unsigned binLen, char* hex, unsigned hexLen) {
    assert(hexLen == 2 * binLen);
    char* p = hex;
    for (unsigned i = 0; i < binLen; i++, p += 2) {
        char buf[3];
        snprintf(buf, 3, "%02X", (unsigned)bin[i]);
        *p = buf[0];
        *(p + 1) = buf[1];
    }
}

}

