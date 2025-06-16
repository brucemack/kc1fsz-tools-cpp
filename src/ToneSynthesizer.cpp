#include <cmath>
#include "kc1fsz-tools/ToneSynthesizer.h"

namespace kc1fsz {

ToneSynthesizer::ToneSynthesizer(float fsHz, float envelopeMs) 
:   _fsHz(fsHz),
    _envCount((fsHz * envelopeMs) / 1000.0) {
}

void ToneSynthesizer::setFreq(float freqHz) {
    _omega = 2.0 * 3.14159265359 * freqHz / _fsHz;
}

void ToneSynthesizer::setEnabled(bool on) {
    if (on) {
        if (_state == State::SILENT || _state == State::RAMP_DOWN) {
            _envPtr = 0;
            _state = State::RAMP_UP;
        }
        // Otherwise, we're already ramping up or ramped up
    } else {
        if (_state == State::RAMP_UP || _state == State::TONE) {
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
        float a = _sin(_phi);
        _phi += _omega;
        float scale = 1.0;
        if (_state == State::RAMP_UP) {
            scale = (float)_envPtr / (float)_envCount;
            _envPtr++;
            // Look for end of ramp
            if (_envPtr == _envCount) {
                _state = State::TONE;
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
        return a * scale;
    }    
}

float ToneSynthesizer::_sin(float phase_rad) const {
    return std::sin(phase_rad);
}

}

