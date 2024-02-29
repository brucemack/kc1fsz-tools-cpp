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
int16_t mult(int16_t var1, int16_t var2);
int16_t add(int16_t var1, int16_t var2);
int16_t sub(int16_t var1, int16_t var2);
int32_t L_mult(int16_t var1, int16_t var2);
int32_t L_add(int32_t L_var1, int32_t L_var2);
int32_t L_sub(int32_t L_var1, int32_t L_var2);

int16_t div2(int16_t var1, int16_t var2) {
    if (var1 == var2) {
        return 1;
    }
    else if (var1 == -var2) {
        return -1;
    }
    else if ( abs(var2) > abs(var1) ) {
        return (int16_t)(((int32_t)var1 << 15) / ((int32_t)var2));
    } 
    else {
        cout << "DIVISION ERROR " << var1 << " " << var2 << endl;
        return 0;
    }
}

static int32_t coeff[8] = {
    27980 * 2,
    26956 * 2,
    25701 * 2,
    24219 * 2,
    19261 * 2,
    16525 * 2,
    13297 * 2,
    9537 * 2
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

    const uint32_t N = 136;

    // Process the most recent 136 samples each time
    int16_t samples[N];
    uint32_t ptr = _historyPtr;
    int16_t maxVal = 0;
    for (uint32_t i = 0; i < N; i++) {
        // Look for reverse wrap
        if (ptr == 0) {
            ptr = _historySize;
        }
        ptr--;
        int16_t sample = _history[ptr];
        samples[i] = sample;
        // Needed for normalization
        int16_t absSample = s_abs(sample);
        if (absSample > maxVal) {
            maxVal = absSample;
        }
    }

    /* STRAIGHT FLOATING-POINT IMPLEMENTATION
    {
        float vk_1[8], vk_2[8];
        for (int k = 0; k < 8; k++) {
            vk_1[k] = 0; 
            vk_2[k] = 0;
        }

        for (uint32_t i = 0; i < N; i++) {        
            // Normalize samples
            float sample = (((float)samples[i] / (float)maxVal)) * 32767.0;
            // Take out a factor to avoid overflow
            sample /= 128;
            // Compute all 8 frequency bins
            for (int k = 0; k < 8; k++) {
                // This has an extra factor of 32767 in it
                float c = (float)(coeff[k]);
                // Remove the extra shift introduced by the multiplication
                float r = (c * vk_1[k]) / 32767.0;
                r -= vk_2[k];
                r += sample;
                vk_2[k] = vk_1[k];
                vk_1[k] = r;
            }
        }     

        for (int k = 0; k < 8; k++) {
            cout << k << " " << vk_1[k] << endl;
        }

        // At this point all number have an extra factor of 32767 because of the 
        // coefficient scaling.

        for (int k = 0; k < 8; k++) {
            // This has an extra factor of 32767 in it
            float c = (float)(coeff[k]);
            // This will be shifted 32767 * 32767 high (original from step 1, plus impact of mult)
            float r = (vk_1[k] * vk_1[k]);
            // This will be shifted 32767 * 32767 high  (original from step 1, plus impact of mult)
            r = r + (vk_2[k] * vk_2[k]);
            // This will be shifted 32767 * 32767 high 
            float m = (c * vk_1[k]);
            // This will be shifted (32767 * 32767) * 32767 / 32767 high
            r = r - ((m / 32767.0) * vk_2[k]);
            // Remove the extra 32767 * 32767
            // Re-introduce the factor (squared)
            r /= (32767.0 * 32767.0);
            r *= (128.0 * 128.0);

            cout << "RESULT " << k << " " << r << endl;
        }
    }
    */

    int16_t magSq[8];

    {
        int32_t vk_1[8], vk_2[8];
        for (int k = 0; k < 8; k++) {
            vk_1[k] = 0; 
            vk_2[k] = 0;
        }

        for (uint32_t i = 0; i < N; i++) {        
            // Normalize samples
            int16_t sample = div2(samples[i], maxVal);
            // Take out a factor to avoid overflow later
            sample >>= 7;
            // Compute all 8 frequency bins
            for (int k = 0; k < 8; k++) {
                // This has an extra factor of 32767 in it
                int32_t c = coeff[k];
                // Remove the extra shift introduced by the multiplication, but we
                // are still high by 32767.
                int32_t r = (c * vk_1[k]) >> 15;
                r -= vk_2[k];
                r += sample;
                vk_2[k] = vk_1[k];
                vk_1[k] = r;
            }
        }

        for (int k = 0; k < 8; k++) {
            cout << k << " " << vk_1[k] << endl;
        }
    
        // At this point all numbers have an extra factor of 32767 because of the 
        // initial coefficient scaling.

        for (int k = 0; k < 8; k++) {
            // This has an extra factor of 32767 in it
            int32_t c = coeff[k];
            // This will be shifted 32767 * 32767 high 
            int32_t r = (vk_1[k] * vk_1[k]);
            // This will be shifted 32767 * 32767 high
            r = r + (vk_2[k] * vk_2[k]);
            // This will be shifted 32767 * 32767 high 
            int32_t m = (c * vk_1[k]);
            // This will be shifted (32767 / 32767) * 32767 * 32767 high
            r = r - (((m >> 15) * vk_2[k]));
            // Remove the extra 32767 (squared, because this is power)
            // Re-introduce the factor (squared, because this is power)
            r >>= (15 + 15 - (7 + 7));
            magSq[k] = r;
            cout << k << " " << magSq[k] << endl;
        }
    }

    return true;
}

}
