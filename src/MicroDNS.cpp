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
 *
 * NOT FOR COMMERCIAL USE WITHOUT PERMISSION.
 */
#include <iostream>
#include <cstring>
#include <cassert>

#include "kc1fsz-tools/Common.h"

using namespace std;

namespace kc1fsz {
namespace microdns {

int makeDNSHeader(uint16_t id, uint8_t* packet, unsigned packetSize) {
    if (packetSize < 12)
        return -1;
    // Make the header
    // This is the unique request ID
    pack_uint16_be(id, packet);
    uint16_t work;
    // QR=0b0 (query), OPCODE=0b0000, AA=x, TC=0b0, RD=0b1 (recursion desired)
    // RA=0bx (recursion available), Z=0b000, RCODE=0b0000 
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

/*
Got this at Tatte - possibly an SOA record (type 0x0006)

ID=0x0001
QR=1, AA=0, RD=1, RA=1, RCODE=0000

0000 | 00 01 81 80 00 01 00 00  00 01 00 00 04 5f 69 61  ········ ·····_ia
0010 | 78 04 5f 75 64 70 05 35  35 35 35 33 05 6e 6f 64  x·_udp·5 5553·nod
0020 | 65 73 0b 61 6c 6c 73 74  61 72 6c 69 6e 6b 03 6f  es·allst arlink·o
0030 | 72 67 00 [00 21] [00 01] [c0  1c] [00 06] 00 01 00 00 00  rg··!··· ········
0040 | 3c 00 48 06 6e 73 2d 38  36 38 09 61 77 73 64 6e  <·H·ns-8 68·awsdn
0050 | 73 2d 34 34 03 6e 65 74  00 11 61 77 73 64 6e 73  s-44·net ··awsdns
0060 | 2d 68 6f 73 74 6d 61 73  74 65 72 06 61 6d 61 7a  -hostmas ter·amaz
0070 | 6f 6e 03 63 6f 6d 00 00  00 00 01 00 00 1c 20 00  on·com·· ········
0080 | 00 03 84 00 12 75 00 00  01 51 80                 ·····u·· ·Q·
*/

int parseDNSAnswer_SRV(const uint8_t* packet, unsigned packetLen, 
    uint16_t* pri, uint16_t* weight, uint16_t* port, 
    char* hostname, unsigned hostnameCapacity) {

    if (packetLen < 4)
        return -1;

    //uint16_t id = unpack_uint16_be(packet);
    uint16_t flags = unpack_uint16_be(packet + 2);
    // Do some checking on the flags
    // QR=1
    if ((flags & 0x8000) == 0)
        return -8;
    // RCODE=0000
    if ((flags & 0x000f) != 0)
        return -9;

    // Skip past the header and all questions 
    int rc0 = skipQuestions(packet, packetLen);
    if (rc0 < 0) {
        return rc0;
    }
    unsigned i = rc0;
    // We just skip the NAME by passing 0s
    char temp[64];
    int rc = parseDomainName(packet, packetLen, i, temp, 64);
    if (rc < 0) {
        return rc;
    }
    i = rc;
    // TYPE
    if (i + 2 > packetLen) {
        return -1;
    }
    uint16_t typeCode = unpack_uint16_be(packet + i);
    i += 2;
    if (typeCode != 0x0021) {
        return -3;
    }
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
    uint32_t* addr) {
    // Skip past the header and all questions 
    int rc0 = skipQuestions(packet, packetLen);
    if (rc0 < 0)
        return rc0;
    unsigned i = rc0;
    // We just skip paste the NAME
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

int parseDNSAnswer_TXT(const uint8_t* packet, unsigned packetLen,  char* txt, unsigned txtCapacity) {
    // Skip past the header and all questions 
    int rc0 = skipQuestions(packet, packetLen);
    if (rc0 < 0)
        return rc0;
    unsigned i = rc0;
    // We just skip paste the NAME
    int rc = parseDomainName(packet, packetLen, i, 0, 0);
    if (rc < 0)
        return rc;
    i = rc;
    // TYPE
    if (i + 2 > packetLen)
        return -1;
    uint16_t typeCode = unpack_uint16_be(packet + i);
    i += 2;
    if (typeCode != 0x0010)
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
    i += 4;
    // RDLENGTH
    if (i + 2 > packetLen)
        return -1;
    uint16_t rdLength = unpack_uint16_be(packet + i);
    i += 2;
    // Sanity check, is the packet large enough to contain the TXT payload
    if (i + rdLength > packetLen)
        return -5;
    // Get the text length
    uint16_t txtLength = packet[i];
    i += 1;
    // Make sure we can handle the complete contents of the TXT payload, including the 
    // null termination
    if (txtLength + 1 > txtCapacity)
        return -6;
    strncpy(txt, (const char*)(packet + i), txtLength);
    // Null terminate
    txt[txtLength] = 0;
    i += txtLength;
    return i;
}

static int makeDNSQuery(uint16_t id, uint16_t qtype, const char* domainName, 
    uint8_t* packet, unsigned packetSize) {
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
    pack_uint16_be(qtype, packet + i);
    i += 2;
    // QCLASS
    pack_uint16_be(0x0001, packet + i);
    i += 2;
    return i;
}

int makeDNSQuery_SRV(uint16_t id, const char* domainName, uint8_t* packet, 
    unsigned packetSize) {
    return makeDNSQuery(id, 0x0021, domainName, packet, packetSize);
}

int makeDNSQuery_A(uint16_t id, const char* domainName, uint8_t* packet, 
    unsigned packetSize) {
    return makeDNSQuery(id, 0x0001, domainName, packet, packetSize);
}

int makeDNSQuery_TXT(uint16_t id, const char* domainName, uint8_t* packet, 
    unsigned packetSize) {
    return makeDNSQuery(id, 0x0010, domainName, packet, packetSize);
}

}
}
