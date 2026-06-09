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
 *
 * NOT FOR COMMERCIAL USE WITHOUT PERMISSION.
 */
#pragma once

#include <cstdint>
#include <functional>
#include <cassert>
#include <cstring>

#include "kc1fsz-tools/CircularQueuePtr.h"

namespace kc1fsz {

class CircularBuffer2 {
public:

    CircularBuffer2(const char* space, unsigned spaceSize, unsigned blockSize)
    :   _space(space), 
        _spaceSize(spaceSize),
        _blockSize(blockSize),
        // This will round down to the number of blocks that will fit in the space
        _ptr(spaceSize / blockSize, false) {
    }

    void push(const void* buf, unsigned len) {
        unsigned wrPtr = _ptr.getAndIncWritePtr();
        unsigned wrOffset = wrPtr * _blockSize;
        assert(len <= _blockSize);
        memcpy((void*)(_space + wrOffset), buf, len);
    }

    void visit(std::function<void(const void* buf, unsigned len)> cb) {
        _ptr.visit([this, cb](uint32_t rdPtr){ 
            unsigned rdOffset = rdPtr * _blockSize;
            cb(_space + rdOffset, _blockSize);
        });
    }

    unsigned getCapacity() const { return _ptr.getCapacity(); }

private:

    const char* _space;
    unsigned _spaceSize;
    unsigned _blockSize;

    CircularQueuePtr _ptr;
};

}
