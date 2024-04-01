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

#ifndef PICO_BUILD
#include <time.h>       /* time_t, struct tm, time, localtime, strftime */
#endif

namespace kc1fsz {

class Log {
public:

    virtual void info(const char* format, ...) {
        va_list argptr;
        va_start(argptr, format);
        char buf[128];
        vsnprintf(buf, 128, format, argptr);
        va_end(argptr);

        char timeBuf[32];
        _fmtTime(timeBuf, 32);

        std::cout << "I: " << timeBuf << " " << buf << std::endl;
    }

    virtual void error(const char* format, ...) {
        va_list argptr;
        va_start(argptr, format);
        char buf[128];
        vsnprintf(buf, 128, format, argptr);
        va_end(argptr);

        char timeBuf[32];
        _fmtTime(timeBuf, 32);

        std::cout << "E: " << timeBuf << " " << buf << std::endl;
    }

protected:

    void _fmtTime(char* buf, uint32_t len) {
#ifdef PICO_BUILD
        buf[0] = 0;
#else
        time_t rawtime;
        time (&rawtime);
        struct tm * timeinfo = localtime(&rawtime);
        strftime (buf, len, "%T.%f", timeinfo);
#endif
    }
};

}

#endif
