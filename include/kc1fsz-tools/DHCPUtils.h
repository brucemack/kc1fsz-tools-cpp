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
 */
#pragma once

#include <cstdint>

namespace kc1fsz {
    namespace dhcputils {
      
/**
 * Creates a DHCP DISCOVER packet for the specified MAC address.
 */
int makeDiscover(const uint8_t* mac, uint16_t reqId, uint8_t* packet, unsigned packetCapacity);

/**
 * Creates a DHCP REQUEST packet for the specified MAC address and IP address.
 *
 * @param reqAddr4 The requested address in network byte ordering.
 */
int makeRequest(const uint8_t* mac, uint16_t reqId, uint32_t reqAddr4, uint8_t* packet, unsigned packetCapacity);
    }
}
