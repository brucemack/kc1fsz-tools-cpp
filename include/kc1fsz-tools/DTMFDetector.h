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
        uint32_t sampleRateHz);

    void reset();

    bool resultAvailable() const;

    /**
     * @returns The latest symbol decoded.
    */
    char getResult();

    // ----- From AudioProcessor ----------------------------------------------

    /**
     * NOTE: Assumes PCM-16 (signed) at the moment
     * NOTE: Only supports 160 or 4x160 size frames at the moment.
     * NOTE: Assumes 8 kHz audio at the moment.
     */
    bool play(const int16_t* frame, uint32_t frameLen);

private:

    void _processFrame(const int16_t* frame, uint32_t frameLen);

    static char _detect(int16_t* samples, uint32_t N);

    int16_t* _history;
    uint32_t _historySize;
    uint32_t _sampleRate;
    uint32_t _historyPtr;
    // Numer of samples in the DFT block
    static const uint32_t N = 136;    

    // Short history
    char _symbol_1, _symbol_2, _symbol_3;
    // The good results
    static const uint32_t _resultSize = 16;
    char _result[_resultSize];
    uint32_t _resultLen;
};

}

#endif
