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
#ifndef _CommContext_h
#define _CommContext_h

#include <cstdint>

#include "HostName.h"
#include "Channel.h"
#include "IPAddress.h"

namespace kc1fsz {

/**
 * An attempt to completely abstract socket communications.  This
 * is needed because some platforms (embedded) may not use 
 * a standard socket API.
 */
class CommContext {
public:

    virtual ~CommContext() { }

    // ------ Request Methods -------------------------------------------------

    /**
     * Clears all state and closes all channels.
    */
    virtual void reset() { }

    /**
     * Starts a DNS lookup.  The result will be delivered via a 
     * DNSLookupEvent.
     */
    virtual void startDNSLookup(HostName hostName) { }

    virtual Channel createTCPChannel() { return Channel(); }
    virtual void closeTCPChannel(Channel c) { }
    virtual void connectTCPChannel(Channel c, IPAddress ipAddr, uint32_t port) { }
    virtual void sendTCPChannel(Channel c, const uint8_t* b, uint16_t len) { }

    virtual Channel createUDPChannel() { return Channel(); }

    /**
     * In socket parlance, this performs the bind.
     * A remote address/port is also associated with the channel which will 
     * be used if no address is specifed on the send.
     */
    virtual void setupUDPChannel(Channel c, uint32_t localPort, 
        IPAddress remoteIpAddr, uint32_t remotePort) { }

    virtual void closeUDPChannel(Channel c) { }

    /**
     * This version assumes the address and port defined when the channel was 
     * first setup.
     */
    virtual void sendUDPChannel(Channel c, const uint8_t* b, uint16_t len) { }

    /**
     * This version uses an explicit target address/port.
     */
    virtual void sendUDPChannel(Channel c, IPAddress remoteIpAddr, uint32_t remotePort,
        const uint8_t* b, uint16_t len) { }
};

}

#endif
