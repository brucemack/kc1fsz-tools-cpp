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
#ifndef _PollTimer_h
#define _PollTimer_h

#include <cstdint>

namespace kc1fsz {

class PollTimer {
public:

    virtual void setIntervalUs(uint32_t us) = 0;

    /**
     * Starts the interval again
     */
    virtual void reset() { }

    /**
     * @return true if the interval has expired.  Will only return 
     *  true once per interval.
    */
    virtual bool poll() = 0;
};

}

#endif
