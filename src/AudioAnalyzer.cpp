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

#include "kc1fsz-tools/AudioAnalyzer.h"

namespace kc1fsz {

AudioAnalyzer::AudioAnalyzer(int16_t* historyArea, uint32_t historyAreaSize) 
:   _history(historyArea),
    _historySize(historyAreaSize) {
    for (uint32_t i = 0; i < _historySize; i++) {
        _history[i] = 0;
    }
}

/**
    * NOTE: Assumes PCM-16 (signed) at the moment
    */
void AudioAnalyzer::sample(int16_t s) {
    _history[_writePtr++] = s;
    if (_writePtr == _historySize) {
        _writePtr = 0;
    }
}

float AudioAnalyzer::getRMS() const {
    float totalSquared = 0;
    for (uint32_t i = 0; i < _historySize; i++) {
        totalSquared += (float)_history[i] * (float)_history[i];
    }
    return std::sqrt(totalSquared / (float)_historySize);
}

float AudioAnalyzer::getPeakDBFS() const {
    int16_t max = getPeak();
    return 2.0 * std::log((float)max / (float)32'767.0);
}

int16_t AudioAnalyzer::getPeak() const {
    int16_t max = 0;
    for (uint32_t i = 0; i < _historySize; i++) {
        max = std::max(max, (int16_t)std::abs(_history[i]));
    }
    return max;
}

}
