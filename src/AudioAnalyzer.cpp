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

#include "kc1fsz-tools/AudioAnalyzer.h"

using namespace std;

namespace kc1fsz {

AudioAnalyzer::AudioAnalyzer(int16_t* historyArea, uint32_t historyAreaSize, uint32_t sampleRate) 
:   _history(historyArea),
    _historySize(historyAreaSize),
    _sampleRate(sampleRate) {
    reset();
}

void AudioAnalyzer::reset() {
    for (uint32_t i = 0; i < _historySize; i++) {
        _history[i] = 0;
    }
    _rollingSum = 0;
    _rollingSumSquared = 0;
}

bool AudioAnalyzer::sanityCheck() const {
    int32_t rs = 0;
    uint32_t rss = 0;
    for (uint32_t i = 0; i < _historySize; i++) {
        rs += (int32_t)_history[i];
        int32_t sq = ((int32_t)_history[i] * (int32_t)_history[i]) >> 9;
        rss += sq;
    }
    return _rollingSum == rs && _rollingSumSquared == rss;
}

bool AudioAnalyzer::play(const int16_t* frame, uint32_t frameLen) {  
    if (!_enabled) {
        return false;
    }
    for (uint32_t i = 0; i < frameLen; i++) {

        // What we are about to overrwrite
        int16_t sample = _history[_historyPtr];
        _rollingSum -= (int32_t)sample;
        // We accumulate a down-shifted version to avoid overflow
        int32_t sq = ((int32_t)sample * (int32_t)sample) >> _scaleShift;
        _rollingSumSquared -= sq;

        // New sample
        sample = frame[i];
        _rollingSum += (int32_t)sample;
        // We accumulate a down-shifted version to avoid overflow
        sq = ((int32_t)sample * (int32_t)sample) >> _scaleShift;
        _rollingSumSquared += sq;

        _history[_historyPtr] = sample;
        // Manage wrap-around
        _historyPtr++;
        if (_historyPtr == _historySize) {
            _historyPtr = 0;
        }
    }
    return true;
}

void AudioAnalyzer::dump(std::ostream& s) const {
    for (uint32_t i = 0; i < _historySize; i++) {
        s << _history[i] << endl;
    }
}

int16_t AudioAnalyzer::getRMS() const {
    // NOTE: The accumulator is scaled down by 512, so we need to 
    // scale back up.
    float mean = ((float)_rollingSumSquared * 512.0) / (float)_historySize;
    return std::sqrt(mean);
}

uint32_t AudioAnalyzer::getMS() const {
    return (_rollingSumSquared / (int32_t)_historySize) << _scaleShift;
}

int16_t AudioAnalyzer::getAvg() const {
    return (int16_t)(_rollingSum / (int32_t)_historySize);
}

float AudioAnalyzer::getPeakDBFS() const {
    int16_t max = getPeak();
    return 20.0 * std::log((float)max / (float)32'767.0);
}

int16_t AudioAnalyzer::getPeak() const {
    int16_t max = 0;
    for (uint32_t i = 0; i < _historySize; i++) {
        max = std::max(max, (int16_t)std::abs(_history[i]));
    }
    return max;
}

int16_t AudioAnalyzer::getPeakPercent() const {
    return (getPeak() * 100) / 32767;
}

float AudioAnalyzer::getTonePower(float freqHz) const {

    float w = 2.0 * 3.1415926 * (float)freqHz / (float)_sampleRate;
    float coeff = 2.0 * std::cos(w);
    int16_t coeff_q14 = (1 << 14) * coeff;
    int16_t z = 0;
    int16_t zprev = 0;
    int16_t zprev2 = 0;

    for (uint32_t n = 0; n < _historySize; n++) {
        int32_t mult = (int32_t)coeff_q14 * (int32_t)zprev;
        z = (_history[n] >> 6) + (mult >> 14) - zprev2;
        zprev2 = zprev;
        zprev = z;
    }

    int32_t mult = (int32_t)coeff_q14 * (int32_t)zprev;
    int32_t pz = zprev2 * zprev2 + zprev * zprev - ((int16_t)(mult >> 14)) * zprev2;
    return (float)pz * std::pow(2.0, 12);
}

}
