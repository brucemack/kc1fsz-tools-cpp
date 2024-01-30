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
#ifndef _KC1FSZCommon_h
#define _KC1FSZCommon_h

#include <cstdint>
#include <iostream>

namespace kc1fsz {

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
    
}

#endif
