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
#include <iostream>

namespace kc1fsz {

template<class T> class fixedqueue;
class fixedstring;

void prettyHexDump(const uint8_t* data, uint32_t len, std::ostream& out,
    bool color = false);

/**
 * @param targetLimit The actual size of the target buffer.  This 
 * function will automatically save a space for the null.
 */
void strcpyLimited(char* target, const char* source, uint32_t targetLimit);

/**
 * Appends the characters in the source string to the target string, being
 * careful not to overrun.
 * 
 * @param targetLimit The actual size of the target buffer.  This 
 * function will automatically save a space for the null.
 */
void strcatLimited(char* target, const char* source, uint32_t targetLimit);

void memcpyLimited(uint8_t* target, const uint8_t* source, 
    uint32_t sourceLen, uint32_t targetLimit);

bool isNullTerminated(const uint8_t* source, uint32_t sourceLen);

void panic(const char* msg);

// ----- Time Related --------------------------------------------------------

struct timestamp {

    timestamp() : ms(0) { }   
    timestamp(int64_t a) : ms(a) { }   
    void advanceMs(int32_t a) { ms += a; }
    void reset() { ms = 0; }
    bool isZero() const { return ms == 0; }

    int64_t ms = 0;
};

/**
 * @returns The current time in milliseconds-since-epoch
*/
timestamp time_ms();

/**
 * Used for testing purposes - sets time artificially.  
 */
void set_time(timestamp t);

/**
 * Used for testing purposes - moves time forward artificially.
 */
void advance_time_ms(int32_t ms);

int32_t ms_between(timestamp first, timestamp second);

/**
 * @returns Milliseconds since specified point, based on current time.
 */
int32_t ms_since(timestamp point);

/**
 * @returns Seconds since the Epoch
 */
uint32_t get_epoch_time();

void set_epoch_time(uint32_t t);

void format_iso_time(char* buf, uint32_t bufLen);

/**
 * Splits the given string according to the delimiter character. Leading
 * or trailing delimiters are ignored. The string doesn't need to start
 * or end with the delimiter. Repeated delimiters are treated as a single
 * delimiter.
 * 
 * TODO: Make a "strict" version that doesn't collapse repeated delimiters.
 * 
 * @returns 0 on success, -1 on failure (i.e. out of capacity on fixed
 * resources)
 */
int tokenize(const char* data, char delim, fixedqueue<fixedstring>& result);

// ------ Pack/Unpack Multibyte Numbers ------

void pack_uint32_be(uint32_t v, uint8_t* out);
void pack_uint16_be(uint16_t v, uint8_t* out);
uint32_t unpack_uint32_be(const uint8_t* in);
uint16_t unpack_uint16_be(const uint8_t* in);

void pack_int16_le(int16_t v, uint8_t* out);
int16_t unpack_int16_le(const uint8_t* out);

/**
 * Takes a ASCII hex string and converts to the binary representation. Helpful
 * for certain crypto use-cases.
 *
 * IMPORTANT: DOES NOT ASSUME/CONSUME A TERMINATING NULL.
 *
 * @param hex A string in two-digit hex format w/ no space like "0a0534bf".
 * @param hexLen The len of the hex string.  Length must be exactly binLen x 2.
 * @param bin The buffer where the binary data will be written.
 * @param binLen The length of the binary buffer. Yes, there is redundancy here, 
 * but it helps the user to keep thing straight.
 */
void asciiHexToBin(const char* hex, unsigned hexLen, uint8_t* bin, unsigned binLen);

/**
 * The opposite of above.
 *
 * IMPORTANT: DOES NOT WRITE A TERMINATING NULL. THE CALLER MUST ADD ONE IF THAT
 * IS NEEDED.
 */
void binToAsciiHex(const uint8_t* bin, unsigned binLen, char* hex, unsigned hexLen);

}
