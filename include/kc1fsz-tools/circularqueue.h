/**
 * Copyright (C) 2025, Bruce MacKinnon KC1FSZ
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

#include <cassert>
#include <functional>

namespace kc1fsz {

/**
 * A circular queue of fixed capacity.
 * TODO: NEED TO COMPLETE THIS
 */
template <typename T> class circularqueue {
public:

    circularqueue(T* data, unsigned capacity)
    :   _data(data), _capacity(capacity) { }

    void push(const T& t) {
        _data[_writePtr++] = t;
        if (_writePtr == _capacity)
            _writePtr = 0;
    }

    void visitAll(std::function<void(const T&)> visitor) const {
        // Start at the oldest entry
        unsigned readPtr = _writePtr + 1;
        if (readPtr == _capacity) 
            readPtr = 0;
        while (readPtr != _writePtr) {
            visitor(_data[readPtr++]);
            if (readPtr == _capacity)
                readPtr = 0;
        }
    }

private:

    T* _data;
    const unsigned _capacity;
    // The next entry to be written
    unsigned _writePtr = 0;
};

}
