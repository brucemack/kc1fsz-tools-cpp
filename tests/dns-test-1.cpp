#include <iostream>
#include <cstring>

#include "kc1fsz-tools/Common.h"

using namespace std;
using namespace kc1fsz;


int makeSRVQuery(uint16_t id, const char* domainName, uint8_t* packet, 
    unsigned packetSize) {
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
    int i = 12;

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
    if (i + 5 == packetSize)
        return -1;
    // Ending null
    packet[i++] = 0;
    // QTYPE
    pack_uint16_be(0x0021, packet + i);
    i += 2;
    // QCLASS
    pack_uint16_be(0x0001, packet + i);
    i += 2;
    return i;
}

int main(int, const char**) {

    const unsigned PACKET_SIZE = 128;
    uint8_t packet[PACKET_SIZE];
    int len = makeSRVQuery(0xc930, "_iax._udp.29999.nodes.allstarlink.org",
        packet, PACKET_SIZE);
    cout << len << endl;
    if (len < 0)
        return -1;
    prettyHexDump(packet, len, cout);
}
