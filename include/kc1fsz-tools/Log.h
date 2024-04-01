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
#ifndef _Log_h
#define _Log_h

#include <cstdarg>
#include <iostream>

#include "kc1fsz-tools/Common.h"

namespace kc1fsz {

class Log {
public:

    virtual void info(const char* format, ...) {
        va_list argptr;
        va_start(argptr, format);
        char buf[128];
        vsnprintf(buf, 128, format, argptr);
        va_end(argptr);
        std::cout << "I: " << buf << std::endl;
    }

    virtual void error(const char* format, ...) {
        va_list argptr;
        va_start(argptr, format);
        char buf[128];
        vsnprintf(buf, 128, format, argptr);
        va_end(argptr);
        std::cout << "E: " << buf << std::endl;
    }

    virtual void debugDump(const char* msg, 
        const uint8_t* data, uint32_t dataLen) {
        std::cout << "D: " << msg << std::endl;
        prettyHexDump(data, dataLen, std::cout);
    }

    virtual void infoDump(const char* msg, 
        const uint8_t* data, uint32_t dataLen) {
        std::cout << "I: " << msg << std::endl;
        prettyHexDump(data, dataLen, std::cout);
    }
};

}

#endif
