/**
 * Copyright (C) 2026, Bruce MacKinnon KC1FSZ
 *
 * THIS IS PROPRIETARY/COMMERCIAL SOFTWARE.
 */
#pragma once

#include <functional>

namespace kc1fsz {

class W5500Driver {
public:

    using writePinCb = std::function<void(bool state)>;

    W5500Driver(writePinCb writeResetPin, writePinCb writeSelectPin);

    void init();

private:

    writePinCb _writeResetPin;
    writePinCb _writeSelectPin;
};

}
