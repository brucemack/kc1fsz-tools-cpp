/**
 * Copyright (C) 2024, Bruce MacKinnon KC1FSZ
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
#ifndef _AsyncChannel_h
#define _AsyncChannel_h

#include <cstdint>

namespace kc1fsz {

class AsyncChannel {
public:

    /**
     * Tells the channel to make forward progress, whatever that 
     * means in its context.
     * 
     * @return true if anything happened during the poll cycle.
    */
    virtual bool poll() { return false; }

    /**
     * @return true if read() can be called productively.
    */
    virtual bool isReadable() const = 0;

    /**
     * @return true if write() can be called productively.
    */
    virtual bool isWritable() const = 0;

    virtual uint32_t bytesReadable() const = 0;
    virtual uint32_t bytesWritable() const = 0;

    /**
     * @return The number of bytes that were actually read.  <= bufCapacity
    */
    virtual uint32_t read(uint8_t* buf, uint32_t bufCapacity) = 0;

    /**
     * @return The number of bytes that were actually written.  <= bufLen, 
     *   so be ready to hold some un-written piece if necessary.
    */
    virtual uint32_t write(const uint8_t* buf, uint32_t bufLen) = 0;

    /**
     * Blocks until all writing is finished. 
     */
    virtual void blockAndFlush(uint32_t toMs) { }
};

}

#endif

