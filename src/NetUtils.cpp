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
#include <netinet/in.h>
#include <arpa/inet.h>

#include <cassert>
#include <cstring>
#include <cstdio>
#include <iostream>

#include "kc1fsz-tools/NetUtils.h"

using namespace std;

namespace kc1fsz {

short getIPAddrFamily(const char* addr) {
    if (strchr(addr, '.') != 0)
        return AF_INET;
    else if (strchr(addr, ':') != 0)
        return AF_INET6;
    else 
        return 0; 
}

void formatIPAddr(const sockaddr& addr, char* ipStr, unsigned len) {
    if (addr.sa_family == AF_INET)
        inet_ntop(addr.sa_family, &((sockaddr_in*)&addr)->sin_addr, ipStr, 32);
    else if (addr.sa_family == AF_INET6)
        inet_ntop(addr.sa_family, &((sockaddr_in6*)&addr)->sin6_addr, ipStr, 32);
    else
        assert(false);
}

void formatIPAddrAndPort(const sockaddr& addr, char* str, unsigned len) {
    const unsigned maxLen = 65;
    char addrArea[maxLen];
    if (addr.sa_family == AF_INET) {
        inet_ntop(addr.sa_family, &((sockaddr_in*)&addr)->sin_addr, addrArea, maxLen);
        snprintf(str, len, "%s:%d", addrArea, 
            (int)ntohs(((sockaddr_in&)addr).sin_port));
    } else if (addr.sa_family == AF_INET6) {
        inet_ntop(addr.sa_family, &((sockaddr_in6&)addr).sin6_addr, addrArea, maxLen);
        snprintf(str, len, "[%s]:%d", addrArea, 
            (int)ntohs(((sockaddr_in6&)addr).sin6_port));            
    } else
        assert(false);
}

unsigned getIPAddrSize(const sockaddr& addr) {
    if (addr.sa_family == AF_INET) {
        return sizeof(sockaddr_in);
    } else if (addr.sa_family == AF_INET6) {
        return sizeof(sockaddr_in6);
    } else
        assert(false);
}

bool equalIPAddr(const sockaddr& a0, const sockaddr& a1) {
    if (a0.sa_family != a1.sa_family)
        return false;
    if (a0.sa_family == AF_INET)
        // These address are unsigned long
        return ((sockaddr_in*)&a0)->sin_addr.s_addr == 
               ((sockaddr_in*)&a1)->sin_addr.s_addr;
    else if (a0.sa_family == AF_INET6)
        // These addresses are unsigned char[16]
        return strcmp(
            (const char*)((sockaddr_in6*)&a0)->sin6_addr.s6_addr, 
            (const char*)((sockaddr_in6*)&a1)->sin6_addr.s6_addr
            ) == 0;
    else
        assert(false);
}

void setIPAddr(sockaddr_storage& addr, const char* strAddr) {
    if (addr.ss_family == AF_INET)
        inet_pton(addr.ss_family, strAddr, &(((sockaddr_in&)addr).sin_addr));
    else if (addr.ss_family == AF_INET6) {
        inet_pton(addr.ss_family, strAddr, &(((sockaddr_in6&)addr).sin6_addr));
    }
    else
        assert(false);
}

void setIPPort(sockaddr_storage& addr, int port) {
    if (addr.ss_family == AF_INET)
        ((sockaddr_in&)addr).sin_port = htons(port);
    else if (addr.ss_family == AF_INET6)
        ((sockaddr_in6&)addr).sin6_port = htons(port);
    else
        assert(false);
}


}
