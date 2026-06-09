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
 */
#pragma once

#include <iostream>
#include <mutex>
#include <functional>

#include "kc1fsz-tools/Log.h"

namespace kc1fsz {

/**
 * A thread-safe logger.  Uses a mutex internally to protect the integrity of 
 * lines being written.
 */
class MTLog3 : public Log {
public:

    using logCb = std::function<void(const char* sev, const char* dt, const char* msg)>;

    MTLog3(logCb cb)
    : _logCb(cb) { }

protected:

    virtual void _out(const char* sev, const char* dt, const char* msg) {
        std::lock_guard<std::mutex> guard(_mutex);
        _logCb(sev, dt, msg);
    }

    logCb _logCb;
    std::mutex _mutex;
};

}
