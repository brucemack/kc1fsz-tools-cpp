/**
 * Copyright (C) 2026, Bruce MacKinnon KC1FSZ
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
 */
#include "kc1fsz-tools/Clock.h"
#include "kc1fsz-tools/Log.h"
#include "kc1fsz-tools/StateMachine.h"

namespace kc1fsz {

StateMachine::StateMachine(Log& log, Clock& clock, int initialState)
:   _log(log), _clock(clock), _initialState(initialState), _timeoutMs(0) {
}

void StateMachine::reset() {
    _state = _initialState;
    _timeoutMs = 0;
}

bool StateMachine::inState(int state) const {
    const_cast<StateMachine*>(this)->checkTimeout();
    return _state == state;
}

bool StateMachine::operator==(int state) const {
    return inState(state);
}

void StateMachine::setState(int state) {
    _state = state;
    _stateStartMs = _clock.timeMs();
    _timeoutMs = 0;
    _timeoutState = 0;
}

void StateMachine::setState(int state, unsigned timeoutMs, int timeoutState) {
    _state = state;
    _stateStartMs = _clock.timeMs();
    _timeoutMs = timeoutMs;
    _timeoutState = timeoutState;
}

void StateMachine::checkTimeout() {
    if (_timeoutMs != 0 && _clock.isPastWindow(_stateStartMs, _timeoutMs)) {
        setState(_timeoutState);
    }
}

}
