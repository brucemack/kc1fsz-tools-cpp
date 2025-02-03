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
 * 
 * For more information about the DTMF receive standard please see 
 * ETSI ES 201 235-3 V1.1.1 (2002-03) section 4.2.2.  
 * 
 * https://www.etsi.org/deliver/etsi_es/201200_201299/20123503/01.02.01_50/es_20123503v010201m.pdf
 * 
 */
#ifndef _DTMFDetector_h
#define _DTMFDetector_h

#include <cstdint>

#include "kc1fsz-tools/AudioProcessor.h"

namespace kc1fsz {

class DTMFDetector : public AudioProcessor {
public:

    /**
     * NOTE: At the moment we only support a sample rate of 8,000 Hz.
     */
    DTMFDetector(uint32_t sampleRateHz);

    void reset();

    /**
     * @returns true if there is a DTMF symbol available to be
     * dequeued.
     */
    bool isAvailable() const;

    /**
     * @returns Pulls the next symbol from the decode queue and
     * returns it.  Use resultsAvailable() to determine if there 
     * is anything waiting to be pulled.
     * 
     * Symbols are returned in the order that they are detected.
     */
    char pullResult();

    // ----- From AudioProcessor ----------------------------------------------

    /**
     * NOTE: Assumes PCM-16 (signed) at the moment
     * NOTE: Only supports 160 or 4x160 size frames at the moment.
     * NOTE: Assumes 8 kHz audio at the moment.
     */
    bool play(const int16_t* frame, uint32_t frameLen);

private:

    void _processFrame(const int16_t* frame, uint32_t frameLen);

    void _processShortBlock(const int16_t* frame, uint32_t frameLen);

    // Runs the DFT detectiopn on a small block of signal and looks for 
    // a "valid signal condition" (VSC).  NOTE: This is not the same as
    // a "detected signal condition" (DSC).
    static char _detectVSC(int16_t* samples, uint32_t N);

    const uint32_t _sampleRate;
    // Numer of samples in the DFT block (17ms of data)
    static const uint32_t N = 136;    

    // VSC history
    static const uint32_t _vscHistSize = 8;
    char _vscHist[_vscHistSize];
    // Indicates whether we are currently in the middle of a valid
    // detection.
    bool _inDSC = false;
    char _detectedSymbol = 0;
    // The good results
    static const uint32_t _resultSize = 16;
    char _result[_resultSize];
    uint32_t _resultLen;
};

}

#endif
