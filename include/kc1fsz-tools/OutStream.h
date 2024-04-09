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
#ifndef _OutStream_h
#define _OutStream_h

#include <cstdint>

namespace kc1fsz {

class OutStream {
public:

    /**
     * @returns Number of bytes actually written.
     */
    virtual int write(uint8_t b) = 0;

    virtual bool isWritable() const = 0;

    /**
     * @returns Number of bytes actually written.
     */
    virtual int write(const uint8_t* s, uint32_t len) {
        uint32_t i;
        for (i = 0; i < len && isWritable(); i++) 
            write(s[i]);
        return i;
    }

    /**
     * @param s null-terminated string (null not written)
     * @returns Number of bytes actually written.
     */
    virtual int write(const char* s) {
        uint32_t i;
        for (i = 0; s[i] != 0 && isWritable(); i++) 
            write((uint8_t)s[i]);
        return i;
    }
};

}

#endif

    
