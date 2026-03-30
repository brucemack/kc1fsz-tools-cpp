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
#include <cstring>

#include "kc1fsz-tools/Common.h"

namespace kc1fsz {
    namespace arputils {

int makeQuery(const uint8_t* sourceMac, uint32_t sourceIp, uint32_t targetIp,
    uint8_t* packet, unsigned packetCapacity) {
    if (packetCapacity < 60)
        return -1;
    memset(packet, 0, 60);
    // Destination MAC
    memset(packet, 0xff, 6);
    // Source MAC
    memcpy(packet + 6, sourceMac, 6);
    // Type 
    packet[12] = 0x08;
    packet[13] = 0x06;
    // ARP
    // Ethernet
    packet[14] = 0x00;
    packet[15] = 0x01;
    // IPv4
    packet[16] = 0x08;
    packet[17] = 0x00;
    // HW size
    packet[18] = 6;
    // Protocol size
    packet[19] = 4;
    // Opcode
    packet[20] = 0;
    packet[21] = 1;
    // Sender MAC
    memcpy(packet + 22, sourceMac, 6);
    // Sender IP
    memcpy(packet + 28, &sourceIp, 4);
    // Target MAC
    memset(packet + 32, 0x00, 6);
    // Target IP
    memcpy(packet + 38, &targetIp, 4);
    // The rest is padding
    return 60;
}

bool isValid(const uint8_t* packet, unsigned packetLen) {
    return packetLen >= 42 &&
        // Ethernet Type=ARP 
        packet[12] == 0x08 &&
        packet[13] == 0x06 &&
        // ARP
        // Ethernet
        packet[14] == 0x00 &&
        packet[15] == 0x01 &&
        // IPv4
        packet[16] == 0x08 &&
        packet[17] == 0x00;
}

    }
}

