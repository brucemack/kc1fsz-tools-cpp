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
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <netinet/in.h>
#include <arpa/inet.h>
#endif 

#include <fcntl.h>

#include <cassert>
#include <cstring>
#include <cstdio>
#include <iostream>

#include "kc1fsz-tools/NetUtils.h"

using namespace std;

namespace kc1fsz {


void formatIP4Address(uint32_t addr, char* dottedAddr, uint32_t dottedAddrSize) {
    // This is the high-order part of the address.
    uint32_t a = (addr & 0xff000000) >> 24;
    uint32_t b = (addr & 0x00ff0000) >> 16;
    uint32_t c = (addr & 0x0000ff00) >> 8;
    uint32_t d = (addr & 0x000000ff);
    // BRM 2025-02-01 changed from %lu to resolve warning
    snprintf(dottedAddr, dottedAddrSize, "%u.%u.%u.%u", a, b, c, d);
}

uint32_t parseIP4Address(const char* dottedAddr, uint32_t len) {
    uint32_t result = 0;
    char acc[8];
    uint32_t accLen = 0;
    const char *p = dottedAddr;
    uint32_t octets = 4;

    // If a limit has been specifed, calculate the stopping point
    const char* lastChar = 0;
    if (len != 0) {
        lastChar = dottedAddr + len - 1;
    }

    while (true) {
        if (*p == '.' || *p == 0 || p == lastChar) {
            acc[accLen] = 0;
            // Shift up
            result <<= 8;
            // Accumulate LSB
            result |= (uint8_t)atoi(acc);
            accLen = 0;
            // Count octets
            octets++;
            // Done yet?
            if (octets == 4 || *p == 0 || p == lastChar) {
                break;
            }
        }
        else {
            acc[accLen++] = *p;
        }
        p++;
    }
#ifdef PICO_BUILD
    return result;
#else
    return htonl(result);
#endif
}

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

int parseIPAddrAndPort(const char* ap, sockaddr_storage& addr) {

    int state = 0;
    char addrStr[64];
    addrStr[0] = 0;
    char portStr[8];
    portStr[0] = 0;
    unsigned p = 0;

    if (ap[0] == '[') {
        // This is assumed to be an IPv6 address
        for (unsigned i = 0; i < strlen(ap); i++) {
            // Hunting for [
            if (state == 0) {
                if (ap[i] == '[') {
                    state = 1;
                    p = 0;
                }
            } 
            // In address
            else if (state == 1) {
                if (ap[i] == ']') {
                    state = 2;
                } else if (p < 63) {
                    addrStr[p++] = ap[i];
                    addrStr[p] = 0;
                }
                else {
                    return -1;
                }
            }
            else if (state == 2) {
                if (ap[i] == ':') {
                    state = 3;
                    p = 0;
                }
                else {
                    return -3;
                }
            }
            // In port
            else if (state == 3) {
                if (p < 7) {
                    portStr[p++] = ap[i];
                    portStr[p] = 0;
                }
                else {
                    return -2;
                }
            }
        }
        addr.ss_family = AF_INET6;
        setIPAddr(addr, addrStr);
        setIPPort(addr, atoi(portStr));
    }
    else {
        // This is assumed to be an IPv4 address
        for (unsigned i = 0; i < strlen(ap); i++) {
            // In address
            if (state == 0) {
                if (ap[i] == ':') {
                    state = 1;
                    p = 0;
                } else if (p < 63) {
                    addrStr[p++] = ap[i];
                    addrStr[p] = 0;
                } else {
                    return -1;
                }
            }
            // In port
            else if (state == 1) {
                if (p < 7) {
                    portStr[p++] = ap[i];
                    portStr[p] = 0;
                } else {
                    return -2;
                }
            }
        }
        addr.ss_family = AF_INET;
        setIPAddr(addr, addrStr);
        setIPPort(addr, atoi(portStr));
    }

    return 0;
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

int getIPPort(const sockaddr& addr) {
    if (addr.sa_family == AF_INET)
        return ntohs(((const sockaddr_in&)addr).sin_port);
    else if (addr.sa_family == AF_INET6)
        return ntohs(((const sockaddr_in6&)addr).sin6_port);
    else
        assert(false);
}

int makeNonBlocking(int sockFd) {
#ifdef _WIN32
    u_long mode = 1; 
    int result = ioctlsocket(sockFd, FIONBIO, &mode);
    if (result != NO_ERROR) {
        return -1;
    }
#else
    int flags = fcntl(sockFd, F_GETFL, 0);
    if (flags == -1) {
        return -1;
    }
    if (fcntl(sockFd, F_SETFL, flags | O_NONBLOCK) == -1) {
        return -1;
    }
#endif
    return 0;
}


}
