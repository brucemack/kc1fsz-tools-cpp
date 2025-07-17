/*
Copyright (C) 2023 - Bruce MacKinnon KC1FSZ

This program is free software: you can redistribute it and/or modify it under 
the terms of the GNU General Public License as published by the Free 
Software Foundation, either version 3 of the License, or (at your option) any 
later version.

This program is distributed in the hope that it will be useful, but WITHOUT 
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS 
FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with 
this program. If not, see <https://www.gnu.org/licenses/>.
*/
#include <algorithm>    // std::max
#include <limits>

#include "kc1fsz-tools/WindowAverage.h"

namespace kc1fsz {

WindowAverage::WindowAverage(unsigned windowSizeLog2, float* windowArea) 
:   _windowSizeLog2(windowSizeLog2),
    _windowArea(windowArea) {
    reset();
}

void WindowAverage::reset() {
    _accumulator = 0;
    _windowPtr = 0;
    if (_windowArea != 0) {
        unsigned areaSize = 1 << _windowSizeLog2;
        for (uint16_t i = 0; i < areaSize; i++)
            _windowArea[i] = 0;
    }
}

void WindowAverage::captureSample(float s) {
    if (_windowArea != 0) {
        uint16_t ptrMask = (1 << _windowSizeLog2) - 1;
        _accumulator += s;
        _accumulator -= _windowArea[_windowPtr];
        _windowArea[_windowPtr] = s;
        // Increment and wrap
        _windowPtr = (_windowPtr + 1) & ptrMask;
    }
}

float WindowAverage::getAvg() const {
    return _accumulator / (float)(1 << _windowSizeLog2);
}

float WindowAverage::getMin() const {
    float min = std::numeric_limits<float>::max();
    if (_windowArea) {
        uint16_t areaSize = 1 << _windowSizeLog2;
        for (uint16_t i = 0; i < areaSize; i++)
            min = std::min(min, _windowArea[i]);
    }
    return min;
}

float WindowAverage::getMax() const {
    float max = std::numeric_limits<float>::min();
    if (_windowArea) {
        uint16_t areaSize = 1 << _windowSizeLog2;
        for (uint16_t i = 0; i < areaSize; i++)
            max = std::max(max, _windowArea[i]);
    }
    return max;
}

}
