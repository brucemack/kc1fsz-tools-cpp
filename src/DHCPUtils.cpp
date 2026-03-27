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

int makeDiscover(const uint8_t* mac, uint16_t reqId, uint8_t* packet, unsigned packetCapacity) {

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

    // BOOTP OPTIONS

    int used = 14 + 20 + 8 + sizeof(bootp_hdr);
    packetCapacity -= used;
   
    // DHCP Message Type (Request)
    //used += addIE_uint8(0x35, 0x03, packet + used, packetCapacity);
    // DHCP Message Type (Discover)
    used += addIE_uint8(0x35, 0x01, packet + used, packetCapacity);
    if (used < 0)
        return -2;
    packetCapacity -= used;

    // Client identifier
    char clientId[7];
    clientId[0] = 0x01;
    memcpy(clientId + 1, mac, 6);
    used += addIE_str(0x3d, clientId, 7, packet + used, packetCapacity);
    if (used < 0)
        return -2;
    packetCapacity -= used;

    // Host Name
    used += addIE_str(0x0c, "rtcm", packet + used, packetCapacity);
    if (used < 0)
        return -2;
    packetCapacity -= used;

    // CFQDN
    char fqdn[] = { 0x0, 0x0, 0x0, 0x64, 0x65, 0x6c, 0x6c, 0x2d, 0x6c, 0x61, 0x70, 0x74, 0x6f, 0x70 };
    used += addIE_str(0x51, fqdn, sizeof(fqdn), packet + used, packetCapacity);
    if (used < 0)
        return -2;
    packetCapacity -= used;

    // Parameter request list
    //char items[] = { 1, 3, 6, 15, 31, 33, 43, 44, 46, 47, 119, 121, 249, 252 };
    const char requestList[] = { 1, 3, 6 };
    used += addIE_str(0x37, requestList, sizeof(requestList), packet + used, packetCapacity);
    if (used < 0)
        return -2;
    packetCapacity -= used;

    // End
    if (packetCapacity == 0)
        return -3;
    packet[used++] = 0xff;
    packetCapacity -= 1;

    // Go back in fill in the lengths
    // NOTE: We don't include the Ethernet header in the IP length
    pack_uint16_be(used - 14, ipHdr + 2);
    // NOTE: We don't include the Ethernet header or the IP header in the UDP length
    pack_uint16_be(used - 14 - 20, udpHdr + 4);

    // Got back and fill in the IP checksums
    uint16_t cs = ipChecksum3(ipHdr, 20, 0, 0, 0, 0);
    ipHdr[10] = cs >> 8;
    ipHdr[11] = cs;

    // Go back and fill in the UDP checksum. 

    // Populate a "pseudo header" with some stuff from the IP header
    uint8_t phdr[12];
    // Source address
    pack_uint32_be(0, phdr);
    // Dest address
    pack_uint32_be(0xffffffff, phdr + 4);
    // Zero
    phdr[8] = 0;
    // Protocol (UDP)
    phdr[9] = 0x11;
    // NOTE: We don't include the Ethernet header or the IP header in the UDP length
    pack_uint16_be(used - 14 - 20, phdr + 10);

    // Checksum spans pseudo header, UDP header, and UDP payload
    cs = ipChecksum3(phdr, sizeof(phdr), udpHdr, 8, 
        packet + 14 + 20 + 8, used - 14 - 20 - 8);
    udpHdr[6] = cs >> 8;
    udpHdr[7] = cs;

    return used;
}
    }
}