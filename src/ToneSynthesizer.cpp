#include <cstdio>
#include <cmath>
#include "kc1fsz-tools/ToneSynthesizer.h"

namespace kc1fsz {

static const float PI2 = 2.0 * 3.14159265359;

ToneSynthesizer::ToneSynthesizer(float fsHz, float envelopeMs) 
:   _fsHz(fsHz),
    _envCount((fsHz * envelopeMs) / 1000.0) {
}

void ToneSynthesizer::setFreq(float freqHz) {
    _mode = Mode::TONE;
    _omega = PI2 * freqHz / _fsHz;
}

void ToneSynthesizer::setPcm(const short* pcm, unsigned int pcmLength, unsigned int rateHz) {
    _mode = Mode::PCM;
    _pcmData = pcm;
    _pcmDataLen = pcmLength;
    _pcmDataRateHz = rateHz;
    _pcmDataPtr = 0;
    _pcmDataMod = 0;
}

void ToneSynthesizer::setEnabled(bool on) {
    if (on) {
        if (_state == State::SILENT || _state == State::RAMP_DOWN) {
            _envPtr = 0;
            _state = State::RAMP_UP;
        }
        // Otherwise, we're already ramping up or ramped up
    } else {
        if (_state == State::RAMP_UP || _state == State::ACTIVE) {
            _envPtr = 0;
            _state = State::RAMP_DOWN;
        }
        // Otherwise, we're already ramping down or ramped down
    }
}

float ToneSynthesizer::getSample() {
    if (_state == State::SILENT) {
        return 0.0;
    }
    else {
        
        // Manage the envelope to avoid sharp discontinuities
        float scale = 1.0;
        if (_state == State::RAMP_UP) {
            scale = (float)_envPtr / (float)_envCount;
            _envPtr++;
            // Look for end of ramp
            if (_envPtr == _envCount) {
                _state = State::ACTIVE;
            }
        }
        else if (_state == State::RAMP_DOWN) {
            scale = (float)(_envCount - _envPtr) / (float)_envCount;
            _envPtr++;
            // Look for end of ramp
            if (_envPtr == _envCount) {
                _state = State::SILENT;
            }
        }

        float sample;
        
        if (_mode == Mode::TONE) {
            sample = _sin(_phi);
            _phi += _omega;
            // This is needed to avoid strange wrapping issues that 
            // occur with the phase.
            _phi = fmod(_phi, PI2);
        } 
        else if (_mode == Mode::PCM) {
            const unsigned int w = _fsHz / _pcmDataRateHz;
            // TODO: DO SOME BETTER DECIMATION!
            // 16-bit PCM to float
            float a0 = (float)_pcmData[_pcmDataPtr] / 32766.0;
            sample = a0 * 3.0;
            _pcmDataMod++;
            if (_pcmDataMod == w) {
                _pcmDataMod = 0;
                // Don't go off the end
                if (_pcmDataPtr + 1 < _pcmDataLen)
                    _pcmDataPtr++;
            }
        }

        return sample * scale;
    }    
}

float ToneSynthesizer::_sin(float phase_rad) const {
    return std::sin(phase_rad);
}

}

