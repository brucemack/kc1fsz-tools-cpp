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
#ifndef _DTMFDetector2_h
#define _DTMFDetector2_h

#include <cstdint>

namespace kc1fsz {

class Clock;

/**
 * An instance of this class is needed because there is some state 
 * involved in capturing and de-bouncing the DTMF detection.
 *
 * NOTE: This only works for 8K sample rates at the moment.
 */
class DTMFDetector2 {
public:

    DTMFDetector2(Clock& clock, unsigned blockSize = 64);

    /**
     * @param block 256 input (8ms) samples in 32-bit signed PCM format.
     * @return An indication of whether a valid symbol has been detected.
     * TODO: CHANGE BACK TO FIXED POINT!
     */
    void processBlock(const float* block);

    /**
     * @return True if a valid detected symbol is availble to be fetched
     * via the popDetection() method.
     */
    bool isDetectionPending() const { return _isDSC; }

    /**
     * @return Pops the queue with the oldest detected symbol, or returns
     * zero if there has not been a detection.
     */
    char popDetection() { 
        if (_isDSC) {
            _isDSC = false; 
            return _detectedSymbol; 
        } else {
            return 0;
        }
    }

    /**
     *
     * The DTMF activity needs to exceed this threshold to even be
     * considered valid.
     */
    void setSignalThreshold(float dbfs);

    float getDiagValue() const { return _diagValue; }

private:

    /*
    * @brief Indicates which valid symbol (if any) is in the block.
    * @return 0 for noise/silence, otherwise the character that is valid.
    */
    char _detectVSC(int16_t* samples, uint32_t N);

    static constexpr int16_t freqRow[4] = { 697, 770, 852, 941 };
    // This is 2 * cos(2 * PI * fk / fs) for each of the 8 frequencies
    static const int32_t coeffRow[4];
    static constexpr int16_t freqCol[4] = { 1209, 1336, 1477, 1633 };
    // This is 2 * cos(2 * PI * fk / fs) for each of the 8 frequencies
    static const int32_t coeffCol[4];
    // This is 2 * cos(2 * PI * (2 * fk) / fs) for each of the 8 frequencies.
    // Used for checking second-order harmonics.
    static const int32_t harmonicCoeffRow[4];
    // This is 2 * cos(2 * PI * (2 * fk) / fs) for each of the 8 frequencies.
    // Used for checking second-order harmonics.
    static const int32_t harmonicCoeffCol[4];
    
    static const char symbolGrid[4 * 4];

    static const unsigned FS = 8000;
    static const unsigned N3 = 136;

    enum State { INVALID, PRE_DSC, DSC, DSC_DROP } _state =
        State::INVALID;

    Clock& _clock;
    unsigned _blockSize;

    // Set the RMS threshold in Q15 format, but squared so that it can be
    // compared to other powers.
    int16_t _signalThresholdPower;
    // This is where the last three blocks of N samples is stored for processing
    int16_t _history[N3];
    // If we are not in a valid symbol, how long has the invalid period lasted?
    unsigned _invalidCount = 0;
    // If we are in a valid symbol, how long has valid symbol lasted?
    unsigned _validCount = 0;
    // What is the valid symbol that we are attempting to detect?
    char _potentialSymbol = 0;
    // Used to track the period with a valid, but incompatible symbol
    unsigned _dropCount = 0;
    // Was a symbol detected?
    bool _isDSC = false;
    // What was the detected symbol that we saw?
    char _detectedSymbol = 0;

    uint32_t _lastVscTime = 0;
    float _diagValue;
};

}

#endif
