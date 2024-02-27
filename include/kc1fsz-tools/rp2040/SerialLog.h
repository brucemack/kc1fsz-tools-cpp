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
#ifndef _SerialLog_h
#define _SerialLog_h

#include <cstdarg>
#include <iostream>

#include "hardware/uart.h"

#include "kc1fsz-tools/Common.h"
#include "kc1fsz-tools/Log.h"
#include "kc1fsz-tools/Runnable.h"

namespace kc1fsz {

class SerialLog : public Log, public Runnable {
public:

    SerialLog(uart_inst_t* uart)
    :   _uart(uart) { }

    void setStdout(bool s) { _stdout = s; }

    virtual void info(const char* format, ...) {
        
        char buf[128];
        snprintf(buf, 10, "%08lu ", time_ms() % 1000000);

        va_list argptr;
        va_start(argptr, format);
        vsnprintf(buf + 9, 128 - 9, format, argptr);
        va_end(argptr);
        _write(buf);
        if (_stdout)
            std::cout << "I: " << buf << std::endl;
    }

    virtual void error(const char* format, ...) {

        char buf[128];
        snprintf(buf, 10, "%08lu ", time_ms() % 1000000);

        va_list argptr;
        va_start(argptr, format);
        vsnprintf(buf + 9, 128 - 9, format, argptr);
        va_end(argptr);
        _write(buf);
        if (_stdout)
            std::cout << "E: " << buf << std::endl;
    }

    virtual bool run() {
        // Burst controls how many characters we can write without yielding
        uint32_t burst = 8;
        while (burst > 0 && uart_is_writable(_uart) && _sendCount < _writeCount) {
            uint32_t slot = _sendCount & _bufSizeMask;
            uart_putc_raw(_uart, _buf[slot]);
            _sendCount++;
            burst--;
        }
        return true;
    }

private:

    /**
     * NOTE: We purposely move all actual log transmission onto the run() 
     * loop to avoid introducing any overhead
     */
    void _write(const char* msg) {
        for (uint32_t i = 0; i < strlen(msg); i++) {
            uint32_t slot = _writeCount & _bufSizeMask;
            _buf[slot] = msg[i];
            _writeCount++;
        }
        // Add NL
        uint32_t slot = _writeCount & _bufSizeMask;
        _buf[slot] = 10;
        _writeCount++;
    }

    uart_inst_t* _uart;
    bool _stdout;

    static const uint32_t _bufSize = 1024;
    static const uint32_t _bufSizeMask = 0x03ff;
    char _buf[_bufSize];
    uint32_t _writeCount = 0;
    uint32_t _sendCount = 0;
};

}

#endif
