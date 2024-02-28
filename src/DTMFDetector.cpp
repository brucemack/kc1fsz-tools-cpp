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
#include <algorithm>
#include <cmath>
#include <iostream>

#include "kc1fsz-tools/DTMFDetector.h"

using namespace std;

namespace kc1fsz {

// TODO: CONSOLIDATE
int16_t s_abs(int16_t var1);
int16_t div(int16_t var1, int16_t var2);
int16_t mult(int16_t var1, int16_t var2);
int16_t add(int16_t var1, int16_t var2);
int16_t sub(int16_t var1, int16_t var2);
int32_t L_mult(int16_t var1, int16_t var2);
int32_t L_add(int32_t L_var1, int32_t L_var2);
int32_t L_sub(int32_t L_var1, int32_t L_var2);

static int16_t coeff[8] = {
    27980,
    26956,
    25701,
    24219,
    19261,
    16525,
    13297,
    9537
 };

DTMFDetector::DTMFDetector(int16_t* historyArea, uint32_t historyAreaSize, uint32_t sampleRate) 
:   _history(historyArea),
    _historySize(historyAreaSize),
    _sampleRate(sampleRate) {
    reset();
}

void DTMFDetector::reset() {
    for (uint32_t i = 0; i < _historySize; i++) {
        _history[i] = 0;
    }
}

bool DTMFDetector::play(const int16_t* frame, uint32_t frameLen) {  
    
    for (uint32_t i = 0; i < frameLen; i++) {
        _history[_historyPtr] = frame[i];
        // Manage wrap-around
        _historyPtr++;
        if (_historyPtr == _historySize) {
            _historyPtr = 0;
        }
    }

    // Process the most recent 136 samples each time
    int16_t samples[136];
    uint32_t ptr = _historyPtr;
    int16_t maxVal = 0;
    for (uint32_t i = 0; i < frameLen; i++) {
        // Look for reverse wrap
        if (ptr == 0) {
            ptr = _historySize;
        }
        ptr -= 1;
        int16_t sample = _history[ptr];
        samples[i] = sample;
        // Needed for normalization
        int16_t absSample = s_abs(sample);
        maxVal = std::max(maxVal, absSample);
    }

    int32_t vk_1[8], vk_2[8];
    for (uint32_t k = 0; k < 8; k++) {
        vk_1[k] = 0; 
        vk_2[k] = 0;
    }

    for (uint32_t i = 0; i < frameLen; i++) {
        // Normalize samples
        int32_t sample = div(samples[i], maxVal);
        // Compute all 8 frequency bins
        for (uint32_t k = 0; k < 8; k++) {
            int32_t r = L_mult(coeff[k], vk_1[k]);
            r = L_sub(r, vk_2[k]);
            r = L_add(r, sample);
            vk_2[k] = vk_1[k];
            vk_1[k] = r;
        }
    }

    int16_t magSq[8];
    for (uint32_t k = 0; k < 8; k++) {
        int16_t vk_1 = vk_1[k] >> 16;
        int16_t vk_2 = vk_2[k] >> 16;
        int16_t r = mult(vk_1, vk_1);
        r = add(r, mult(vk_2, vk_2));
        r = sub(r, mult(coeff[k], vk_2));
        magSq[k] = r;
        cout << k << " " << magSq[k] << endl;
    }

    return true;
}

}
