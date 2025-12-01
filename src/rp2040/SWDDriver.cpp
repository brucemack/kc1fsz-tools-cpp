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
 */
#include <stdio.h>

#include "hardware/gpio.h"
#include "pico/time.h"

#include "kc1fsz-tools/rp2040/SWDDriver.h"

namespace kc1fsz {

static const char* SELECTION_ALERT = "0100_1001_1100_1111_1001_0000_0100_0110_1010_1001_1011_0100_1010_0001_0110_0001_"
                                     "1001_0111_1111_0101_1011_1011_1100_0111_0100_0101_0111_0000_0011_1101_1001_1000";

//static const char* ACTIVATION_CODE = "0000_0101_1000_1111";
static const char* ACTIVATION_CODE = "0101_1000";

#define DAP_ADDR_CORE0 (0x01002927)

SWDDriver::SWDDriver(unsigned int clock_pin, unsigned int dio_pin) {
    _clkPin = clock_pin;
    _dioPin = dio_pin;
}

void SWDDriver::init() {

    gpio_set_dir(_clkPin, GPIO_OUT);        
    gpio_put(_clkPin, 0);

    gpio_set_dir(_dioPin, GPIO_OUT);        
    gpio_put(_dioPin, 0);
    gpio_set_pulls(_dioPin, false, false);        
}

int SWDDriver::connect() {   

    setDIO(false);
    writeBitPattern("11111111");

    // For ease of tracing
    _delayPeriod();

    // From "Low Pin-count Debug Interfaces for Multi-device Systems"
    // by Michael Williams
    //
    // "Hence the designers of multi-drop SWD chose an unlikely
    // data sequence approach. The selection message consists of a
    // 128-bit selection alert, followed by a protocol selection
    // command. This selection alert method has been adopted by
    // IEEE 1149.7, and multi-drop SWD has adopted the IEEE
    // 1149.7 protocol selection command, ensuring compatibility
    // between the two protocols."

    // Selection alert (128 bits)
    writeSelectionAlert();

    // RP2350 datasheet: 
    writeBitPattern("0000");

    // ARM Coresight activation code (0x1a, LSB first)
    writeActivationCode();

    // Here is where we start to follow the protocol described
    // in ARM IHI0031A section 5.4.1.
    //
    // More specifically, this from IHI0031G section B4.3.4:
    //
    // B4.3.4 Target selection protocol, SWD protocol version 2
    // ---------------------------------------------------------
    // 1. Perform a line reset. See Figure B4-9 on page B4-124.
    // 2. Write to DP register 0xC, TARGETSEL, where the data indicates 
    //    the selected target. The target response must be ignored. See Figure 
    //    B4-9 on page B4-124.
    // 3. Read from the DP register 0x0, DPIDR, to verify that the target 
    //    has been successfully selected.

    // Line reset
    writeLineReset();

    // Testing verified that this is required
    writeBitPattern("00000000");

    // For ease of tracing
    //_delayPeriod();

    // ===============================================================
    // NOTE: This part is only relevant to the RP2040.  The RP2350
    // doesn't use a multi-drop DP architecture.
    // ===============================================================

    // DP TARGETSEL, DP for Core 0.  We ignore the ACK here!
    // At this point there are multiple cores listening on the 
    // SWD bus (i.e. multi-drop) so this selection determines 
    // which one is activated. This is the reason that there 
    // is no ACK - we don't want the cores interfering with each 
    // other's response.
    //
    // Some RP2040-specific commentary (from the datasheet):
    //
    // Debug access is via independent DAPs (one per core) attached to a 
    // shared multidrop SWD bus (SWD v2). Each DAP will only respond to 
    // debug commands if correctly addressed by a SWD TARGETSEL command; 
    // all others tristate their outputs.  Additionally, a Rescue DP (see 
    // Section 2.3.4.2) is available which is connected to system control 
    // features. Default addresses of each debug port are given below:
    //
    // For RP2040 we use:
    //
    //   Core 0   : 0x01002927
    //   Core 1   : 0x11002927
    //   Rescue DP: 0xf1002927
    // 
    // The Instance IDs (top 4 bits of ID above) can be changed via a sysconfig 
    // register which may be useful in a multichip application. However note 
    // that ID=0xf is reserved for the internal Rescue DP (see Section 2.3.4.2).
    //

    // Needed for RP2040
    if (const auto r = writeDP(0b1100, DAP_ADDR_CORE0, true); r != 0) 
        return -1;

    // ===============================================================
    // RP2350 NOTE: In the next few reads/writes we are assuming that 
    // register address 0x0 is the DPIDR register and register address 
    // 0x4 is the CTRL/STAT register.  This is consistent with 
    // DPBANKSEL=0x0 as per the DP SELECT register described in section
    // B2.2.11 in the ADIv6.0 specification document.
    // ===============================================================

    // Read from the ID CODE register
    if (const auto r = readDP(0x0); r.has_value()) {
        // Good outcome
        printf("IDCODE is %08X\n", *r);
    }
    else {
        printf("IDCODE ERROR %d\n", r.error());
        return -2;
    }

    // Abort (in case anything is in process)
    if (const auto r = writeDP(0x0, 0x0000001e); r != 0)
        return -3;

    //const uint32_t ap_base = 0x00002000;

    // For RP2040:
    //
    // Set AP and DP bank 0
    // (Not completely sure why this is needed)
    if (const auto r = writeDP(0b1000, 0x00000000); r != 0)
        return -4;

    // Write into CTRL/STAT to power up.
    // Power up CSYSPWRUPREQ=1, CDBGPWRUPREQ=1, and ORUNDETECT=1
    if (const auto r = writeDP(0x4, 0x50000001); r != 0)
        return -5;

    // TODO: POLLING FOR POWER UP NEEDED?

    // Read DP CTRL/SEL and check for CSYSPWRUPACK and CDBGPWRUPACK
    if (const auto r = readDP(0x4); !r.has_value()) {
        return -6;
    } else {
        // TODO: MAKE SURE THIS STILL WORKS
        if (!((*r & 0x80000000) && (*r & 0x20000000)))
            return -7;
    }

    // For RP2040:
    //
    // DP SELECT - Set AP bank F, AP 0
    // [31:24] AP Select
    // [7:4]   AP Bank Select (the active four-word bank)
    if (const auto r = writeDP(0x8, 0x000000f0); r != 0)
        return -8;

    // For RP2350:
    //
    // DP SELECT - Set AP bank x02000 + 0xf0
    //if (const auto r = writeDP(0x8, ap_base | 0xf0); r != 0)
    //    return -8;

    // Read AP addr 0xFC. [7:4] bank address set previously, [3:0] set here.
    // 0xFC is the AP identification register.
    // The actual data comes back during the DP RDBUFF read
    if (const auto r = readAP(0xc); !r.has_value())
        return -9;
    // DP RDBUFF - Read AP result
    if (const auto r = readDP(0xc); !r.has_value()) {
        return -10;
    } else {
        _apId = *r;
    }

    // Leave the debug system in the proper state (post-connect)

    // For RP2040:
    //
    // DP SELECT - Set AP 0, AP bank 0, and DP bank 0
    if (const auto r = writeDP(0x8, 0x00000000); r != 0)
        return -11;

    // For RP2350
    //
    // DP SELECT - Set AP 0, AP bank 0, and DP bank 0
    //if (const auto r = writeDP(0x8, ap_base); r != 0)
    //    return -11;

    // Write to the AP Control/Status Word (CSW), auto-increment, 32-bit 
    // transfer size.
    //
    // 1010_0010_0000_0000_0001_0010
    // 
    // [5:4] 01  : Auto Increment set to "Increment Single," which increments by the size of the access.
    // [2:0] 010 : Size of the access to perform, which is 32 bits in this case. 
    //
    if (const auto r = writeAP(0b0000, 0x22000012); r != 0)
        return -12;

    return 0;
}

/**
 * Writes a 32-bit word into the processor memory space via the MEM-AP
 * mechanism.  This involves seting the AP TAR register first and then 
 * writing to the AP DRW register.
 * 
 * @param addr The processor address
 * @param data The data to write
 * @returns 0 on success.
 * 
 * IMPORTANT: This function assumes that the appropriate AP
 * and AP register bank 0 have already been selected via a 
 * previous DP SELECT call.  This function does not do those
 * steps in order to save time.
 * 
 * IMPORTANT: This function assumes that the CSW has been 
 * configured for a 4-byte transfer.
 */
int SWDDriver::writeWordViaAP(uint32_t addr, uint32_t data) {
    // Write to the AP TAR register. This is the memory address that we will 
    // be reading/writing from/to.
    if (const auto r = writeAP(0x4, addr); r != 0)
        return r;
    // Write to the AP DRW register
    if (const auto r = writeAP(0xc, data); r != 0)
        return r;
    return 0;
}

   /** 
     * IMPORTANT: This function assumes that the CSW has been 
     * configured for a 4-byte transfer and for auto-increment.
     * 
     * NOTE: Refer to the ARM IHI 0031A document in section 8.2.2.
     * "Automatic address increment is only guaranteed to operate 
     * on the bottom 10-bits of the address held in the TAR."
     */
   int SWDDriver::writeMultiWordViaAP(uint32_t start_addr, const uint32_t* data, 
        unsigned int word_count) {
        
        uint32_t addr = start_addr, last_tar_addr = 0;
        const uint32_t TEN_BITS = 0b1111111111;

        for (unsigned int i = 0; i < word_count; i++) {

            if (i == 0 || (addr & TEN_BITS) != (last_tar_addr & TEN_BITS)) {
                // Write to the AP TAR register. This is the starting memory 
                // address that we will be reading/writing from/to. Since
                // auto-increment is enabled this only needs to happen when 
                // we cross the 10-bit boundaries
                if (const auto r = writeAP(0x4, addr); r != 0)
                    return r;
                last_tar_addr = addr;
            }

            // Write to the AP DRW register
            if (const auto r = writeAP(0xc, *(data + i)); r != 0)
                return r;

            addr += 4;
        }

        return 0;
    }

    /**
     * IMPORTANT: This function assumes that the appropriate AP
     * and AP register bank 0 have already been selected via a 
     * previous DP SELECT call.  This function does not do those
     * steps in order to save time.
     * 
     * IMPORTANT: This function assumes that the CSW has been 
     * configured for a 4-byte transfer.
     */
    std::expected<uint32_t, int> SWDDriver::readWordViaAP(uint32_t addr) {

        // Write to the AP TAR register. This is the memory address that we will 
        // be reading/writing from/to.
        if (const auto r = writeAP(0x4, addr); r != 0) {
            return std::unexpected(r);
        }
        // Read from the AP DRW register (actual data is buffered and comes later)
        if (const auto r = readAP(0xc); !r.has_value()) {
            return std::unexpected(r.error());
        }
        // Fetch result of previous AP read
        if (const auto r = readDP(0xc); !r.has_value()) {
            return std::unexpected(r.error());
        } else {
            return *r;
        }
    }

std::expected<uint16_t, int> SWDDriver::readHalfWordViaAP(uint32_t addr) {

    // Write to the AP TAR register. This is the memory address that we will 
    // be reading from. Notice that the read is forced to be word-aligned by 
    // masking off the bottom two bits.
    if (const auto r = writeAP(0x4, addr & 0xfffffffc); r != 0) {
        return std::unexpected(r);
    }
    // Read from the AP DRW register (actual data is buffered in the DP and 
    // comes in the next step)
    if (const auto r = readAP(0xc); !r.has_value()) {
        return std::unexpected(r.error());
    }
    // Fetch result of previous AP read from the DP READBUF register. Remember, 
    // this is a full 32-bit word!
    if (const auto r = readDP(0xc); !r.has_value()) {
        return std::unexpected(r.error());
    } else {
        // For the even half-words (i.e. word boundary) just return the 
        // 16 least-significant bits of the word.
        if ((addr & 0x3) == 0)
            return (*r & 0xffff);
        // For the odd half-words return the 16 most significant bits of
        // of the word.
        else
            return (*r >> 16) & 0xffff;
    }
}

/**
 * Polls the S_REGRDY bit of the DHCSR register to find out whether
 * a core register read/write has completed successfully.
 */
int SWDDriver::pollREGRDY(unsigned int timeoutUs) {
    while (true) {
        const auto r = readWordViaAP(ARM_DHCSR);
        if (!r.has_value())
            return -1;
        if (*r & ARM_DHCSR_S_REGRDY)
            return 0;
    }
}

std::expected<uint32_t, int> SWDDriver::_read(bool isAP, uint8_t addr) {

    // Parity calculation. The only variable bits are the address and 
    // the DP/AP flag. Start with read flag.
    unsigned int ones = 1;
    if (addr & 0b0100)
        ones++;
    if (addr & 0b1000)
        ones++;
    if (isAP)
        ones++;

    // Start bit
    writeBit(true);
    // 0=DP, 1=AP
    writeBit(isAP);
    // 1=Read
    writeBit(true);
    // Address[3:2] (LSB first)
    writeBit((addr & 0b0100) != 0);
    writeBit((addr & 0b1000) != 0);
    // This system uses even parity, so an extra one should be 
    // added only if the rest of the count is odd.
    writeBit((ones % 2) == 1);
    // Stop bit
    writeBit(false);
    // Park bit
    writeBit(true);

    // Release the DIO pin so the slave can drive it
    _releaseDIO();
    // One cycle turnaround 
    readBit();

    // Read three acknowledgment bits (LSB first)
    uint8_t ack = 0;
    if (readBit()) ack |= 1;
    if (readBit()) ack |= 2;
    if (readBit()) ack |= 4;

    // 0b001 is OK
    if (ack != 0b001) {
        // TODO: DECIDE HOW TO DEAL WITH THIS
        _holdDIO();
        return std::unexpected(-1);
    }

    // Read data, LSB first
    uint32_t data = 0;
    ones = 0;
    for (unsigned int i = 0; i < 32; i++) {
        bool bit = readBit();
        ones += (bit) ? 1 : 0;
        data = data >> 1;
        data |= (bit) ? 0x80000000 : 0;
    }

    // Read parity
    bool parity = readBit();
    if (ones % 2 == 1 && !parity)
        return std::unexpected(-2);

    // Grab the DIO pin back again so that we can drive it
    _holdDIO();
    // One cycle turnaround 
    writeBit(false);

    return data;
}

int SWDDriver::_write(bool isAP, uint8_t addr, uint32_t data, bool ignoreAck) {

    // The only variable bits are the address and the DP/AP flag
    unsigned int ones = 0;
    if (addr & 0b0100)
        ones++;
    if (addr & 0b1000)
        ones++;
    if (isAP)
        ones++;

    // Start bit
    writeBit(true);
    // 0=DP, 1=AP
    writeBit(isAP);
    // 0=Write
    writeBit(false);
    // Address[3:2] (LSB first)
    writeBit((addr & 0b0100) != 0);
    writeBit((addr & 0b1000) != 0);
    // This system uses even parity, so an extra one should be 
    // added only if the rest of the count is odd.
    writeBit((ones % 2) == 1);
    // Stop bit
    writeBit(false);
    // Park bit: drive the DIO high and leave it there
    writeBit(true);

    // Let go of the DIO pin so that the slave can drive it
    _releaseDIO();    
    // One cycle turnaround 
    readBit();

    // Read three acknowledgement bits (LSB first)
    uint8_t ack = 0;
    if (readBit()) ack |= 1;
    if (readBit()) ack |= 2;
    if (readBit()) ack |= 4;

    // Grab the DIO pin back so that we can drive
    _holdDIO();
    // One cycle turnaround 
    writeBit(false);

    // 001 is OK
    if (!ignoreAck) {
        if (ack != 0b001) {
            return -1;
        }
    }

    // Write data, LSB first
    ones = 0;
    for (unsigned int i = 0; i < 32; i++) {
        bool bit = (data & 1) == 1;
        ones += (bit) ? 1 : 0;
        writeBit(bit);
        data = data >> 1;
    }

    // Write parity in order to make the one count even
    writeBit((ones % 2) == 1);

    return 0;
}

void SWDDriver::writeBitPattern(const char* pattern) {
    const char* b = pattern;
    while (*b != 0) {
        if (*b == '0') {
            writeBit(false);
        } else if (*b == '1') {
            writeBit(true);
        }
        b++;
    }
}

void SWDDriver::writeLineReset() {
    for (unsigned int i = 0; i < 64; i++)
        writeBit(true);
}

void SWDDriver::writeSelectionAlert() {
    writeBitPattern(SELECTION_ALERT);
}

void SWDDriver::writeActivationCode() {
    writeBitPattern(ACTIVATION_CODE);
}

void SWDDriver::writeBit(bool b) const {
    // Assert the outbound bit to the slave on the SWDIO data pin
    _setDIO(b);
    // Wait about 1uS for setup.
    _delayPeriod();
    // Raise the SWLCK clock pin. Slave will capture the data on this rising edge.
    _setCLK(true);
    // Wait around 1uS for hold
    _delayPeriod();
    // Lower the SWCLK clock pin.
    _setCLK(false);
}

bool SWDDriver::readBit() const {
    // The inbound data is already asserted by the slave on the previous falling 
    // clock edge.  Wait about 1uS for setup.
    _delayPeriod();
    // Read the value from the SWDIO data pin
    bool r = _getDIO();
    // Raise the SWCLK clock pin.
    _setCLK(true);
    // Wait about 1uS.
    _delayPeriod();
    // Lower the SWCLK clock pin.
    _setCLK(false);
    return r;
}

void SWDDriver::_setCLK(bool h) const {
    gpio_put(_clkPin, h ? 1 : 0);
}

void SWDDriver::_setDIO(bool h) const {
    gpio_put(_dioPin, h ? 1 : 0);    
}

bool SWDDriver::_getDIO() const {
    return gpio_get(_dioPin) == 1;
}

void SWDDriver::_holdDIO() {
    gpio_set_dir(_dioPin, GPIO_OUT);                
}

void SWDDriver::_releaseDIO() {
    gpio_set_pulls(_dioPin, false, false);        
    gpio_set_dir(_dioPin, GPIO_IN);                
}

void SWDDriver::_delayPeriod() const {
    sleep_us(1);
}

void SWDDriver::delayMs(unsigned int ms) {
    sleep_ms(ms);
}

}
