/**
 * MicroLink EchoLink Station
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
 * FOR AMATEUR RADIO USE ONLY.
 * NOT FOR COMMERCIAL USE WITHOUT PERMISSION.
 */
#ifndef _IPLib_h
#define _IPLib_h

#include "kc1fsz-tools/Channel.h"
#include "kc1fsz-tools/HostName.h"
#include "kc1fsz-tools/IPAddress.h"

namespace kc1fsz {

class IPLibEvents {
public:

    virtual void dns(HostName name, IPAddress addr) = 0;
    virtual void bind(Channel ch) = 0;
    virtual void conn(Channel ch) = 0;
    virtual void disc(Channel ch) = 0;
    virtual void recv(Channel ch, const uint8_t* data, uint32_t dataLen, IPAddress fromAddr,
        uint16_t fromPort) = 0;
    virtual void err(Channel ch, int type) = 0;
};

/**
 * An abstraction of a TCP/IP capability. Used to avoid building specific 
 * socket libraries or AT command sets into an application.
*/
class IPLib {
public:

    virtual bool isLinkUp() const = 0;

    /**
     * This can be called more than once to register multiple listeners.
     */
    virtual void addEventSink(IPLibEvents* e) = 0;

    virtual void queryDNS(HostName hostName) = 0;

    virtual Channel createTCPChannel() = 0;
    virtual void connectTCPChannel(Channel c, IPAddress addr, uint32_t port) = 0;
    virtual void sendTCPChannel(Channel c, const uint8_t* b, uint16_t len) = 0;

    virtual Channel createUDPChannel() = 0;
    virtual void bindUDPChannel(Channel c, uint32_t localPort) = 0;
    virtual void sendUDPChannel(const Channel& c, 
        const IPAddress& remoteAddr, uint32_t remotePort,
        const uint8_t* b, uint16_t len) = 0;

    virtual void closeChannel(Channel c) = 0;
};

}

#endif
