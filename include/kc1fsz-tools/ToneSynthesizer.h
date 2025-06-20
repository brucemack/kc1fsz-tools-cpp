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
#ifndef _ToneSynthesizer_h
#define _ToneSynthesizer_h

namespace kc1fsz {

class ToneSynthesizer {
public:

    /**
     * @param envelopeMs The length of the shaping envelope in milliseconds.
     */
    ToneSynthesizer(float fsHz, float envelopeMs);

    void setEnabled(bool on);
    float getSample();
    bool isActive() { return _state != State::SILENT; }

    void setFreq(float freqHz);
    void setPcm(const short* pcm, unsigned int pcmLength, unsigned int rateHz);

protected:

    /**
     * @brief Allows an override of the standard trig function in case of 
     * performance concerns.
     */
    virtual float _sin(float phase_rad) const;

private:

    const float _fsHz;
    const unsigned int _envCount;

    enum State { SILENT, RAMP_UP, RAMP_DOWN, ACTIVE };
    State _state = State::SILENT;

    enum Mode { TONE, PCM };
    Mode _mode = Mode::TONE;

    // Internal state for tone generations
    unsigned int _envPtr = 0;
    float _omega = 0;
    float _phi = 0;

    // Internal state for PCM
    const short* _pcmData;
    unsigned int _pcmDataLen;
    unsigned int _pcmDataRateHz;
    unsigned int _pcmDataPtr = 0;
    unsigned int _pcmDataMod = 0;
};

}

#endif
