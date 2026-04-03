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
#pragma once

#include <cstdint>

namespace kc1fsz {

class Log;
class Clock;

/**
 * A convenience class for managing the state in a state machine. Deals with 
 * optional state time-outs automatically. 
 */
class StateMachine {
public:

    /**
     * @param initialState The initial state and the state that is returned 
     * on on any call to reset().
     */
    StateMachine(Log& log, Clock& clock, int initialState);

    void reset();
    bool inState(int state) const;
    bool operator==(int state) const;
    void setState(int state);
    StateMachine& operator=(int state) { setState(state); return *this; }
    void setState(int state, unsigned timeoutMs, int timeoutState);
    void checkTimeout();
    int getState() const { return _state; }

private:

    Log& _log;
    Clock& _clock;
    const int _initialState;
    int _state;
    int _timeoutState;
    unsigned _timeoutMs;
    uint64_t _stateStartMs;
};

}
