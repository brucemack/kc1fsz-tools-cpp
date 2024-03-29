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

#include "kc1fsz-tools/IPAddress.h"

namespace kc1fsz {

/**
 * Parses a string in a.b.c.d:p format.
 */
IP4AddressAndPort parseAddressAndPort(const char* a) {
    const char* e = strchr(a, ':');
    if (e == 0) {
        return { IPAddress(0), 0 };
    }
    uint32_t colonIdx = (e - a);
    return { IPAddress(parseIP4Address(a, colonIdx)), 
             (uint16_t)atoi(a + colonIdx + 1) };
}

}
