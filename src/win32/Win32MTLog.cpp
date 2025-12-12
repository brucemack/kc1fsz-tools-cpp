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
#include <unistd.h>
#include <sys/types.h>

#include "kc1fsz-tools/Common.h"
#include "kc1fsz-tools/win32/Win32MTLog.h"

namespace kc1fsz {

Win32MTLog::Win32MTLog() {
    _mutex = CreateMutex(NULL, FALSE, TEXT("Win32MTLog")); 
}

void Win32MTLog::_out(const char* sev, const char* dt, const char* msg) {
    char tid[16];
    tid[0] = 0;
    WaitForSingleObject(_mutex, INFINITE);
    std::cout << tid << " " << sev << dt << " " << msg << std::endl;
    ReleaseMutex(_mutex);
}

}

