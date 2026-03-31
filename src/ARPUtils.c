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
#include <string.h>

#include "kc1fsz-tools/ARPUtils.h"

static const uint8_t arputils_broadcastMac[6] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
static const uint8_t arputils_zeroMac[6] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

int arputils_makePacket4(const uint8_t* destMac, const uint8_t* sourceMac, 
    uint16_t opCode,
    const uint8_t* senderMac, uint32_t senderIpN, 
    const uint8_t* targetMac, uint32_t targetIpN,
    uint8_t* packet, unsigned packetCapacity) {
    if (packetCapacity < 60)
        return -1;
    memset(packet, 0, 60);
    // Destination MAC
    memcpy(packet, destMac, 6);
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
    packet[20] = (opCode >> 8) & 0xff;
    packet[21] = (opCode & 0xff);
    // Sender MAC
    memcpy(packet + 22, senderMac, 6);
    // Sender IP
    memcpy(packet + 28, &senderIpN, 4);
    // Target MAC
    memcpy(packet + 32, targetMac, 6);
    // Target IP
    memcpy(packet + 38, &targetIpN, 4);
    // The rest is padding
    return 60;
}

int arputils_makeQuery4(const uint8_t* sourceMac, uint32_t sourceIp, uint32_t targetIp,
    uint8_t* packet, unsigned packetCapacity) {
    return arputils_makePacket4(arputils_broadcastMac, sourceMac, 0x0001, sourceMac, sourceIp,
        arputils_zeroMac, targetIp, packet, packetCapacity);
}

int arputils_extractSenderMac4(const uint8_t* packet, unsigned packetLen, uint8_t* macAddr) {
    if (!arputils_isValid4(packet, packetLen))
        return -1;
    memcpy(macAddr, packet + 22, 6);
    return 0;
}

int arputils_isSenderIp4(const uint8_t* packet, unsigned packetLen, uint32_t addr4) {
    uint32_t replyIpN = 0;
    if (arputils_extractSenderIp4(packet, packetLen, &replyIpN) == -1)
        return 0;
    return (replyIpN == addr4) ? 1 : 0;
}

int arputils_extractSenderIp4(const uint8_t* packet, unsigned packetLen, uint32_t* addr4) {
    if (!arputils_isValid4(packet, packetLen))
        return -1;
    memcpy(addr4, packet + 28, 4);
    return 0;
}

int arputils_isTargetIp4(const uint8_t* packet, unsigned packetLen, uint32_t addr4) {
    if (!arputils_isValid4(packet, packetLen))
        return 0;
    uint32_t targetIpN = 0;
    memcpy(&targetIpN, packet + 38, 4);
    return (targetIpN == addr4) ? 1 : 0;
}

int arputils_isValid4(const uint8_t* packet, unsigned packetLen) {
    if (packetLen >= 42 &&
        // Ethernet Type=ARP 
        packet[12] == 0x08 &&
        packet[13] == 0x06 &&
        // ARP
        // Ethernet
        packet[14] == 0x00 &&
        packet[15] == 0x01 &&
        // IPv4
        packet[16] == 0x08 &&
        packet[17] == 0x00)
        return 1;
    else 
        return 0;
}

int arputils_isValidRequest4(const uint8_t* packet, unsigned packetLen) {
    if (arputils_isValid4(packet, packetLen) &&
        packet[20] == 0x00 && 
        packet[21] == 0x01)
        return 1;
    else 
        return 0;
}

int arputils_isValidReply4(const uint8_t* packet, unsigned packetLen) {
    if (arputils_isValid4(packet, packetLen) &&
        packet[20] == 0x00 && 
        packet[21] == 0x02)
        return 1;
    else 
        return 0;
}
