#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <iostream>
#include <cstring>

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
    int i = 0;
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
    if (i + 4 >= packetSize)
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
    if (i + 4 >= packetSize)
        return -1;
    // QTYPE (A type)
    pack_uint16_be(0x0001, packet + i);
    i += 2;
    // QCLASS
    pack_uint16_be(0x0001, packet + i);
    i += 2;
    return i;
}

int main(int, const char**) {

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
        }
    }
}
