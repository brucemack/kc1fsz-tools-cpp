/**
 * Copyright (C) 2026, Bruce MacKinnon KC1FSZ
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
#include <cstring> 
#include <cassert>

#include "kc1fsz-tools/Common.h"
#include "kc1fsz-tools/PacketBuffer.h"

namespace kc1fsz {

void PacketBuffer::clear() {
    _spaceUsed = 0;
}

bool PacketBuffer::push(const uint8_t* packet, unsigned len) {
    if (_spaceUsed + len > _spaceCapacity)
        return false;
    if (_lenOffset >= len)
        return false;
    // Make sure there is no inconsistency
    uint16_t embeddedLen = unpack_uint16_be(packet + _lenOffset);
    if (embeddedLen != len)
        return false;
    // Pull onto end of space
    memcpy(_space + _spaceUsed, packet, len);
    _spaceUsed += len;
    return true;
}

bool PacketBuffer::tryPop(uint8_t* packet, unsigned* packetLen) {
    if (_spaceUsed == 0)
        return false;
    // Get length of first packet
    uint16_t len = unpack_uint16_be(_space + _lenOffset);
    // Give the packet to the caller
    memcpy(packet, _space, len);
    *packetLen = len;
    // Shift left (overlapping)
    if (_spaceUsed > len)
        memmove(_space, _space + len, _spaceUsed - len);
    _spaceUsed -= len;
    return true;
}

void PacketBuffer::visitAll(std::function<void(const uint8_t* packet, unsigned len)> cb) {
    unsigned i = 0;
    while (i < _spaceUsed) {
        uint16_t len = unpack_uint16_be(_space + i + _lenOffset);
        cb(_space + i, len);
        i += len;
    }
}

void PacketBuffer::removeFirstIf(std::function<bool(const uint8_t* packet, unsigned len)> cb) {
    removeIf(cb, true);
}

void PacketBuffer::removeIf(std::function<bool(const uint8_t* packet, unsigned len)> cb,
    bool firstOnly) {    
    unsigned i = 0;
    while (i < _spaceUsed) {
        const uint16_t embeddedLen = unpack_uint16_be(_space + i + _lenOffset);
        assert(i + embeddedLen <= _spaceUsed);
        // Call the predicate to decide if we need to remove
        if (cb(_space + i, embeddedLen)) {
            // Shift left (overlapping)
            if (_spaceUsed > i + embeddedLen)
                memmove(_space + i, _space + i + embeddedLen, _spaceUsed - i - embeddedLen);
            _spaceUsed -= embeddedLen;
            if (firstOnly)
                return;
            // Here i stays in the same place (no forward progress made)
        }
        else {
            // Move right to the next packet
            i += embeddedLen;
        }
    }
}

}
