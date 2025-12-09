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

#include <pthread.h>
#include <iostream>

#include "kc1fsz-tools/Log.h"

namespace kc1fsz {

/**
 * A thread-safe logger
 */
class MTLog : public Log {
public:

    MTLog();

protected:

    virtual void _out(const char* sev, const char* dt, const char* msg);

private:

    pthread_mutex_t _mutex;
};
}
