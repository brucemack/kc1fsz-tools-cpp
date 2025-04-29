#ifndef _SWDDriver_h
#define _SWDDriver_h

#include <cstdint>
#include <expected>

#define ARM_DCRSR (0xE000EDF4)
#define ARM_DCRDR (0xE000EDF8) 
#define ARM_DHCSR (0xe000edf0)
#define ARM_DHCSR_S_REGRDY (0x00010000)

namespace kc1fsz {

class SWDDriver {

public:

    SWDDriver(unsigned int clock_pin, unsigned int dio_pin);

    void init();

    /**
     * Goes through the initial line rest and initialization process.
     * @returns 0 on a good handshake, -1 on an error
     */
    int connect();

    uint32_t getAPID() const { return _apId; }

    // NOTE: Slave captures outbound data on the rising edge of the clock
    void writeBit(bool b) const;
    // NOTE: Master captures inbound data on the falling edge of the clock
    bool readBit() const;

    // Pattern is made up of "0", "1", and "_" (ignored)
    void writeBitPattern(const char* pattern);

    /**
     * DIO high, CLK low for 64 cycles
     */
    void writeLineReset();

    void writeSelectionAlert();
    void writeActivationCode();

    /**
     * @param addr 4-bit address, but [1:0] are always zero
     * @param ignoreAck When true, the slave-generated acknowledgement is ignored. This
     * is most relevant in the case where a target is being established in a multi-drop
     * system (i.e. the slaves don't assert an ACK in this case).SELECTION_ALERT
     */
    int writeDP(uint8_t addr, uint32_t data, bool ignoreAck = false) {
        return _write(false, addr, data, ignoreAck);
    }

    int writeAP(uint8_t addr, uint32_t data) {
        return _write(true, addr, data);
    }

    std::expected<uint32_t, int> readDP(uint8_t addr) {
        return _read(false, addr);
    }

    std::expected<uint32_t, int> readAP(uint8_t addr) {
        return _read(true, addr);
    }

    // ------ Convenience Functions -------------------------------------------

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
    int writeWordViaAP(uint32_t addr, uint32_t data);

   /** 
     * IMPORTANT: This function assumes that the CSW has been 
     * configured for a 4-byte transfer and for auto-increment.
     * 
     * NOTE: Refer to the ARM IHI 0031A document in section 8.2.2.
     * "Automatic address increment is only guaranteed to operate 
     * on the bottom 10-bits of the address held in the TAR."
     */
   int writeMultiWordViaAP(uint32_t start_addr, const uint32_t* data, 
        unsigned int word_count);

    /**
     * IMPORTANT: This function assumes that the appropriate AP
     * and AP register bank 0 have already been selected via a 
     * previous DP SELECT call.  This function does not do those
     * steps in order to save time.
     * 
     * IMPORTANT: This function assumes that the CSW has been 
     * configured for a 4-byte transfer.
     */
    std::expected<uint32_t, int> readWordViaAP(uint32_t addr);

    /**
     * IMPORTANT: This function assumes that the appropriate AP
     * and AP register bank 0 have already been selected via a 
     * previous DP SELECT call.  This function does not do those
     * steps in order to save time.
     * 
     * IMPORTANT: This function assumes that the CSW has been 
     * configured for a 2-byte transfer.
     */
    std::expected<uint16_t, int> readHalfWordViaAP(uint32_t addr);

    /**
     * Polls the S_REGRDY bit of the DHCSR register to find out whether
     * a core register read/write has completed successfully.
     */
    int pollREGRDY(unsigned int timeoutUs = 0);

    void setDIO(bool h) { _setDIO(h); }

protected:

    int _write(bool isAP, uint8_t addr, uint32_t data, bool ignoreAck = false);
    std::expected<uint32_t, int> _read(bool isAP, uint8_t addr);

    void _setCLK(bool h) const;
    void _setDIO(bool h) const;
    bool _getDIO() const;
    void _holdDIO();
    void _releaseDIO();
    void _delayPeriod() const;

private:

    unsigned int _clkPin;
    unsigned int _dioPin;
    uint32_t _apId;
};

}

#endif
