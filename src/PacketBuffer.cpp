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

bool PacketBuffer::push(uint32_t stamp, const uint8_t* packet, unsigned len) {
    // Make sure we have room
    if (_spaceUsed + sizeof(Header) + len > _spaceCapacity)
        return false;
    Header hdr = { .len = (uint32_t)sizeof(Header) + len, .stamp = stamp };
    memcpy(_space + _spaceUsed, &hdr, sizeof(Header));
    _spaceUsed += sizeof(Header);
    memcpy(_space + _spaceUsed, packet, len);
    _spaceUsed += len;
    return true;
}

bool PacketBuffer::push(uint32_t stamp, const uint8_t* packet0, unsigned len0, 
    const uint8_t* packet1, unsigned len1) {
    // Make sure the new packet can fit
    if (_spaceUsed + sizeof(Header) + len0 + len1 > _spaceCapacity)
        return false;
    Header hdr = { .len = (uint32_t)sizeof(Header) + len0 + len1, .stamp = stamp };
    memcpy(_space + _spaceUsed, &hdr, sizeof(Header));
    _spaceUsed += sizeof(Header);
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

bool PacketBuffer::tryPop(uint32_t* stamp, uint8_t* packet, unsigned* packetLen) {
    if (_spaceUsed == 0)
        return false;
    const Header* hdr = (const Header*)_space;
    // Buffer is truncated if it is too long to it in the space
    unsigned copyLen = std::min(hdr->len - (unsigned)sizeof(Header), *packetLen);
    // Give the packet to the caller
    memcpy(packet, _space + sizeof(Header), copyLen);
    *stamp = hdr->stamp;
    *packetLen = copyLen;
    // Shift left (overlapping)
    if (_spaceUsed > hdr->len)
        memmove(_space, _space + hdr->len, _spaceUsed - hdr->len);
    _spaceUsed -= hdr->len;
    return true;
}

bool PacketBuffer::tryPeek(uint32_t* stamp, uint8_t* packet, unsigned* packetLen) {
    if (_spaceUsed == 0)
        return false;
    const Header* hdr = (const Header*)_space;
    // Buffer is truncated if it is too long to it in the space
    unsigned copyLen = std::min(hdr->len - (unsigned)sizeof(Header), *packetLen);
    // Show the packet to the caller
    memcpy(packet, _space + sizeof(Header), copyLen);
    *stamp = hdr->stamp;
    *packetLen = copyLen;
    return true;
}

void PacketBuffer::pop() {
    if (_spaceUsed == 0)
        return;
    const Header* hdr = (const Header*)_space;
    // Shift left (overlapping)
    if (_spaceUsed > hdr->len)
        memmove(_space, _space + hdr->len, _spaceUsed - hdr->len);
    _spaceUsed -= hdr->len;
}

void PacketBuffer::visitAll(std::function<void(uint32_t stamp, const uint8_t* packet, unsigned len)> cb) {
    unsigned i = 0;
    while (i < _spaceUsed) {
        const Header* hdr = (const Header*)_space + i;
        cb(hdr->stamp, _space + i + sizeof(Header), hdr->len - sizeof(Header));
        i += hdr->len;
    }
}

void PacketBuffer::removeFirstIf(std::function<bool(uint32_t stamp, const uint8_t* packet, unsigned len)> cb) {
    removeIf(cb, true);
}

void PacketBuffer::removeIf(std::function<bool(uint32_t stamp, const uint8_t* packet, unsigned len)> cb,
    bool firstOnly) {    
    unsigned i = 0;
    while (i < _spaceUsed) {
        const Header* hdr = (const Header*)_space + i;
        // Call the predicate to decide if we need to remove
        if (cb(hdr->stamp, _space + i + sizeof(Header), hdr->len - sizeof(Header))) {
            // Shift left (overlapping)
            if (_spaceUsed > i + hdr->len)
                memmove(_space + i, _space + i + hdr->len, _spaceUsed - i - hdr->len);
            _spaceUsed -= hdr->len;
            if (firstOnly)
                return;
            // Here i stays in the same place (no forward progress made)
        }
        else {
            // Move right to the next packet
            i += hdr->len;
        }
    }
}

}
