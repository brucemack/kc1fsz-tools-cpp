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
#ifndef _CallSign_h
#define _CallSign_h

#include <cstring>

#include "Common.h"

namespace kc1fsz {

class CallSign {
public:

    CallSign() { _callSign[0] = 0; }
    CallSign(const CallSign& that) { strcpyLimited(_callSign, that._callSign, 32); }
    CallSign(const char* cs) { strcpyLimited(_callSign, cs, 32); }
    const char* c_str() { return _callSign; }
    uint32_t len() const { return std::strlen(_callSign); }
    bool operator== (const char* other) const { return strcmp(_callSign, other) == 0; }
    bool operator== (const CallSign& that) const { return strcmp(_callSign, that._callSign) == 0; }

private:

    char _callSign[32];
};

}

#endif
