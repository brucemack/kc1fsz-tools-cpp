/**
 * Software Defined Repeater Controller
 * Copyright (C) 2025, Bruce MacKinnon KC1FSZ
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
#include <iostream>
#include <cmath>
#include <cstring>

#include "kc1fsz-tools/Clock.h"
#include "kc1fsz-tools/DTMFDetector2.h"

#define PI (3.1415926f)

using namespace std;

namespace kc1fsz {

/**
 * Example for sanity: 0 dBv is 1 Vpp, which is 0.5 Vp, 
 * which is 0.3535 Vrms.
 */
static constexpr float dbvToVrms(float dbv) {
    float vpp = pow(10, (dbv / 20));
    float vp = vpp / 2.0;
    return vp * 0.707;
}

/**
 * IMPORTANT: Requires that the numerator (var1) be smaller than the 
 * denominator (var2)!
 */
static int16_t div2(int16_t var1, int16_t var2) {
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

// This is 2 * cos(2 * PI * fk / fs) for each of the frequencies
const int32_t DTMFDetector2::coeffRow[4] = {
    (int32_t)(2.0 * std::cos(2.0 * PI * freqRow[0] / FS) * 32767.0),
    (int32_t)(2.0 * std::cos(2.0 * PI * freqRow[1] / FS) * 32767.0),
    (int32_t)(2.0 * std::cos(2.0 * PI * freqRow[2] / FS) * 32767.0),
    (int32_t)(2.0 * std::cos(2.0 * PI * freqRow[3] / FS) * 32767.0),
 };

 // This is 2 * cos(2 * PI * fk / fs) for each of the frequencies
const int32_t DTMFDetector2::coeffCol[4] = {
    (int32_t)(2.0 * std::cos(2.0 * PI * freqCol[0] / FS) * 32767.0),
    (int32_t)(2.0 * std::cos(2.0 * PI * freqCol[1] / FS) * 32767.0),
    (int32_t)(2.0 * std::cos(2.0 * PI * freqCol[2] / FS) * 32767.0),
    (int32_t)(2.0 * std::cos(2.0 * PI * freqCol[3] / FS) * 32767.0),
 };

// This is 2 * cos(2 * PI * (2 * fk) / fs) for each of the frequencies.
// Used for checking second-order harmonics.
const int32_t DTMFDetector2::harmonicCoeffRow[4] = {
    (int32_t)(2.0 * std::cos(2.0 * PI * freqRow[0] * 2.0 / FS) * 32767.0),
    (int32_t)(2.0 * std::cos(2.0 * PI * freqRow[1] * 2.0 / FS) * 32767.0),
    (int32_t)(2.0 * std::cos(2.0 * PI * freqRow[2] * 2.0 / FS) * 32767.0),
    (int32_t)(2.0 * std::cos(2.0 * PI * freqRow[3] * 2.0 / FS) * 32767.0),
 };

// This is 2 * cos(2 * PI * (2 * fk) / fs) for each of the frequencies.
// Used for checking second-order harmonics.
const int32_t DTMFDetector2::harmonicCoeffCol[4] = {
    (int32_t)(2.0 * std::cos(2.0 * PI * freqCol[0] * 2.0 / FS) * 32767.0),
    (int32_t)(2.0 * std::cos(2.0 * PI * freqCol[1] * 2.0 / FS) * 32767.0),
    (int32_t)(2.0 * std::cos(2.0 * PI * freqCol[2] * 2.0 / FS) * 32767.0),
    (int32_t)(2.0 * std::cos(2.0 * PI * freqCol[3] * 2.0 / FS) * 32767.0),
 };

const char DTMFDetector2::symbolGrid[4 * 4] = {
    '1', '2', '3', 'A',
    '4', '5', '6', 'B',
    '7', '8', '9', 'C',
    '*', '0', '#', 'D'
};

DTMFDetector2::DTMFDetector2(Clock& clock, unsigned blockSize) 
:   _clock(clock),
    _blockSize(blockSize),
    // Convert the dBv to power
    _signalThresholdPower(std::pow(dbvToVrms(-50), 2.0) * 32767.0)
{
    // TODO: REMOVE THIS LIMITATION
    assert(_blockSize < N3);
    for (unsigned i = 0; i < N3; i++)
        _history[i] = 0;
}

void DTMFDetector2::setSignalThreshold(float dbfs) { 
    // Convert the dBv to power
    _signalThresholdPower = pow(dbvToVrms(dbfs), 2.0) * 32767.0; 
}

void DTMFDetector2::processBlock(const float* block) {  

    // Shift the history to the left. Areas are overlapping.
    const unsigned preserve = N3 - _blockSize;
    std::memmove((void*)_history, (const void*)(_history + _blockSize), preserve * sizeof(int16_t));

    // Convert to 16-bit PCM and place in the most recent section 
    // of the history buffer
    int16_t* historyStart = &(_history[preserve]);
    for (unsigned i = 0; i < _blockSize; i++)
        // Convert to q15
        historyStart[i] = block[i] * 32767.0;
    // Run VSC detection on the last N3 (136) samples.
    const char vscSymbol = _detectVSC(_history, N3);
    if (vscSymbol != 0)
        _lastVscTime = _clock.time();
    //cout << "VSC Symbol " << (int)vscSymbol << " " << vscSymbol << endl;

    // The VSC->DSC transition requires some history.
    //
    // Look at the recent VSC history and decide on the detection status.
    // (See ETSI ES 201 235-3 V1.1.1 (2002-03) section 4.2.2)
    //
    // * Timing requirements are as follows.
    //   - A symbol must be transmitted for at least 40ms. Symbols shorter 
    //     than 23ms must be rejected.
    //   - The gap between symbols must be at least 40ms.

    const unsigned THR_BLOCKS_20MS = 2;
    const unsigned THR_BLOCKS_40MS = 4;

    unsigned priorInvalidCount = _invalidCount;

    // Always track the duration of invalid periods
    if (vscSymbol == 0) 
        _invalidCount++;
    else 
        _invalidCount = 0;

    if (_state == State::INVALID) {
        // Look for for the start of a potential DSC
        if (vscSymbol != 0) {
            if (priorInvalidCount >= THR_BLOCKS_40MS) {
                _state = State::PRE_DSC;
                _potentialSymbol = vscSymbol;
                _validCount = 1;
            }
        }
    }
    else if (_state == State::PRE_DSC) {
        // Still hearing the same symbol?
        if (vscSymbol == _potentialSymbol) {
            _validCount++;
            // Has the potential symbol persisted long enough 
            // to be detected?
            if (_validCount >= THR_BLOCKS_40MS) {
                _state = State::DSC;
                // Queue the detected symbol
                _isDSC = true;
                _detectedSymbol = vscSymbol;
            }
        }
        else {
            // If anything goes wrong during the pre-phase
            // then we go back to invalid and start trying again.
            _state = State::INVALID;
            _potentialSymbol = 0;
        }
    }   
    else if (_state == State::DSC) {
        // Look for a drop, which could be an invalid period
        // or another (different) valid symbol.
        if (vscSymbol != _potentialSymbol) {
            _state = State::DSC_DROP;
            _dropCount = 1;
        }
        // Otherwise just hang out here listening to the
        // detected symbol.
    }
    else if (_state == State::DSC_DROP) {
        // Check for recovery from drop
        if (vscSymbol == _potentialSymbol) {
            // Here we return to DSC **without** reporting
            // a detection (we already reported it).
            _state = State::DSC;
        }
        // If the drop didn't recover then check to see 
        // if we're past the point of recovery.  20ms is
        // the threshold in the specification but we're using
        // 24ms here.
        else {
            if (++_dropCount > THR_BLOCKS_20MS) {
                _state = State::INVALID;
                _potentialSymbol = 0;
            }
        }
    }
}

/**
 * A classic implementation of the Goertzel algorithm in fixed point.
 * 
 * @param coeff This is what controls the frequency that we are filtering
 * for. Notice that the coefficient is 32-bits and starts off 32,767 
 * higher in magnitude than the samples.
 *
 * @returns An "MS" magnitude of the signal at the designed frequency.
 * The final root in RMS is not performed for efficiency sake.
 * The value returned has the units of power rather than voltage.
 */
static int16_t computePower(int16_t* samples, uint32_t n, int32_t coeff) {

    int32_t vk_1 = 0, vk_2 = 0;

    // This is the amount that we down-shift the sample in order to 
    // preserve precision as we go through the Goertzel iterations.
    // This number very much depends on N, so pay close attention 
    // if N changes.
    const int sampleShift = 7;

    for (unsigned i = 0; i < n; i++) {        
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
    // ORGINAL
    //r >>= (15 + 15 - (sampleShift + sampleShift));
    // TODO: FIGURE OUT THIS EXTRA FACTOR OF TWO
    r >>= (14 + 15 - (sampleShift + sampleShift));
    return (int16_t)r;
}

char DTMFDetector2::_detectVSC(int16_t* samples, uint32_t n) {

    // Compute the power on the fundamental frequencies across rows
    // and columns.
    bool nonZeroFound = false;
    int16_t powerRow[4], powerCol[4];
    for (unsigned k = 0; k < 4; k++) {
        powerRow[k] = computePower(samples, n, coeffRow[k]);
        //printf("Row %d %f\n", k, sqrt((float)powerRow[k] / 32767.0));
        powerCol[k] = computePower(samples, n, coeffCol[k]);
        //printf("Col %d %f\n", k, sqrt((float)powerCol[k] / 32767.0));
        if (powerRow[k] > 0 || powerCol[k] > 0)
            nonZeroFound = true;
    }

    // This could happen in the case where a DC signal is sent in
    if (!nonZeroFound)
        return 0;

    // Find the maximum of the **combined** powers
    unsigned maxRow = 0, maxCol = 0;
    int32_t maxRowPower = 0, maxColPower = 0;
    for (unsigned r = 0; r < 4; r++) {
        int16_t rowPower = powerRow[r];
        if (rowPower > maxRowPower) {
            maxRowPower = rowPower;
            maxRow = r;
        }
    }
    for (unsigned c = 0; c < 4; c++) {
        int16_t colPower = powerCol[c];
        if (colPower > maxColPower) {
            maxColPower = colPower;
            maxCol = c;
        }
    }

    // Compute the power for the harmonic frequency of the potential
    // for each band. Note that this is "early" given that the data
    // isn't used until later, but we want to make the run-time for 
    // each processing cycle reasonably consistent/worse-case, so this
    // step gets moved early.
    int16_t maxRowHarmonicPower = computePower(samples, n, harmonicCoeffRow[maxRow]);
    int16_t maxColHarmonicPower = computePower(samples, n, harmonicCoeffCol[maxCol]);

    // Per TI app note: "the sum of row and column peak provides a better
    // parameter for signal strength than separate row and column checks."
    //
    // It is safe to sum these because they are all (Vrms)^2
    int32_t combPower = maxRowPower + maxColPower;
    //printf("Combined %f\n", sqrt(combPower / 32767.0));
    if (combPower < (int32_t)_signalThresholdPower) {
        //cout << "Below threshold" << endl;
        return 0;
    }

    // Per TI app note: "The spectral information can reflect two types of twists. 
    // The more likely one, called “reverse twist”, assumes the row peak to be 
    // larger than the column peak. Row frequencies (lower frequency band) are 
    // typically less attenuated as than column frequencies (higher frequency
    // band), assuming a low-pass filter type telephone line. The decoder computes 
    // therefore a reverse twist ratio and sets a threshold (THR_TWIREV) of 8dB 
    // acceptable reverse twist.
    //
    // In other words, we want to make sure that the row energy is not more
    // than +8dB above the column energy.
    //
    static const int16_t threshold8dB = std::pow(10, -8.0 / 10.0) * 32767.0;
    if (maxRowPower > maxColPower) {
        int16_t reverseTwistRatio = div2(maxColPower, maxRowPower);
        // INEQUALITY IS REVERSED BECAUSE WE ARE COMPARING 1/a to 1/b
        if (reverseTwistRatio < threshold8dB) {
            //cout << "Reverse twist problem" << endl;
            return 0;
        }
    }

    // The other twist, called “standard twist”, occurs when the row peak is 
    // smaller than the column peak. Similarly, a “standard twist ratio” is 
    // computed and its threshold (THR_TWISTD) is set to 4dB acceptable standard twist.
    //
    // In other words, we want to make sure that the column energy is not more
    // than +4dB above the row energy.
    //
    static const int16_t threshold4dB = std::pow(10, -4.0 / 10.0) * 32767.0;
    if (maxColPower > maxRowPower) {
        int16_t standardTwistRatio = div2(maxRowPower, maxColPower);
        // INEQUALITY IS REVERSED BECAUSE WE ARE COMPARING 1/a to 1/b
        if (standardTwistRatio < threshold4dB) {
            //cout << "Standard twist problem" << endl;
            return 0;
        }
    }

    // The program makes a comparison of spectral components within the row group 
    // as well as within the column group. The strongest component must stand out 
    // (in terms of squared amplitude) from its proximity tones within its group 
    // by more than a certain threshold ratio (THR_ROWREL, THR_COLREL).
    for (unsigned r = 0; r < 4; r++)
        if (r != maxRow) {
            // INEQUALITY IS REVERSED BECAUSE WE ARE COMPARING 1/a to 1/b
            int16_t r0 = div2(powerRow[r], maxRowPower);
            if (r0 > threshold8dB) {
                return 0;
            }
        }
    for (unsigned c = 0; c < 4; c++)
        if (c != maxCol)
            // INEQUALITY IS REVERSED BECAUSE WE ARE COMPARING 1/a to 1/b
            if (div2(powerCol[c], maxColPower) > threshold8dB) {
                return 0;
            }

    // Make sure the harmonics are -20dB down from the fundamentals
    // NOTE: Threshold is shifted down to avoid overflow
    static const int16_t threshold20dB = std::pow(10, -20.0 / 10.0) * 32767.0;
    // NOTE: When testing with the FT-65 (TX) and IC-2000H (RX) on 26-July-25
    // we noted a problem with this threshold check. A row harmonic that 
    // was only -16dB down.
    static const int16_t thresholdMinus16dB = std::pow(10, -16.0 / 10.0) * 32767.0;

    if (maxColHarmonicPower != 0 && 
        ((maxColHarmonicPower > maxColPower) ||
        (div2(maxColHarmonicPower, maxColPower) > threshold20dB))) {
        return 0;
    }

    if (maxRowHarmonicPower != 0) {
        if (maxRowHarmonicPower > maxRowPower) {
            return 0;
        }
        int16_t r0 = div2(maxRowHarmonicPower, maxRowPower);
        if (r0 > thresholdMinus16dB) {
            return 0;
        }
    }

    // Made it to a valid symbol!
    return symbolGrid[4 * maxRow + maxCol];
}

}
