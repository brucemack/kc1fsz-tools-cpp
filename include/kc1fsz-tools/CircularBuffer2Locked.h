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

#include <mutex>

#include "kc1fsz-tools/CircularBuffer2.h"

namespace kc1fsz {

class CircularBuffer2Locked {
public:

    CircularBuffer2Locked(CircularBuffer2& buf)
    :  _buf(buf) { }

    void push(const void* buf, unsigned len) {
        std::lock_guard<std::mutex> guard(_mutex);
        _buf.push(buf, len);
    }

    void visit(std::function<void(const void* buf, unsigned len)> cb) {
        std::lock_guard<std::mutex> guard(_mutex);
        _buf.visit(cb);
    }

    unsigned getCapacity() const { 
        return _buf.getCapacity(); 
    }

private:

    CircularBuffer2& _buf;
    std::mutex _mutex;
};

}
