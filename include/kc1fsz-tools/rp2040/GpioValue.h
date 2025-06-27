/**
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
#ifndef _GpioValue_h
#define _GpioValue_h

#include <hardware/gpio.h>
#include "kc1fsz-tools/BinaryWrapper.h"

namespace kc1fsz {

class GpioValue : public BinaryWrapper {
public:

    GpioValue(int pin) 
    :   _pin(pin) { }

    bool get() const { return (gpio_get(_pin) == 1) ^ _activeLow; }

    void setActiveLow(bool b) { _activeLow = b; }

private:

    const int _pin;
    bool _activeLow = false;
};

}

#endif
