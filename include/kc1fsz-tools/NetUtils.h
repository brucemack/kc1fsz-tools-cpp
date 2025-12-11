/**
 * Copyright (C) 2025, Bruce MacKinnon KC1FSZ
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

#ifdef _WIN32
#include <winsock2.h>
#else
#include <sys/socket.h>
#endif

namespace kc1fsz {

/**
 * Puts the address into a string in decimal-dotted format.
 *
 * @param addr IP4 address expressed as a 32-bit integer.  The
 * assumption is that the endian thing has been sorted out 
 * before this point and the most-significant bits of the integer
 * are the left-most parts of the dotted address display.
 */
void formatIP4Address(uint32_t addr, char* dottedAddr, uint32_t dottedAddrSize);

/**
 * Converts the dotted-decimal IP address into a 32-bit integer.
 * @param len If non-zero, only this many characters are processed. 
 *   Otherwise, go to the null.
 */
uint32_t parseIP4Address(const char* dottedAddr, uint32_t len = 0);

/**
 * @returns AF_INET or AF_INET6 based on the format of the address provided.
 */
short getIPAddrFamily(const char* addr);

/**
 * Converts an IP address to a string. Works for IPv4 and IPv6.
 */
void formatIPAddr(const sockaddr& addr, char* str, unsigned len);

/**
 * @return The size in bytes of the address. Works for IPv4 and IPv6.
 */
unsigned getIPAddrSize(const sockaddr& addr);

/**
 * Converts an IP address and port combination to a string following the 
 * usual conventions. Works for IPv4 and IPv6:
 * 
 * For IPv4: 10.10.10.10:80
 * For IPv6: [fe80::987e:49de:e95d:90d]:80
 */
void formatIPAddrAndPort(const sockaddr& addr, char* str, unsigned len);

/**
 * Parses an IP address and port.  Follows the ususal convetions:
 * 
 * For IPv4: 10.10.10.10:80
 * For IPv6: [fe80::987e:49de:e95d:90d]:80
 * 
 * @returns 0 On success, -1 when address is too long, -2 when port 
 * is too long.
 */
int parseIPAddrAndPort(const char* addrAndPort, sockaddr_storage& addr);

/**
 * @returns true if the addresses are the same (ignores port)
 */
bool equalIPAddr(const sockaddr& a0, const sockaddr& a1);

/**
 * Converts an IP address in string format. Works for IPv4 and IPv6.
 */
void setIPAddr(sockaddr_storage& addr, const char* strAddr);

/**
 * Sets the port part of an address. Works for IPv4 and IPv6.
 * @param port port number in host format
 */
void setIPPort(sockaddr_storage& addr, int port);

/**
 * Gets the port part of an address. Works for IPv4 and IPv6.
 */
int getIPPort(const sockaddr& addr);

/**
 * @returns 0 on success, -1 on failure
 */
int makeNonBlocking(int sockFd);

}
