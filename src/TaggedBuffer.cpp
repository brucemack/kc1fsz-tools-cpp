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
#include "kc1fsz-tools/TaggedBuffer.h"

#define HL ((unsigned)sizeof(Header))

namespace kc1fsz {

void TaggedBuffer::clear() {
    _spaceUsed = 0;
}

bool TaggedBuffer::push(uint32_t stamp, unsigned id, const uint8_t* packet, unsigned len) {
    return push(stamp, id, packet, len, 0, 0, 0, 0);
}

bool TaggedBuffer::push(uint32_t stamp, unsigned id, const uint8_t* packet0, unsigned len0, 
    const uint8_t* packet1, unsigned len1) {
    return push(stamp, id, packet0, len0, packet1, len1, 0, 0);
}

bool TaggedBuffer::push(uint32_t stamp, unsigned id, const uint8_t* packet0, unsigned len0, 
    const uint8_t* packet1, unsigned len1, const uint8_t* packet2, unsigned len2) {
    // Make sure the new packet can fit
    if (_spaceUsed + HL + len0 + len1 + len2 > _spaceCapacity)
        return false;
    Header hdr = { .len = HL + len0 + len1 + len2, .stamp = stamp, .id = id };
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

bool TaggedBuffer::tryPop(uint32_t* stamp, unsigned* id, uint8_t* packet, unsigned* packetLen) {
    return _tryPeekPop(stamp, id, packet, packetLen, true);
}

bool TaggedBuffer::tryPeek(uint32_t* stamp, unsigned* id, uint8_t* packet, unsigned* packetLen) {
    return _tryPeekPop(stamp, id, packet, packetLen, false);
}

bool TaggedBuffer::_tryPeekPop(uint32_t* stamp, unsigned* id, uint8_t* packet, unsigned* packetLen, bool pop) {
    if (_spaceUsed == 0)
        return false;
    const unsigned len = ((Header*)_space)->len;
    const uint32_t packetStamp = ((Header*)_space)->stamp;
    const unsigned packetId = ((Header*)_space)->id;
    // Buffer is truncated if it is too long to it in the space
    unsigned copyLen = std::min(len - HL, *packetLen);
    // Give the packet to the caller
    memcpy(packet, _space + HL, copyLen);
    *packetLen = copyLen;
    if (stamp)
        *stamp = packetStamp;
    if (id)
        *id = packetId;
    if (pop) {
        // Shift left (overlapping)
        if (_spaceUsed > len)
            memmove(_space, _space + len, _spaceUsed - len);
        _spaceUsed -= len;
    }
    return true;
}

void TaggedBuffer::pop() {
    if (_spaceUsed == 0)
        return;
    const unsigned len = ((Header*)_space)->len;
    // Shift left (overlapping)
    if (_spaceUsed > len)
        memmove(_space, _space + len, _spaceUsed - len);
    _spaceUsed -= len;
}

void TaggedBuffer::visitAll(visitCb cb) const {
    unsigned i = 0;
    while (i < _spaceUsed) {
        const unsigned len = ((Header*)(_space + i))->len;
        const uint32_t packetStamp = ((Header*)(_space + i))->stamp;
        const uint32_t packetId = ((Header*)(_space + i))->id;
        cb(packetStamp, packetId, _space + i + HL, len - HL);
        i += len;
    }
}

void TaggedBuffer::removeFirstIf(predCb cb) {
    removeIf(cb, true);
}

void TaggedBuffer::removeIf(predCb cb, bool firstOnly) {    
    unsigned i = 0;
    while (i < _spaceUsed) {
        const unsigned len = ((Header*)(_space + i))->len;
        const uint32_t packetStamp = ((Header*)(_space + i))->stamp;
        const unsigned packetId = ((Header*)(_space + i))->id;
        // Call the predicate to decide if we need to remove
        if (cb(packetStamp, packetId, _space + i + HL, len - HL)) {
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
