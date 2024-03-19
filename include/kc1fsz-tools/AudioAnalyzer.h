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
#ifndef _AudioAnalyzer_h
#define _AudioAnalyzer_h

#include <cstdint>
#include <iostream>

#include "kc1fsz-tools/AudioProcessor.h"

namespace kc1fsz {

class AudioAnalyzer : public AudioProcessor {
public:

    AudioAnalyzer(int16_t* historyArea, uint32_t historyAreaSize, 
        uint32_t sampleRate);

    void reset();

    void setEnabled(bool en) { _enabled = en; }

    /**
     * Determines the amount of power at a given frequency.  
     * @param freqHz The frequency to test.
     * 
     * NOTE: Even though the input/output are floats, the internal 
     * implementation is fixed point for efficiency sake.
     * 
     * The theoretical max power will be (amp * N / 2) ^ 2
    */
    float getTonePower(float freqHz) const;

    /**
     * NOTE: This contains a square root
     */
    int16_t getRMS() const;

    /**
     * @returns Mean squared.  Will always be positive.
     * NOTE: This does not contains a square root.
     */
    uint32_t getMS() const;

    /**
     * Peak value in decibels relative to full scale
     */
    float getPeakDBFS() const;

    int16_t getPeak() const; 

    /**
     * @returns 0 to 100
     */
    int16_t getPeakPercent() const; 

    /**
     * Used for managing DC offset
     */
    int16_t getAvg() const; 

    void dump(std::ostream& s) const;

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
    int32_t _rollingSum = 0;
    uint32_t _rollingSumSquared = 0;
    bool _enabled = false;
};

}

#endif
