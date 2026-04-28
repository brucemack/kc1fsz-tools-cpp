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
#pragma once

#include <cstdint>
#include <functional>

#include "kc1fsz-tools/CircularQueuePointers.h"

namespace kc1fsz {

class TaggedBuffer {
public:

    using visitCb = std::function<void(uint32_t stamp, unsigned id, const uint8_t* packet, unsigned len)>;
    using predCb = std::function<bool(uint32_t stamp, unsigned id, const uint8_t* packet, unsigned len)>;

    /**
     * IMPORTANT: Length is assumed to be 16-bits big-endian!
     */
    TaggedBuffer(uint8_t* space, unsigned spaceCapacity)
    :   _space(space), 
        _spaceCapacity(spaceCapacity) {        
    }

    void clear();

    bool isEmpty() const { return _spaceUsed == 0; }

    /**
     * @returns Bytes actually used
     */
    unsigned getUsed() const { return _spaceUsed; }

    /**
     * @param stamp The timestamp of the item
     * @param id An arbitrary ID used for tracking the item
     * @returns true if successful, false if no (i.e. no space)
     */
    bool push(uint32_t stamp, unsigned id, const uint8_t* packet, unsigned len);

    /**
     * Has exactly the same semantics of push, but takes the packet in two
     * parts that will be concatenated on the queue. This can be helpful
     * when the header and body of a packet are coming in separate  parts.
     *
     * @returns true if successful, false if no (i.e. no space)
     */
    bool push(uint32_t stamp, unsigned id, const uint8_t* packet0, unsigned len0, 
        const uint8_t* packet1, unsigned len1);

    /**
     * Has exactly the same semantics of push, but takes the packet in three
     * parts that will be concatenated on the queue. This can be helpful
     * when the header and body of a packet are coming in separate parts.
     *
     * @returns true if successful, false if no (i.e. no space)
     */
    bool push(uint32_t stamp, unsigned id, const uint8_t* packet0, unsigned len0, 
        const uint8_t* packet1, unsigned len1,
        const uint8_t* packet2, unsigned len2);

    /**
     * Packet will be truncated if it's not large enough to fit in the space
     * provided.
     * @param len Should start off pointing to the available space. Will be 
     * overwritten with the size of the packet popped.
     * @returns false if the buffer was empty.
     */
    bool tryPop(uint32_t* stamp, unsigned* id, uint8_t* packet, unsigned* len);

    /**
     * Provides a peek at the first element without removing it.
     * Packet will be truncated if it's not large enough to fit in the space
     * provided.
     * @param len Should start off pointing to the available space. Will be 
     * overwritten with the size of the packet peeked.
     * @returns false if the buffer was empty
     */
    bool tryPeek(uint32_t* stamp, unsigned* id, uint8_t* packet, unsigned* len);

    /**
     * Takes off and discards the first item, if any.
     */
    void pop();

    void visitAll(visitCb visitor) const;

    void removeFirstIf(predCb pred);

    /**
     * @param firstOnly If this is true then the process stops after the first match.
     */
    void removeIf(predCb pred, bool firstOnly = false);

private:

    struct Header {
        unsigned len;
        uint32_t stamp;
        unsigned id;
    };

    uint8_t* _space;
    unsigned _spaceCapacity;
    unsigned _spaceUsed = 0;

    bool _tryPeekPop(uint32_t* stamp, unsigned* id, uint8_t* packet, unsigned* packetLen, bool pop);
};

}