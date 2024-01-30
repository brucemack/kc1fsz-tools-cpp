
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
#include <algorithm>

#include "kc1fsz-tools/Common.h"

namespace kc1fsz {

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

}

