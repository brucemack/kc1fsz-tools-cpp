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
#include "kc1fsz-tools/linux/MTLog.h"

namespace kc1fsz {

MTLog::MTLog() {
    pthread_mutex_init(&_mutex, NULL);
}

void MTLog::_out(const char* sev, const char* dt, const char* msg) {
    char tid[16];
    pthread_getname_np(pthread_self(), tid, sizeof(tid));
    char tid2[32];
    snprintf(tid2, 32, "%10s", tid);
    pthread_mutex_lock(&_mutex);
    std::cout << tid2 << " " << sev << ": " << dt << " " << msg << std::endl;
    pthread_mutex_unlock(&_mutex);
}

}

