#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <iostream>
#include <cstring>
#include <cassert>

#include "kc1fsz-tools/Common.h"

using namespace std;
using namespace kc1fsz;

static const char* DNS_IP_ADDR = "208.67.222.222";
static const char* DOMAIN_0 = "_iax._udp.29999.nodes.allstarlink.org";
static const char* DOMAIN_1 = "29999.nodes.allstarlink.org";

int makeDNSHeader(uint16_t id, uint8_t* packet, unsigned packetSize) {
    if (packetSize < 12)
        return -1;
    // Make the header
    pack_uint16_be(id, packet);
    uint16_t work;
    // QR = 0, RD=1
    work = 0x0100;
    pack_uint16_be(work, packet + 2);
    // QDCOUNT=1
    work = 0x0001;
    pack_uint16_be(work, packet + 4);
    // ANCOUNT=0, NSCOUNT=0, ARCOUNT=0
    work = 0x0000;
    pack_uint16_be(work, packet + 6);
    pack_uint16_be(work, packet + 8);
    pack_uint16_be(work, packet + 10);
    return 12;
}

/**
 * @returns Number of bytes written.
 */
int writeDomainName(const char* domainName, uint8_t* packet, 
    unsigned packetSize) {
    unsigned int i = 0;
    // Write the domain name one label at a time.
    // Labels are delimited by dots.
    int len = 0;
    bool postDot = true;
    for (unsigned j = 0; j < strlen(domainName); j++) {
        // Deal with a new label
        if (postDot) {
            // A leading or consecutive . is not allowed
            if (domainName[j] == '.')
                return -2;
            // Placeholder in len field, will be corrected later
            packet[i++] = 0;
            if (i == packetSize)
                return -1;
            len = 0;
            postDot = false;
        }
        // Look for the end of a label
        if (domainName[j] == '.') {
            // Fill in the correct length now that we know it
            packet[i - len - 1] = len;
            postDot = true;
        } else {
            packet[i++] = domainName[j];
            if (i == packetSize)
                return -1;
            len++;
        }
    }
    // We should never have a dot as a final character
    if (postDot)
        return -3;
    // Fill in the correct length for the last label now 
    // that we know it
    packet[i - len - 1] = len;
    // Ending null
    packet[i++] = 0;
    return i;
}

/**
 * @param packet A pointer to the ENTIRE DNS packet. This
 * is important since some names contain pointers to 
 * other locations in the packet.
 * @param nameOffset The starting location of the name 
 * to be parsed.
 * @param name If non-null, will be populated with a null-terminated
 * domain name in dotted format. A null can be passed if the goal 
 * is just to skip over the name in the packet.
 * @returns A negative number on error, or the ending offset 
 * in the packet immediately following the name.
 */
int parseDomainName(const uint8_t* packet, unsigned packetSize,
    unsigned nameOffset, char* name, unsigned nameSize) {
    unsigned i = nameOffset;
    unsigned len = 0;
    bool first = true;
    unsigned namePtr = 0;
    while (true) {
        if (i == packetSize) 
            return -1;
        len = packet[i++];
        // End?
        if (len == 0) {
            if (name) {
                if (namePtr == nameSize)
                    return -2;
                name[namePtr++] = 0;
            }
            return i;
        }
        // Terminate previous label
        if (!first) {
            if (name) {
                if (namePtr == nameSize)
                    return -2;
                name[namePtr++] = '.';
            }
        }
        first = false;
        // Pointer? 
        if ((len & 0xc0) == 0xc0) {
            if (i == packetSize) 
                return -1;
            // The offset is 14 bits.
            unsigned offset = ((len & 0b00111111) << 8) | packet[i++];
            // Make sure its contained in the packet
            if (offset >= packetSize)
                return -3;
            // Follow the pointer and accumulate more labels
            int rc = parseDomainName(packet, packetSize, offset, 
                name ? name + namePtr : 0, 
                name ? nameSize - namePtr : 0);
            if (rc < 0)
                return rc;
            // It is assumed that the null termination was handled by the 
            // parseDomainName() call above.
            return i;
        }
        // Normal accumulation
        else {
            // Error checks
            if (i + len >= packetSize) 
                return -1;
            if (name) {
                if (namePtr + len >= nameSize) 
                    return -2;
            }
            for (unsigned j = 0; j < len; j++) {
                if (name)
                    name[namePtr++] = packet[i];
                i++;
            }
        }
    }
}

/**
 * @returns The offset into the packet immediately following 
 * the questions. 
 */
int skipQuestions(const uint8_t* packet, unsigned packetLen) {
    if (packetLen < 12)
        return -1;
    // How many questions are there?
    uint16_t qdcount = unpack_uint16_be(packet + 4);
    unsigned i = 12;
    for (unsigned q = 0; q < qdcount; q++) {
        // Parse out the QNAME
        const unsigned nameCapacity = 65;
        char name[nameCapacity];
        int rc = parseDomainName(packet, packetLen, i, name, nameCapacity);
        if (rc < 0)
            return rc;
        i = rc;
        // Skip the QTYPE and QCLASS
        if (i + 4 >= packetLen)
            return -1;
        i += 4;
    }
    return i;
}

int parseDNSAnswer_SRV(const uint8_t* packet, unsigned packetLen, 
    unsigned answerOffset,
    uint16_t* pri, uint16_t* weight, uint16_t* port, 
    char* hostname, unsigned hostnameCapacity) {
    unsigned i = answerOffset;
    // We just skip the NAME by passing 0s
    int rc = parseDomainName(packet, packetLen, i, 0, 0);
    if (rc < 0)
        return rc;
    i = rc;
    // TYPE
    if (i + 2 > packetLen)
        return -1;
    uint16_t typeCode = unpack_uint16_be(packet + i);
    i += 2;
    if (typeCode != 0x0021)
        return -3;
    // CLASS
    if (i + 2 > packetLen)
        return -1;
    uint16_t classCode = unpack_uint16_be(packet + i);
    i += 2;
    if (classCode != 0x0001)
        return -4;
    // TTL (not used)
    if (i + 4 > packetLen)
        return -1;
    //uint32_t ttl = unpack_uint32_be(packet + i);
    i += 4;
    // RDLENGTH
    if (i + 2 > packetLen)
        return -1;
    uint16_t rdLength = unpack_uint16_be(packet + i);
    i += 2;
    // RDATA 
    if (i + rdLength > packetLen)
        return -1;
    // RDATA - PRI
    if (i + 2 > packetLen)
        return -1;
    *pri = unpack_uint16_be(packet + i);
    i += 2;
    // RDATA - WEIGHT
    if (i + 2 > packetLen)
        return -1;
    *weight = unpack_uint16_be(packet + i);
    i += 2;
    // RDATA - PORT
    if (i + 2 > packetLen)
        return -1;
    *port = unpack_uint16_be(packet + i);
    i += 2;
    // RDATA - HOSTNAME
    rc = parseDomainName(packet, packetLen, i, hostname, hostnameCapacity);
    if (rc < 0)
        return rc;
    i = rc;
    return i;
}

int parseDNSAnswer_A(const uint8_t* packet, unsigned packetLen, 
    unsigned answerOffset,
    uint32_t* addr) {
    unsigned i = answerOffset;
    // We just skip the NAME by passing 0s
    int rc = parseDomainName(packet, packetLen, i, 0, 0);
    if (rc < 0)
        return rc;
    i = rc;
    // TYPE
    if (i + 2 > packetLen)
        return -1;
    uint16_t typeCode = unpack_uint16_be(packet + i);
    i += 2;
    if (typeCode != 0x0001)
        return -3;
    // CLASS
    if (i + 2 > packetLen)
        return -1;
    uint16_t classCode = unpack_uint16_be(packet + i);
    i += 2;
    if (classCode != 0x0001)
        return -4;
    // TTL (not used)
    if (i + 4 > packetLen)
        return -1;
    //uint32_t ttl = unpack_uint32_be(packet + i);
    i += 4;
    // RDLENGTH
    if (i + 2 > packetLen)
        return -1;
    uint16_t rdLength = unpack_uint16_be(packet + i);
    i += 2;
    if (rdLength != 4)
        return -4;
    *addr = unpack_uint32_be(packet + i);
    i += 4;
    return i;
}

int makeDNSQuery_SRV(uint16_t id, const char* domainName, uint8_t* packet, 
    unsigned packetSize) {
    int i = makeDNSHeader(id, packet, packetSize);
    if (i < 0)
        return i;
    // Write the domain name
    int i2 = writeDomainName(domainName, packet + i, packetSize - i);
    if (i2 < 0)
        return i2;
    i += i2;
    // Space for the rest?
    if ((unsigned)(i + 4) >= packetSize)
        return -1;
    // QTYPE
    pack_uint16_be(0x0021, packet + i);
    i += 2;
    // QCLASS
    pack_uint16_be(0x0001, packet + i);
    i += 2;
    return i;
}

int makeDNSQuery_A(uint16_t id, const char* domainName, uint8_t* packet, 
    unsigned packetSize) {
    int i = makeDNSHeader(id, packet, packetSize);
    if (i < 0)
        return i;
    // Write the domain name
    int i2 = writeDomainName(domainName, packet + i, packetSize - i);
    if (i2 < 0)
        return i2;
    i += i2;
    // Space for the rest?
    if ((unsigned)(i + 4) >= packetSize)
        return -1;
    // QTYPE (A type)
    pack_uint16_be(0x0001, packet + i);
    i += 2;
    // QCLASS
    pack_uint16_be(0x0001, packet + i);
    i += 2;
    return i;
}

void unit_tests_1() {
    {
        const char* td0 = "a.bc.d";
        uint8_t packet[64];
        int rc = writeDomainName(td0, packet, 64);
        const unsigned expectedPacketLen = 8;
        uint8_t expectedPacket[expectedPacketLen] = 
            { 1, 'a', 2, 'b', 'c', 1, 'd', 0 };
        assert(rc == 8);
        assert(memcmp(expectedPacket, packet, 8) == 0);
    }
    {
        const unsigned expectedPacketLen = 8;
        uint8_t expectedPacket[expectedPacketLen] = 
            { 1, 'a', 2, 'b', 'c', 1, 'd', 0 };
        char name[32];
        int rc1 = parseDomainName(expectedPacket, expectedPacketLen,
            0, name, 32);
        assert(rc1 == 8);
        //prettyHexDump((const uint8_t*)name, 32, cout);
        assert(strcmp(name, "a.bc.d") == 0);
    }
    {
        const unsigned expectedPacketLen = 14;
        uint8_t expectedPacket[expectedPacketLen] = 
            { 0, 1, 'e', 0, 1, 'a', 2, 'b', 'c', 1, 'd', 0xc0, 1, 0 };

        char name[32];
        int rc1 = parseDomainName(expectedPacket, expectedPacketLen,
            4, name, 32);
        assert(rc1 == 13);
        //prettyHexDump((const uint8_t*)name, 32, cout);
        assert(strcmp(name, "a.bc.d.e") == 0);
        
        // Name intentionally too small
        char name2[6];
        int rc2 = parseDomainName(expectedPacket, expectedPacketLen,
            4, name2, 6);
        assert(rc2 == -2);
    }
    {
        // Make a packet with a pointer that is outside of the range 
        // of the packet.
        const unsigned packetLen = 13;
        uint8_t packet[packetLen] = 
            { 0, 1, 'e', 0, 1, 'a', 2, 'b', 'c', 1, 'd', 0xc0, 13 };
        char name[32];
        int rc = parseDomainName(packet, packetLen, 4, name, 32);
        assert(rc == -3);
    }
    {
        const unsigned packetLen = 8;
        uint8_t packet[packetLen] = 
            { 1, 'a', 2, 'b', 'c', 1, 'd', 0 };
        char name[32];
        int rc0 = parseDomainName(packet, packetLen, 0, name, 32);
        assert(rc0 == 8);
        //prettyHexDump((const uint8_t*)name, 32, cout);

        // Construct a case where the packet isn't long enough 
        rc0 = parseDomainName(packet, 6, 0, name, 32);
        assert(rc0 == -1);

        // Construct a case where the packet isn't long enough - specifically
        // running out of space before the trailing 0
        rc0 = parseDomainName(packet, 7, 0, name, 32);
        assert(rc0 == -1);
    }
}

// Response to an A record query - CAPTURED FROM A REAL DNS SERVER RESPONSE
static const uint8_t PACKET_0[] = {0xa8, 0xd6, 0x81, 0x80, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x05, 0x32, 0x39, 0x39, 0x39, 0x39, 0x05, 0x6e, 0x6f, 0x64, 0x65, 0x73, 0x0b, 0x61, 0x6c, 0x6c, 0x73, 0x74, 0x61, 0x72, 0x6c, 0x69, 0x6e, 0x6b, 0x03, 0x6f, 0x72, 0x67, 0x00, 0x00, 0x01, 0x00, 0x01, 0xc0, 0x0c, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x3c, 0x00, 0x04, 0xad, 0xc7, 0x77, 0xb1};

static void unit_tests_2() {
    int answerStart = skipQuestions(PACKET_0, sizeof(PACKET_0));
    uint32_t addr = 0;
    int rc1 = parseDNSAnswer_A(PACKET_0, sizeof(PACKET_0), answerStart, &addr);
    assert(rc1 > 0);
    char addrStr[32];
    formatIP4Address(addr, addrStr, 32);
    assert(strcmp(addrStr, "173.199.119.199") == 0);
}

int query_1() {

    const unsigned PACKET_SIZE = 128;
    uint8_t packet[PACKET_SIZE];
    //int len = makeDNSQuery_SRV(0xc930, DOMAIN_0, packet, PACKET_SIZE);
    int len = makeDNSQuery_A(0xa8d6, DOMAIN_1, packet, PACKET_SIZE);
    cout << len << endl;
    if (len < 0)
        return -1;
    prettyHexDump(packet, len, cout);

    // Make a UDP 
    int sockFd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockFd < 0) {
        return -1;
    }

    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    // Bind to all interfaces
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(0);
    if (::bind(sockFd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        cout << "Cant bing" << endl;
        ::close(sockFd);
        return -1;
    }
    // Make non-blocking
    int flags = ::fcntl(sockFd, F_GETFL, 0);
    if (flags == -1) {
        cout << "Non blocking error" << endl;
        ::close(sockFd);
        return -1;
    }
    if (fcntl(sockFd, F_SETFL, flags | O_NONBLOCK) == -1) {
        cout << "Non blocking error" << endl;
        ::close(sockFd);
        return -1;
    }

    // Send the query to a DNS server
    struct sockaddr_in dest_addr;
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(53);
    inet_pton(AF_INET, DNS_IP_ADDR, &dest_addr.sin_addr); 
    int rc = ::sendto(sockFd, packet, len, 0, 
        (struct sockaddr*)&dest_addr, sizeof(dest_addr));
    if (rc < 0) {
        cout << "Send error " << endl;
        return -1;
    }

    // RX loop

    cout << "receive loop" << endl;
    while (true) {
        // Check for input
        const unsigned readBufferSize = 512;
        uint8_t readBuffer[readBufferSize];
        struct sockaddr_in peerAddr;
        socklen_t peerAddrLen = sizeof(peerAddr);
        int rc = recvfrom(sockFd, readBuffer, readBufferSize, 0,
            (struct sockaddr *)&peerAddr, &peerAddrLen);
        if (rc > 0) {

            prettyHexDump(readBuffer, rc, cout);

            /*
            // Dump full packet for unit tests
            cout << "static const uint8_t PACKET[] = {";
            for (unsigned i = 0; i < (unsigned)rc; i++) {
                if (i > 0)
                    cout << ", ";
                char buf[32];
                snprintf(buf, 32, "0x%02x", (int)readBuffer[i]);
                cout << buf;
            }
            cout << "};" << endl;
            */

            // Skip all of the questions
            int answerStart = skipQuestions(readBuffer, rc);
            // Parse the answer
            /*
            uint16_t pri, weight, port;
            char srvHost[65];
            int rc1 = parseDNSAnswer_SRV(readBuffer, rc, answerStart,
                    &pri, &weight, &port, srvHost, 65);
            if (rc1 < 0)
                cout << "rc1 " << rc1 << endl;
            else {
                cout << "SRV: " << srvHost << ":" << port << endl;
            }
            */
            uint32_t addr;
            int rc1 = parseDNSAnswer_A(readBuffer, rc, answerStart, &addr);
            if (rc1 < 0)
                cout << "rc1 " << rc1 << endl;
            else {
                char addrStr[32];
                formatIP4Address(addr, addrStr, 32);
                cout << "A: " << addrStr << endl;
            }
        }
    }
}

int main(int, const char**) {
    unit_tests_1();
    unit_tests_2();
    query_1();
}
