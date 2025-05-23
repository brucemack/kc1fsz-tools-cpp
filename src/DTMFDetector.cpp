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
#include <cassert>

#include "kc1fsz-tools/DTMFDetector.h"
#include "kc1fsz-tools/Common.h"

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
        return 0;
    }
}

// This is 2 * cos(2 * PI * fk / fs) for each of the 8 frequencies
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

// This is 2 * cos(2 * PI * (2 * fk) / fs) for each of the 8 frequencies.
// Used for checking second-order harmonics.
static int32_t harmonicCoeff[8] = {
    15014 * 2,
    11583 * 2,
     7549 * 2,
     3032 * 2,
   -10565 * 2,
   -16503 * 2,
   -22318 * 2,
   -27472 * 2
 };

static char symbolGrid[4 * 4] = {
    '1', '2', '3', 'A',
    '4', '5', '6', 'B',
    '7', '8', '9', 'C',
    '*', '0', '#', 'D'
};

DTMFDetector::DTMFDetector(uint32_t sampleRate) 
:   _sampleRate(sampleRate) {
    reset();
    assert(sampleRate == 8000);
}

void DTMFDetector::reset() {
    for (uint32_t i = 0; i < _vscHistSize; i++) {
        _vscHist[i] = 0;
    }
    _resultLen = 0;
}

bool DTMFDetector::play(const int16_t* frame, uint32_t frameLen) {  
    // At the moment we can only process 160 sample or 4x160 sample frames
    if (frameLen != 4 * 160 && frameLen != 160) {
        panic("Frame length error");
    }
    _processFrame(frame, 160);
    if (frameLen > 160) {
        _processFrame(frame + 160, 160);
        _processFrame(frame + 320, 160);
        _processFrame(frame + 480, 160);
    }
    return true;
}

bool DTMFDetector::isAvailable() const {
    return _resultLen > 0;
}

char DTMFDetector::pullResult() {
    if (_resultLen > 0) {
        char r = _result[0];
        // Pop the queue
        for (uint32_t i = 0; i < _resultLen - 1; i++) {
            _result[i] = _result[i + 1];
        }
        _resultLen--;
        return r;
    }
    else {
        return 0;
    }
}

void DTMFDetector::_processFrame(const int16_t* frame, uint32_t frameLen) { 
    // TOOD: REMOVE HARDCODING
    assert(frameLen == 160);
    // Divide the frame into two 10ms blocks
    _processShortBlock(frame, 80);
    _processShortBlock(&(frame[80]), 80);
}

void DTMFDetector::_processShortBlock(const int16_t* block, uint32_t blockLen) {  
    // TOOD: REMOVE HARDCODING
    assert(blockLen == 80);
    assert(blockLen < N);

    // Put the block into the center of the window with zero padding on both sides
    int16_t samples[N];
    int16_t maxVal = 0;
    unsigned int zeroPadSize = (N - blockLen) / 2;

    for (unsigned int i = 0; i < N; i++) {
        if (i < zeroPadSize) 
            samples[i] = 0;
        else if (i < zeroPadSize + blockLen)
            samples[i] = block[i - zeroPadSize];
        else
            samples[i] = 0;
        // Max amplitude needed for normalization
        int16_t absSample = s_abs(samples[i]);
        if (absSample > maxVal)
            maxVal = absSample;
    }
    
    // Apply the normalization
    for (uint32_t i = 0; i < N; i++) {
        samples[i] = div2(samples[i], maxVal);
    }

    char vscSymbol = _detectVSC(samples, N);
    //cout << "VSC Symbol " << (int)vscSymbol << " " << vscSymbol << endl;

    // The VSC->DSC transition requires some history.
    // First "push the stack" to make room for the new symbol
    for (unsigned int i = 1; i < _vscHistSize; i++)
        _vscHist[_vscHistSize - i] = _vscHist[_vscHistSize - i - 1];
    // Record the latest symbol
    _vscHist[0] = vscSymbol;

    // Look at the recent VSC history and decide on the detection status.
    // (See ETSI ES 201 235-3 V1.1.1 (2002-03) section 4.2.2)

    // A valid DSC recognition requires a consistent detection over 40ms 
    if (!_inDSC) {
        if (_vscHist[0] != 0 &&
            _vscHist[0] == _vscHist[1] &&
            _vscHist[0] == _vscHist[2] &&
            _vscHist[0] == _vscHist[3]) {
            _inDSC = true;
            _detectedSymbol = _vscHist[0];
            // Record the symbol on transition
            if (_resultLen < _resultSize) {
                _result[_resultLen++] = _detectedSymbol;
            }
        }
    }
    // A valid DSC cesation requires an interruption of at least 40ms
    else {
        if (_vscHist[0] != _detectedSymbol &&
            _vscHist[1] != _detectedSymbol &&
            _vscHist[2] != _detectedSymbol &&
            _vscHist[3] != _detectedSymbol) {
            _inDSC = false;
        }
    }
}

/**
 * A classic implementation of the Goertzel algorithm in fixed point.
*/
static int16_t computePower(int16_t* samples, uint32_t N, int32_t coeff) {

    int32_t vk_1 = 0, vk_2 = 0;

    // This is the amount that we down-shift the sample in order to 
    // preserve precision as we go through the Goertzel iterations.
    // This number very much depends on N, so pay close attention 
    // if N changes.
    const int sampleShift = 7;

    for (uint32_t i = 0; i < N; i++) {        
        int16_t sample = samples[i];
        // Take out a factor to avoid overflow later
        sample >>= sampleShift;
        // This has an extra factor of 32767 in it
        int32_t c = coeff;
        // Remove the extra shift introduced by the multiplication, but we
        // are still high by 32767.
        int32_t r = (c * vk_1) >> 15;
        r -= vk_2;
        r += sample;
        vk_2 = vk_1;
        vk_1 = r;
    }

    // At this point all numbers have an extra factor of 32767 because of the 
    // initial coefficient scaling.

    // This has an extra factor of 32767 in it
    int32_t c = coeff;
    // This will be shifted 32767 * 32767 high 
    int32_t r = (vk_1 * vk_1);
    // This will be shifted 32767 * 32767 high
    r = r + (vk_2 * vk_2);
    // This will be shifted 32767 * 32767 high 
    int32_t m = (c * vk_1);
    // This will be shifted (32767 / 32767) * 32767 * 32767 high
    r = r - (((m >> 15) * vk_2));
    // Remove the extra 32767 (squared, because this is power)
    // Re-introduce the factor (squared, because this is power)
    r >>= (15 + 15 - (sampleShift + sampleShift));
    return (int16_t)r;
}

char DTMFDetector::_detectVSC(int16_t* samples, uint32_t N) {

    // Compute the power on the fundamental frequencies
    int16_t power[8];
    bool nonZeroFound = false;
    for (int k = 0; k < 8; k++) {
        power[k] = computePower(samples, N, coeff[k]);
        if (power[k] > 0)
            nonZeroFound = true;
    }

    // This could happen in the case where a DC signal is sent in
    if (!nonZeroFound)
        return 0;

    // Find the maximum of the combined powers
    int32_t maxCombPower = 0;
    int maxRow = 0, maxCol = 0;
    int32_t maxRowPower = 0, maxColPower = 0;
    for (int r = 0; r < 4; r++) {
        int16_t rowPower = power[r];
        for (int c = 0; c < 4; c++) {
            int16_t colPower = power[4 + c];
            int32_t combPower = rowPower + colPower;
            if (combPower > maxCombPower) {
                maxCombPower = combPower;
                maxRow = r;
                maxRowPower = rowPower;
                maxCol = c;
                maxColPower = colPower;
            }
        }
    }

    // Now see if any pair comes close to the maximum
    for (int r = 0; r < 4; r++) {
        int16_t rowPower = power[r];
        for (int c = 0; c < 4; c++) {
            int16_t colPower = power[4 + c];
            int32_t combPower = rowPower + colPower;
            // If the power advantage of the first place is less than 10x
            // of the second place then the symbol is not valid.
            if (r != maxRow && c != maxCol && 
                combPower != 0 && 
                (maxCombPower / combPower) < 10) {
                return 0;
            }
        }
    }

    // Now check the twist between the two.  

    // Make sure that the column (high group) power is not >4dB the row (low 
    // group) power. We are shifting the numerator by 4 to allow more accurate 
    // comparison in fixed point.
    if ((4 * maxColPower) / maxRowPower > 6) {
        //cout << "Twist failed: col too high" << endl;
        return 0;
    }
    
    // Make sure that the row (low group) power is not >8dB the column (high
    // group) power. We are shifting the numerator by 4 to allow more accurate 
    // comparison in fixed point.
    // TEMP: EXPANDING THE RANGE TO 12dB BECAUSE OF PROBLEMS ON THE HIGH END WITH
    // THE FT-65
    //if ((4 * maxRowPower) / maxColPower > 10) {
    if ((4 * maxRowPower) / maxColPower > 11) {
        //cout << "Twist failed: row too high " << maxRowPower << " " << maxColPower << endl;
        return 0;
    }

    // Compute the harmonic for the row and column
    int16_t maxRowHarmonicPower = computePower(samples, N, harmonicCoeff[maxRow]);
    int16_t maxColHarmonicPower = computePower(samples, N, harmonicCoeff[4 + maxCol]);

    // Make sure the harmonics are 20dB down from the fundamentals
    if (maxRowHarmonicPower != 0 && maxRowPower / maxRowHarmonicPower < 10) {
        //cout << "Failed on harmonic power test row" << endl;
        return 0;
    }
    if (maxColHarmonicPower != 0 && maxColPower / maxColHarmonicPower < 10) {
        //cout << "Failed on harmonic power test col " << maxColPower << " " << maxColHarmonicPower << endl;
        return 0;
    }

    return symbolGrid[4 * maxRow + maxCol];
}

}
