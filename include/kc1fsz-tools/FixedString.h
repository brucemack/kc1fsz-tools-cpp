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
#ifndef _FixedString_h
#define _FixedString_h

#include <cstring>
#include <cstdint>

#include "Common.h"

namespace kc1fsz {

/**
 * IMPORTANT: Maximum length is 64 bytes!
 */
class FixedString {
public:

    FixedString() { _s[0] = 0; }
    FixedString(const FixedString& that) { strcpyLimited(_s, that._s, _size); }
    FixedString(const char* s) { strcpyLimited(_s, s, _size); }
    bool operator== (const char* other) const { return strcmp(_s, other) == 0; }

    const char* c_str() const { return _s; }
    uint32_t len() const { return std::strlen(_s); }

    void append(char c) { 
        uint32_t l = len();
        if (l < _size - 1) {
            _s[l] = c;
            _s[l + 1] = 0;
        }
    }

    void toUpper() {
        for (uint32_t i = 0; i < _size && _s[i] != 0; i++)
            _s[i] = toupper(_s[i]);
    }

private:

    static const uint32_t _size = 64;
    char _s[_size];
};

}

#endif
