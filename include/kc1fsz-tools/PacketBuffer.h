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

class PacketBuffer {
public:

    /**
     * IMPORTANT: Length is assumed to be 16-bits big-endian!
     */
    PacketBuffer(uint8_t* space, unsigned spaceCapacity, unsigned lenOffset)
    :   _space(space), _spaceCapacity(spaceCapacity), _lenOffset(lenOffset) {        
    }

    void clear();

    /**
     * @returns Bytes actually used
     */
    unsigned getUsed() const { return _spaceUsed; }

    /**
     * @returns true if successful, false if no (i.e. no space)
     */
    bool push(const uint8_t* packet, unsigned len);

    bool tryPop(uint8_t* packet, unsigned* len);

    void visitAll(std::function<void(const uint8_t* packet, unsigned len)> cb);

    void removeFirstIf(std::function<bool(const uint8_t* packet, unsigned len)> cb);

    void removeIf(std::function<bool(const uint8_t* packet, unsigned len)> cb,
        bool firstOnly = false);

private:

    uint8_t* _space;
    unsigned _spaceCapacity;
    unsigned _spaceUsed = 0;
    unsigned _lenOffset;
};

}