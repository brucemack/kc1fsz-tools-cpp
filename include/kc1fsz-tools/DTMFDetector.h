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
#ifndef _DTMFDetector_h
#define _DTMFDetector_h

#include <cstdint>

#include "kc1fsz-tools/AudioProcessor.h"

namespace kc1fsz {

class DTMFDetector : public AudioProcessor {
public:

    DTMFDetector(int16_t* historyArea, uint32_t historyAreaSize, 
        uint32_t sampleRate);

    void reset();

    // ----- From AudioProcessor ----------------------------------------------

    /**
     * NOTE: Assumes PCM-16 (signed) at the moment
     */
    bool play(const int16_t* frame, uint32_t frameLen);

private:

    int16_t* _history;
    uint32_t _historySize;
    uint32_t _sampleRate;
    uint32_t _historyPtr = 0;
};

}

#endif
