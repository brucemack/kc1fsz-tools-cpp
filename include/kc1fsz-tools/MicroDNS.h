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

#include <cstdint>

/**
 * Some simple utility functions for creating/parsing DNS
 * request/response packets. There is no networking code
 * here, that is assumed to be handled elsewhere.
 * 
 */
namespace kc1fsz {

    namespace microdns {

/**
 * @returns Negative number on error, or the number of bytes
 * written into the packet.
 */
int makeDNSHeader(uint16_t id, uint8_t* packet, unsigned packetCapacity);

/**
 * Writes a domain name into the packet provided, per the DNS
 * specification.
 * 
 * @returns Number of bytes written.
 */
int writeDomainName(const char* domainName, uint8_t* buf, 
    unsigned bufCapacity);

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
    unsigned nameOffset, char* name, unsigned nameCapacity);

/**
 * @returns The offset into the packet immediately following 
 * the questions. 
 */
int skipQuestions(const uint8_t* packet, unsigned packetLen);

/**
 * Parses key data elements out of a response to an SRV query.
 */
int parseDNSAnswer_SRV(const uint8_t* packet, unsigned packetLen, 
    uint16_t* pri, uint16_t* weight, uint16_t* port, 
    char* hostname, unsigned hostnameCapacity);

/**
 * Parses key data elements out of a response to an A query.
 */
int parseDNSAnswer_A(const uint8_t* packet, unsigned packetLen, 
    uint32_t* addr);
/**
 * Parses key data elements out of a response to a TXT query.
 */
int parseDNSAnswer_TXT(const uint8_t* packet, unsigned packetLen, 
    char* txt, unsigned txtCapacity);

int makeDNSQuery_SRV(uint16_t id, const char* domainName, uint8_t* packet, 
    unsigned packetSize);

int makeDNSQuery_A(uint16_t id, const char* domainName, uint8_t* packet, 
    unsigned packetSize);

int makeDNSQuery_TXT(uint16_t id, const char* domainName, uint8_t* packet, 
    unsigned packetSize);

    }
}

