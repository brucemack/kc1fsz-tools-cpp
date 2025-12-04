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

#include <sys/socket.h>

namespace kc1fsz {

/**
 * @returns AF_INET or AF_INET6 based on the format of the address provided.
 */
short getIPAddrFamily(const char* addr);

/**
 * Converts an IP address to a string. Works for IPv4 and IPv6.
 */
void formatIPAddr(const sockaddr& addr, char* ipStr, unsigned len);

/**
 * Converts an IP address in string format. Works for IPv4 and IPv6.
 */
void setIPAddr(sockaddr& addr, const char* strAddr);

/**
 * Sets the port part of an address. Works for IPv4 and IPv6.
 * @param port port number in host format
 */
void setIPPort(sockaddr& addr, int port);

/**
 * @returns true if the addresses are the same (ignores port)
 */
bool equalIPAddr(const sockaddr& a0, const sockaddr& a1);

}
