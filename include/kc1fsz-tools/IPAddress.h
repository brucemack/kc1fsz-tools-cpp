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
#ifndef _IPAddress_h
#define _IPAddress_h

#include <cstdint>
#include "Common.h"

namespace kc1fsz {

class IPAddress {
public:

    IPAddress() : _addr(0) { }
    IPAddress(const IPAddress& that) : _addr(that._addr) { }
    IPAddress(uint32_t addr) : _addr(addr) { }
    bool operator== (const IPAddress& that) const { return _addr == that._addr; }

    uint32_t getAddr() const { return _addr; }

    /**
     * Fills in the buffer with the decimal representation.  The 
     * buffer must include space for the null-termination.
     */
    void formatAsDottedDecimal(char* dottedAddr, uint32_t dottedAddrSize) {
        formatIP4Address(_addr, dottedAddr, dottedAddrSize);
    }

private:

    uint32_t _addr;
};

}

#endif
