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

#include <iostream>
#include <mutex>
#include <functional>
#include <cassert>

#include "kc1fsz-tools/Log.h"

namespace kc1fsz {

/**
 * A thread-safe logger.  Uses a mutex internally to protect the integrity of 
 * lines being written.
 *
 * IMPORTANT: The _lockedOut method is called under the lock. 
 */
class MTLog2 : public Log {
protected:

    virtual void _out(const char* sev, const char* dt, const char* msg) {
        char tid[16];
        tid[0] = 0;
        {
            std::lock_guard<std::mutex> guard(_mutex); 
            std::cout << tid << " " << sev << dt << " " << msg << std::endl;
            _lockedOut(sev, dt, msg);
        }
    }

    /**
     * @param sev Severity
     * @param dt Date/time
     * @param msg Text
     */
    virtual void _lockedOut(const char* sev, const char* dt, const char* msg) { }

    std::mutex _mutex;
};

}
