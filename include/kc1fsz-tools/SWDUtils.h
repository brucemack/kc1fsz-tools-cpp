#ifndef _SWDUtils_h
#define _SWDUtils_h

#include <cstdint>
#include "kc1fsz-tools/rp2040/SWDDriver.h"

namespace kc1fsz {

std::expected<uint32_t, int> call_function(SWDDriver& swd, 
    uint32_t trampoline_addr, uint32_t func_addr, 
    uint32_t a0, uint32_t a1, uint32_t a2, uint32_t a3);

int reset(SWDDriver& swd);

int reset_into_debug(SWDDriver& swd);

int flash_and_verify(SWDDriver& swd, uint32_t offset, const uint8_t* image_bin, uint32_t image_bin_len);

}

#endif
