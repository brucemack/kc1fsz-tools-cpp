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

#include "Common.h"

namespace kc1fsz {

/**
 * IMPORTANT: Maximum length is 64 bytes!
 */
class FixedString {
public:

    FixedString() { _s[0] = 0; }
    FixedString(const FixedString& that) { strcpyLimited(_s, that._s, 64); }
    FixedString(const char* s) { strcpyLimited(_s, s, 64); }
    const char* c_str() { return _s; }
    uint32_t len() const { return std::strlen(_s); }

private:

    char _s[64];
};

}

#endif
