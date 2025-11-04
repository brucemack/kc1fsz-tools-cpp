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
#ifndef _MicroDNS_h
#define _MicroDNS_h

#include <cstdint>

namespace kc1fsz {
    namespace microdns {

int makeDNSHeader(uint16_t id, uint8_t* packet, unsigned packetSize);

/**
 * @returns Number of bytes written.
 */
int writeDomainName(const char* domainName, uint8_t* packet, 
    unsigned packetSize);

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
    unsigned nameOffset, char* name, unsigned nameSize);

/**
 * @returns The offset into the packet immediately following 
 * the questions. 
 */
int skipQuestions(const uint8_t* packet, unsigned packetLen);

int parseDNSAnswer_SRV(const uint8_t* packet, unsigned packetLen, 
    unsigned answerOffset,
    uint16_t* pri, uint16_t* weight, uint16_t* port, 
    char* hostname, unsigned hostnameCapacity);

int parseDNSAnswer_A(const uint8_t* packet, unsigned packetLen, 
    unsigned answerOffset,
    uint32_t* addr);

int makeDNSQuery_SRV(uint16_t id, const char* domainName, uint8_t* packet, 
    unsigned packetSize);

int makeDNSQuery_A(uint16_t id, const char* domainName, uint8_t* packet, 
    unsigned packetSize);

    }
}

#endif

