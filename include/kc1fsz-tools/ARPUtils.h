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

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int arputils_isValid4(const uint8_t* packet, unsigned packetLen);

int arputils_isValidRequest4(const uint8_t* packet, unsigned packetLen);

int arputils_isValidReply4(const uint8_t* packet, unsigned packetLen);

/**
 * Pulls sender MAC address out of ARP reply packet.
 * @returns 0 on success, -1 if packet is invalid.
 */
int arputils_extractSenderMac4(const uint8_t* packet, unsigned packetLen, uint8_t* macAddr);

/**
 * @param addr4 pointer to result address buffer (network order)
 * @returns 0 on success, -1 if packet is invalid.
 */
int arputils_extractSenderIp4(const uint8_t* packet, unsigned packetLen, uint32_t* addr4);

/**
 * Checks to see if the sender IP address of the ARP reply packet matches the one provided.
 * @returns 1 if yes, 0 if no.
 */
int arputils_isSenderIp4(const uint8_t* packet, unsigned packetLen, uint32_t addr4);

/**
 * Checks to see if the target IP address of the ARP reply packet matches the one provided.
 * @returns 1 if yes, 0 if no.
 */
int arputils_isTargetIp4(const uint8_t* packet, unsigned packetLen, uint32_t addr4);

/**
 * @param sourceIp in network order
 * @param targetIp in network order
 */
int arputils_makeQuery4(const uint8_t* sourceMac, uint32_t sourceIpN, uint32_t targetIpN,
    uint8_t* packet, unsigned packetCapacity);

/**
 * @param senderIpN in network order
 * @param targetIpN in network order
 */
int arputils_makePacket4(const uint8_t* destMac, const uint8_t* sourceMac, 
    uint16_t opCode,
    const uint8_t* senderMac, uint32_t senderIpN, 
    const uint8_t* targetMac, uint32_t targetIpN,
    uint8_t* packet, unsigned packetCapacity);

#ifdef __cplusplus
}
#endif
