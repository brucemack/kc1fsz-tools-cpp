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
#include "arpa/inet.h"

#include <cstring>

#include "kc1fsz-tools/Common.h"
#include "kc1fsz-tools/NetUtils.h"
#include "kc1fsz-tools/DHCPUtils.h"
#include "kc1fsz-tools/ipchecksum.h"

namespace kc1fsz {
    namespace dhcputils {

struct bootphdr {
    uint8_t req;
    uint8_t hwType;
    uint8_t hwAddrLen;
    uint8_t hops;
    uint32_t transId;
    uint16_t elapsed;
    uint16_t flags;
    uint32_t clientAddr;
    uint32_t yourAddr;
    uint32_t nextServerAddr;
    uint32_t relayAgentAddr;
    uint8_t clientHwAddr[16];
    uint8_t serverHostName[64];
    uint8_t bootFileName[128];
    uint8_t magic[4];
};

// Shared between DISCOVER and REQUEST
static int makeReq(const uint8_t* mac, uint16_t reqId, uint8_t reqType, 
    uint32_t addr4, uint8_t* packet, unsigned packetCapacity) {

    // The request will look like this:
    // 
    // Ethernet Header [14 bytes]
    // IP Header [20 bytes]
    // UDP Header [8 bytes]
    // BOOTP 
    //   Fixed Header [236 bytes]
    //   DHCP Magic Cookie [4 bytes]
    //   Options [Variable Length]

    // ETHERNET HEADER
    uint8_t* ethHdr = packet;

    // Destination MAC (broadcast)
    memset(ethHdr, 0xff, 6);
    // Source MAC 
    memcpy(ethHdr + 6, mac, 6);
    // IPv4
    ethHdr[12] = 0x08;
    ethHdr[13] = 0x00;

    // IPv4 HEADER

    uint8_t* ipHdr = packet + 14;
    // V4, header len = 20
    ipHdr[0] = 0x45;
    // DSF
    ipHdr[1] = 0;
    // Length PLACEHOLDER, will be completed later
    ipHdr[2] = 0;
    ipHdr[3] = 0;
    // Request
    pack_uint16_be(reqId, ipHdr + 4);
    // Flags/offset
    ipHdr[6] = 0;
    ipHdr[7] = 0;
    // TTL
    ipHdr[8] = 0x80;
    // Protocol = UDP
    ipHdr[9] = 0x11;
    // Checksum - MUST START AS ZERO
    ipHdr[10] = 0;
    ipHdr[11] = 0;
    // Source address (0)
    memset(ipHdr + 12, 0, 4);
    // Dest address (broadcast)
    memset(ipHdr + 16, 0xff, 4);

    // UDP HEADER

    uint8_t* udpHdr = packet + 14 + 20;
    // Source port
    pack_uint16_be(68, udpHdr);
    // Dest port
    pack_uint16_be(67, udpHdr + 2);
    // UDP header + body length (placeholder) - WILL BE FILLED IN LATER
    udpHdr[4] = 0;
    udpHdr[5] = 0;
    // UDP checksum - MUST START AS ZERO
    udpHdr[6] = 0;
    udpHdr[7] = 0;

    // BOOTP FIXED HEADER

    bootphdr bootp_hdr = { 0 };
    // Request
    bootp_hdr.req = 1;
    // Ethernet
    bootp_hdr.hwType = 1;
    bootp_hdr.hwAddrLen = 6;
    bootp_hdr.hops = 0;
    bootp_hdr.transId = htonl(reqId);
    bootp_hdr.elapsed = 0;
    // Bootp flags (broadcast)
    bootp_hdr.flags = htons(0x8000);
    bootp_hdr.clientAddr = 0;
    bootp_hdr.yourAddr = 0;
    bootp_hdr.nextServerAddr = 0;
    bootp_hdr.relayAgentAddr = 0;
    memcpy(bootp_hdr.clientHwAddr, mac, 6);
    bootp_hdr.magic[0] = 0x63;
    bootp_hdr.magic[1] = 0x82;
    bootp_hdr.magic[2] = 0x53;
    bootp_hdr.magic[3] = 0x63;
    // Transfer to packet buffer
    memcpy(packet + 14 + 20 + 8, &bootp_hdr, sizeof(bootp_hdr));

    // BOOTP OPTIONS (Variable Length)

    int used = 14 + 20 + 8 + sizeof(bootp_hdr);
    packetCapacity -= used;
    int rc; 

    // DHCP Message Type (Request)
    rc = addIE_uint8(0x35, reqType, packet + used, packetCapacity);
    if (rc < 0)
        return -2;
    used += rc;
    packetCapacity -= rc;

    // Client identifier (Ethernet + MAC)
    const uint8_t clientId[] = { 0x01, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5] };
    rc = addIE_str(0x3d, (const char*)clientId, sizeof(clientId), packet + used, packetCapacity);
    if (rc < 0)
        return -2;
    used += rc;
    packetCapacity -= rc;

    if (reqType == 0x03) {
        // DIFFERENCE BETWEEN DISCOVER AND REQUEST!!
        // NOTE: The address is already in network order!
        rc = addIE_str(0x32, (const char*)&addr4, 4, packet + used, packetCapacity);
        if (rc < 0)
            return -2;
        used += rc;
        packetCapacity -= rc;
    }

    // Parameter request list
    const char requestList[] = { 1, 3, 6 };
    rc = addIE_str(0x37, requestList, sizeof(requestList), packet + used, packetCapacity);
    if (rc < 0)
        return -2;
    used += rc;
    packetCapacity -= rc;

    // End
    if (packetCapacity == 0)
        return -3;
    packet[used++] = 0xff;
    packetCapacity -= 1;

    // Go back in fill in the IP packet length
    // NOTE: We don't include the Ethernet header in the IP length
    pack_uint16_be(used - 14, ipHdr + 2);

    // Go back in fill in the UDP packet length
    // NOTE: We don't include the Ethernet header or the IP header in the UDP length
    pack_uint16_be(used - 14 - 20, udpHdr + 4);

    // Got back and fill in the IP checksums
    pack_uint16_be(ipChecksum3(ipHdr, 20, 0, 0, 0, 0), ipHdr + 10);

    // Go back and fill in the UDP checksum. 
    // Populate a "pseudo header" with some stuff from the IP header that gets
    // factored into the UDP checksum calculation.
    uint8_t phdr[12] = { 0 };
    // Source address
    pack_uint32_be(0, phdr);
    // Dest address
    pack_uint32_be(0xffffffff, phdr + 4);
    // Protocol (UDP)
    phdr[9] = 0x11;
    // NOTE: We don't include the Ethernet header or the IP header in the UDP length
    pack_uint16_be(used - 14 - 20, phdr + 10);
    // UDP checksum spans pseudo header, UDP header, and UDP payload
    pack_uint16_be(ipChecksum3(phdr, sizeof(phdr), udpHdr, 8, 
        packet + 14 + 20 + 8, used - 14 - 20 - 8), udpHdr + 6);

    return used;
}

int makeDISCOVER(const uint8_t* mac, uint16_t reqId, uint8_t* packet, unsigned packetCapacity) {
    return makeReq(mac, reqId, 0x01, 0, packet, packetCapacity);
}
/*
    // The request will look like this:
    // 
    // Ethernet Header [14 bytes]
    // IP Header [20 bytes]
    // UDP Header [8 bytes]
    // BOOTP 
    //   Fixed Header [236 bytes]
    //   DHCP Magic Cookie [4 bytes]
    //   Options [Variable Length]

    // ETHERNET HEADER
    uint8_t* ethHdr = packet;

    // Destination MAC (broadcast)
    memset(ethHdr, 0xff, 6);
    // Source MAC 
    memcpy(ethHdr + 6, mac, 6);
    // IPv4
    ethHdr[12] = 0x08;
    ethHdr[13] = 0x00;

    // IPv4 HEADER

    uint8_t* ipHdr = packet + 14;
    // V4, header len = 20
    ipHdr[0] = 0x45;
    // DSF
    ipHdr[1] = 0;
    // Length PLACEHOLDER, will be completed later
    ipHdr[2] = 0;
    ipHdr[3] = 0;
    // Request
    pack_uint16_be(reqId, ipHdr + 4);
    // Flags/offset
    ipHdr[6] = 0;
    ipHdr[7] = 0;
    // TTL
    ipHdr[8] = 0x80;
    // Protocol = UDP
    ipHdr[9] = 0x11;
    // Checksum - MUST START AS ZERO
    ipHdr[10] = 0;
    ipHdr[11] = 0;
    // Source address (0)
    memset(ipHdr + 12, 0, 4);
    // Dest address (broadcast)
    memset(ipHdr + 16, 0xff, 4);

    // UDP HEADER

    uint8_t* udpHdr = packet + 14 + 20;
    // Source port
    pack_uint16_be(68, udpHdr);
    // Dest port
    pack_uint16_be(67, udpHdr + 2);
    // UDP header + body length (placeholder) - WILL BE FILLED IN LATER
    udpHdr[4] = 0;
    udpHdr[5] = 0;
    // UDP checksum - MUST START AS ZERO
    udpHdr[6] = 0;
    udpHdr[7] = 0;

    // BOOTP FIXED HEADER

    bootphdr bootp_hdr = { 0 };
    // Request
    bootp_hdr.req = 1;
    // Ethernet
    bootp_hdr.hwType = 1;
    bootp_hdr.hwAddrLen = 6;
    bootp_hdr.hops = 0;
    bootp_hdr.transId = htonl(reqId);
    bootp_hdr.elapsed = 0;
    // Bootp flags (broadcast)
    bootp_hdr.flags = htons(0x8000);
    bootp_hdr.clientAddr = 0;
    bootp_hdr.yourAddr = 0;
    bootp_hdr.nextServerAddr = 0;
    bootp_hdr.relayAgentAddr = 0;
    memcpy(bootp_hdr.clientHwAddr, mac, 6);
    bootp_hdr.magic[0] = 0x63;
    bootp_hdr.magic[1] = 0x82;
    bootp_hdr.magic[2] = 0x53;
    bootp_hdr.magic[3] = 0x63;
    // Transfer to packet buffer
    memcpy(packet + 14 + 20 + 8, &bootp_hdr, sizeof(bootp_hdr));

    // BOOTP OPTIONS (Variable Length)

    int used = 14 + 20 + 8 + sizeof(bootp_hdr);
    packetCapacity -= used;
    int rc; 

    // DHCP Message Type (Discover)
    rc = addIE_uint8(0x35, 0x01, packet + used, packetCapacity);
    if (rc < 0)
        return -2;
    used += rc;
    packetCapacity -= rc;

    // Client identifier (Ethernet + MAC)
    const char clientId[] = { 0x01, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5] };
    rc = addIE_str(0x3d, clientId, sizeof(clientId), packet + used, packetCapacity);
    if (rc < 0)
        return -2;
    used += rc;
    packetCapacity -= rc;

    // Parameter request list
    const char requestList[] = { 1, 3, 6 };
    rc = addIE_str(0x37, requestList, sizeof(requestList), packet + used, packetCapacity);
    if (rc < 0)
        return -2;
    used += rc;
    packetCapacity -= rc;

    // End
    if (packetCapacity == 0)
        return -3;
    packet[used++] = 0xff;
    packetCapacity -= 1;

    // Go back in fill in the IP packet length
    // NOTE: We don't include the Ethernet header in the IP length
    pack_uint16_be(used - 14, ipHdr + 2);

    // Go back in fill in the UDP packet length
    // NOTE: We don't include the Ethernet header or the IP header in the UDP length
    pack_uint16_be(used - 14 - 20, udpHdr + 4);

    // Got back and fill in the IP checksums
    pack_uint16_be(ipChecksum3(ipHdr, 20, 0, 0, 0, 0), ipHdr + 10);

    // Go back and fill in the UDP checksum. 
    // Populate a "pseudo header" with some stuff from the IP header that gets
    // factored into the UDP checksum calculation.
    uint8_t phdr[12] = { 0 };
    // Source address
    pack_uint32_be(0, phdr);
    // Dest address
    pack_uint32_be(0xffffffff, phdr + 4);
    // Protocol (UDP)
    phdr[9] = 0x11;
    // NOTE: We don't include the Ethernet header or the IP header in the UDP length
    pack_uint16_be(used - 14 - 20, phdr + 10);
    // UDP checksum spans pseudo header, UDP header, and UDP payload
    pack_uint16_be(ipChecksum3(phdr, sizeof(phdr), udpHdr, 8, 
        packet + 14 + 20 + 8, used - 14 - 20 - 8), udpHdr + 6);

    return used;
}
*/

int makeREQUEST(const uint8_t* mac, uint16_t reqId, uint32_t addr4, uint8_t* packet, unsigned packetCapacity) {
    return makeReq(mac, reqId, 0x03, addr4, packet, packetCapacity);
}

/**
 * Checks an Ethernet packet to see if it is a valid DHCP response.
 */
bool isResponse(const uint8_t* packet, unsigned len) {
    if (len < 282 ||
        // Ethernet, IPv4
        packet[12] != 0x08 ||
        packet[13] != 0x00 ||
        // IP, service type
        packet[14 + 9] != 0x11 ||
        // BOOTP, message type = REPLY
        packet[14 + 20 + 8] != 0x02 ||
        // Magic number 
        packet[14 + 20 + 8 + 236] != 0x63 ||
        packet[14 + 20 + 8 + 237] != 0x82 ||
        packet[14 + 20 + 8 + 238] != 0x53 ||
        packet[14 + 20 + 8 + 239] != 0x63)
        return false;
    else 
        return true;
}

static bool _isValidDHCPResponse(const uint8_t* packet, unsigned len, uint32_t transId) {
    if (!isResponse(packet, len))
        return false;
    if (unpack_uint32_be(packet + 14 + 20 + 8 + 4) != transId)
        return false;
    return true;
}

int parseOFFER(const uint8_t* packet, unsigned len, uint32_t transId, uint32_t* leasedAddr4) {
    if (!_isValidDHCPResponse(packet, len, transId))
        return -1;
    // The size of all of the fixed headers that we have to skip past to find the options
    const unsigned preLen = 14 + 20 + 8 + 240;
    int rc;
     // Pull out the DHCP packet type and validate
    uint8_t dhcpType = 0;
    rc = extractIE_uint8(packet + preLen, len - preLen, 0x35, &dhcpType);
    if (rc != 0) {
        return -2;
    }
    if (dhcpType != 2)
        return -1;
    // Grab the IP address in network format
    memcpy(leasedAddr4, packet + 14 + 20 + 8 + 16, 4);
    return 0;
}

int parseACK(const uint8_t* packet, unsigned len, uint32_t transId, uint32_t* routerAddr4, uint32_t* mask4) {
    if (!_isValidDHCPResponse(packet, len, transId))
        return -1;

    // The size of all of the fixed headers that we have to skip past to find the options
    const unsigned preLen = 14 + 20 + 8 + 240;
    int rc;

    // Pull out the DHCP packet type and validate
    uint8_t dhcpType = 0;
    rc = extractIE_uint8(packet + preLen, len - preLen, 0x35, &dhcpType);
    if (rc != 0) {
        return -2;
    }
    if (dhcpType != 5)
        return -1;

    uint32_t router = 0;
    rc = extractIE_uint32(packet + preLen, len - preLen, 0x03, &router);
    if (rc != 0) {
        return -2;
    }
    *routerAddr4 = htonl(router);
    uint32_t mask = 0;
    rc = extractIE_uint32(packet + preLen, len - preLen, 0x01, &mask);
    if (rc != 0) {
        return -2;
    }
    *mask4 = htonl(mask);
    return 0;
}

    }
}