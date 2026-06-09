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

namespace kc1fsz {

/**
 * Used for tracking the read and write pointers of a circular buffer. 
 * NOTE: This doesn't handle the buffer itself, just the pointers.
 */
class CircularQueuePtr {
public:
    
    /**
     * @param writeLimited When this is true the queue will stop accepting
     * new entries when the maximum size is reached. When this is false the 
     * queue will always throw away the oldest entry to make room for new 
     * entries.
    */
    CircularQueuePtr(uint32_t size, bool writeLimited = true) 
    : _size(size), _writeLimited(writeLimited) { }

    /**
     * @returns false if a read is possible.
     */
    bool isEmpty() const { return _readPtr == _writePtr; }

    /**
     * @returns true if a write is possible.
     */
    bool hasCapacity() const { return !_writeLimited || _nextWithWrap(_writePtr) != _readPtr; }

    uint32_t getReadPtr() const { return _readPtr; }

    /**
     * @return The location that should be read.
     */
    uint32_t getAndIncReadPtr() { 
        uint32_t p = _readPtr;
        if (_depth == 0) {
            _underflowCount++;
        } else {
            _readPtr = _nextWithWrap(p);
            _depth = _depth - 1;
        }
        return p;
    }

    uint32_t getWritePtr() const { return _writePtr; }

    /**
     * @return The location that should be written.
     */
    uint32_t getAndIncWritePtr() { 
        uint32_t p = _writePtr;
        uint32_t next = _nextWithWrap(_writePtr);
        if (_writeLimited) {
            // Check for overflow
            if (next == _readPtr) {
                _overflowCount = _overflowCount + 1;
            }  else {
                _writePtr = next;
                _depth = _depth + 1;
                _maxDepth = std::max(_depth, _maxDepth);
            }
        }
        else {
            // If we're writing into the last available slot then 
            // discard the oldest entry by bumping the read pointer forward
            // as well.
            if (next == _readPtr) {
                _readPtr = _nextWithWrap(_readPtr);
                _writePtr = next;
            }
            else {
                _writePtr = next;
                _depth = _depth + 1;
                _maxDepth = std::max(_depth, _maxDepth);
            }
        }
        return p;
    }

    uint32_t getOverflowCount() const { return _overflowCount; }
    uint32_t getUnderflowCount() const { return _underflowCount; }
    uint32_t getDepth() const { return _depth; }
    uint32_t getCapacity() const { return _size; }

    /**
     * Visits all entries in order.
     */
    void visit(std::function<void(uint32_t i)> cb) const {
        uint32_t p = _readPtr;
        while (p != _writePtr) {
            cb(p);
            p = _nextWithWrap(p);
        }
    }

private:

    uint32_t _nextWithWrap(uint32_t a) const {
        if (a + 1 == _size)
            return 0;
        else 
            return a + 1;
    }

    const uint32_t _size;
    const bool _writeLimited;
    volatile uint32_t _readPtr = 0;
    volatile uint32_t _writePtr = 0;
    volatile uint32_t _depth = 0;
    volatile uint32_t _maxDepth = 0;
    uint32_t _overflowCount = 0;
    uint32_t _underflowCount = 0;
};

}
