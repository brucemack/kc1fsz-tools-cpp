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
#include "kc1fsz-tools/MicroDNS.h"

using namespace std;
using namespace kc1fsz;
using namespace kc1fsz::microdns;

static const char* DNS_IP_ADDR = "208.67.222.222";
static const char* DOMAIN_0 = "_iax._udp.29999.nodes.allstarlink.org";
static const char* DOMAIN_1 = "29999.nodes.allstarlink.org";

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
