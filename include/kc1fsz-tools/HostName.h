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
#ifndef _HostName_h
#define _HostName_h

#include <cstring>

#include "Common.h"

namespace kc1fsz {

class HostName {
public:

    HostName() { _name[0] = 0; }
    HostName(const char* n) { setName(n); }

    void setName(const char* n) { strcpyLimited(_name, n, 32); }
    const char* c_str() const { return _name; }

    bool operator== (const HostName& that) const { return strcmp(_name, that._name) == 0; }

private:

    char _name[32];
};

}

#endif
