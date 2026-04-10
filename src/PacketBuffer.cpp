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

#define HL ((unsigned)sizeof(Header))

namespace kc1fsz {

void PacketBuffer::clear() {
    _spaceUsed = 0;
}

bool PacketBuffer::push(uint32_t stamp, const uint8_t* packet, unsigned len) {
    if (_spaceUsed + HL + len > _spaceCapacity)
        return false;
    Header hdr = { .len = HL + len, .stamp = stamp };
    memcpy(_space + _spaceUsed, &hdr, HL);
    _spaceUsed += HL;
    memcpy(_space + _spaceUsed, packet, len);
    _spaceUsed += len;
    return true;
}

bool PacketBuffer::push(uint32_t stamp, const uint8_t* packet0, unsigned len0, 
    const uint8_t* packet1, unsigned len1) {
    // Make sure the new packet can fit
    if (_spaceUsed + HL + len0 + len1 > _spaceCapacity)
        return false;
    Header hdr = { .len = HL + len0 + len1, .stamp = stamp };
    memcpy(_space + _spaceUsed, &hdr, HL);
    _spaceUsed += HL;
    if (len0) {
        memcpy(_space + _spaceUsed, packet0, len0);
        _spaceUsed += len0;
    }
    if (len1) {
        memcpy(_space + _spaceUsed, packet1, len1);
        _spaceUsed += len1;
    }
    return true;
}

// #### CONSOLIDATE THESE THREE METHODS

bool PacketBuffer::push(uint32_t stamp, const uint8_t* packet0, unsigned len0, 
    const uint8_t* packet1, unsigned len1, const uint8_t* packet2, unsigned len2) {
    // Make sure the new packet can fit
    if (_spaceUsed + HL + len0 + len1 + len2 > _spaceCapacity)
        return false;
    Header hdr = { .len = HL + len0 + len1 + len2, .stamp = stamp };
    memcpy(_space + _spaceUsed, &hdr, HL);
    _spaceUsed += HL;
    if (len0) {
        memcpy(_space + _spaceUsed, packet0, len0);
        _spaceUsed += len0;
    }
    if (len1) {
        memcpy(_space + _spaceUsed, packet1, len1);
        _spaceUsed += len1;
    }
    if (len2) {
        memcpy(_space + _spaceUsed, packet2, len2);
        _spaceUsed += len2;
    }
    return true;
}

bool PacketBuffer::tryPop(uint32_t* stamp, uint8_t* packet, unsigned* packetLen) {
    return _tryPeekPop(stamp, packet, packetLen, true);
}

bool PacketBuffer::tryPeek(uint32_t* stamp, uint8_t* packet, unsigned* packetLen) {
    return _tryPeekPop(stamp, packet, packetLen, false);
}

bool PacketBuffer::_tryPeekPop(uint32_t* stamp, uint8_t* packet, unsigned* packetLen, bool pop) {
    if (_spaceUsed == 0)
        return false;
    const unsigned len = ((Header*)_space)->len;
    const uint32_t packetStamp = ((Header*)_space)->stamp;
    // Buffer is truncated if it is too long to it in the space
    unsigned copyLen = std::min(len - HL, *packetLen);
    // Give the packet to the caller
    memcpy(packet, _space + HL, copyLen);
    *packetLen = copyLen;
    if (stamp)
        *stamp = packetStamp;
    if (pop) {
        // Shift left (overlapping)
        if (_spaceUsed > len)
            memmove(_space, _space + len, _spaceUsed - len);
        _spaceUsed -= len;
    }
    return true;
}

void PacketBuffer::pop() {
    if (_spaceUsed == 0)
        return;
    const unsigned len = ((Header*)_space)->len;
    // Shift left (overlapping)
    if (_spaceUsed > len)
        memmove(_space, _space + len, _spaceUsed - len);
    _spaceUsed -= len;
}

void PacketBuffer::visitAll(std::function<void(uint32_t stamp, const uint8_t* packet, unsigned len)> cb) {
    unsigned i = 0;
    while (i < _spaceUsed) {
        const unsigned len = ((Header*)(_space + i))->len;
        const uint32_t packetStamp = ((Header*)(_space + i))->stamp;
        cb(packetStamp, _space + i + HL, len - HL);
        i += len;
    }
}

void PacketBuffer::removeFirstIf(std::function<bool(uint32_t stamp, const uint8_t* packet, unsigned len)> cb) {
    removeIf(cb, true);
}

void PacketBuffer::removeIf(std::function<bool(uint32_t stamp, const uint8_t* packet, unsigned len)> cb,
    bool firstOnly) {    
    unsigned i = 0;
    while (i < _spaceUsed) {
        const unsigned len = ((Header*)(_space + i))->len;
        const uint32_t packetStamp = ((Header*)(_space + i))->stamp;
        // Call the predicate to decide if we need to remove
        if (cb(packetStamp, _space + i + HL, len - HL)) {
            // Shift left (overlapping)
            if (_spaceUsed > i + len)
                memmove(_space + i, _space + i + len, _spaceUsed - i - len);
            _spaceUsed -= len;
            if (firstOnly)
                return;
            // Here i stays in the same place (no forward progress made)
        }
        else {
            // Move right to the next packet
            i += len;
        }
    }
}

}
