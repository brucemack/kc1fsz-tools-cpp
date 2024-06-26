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
#ifndef _AudioOutputContext_h
#define _AudioOutputContext_h

#include <cstdint>

#include "kc1fsz-tools/Runnable.h"
#include "kc1fsz-tools/AudioProcessor.h"

namespace kc1fsz {

/**
 * An abstract interface for the ability to play a continuous stream of 
 * PCM audio.  We do this because there are completely different ways
 * of generating audio output on different platforms.
 */
class AudioOutputContext : public AudioProcessor, public Runnable {
public:

    AudioOutputContext(uint32_t frameSize, uint32_t samplesPerSecond) :
        _frameSize(frameSize),
        _samplesPerSecond(samplesPerSecond) { }
    virtual ~AudioOutputContext() { }

    /**
     * Clears any internal state/queues
    */
    virtual void reset() { }

    /**
     * This should be called continuously from the event loop, or at
     * least fast enough to keep up with the frame rate.
     */
    virtual void run() = 0;

    /**
     * Generates a CW tone
     */
    virtual void tone(uint32_t freq, uint32_t durationMs) { }

    /**
     * @returns The number of times that synchronization problems 
     *   have been detected since the last reset.
    */
    virtual uint32_t getSyncErrorCount() { return 0; }

    // ----- From AudioProcessor ----------------------------------------------

    /**
     * Loads the "next" frame of PMC audio to be played.  Be careful
     * to keep this running at the frame rate since the internal 
     * implementation may not necessarily have the ability to buffer
     * extra frames.
     * 
     * It is assumed that this is 16-bit audio, meaning that if there
     * are fewer than 16-bits of significance in the audio stream 
     * then those significant bits will be on the high (MSB) end 
     * of the 16-bit word and some zeroes may appear in teh LSB
     * end of the word.
     */   
    virtual bool play(const int16_t* frame, uint32_t frameSize) = 0;

protected:

    uint32_t _frameSize;
    uint32_t _samplesPerSecond;
};

}

#endif
