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
 * @param mac MAC address in binary form (6 bytes)
 * @param reqId An ID that will be returned on the OFFER message.
 */
int makeDISCOVER(const uint8_t* mac, uint16_t reqId, uint8_t* packet, unsigned packetCapacity);

/**
 * Creates a DHCP REQUEST packet for the specified MAC address and IP address.
 * @param mac MAC address in binary form (6 bytes)
 * @param reqId An ID that will be returned on the ACK message.
 * @param reqAddr4 The requested address in network byte ordering.
 */
int makeREQUEST(const uint8_t* mac, uint16_t reqId, uint32_t reqAddr4, uint8_t* packet, unsigned packetCapacity);

/**
 * Parses a full Ethernet packet to extract DHCP information. It is assumed that this packet
 * is a DHCP OFFER message type.
 * @param leasedAddr4 The addressed assigned by the DHCP server in network order.
 * @returns 0 if the message is valid and parsed
 */
int parseOFFER(const uint8_t* packet, unsigned len, uint32_t transId, uint32_t* leasedAddr4);

/**
 * Parses a full Ethernet packet to extract DHCP information. It is assumed that this packet
 * is a DHCP ACK message type.
 * @param routerAddr4 The address of the router in network order.
 * @param subnetMask4 The subnet mask in network order.
 * @returns 0 if the message is valid and parsed
 */
int parseACK(const uint8_t* packet, unsigned len, uint32_t transId, uint32_t* routerAddr4, 
    uint32_t* subnetMask4);

    }
}
