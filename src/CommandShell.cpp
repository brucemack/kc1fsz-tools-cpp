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
#include <cctype>
#include "kc1fsz-tools/OutStream.h"
#include "kc1fsz-tools/CommandShell.h"

namespace kc1fsz {

CommandShell::CommandShell() {
    reset();
}

void CommandShell::process(char c) {
   if (c == '\r') {
        if (_sink) {
            _os->write('\n');
            if (_cmdBuffer[0] != 0)
                _sink->process(_cmdBuffer);
            reset();
        }
    }
    else if (c == '\n') {
    }
    else if (c == 0x08) {
        if (_cmdBufferLen > 0) {
            _cmdBufferLen--;
            _cmdBuffer[_cmdBufferLen] = 0;
            _os->write(0x08);
            _os->write(' ');
            _os->write(0x08);
        }
    }
    else if (c == 27) {
        _os->write('\n');
        reset();
    }
    else if (std::isprint(c)) {
        if (_cmdBufferLen < _cmdBufferSize - 1) {
            _cmdBuffer[_cmdBufferLen++] = c;
            _cmdBuffer[_cmdBufferLen] = 0;
            _os->write((uint8_t)c);
        }
    }
}

void CommandShell::reset() {
    _cmdBuffer[0] = 0;
    _cmdBufferLen = 0;
    if (_os) {
        _os->write('$');
        _os->write(' ');
    }
}

}
