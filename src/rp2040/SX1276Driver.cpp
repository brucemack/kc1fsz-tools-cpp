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
#include <cmath>
#include <cstring>

#include "pico/stdlib.h"
#include "hardware/gpio.h"

#include "kc1fsz-tools/rp2040/SX1276Driver.h"

#define SX1276_EXPECTED_VERSION (18)

// The time we will wait for a TxDone interrupt before giving up.  This should
// be an unusual case.
#define TX_TIMEOUT_MS (30UL * 1000UL)

// The time we will wait for a CadDone interrupt before giving up. 
#define CAD_TIMEOUT_MS (50UL)

// The time we wait for a RxDone interrupt before giving up and going 
// into idle mode.  This is done to avoid any bugs that might come
// up where we get completely stuck in the receive state.
#define RX_TIMEOUT_MS 60 * 1000

#define STATION_FREQUENCY (906.5)

namespace kc1fsz {

SX1276Driver::SX1276Driver(Log& log, Clock& clock, int reset_pin, int cs_pin, spi_inst_t* spi) 
:   _log(log),
    _mainClock(clock),
    _resetPin(reset_pin),
    _csPin(cs_pin),
    _txBuffer(0),
    _rxBuffer(2),
    _spi(spi) {
}

bool SX1276Driver::send(const uint8_t* msg, uint32_t msg_len) {
    return _txBuffer.push(0, msg, msg_len);
}

bool SX1276Driver::popReceiveIfNotEmpty(void* oobBuf, void* buf, unsigned int* len) {
    return _rxBuffer.popIfNotEmpty(oobBuf, buf, len);
}

/**
 * @brief This should be called by the ISR
 */
void SX1276Driver::event_int() {
    // NOTE: We've had so many problems with interrupt enable/disable on the ESP32
    // so now we're just going to set a flag and let everything happen in the loop()
    // context. This eliminates a lot of risk around concurrency, etc.
    _isrHit = true;
}

void SX1276Driver::start_Tx() {

    //_log.info("start_Tx");

    // At this point we have something pending to be sent.
    // Go into stand-by so we are allowed to fill the FIFO
    // (see page 34)
    set_mode_STDBY();

    // Pop the data off the TX queue into the transmit buffer.  
    unsigned int tx_buf_len = 256;
    uint8_t tx_buf[tx_buf_len];
    _txBuffer.pop(0, tx_buf, &tx_buf_len);

    // Move the data into the radio FIFO
    write_message(tx_buf, tx_buf_len);
    
    // Go into transmit mode
    _state = State::TX_STATE;
    _startTxTime = _mainClock.time();
    enable_interrupt_TxDone();
    set_mode_TX();
}

/**
 * @brief Put the radio into receive mode and enable the RxDone interrupt.
 */
void SX1276Driver::start_Rx() {
    _state = State::RX_STATE;
    _startRxTime = _mainClock.time();
    // Ask for interrupt when receiving
    enable_interrupt_RxDone();
    set_mode_RXCONTINUOUS();
}

/**
 * @brief Put the radio into CAD (channel activity detect) mode
 * and enable the CadDone interrupt.
 * 
 * NOTE: It appears that we don't get the CadDone interrupt under
 * normal (inactive) circumstances.  So a timeout is used to stop
 * the CAD process later.
 */
void SX1276Driver::start_Cad() {      
    //_log.info("start_Cad");
    _state = State::CAD_STATE;
    _startCadTime = _mainClock.time();
    enable_interrupt_CadDone();
    set_mode_CAD();  
}

void SX1276Driver::start_Idle() {
    _state = State::IDLE_STATE;
    set_mode_STDBY();
}

void SX1276Driver::start_Restart() {
    _state = State::RESTART_STATE;
}

/**
 * @brief This is called when the radio reports the end of a 
 * transmission sequence.
 * 
 * This gets called from the main processing loop.
 */
void SX1276Driver::event_TxDone(uint8_t irqFlags) {   

    // If we expected this event
    if (_state == State::TX_STATE) {
        //_log.info("event_TxDone");
        // After transmit the radio goes back to standby mode
        // automatically
        _state = State::IDLE_STATE;
    }
    // Unexpected event
    else {
        _log.error("Unexpected event_TxDone");
        start_Idle();
    }
} 

/**
 * @brief This is called when a complete message is received.
 * 
 * This gets called from the main processing loop.
 */
void SX1276Driver::event_RxDone(uint8_t irqFlags) {

    // Record that some activity was seen on the channel
    _lastActivityTime = _mainClock.time();

    // Expected
    if (_state == State::RX_STATE) {
        //_log.info("event_RxDone");
        // Make sure we don't have any errors
        if (irqFlags & 0x20) {
            _log.error("CRC error");
            // Message is ignored
        }
        else {
            // How much data is available? RxBytesNb
            const uint8_t len = spi_read(0x13);

            // We do nothing for zero-length messages
            if (len == 0) {
                return;
            }

            // Set the FIFO read pointer to the beginning of the 
            // packet we just got. FifoAddrPtr=FifoRxCurrentAddr.  
            spi_write(0x0d, spi_read(0x10));

            // Stream received data in from the FIFO. 
            uint8_t rx_buf[256];
            spi_read_multi(0x00, rx_buf, len);
            
            // Grab the RSSI value from the radio
            int8_t lastSnr = (int8_t)spi_read(0x19) / 4;
            int16_t lastRssi = spi_read(0x1a);
            if (lastSnr < 0)
                lastRssi = lastRssi + lastSnr;
            else
                lastRssi = (int)lastRssi * 16 / 15;
            // We are using the high frequency port
            lastRssi -= 157;

            // Put the RSSI (OOB) and the entire packet into the circular queue for 
            // later processing.
            _rxBuffer.push((const uint8_t*)&lastRssi, rx_buf, len);
        }
        // NOTE: Stay in RX state
    }
    // Unexpected 
    else {
        _log.error("Unexpected event_RxDone");
        start_Idle();
    }
}

/**
 * @brief This function is called when the radio completes a CAD cycle
 * and it is determined that these is channel activity. 
 */
void SX1276Driver::event_CadDone(uint8_t irqFlags) {

    // Expected
    if (_state == State::CAD_STATE) {
        //_log.info("event_CadDone");
        // This is the case where activity was detected
        if (irqFlags & 0x01) {
            _lastActivityTime = _mainClock.time();
            // Radio goes back to standby automatically after CAD
            _state = State::IDLE_STATE;
        }
    }
    // Unexpected
    else {
        _log.error("Unexpected event_CadDone");
        start_Idle();
    }
}

/**
 * @brief This function gets called from inside of the main processing loop.  It looks
 * to see if any interrupt activity has been detected and, if so, figure 
 * out what kind of interrupt was reported and call the correct handler.
 */ 
void SX1276Driver::event_poll() {

    // Look at the flag that gets set by the ISR itself
    if (!_isrHit) {
        return;
    }
    
    //_log.info("Radio interrupt");

    // *******************************************************************************
    // Critical Section:
    // Here we make sure that the clearing of the isr_hit flag and the unloading 
    // of the pending interrupts in the radio's IRQ register happen atomically.
    // We are avoding the case where 
    //disable_interrupts();

    _isrHit = false;

    // Read and reset the IRQ register at the same time:
    uint8_t irq_flags = spi_write(0x12, 0xff);    
    
    // We saw a comment in another implementation that said that there are problems
    // clearing the ISR sometimes.  Notice we do a logical OR so we don't loose anything.
    irq_flags |= spi_write(0x12, 0xff);    

    //enable_interrupts();
    // *******************************************************************************
    
    // RxDone 
    if (irq_flags & 0x40) {
        event_RxDone(irq_flags);
    }
    // TxDone
    if (irq_flags & 0x08) {
        event_TxDone(irq_flags);
    }
    // CadDone
    if (irq_flags & 0x04) {
        event_CadDone(irq_flags);
    }
}

void SX1276Driver::event_tick_Idle() {

    // Make sure the radio is in the state that we expect
    uint8_t state = spi_read(0x01);  
    if (state != 0x81) {
        _log.error("Radio in unexpected state (%d), resetting", (int)state);
        start_Restart();
    }
    else {   
        // Check to see if there is data waiting to go out
        if (!_txBuffer.isEmpty()) {
            // Launch a CAD check to see if the channel is clear before transmitting
            start_Cad(); 
        } 
        else {
            start_Rx();
        }
    }
}

void SX1276Driver::event_tick_Tx() {
    // Check for the case where a transmission times out
    if (_mainClock.time() - _startTxTime > TX_TIMEOUT_MS) {
        _log.error("TX time out");
        start_Idle();
    }
}

void SX1276Driver::event_tick_Rx() {

    // Check for pending transmissions.  If nothing is pending then 
    // return without any state change.
    if (!_txBuffer.isEmpty()) {
        // At this point we know there is something pending.  We first 
        // go into CAD mode to make sure the channel is innactive.
        // A successful CAD check (with no detection) will trigger 
        // the transmission.
        start_Cad();
    }

    // Check for the case where a receive times out
    if (_mainClock.time() - _startRxTime > RX_TIMEOUT_MS) {
        start_Idle();
    }
}

void SX1276Driver::event_tick_Cad() {
    // Check for the case where a CAD check times out.  A random timeout is 
    // used to try to reduce collisions
    //if ((_mainClock.time() - _startCadTime) > (CAD_TIMEOUT_MS * _random(1, 3))) {
    // TODO: GET THE RANDOMNESS BACK
    if ((_mainClock.time() - _startCadTime) > (CAD_TIMEOUT_MS * 2)) {
        // If a CAD times out then that means that it is safe 
        // to transmit. 
        if (!_txBuffer.isEmpty()) {
            start_Tx();
        } else {
            start_Idle();
        }
    }
}

// Call periodically to look for timeouts or other pending activity.  
// This will happen on the regular application thread.
void SX1276Driver::event_tick() {
    if (_state == State::RX_STATE) {
        event_tick_Rx();
    } else if (_state == State::TX_STATE) {
        event_tick_Tx();
    } else if (_state == State::CAD_STATE) {
        event_tick_Cad();
    } else if (_state == State::IDLE_STATE) {
        event_tick_Idle(); 
    } else if (_state == State::RESTART_STATE) {
        reset_radio();
    }
}

// --------------------------------------------------------------------------
// Radio Utilty Functions
// 
// This reference will be very important to you:
// https://www.hoperf.com/data/upload/portal/20190730/RFM95W-V2.0.pdf
// 
// The LoRa register map starts on page 103.

void SX1276Driver::set_mode_SLEEP() {
    spi_write(0x01, 0x00);  
}

void SX1276Driver::set_mode_STDBY() {
    spi_write(0x01, 0x01);  
}

void SX1276Driver::set_mode_TX() {
    spi_write(0x01, 0x03);
}

void SX1276Driver::set_mode_RXCONTINUOUS() {
    spi_write(0x01, 0x05);
}

void SX1276Driver::set_mode_RXSINGLE() {
    spi_write(0x01, 0x06);
}

void SX1276Driver::set_mode_CAD() {
    spi_write(0x01, 0x07);
}

// See table 17 - DIO0 is controlled by bits 7-6
void SX1276Driver::enable_interrupt_TxDone() {
    spi_write(0x40, 0x40);
}

// See table 17 - DIO0 is controlled by bits 7-6
void SX1276Driver::enable_interrupt_RxDone() {
    spi_write(0x40, 0x00);
}

// See table 17 - DIO0 is controlled by bits 7-6
void SX1276Driver::enable_interrupt_CadDone() {
    spi_write(0x40, 0x80);
}

// See page 103
void SX1276Driver::set_frequency(float freq_mhz) {
    const float CRYSTAL_MHZ = 32000000.0;
    const float FREQ_STEP = (CRYSTAL_MHZ / 524288);
    const uint32_t f = (freq_mhz * 1000000.0) / FREQ_STEP;
    spi_write(0x06, (f >> 16) & 0xff);
    spi_write(0x07, (f >> 8) & 0xff);
    spi_write(0x08, f & 0xff);
}

void SX1276Driver::write_message(uint8_t* data, uint8_t len) {
    // Move pointer to the start of the FIFO
    spi_write(0x0d, 0);
    // The message
    spi_write_multi(0x00, data, len);
    // Update the length register
    spi_write(0x22, len);
}

int SX1276Driver::reset_radio() {

    // Hard reset
    gpio_set_dir(_resetPin, GPIO_OUT);
    gpio_put(_resetPin, 0);
    _delay(5);
    gpio_put(_resetPin, 1);
    // Float the reset pin?  This was done in the ESP32 version
    // but appears not to be necessary in the RP2040 version.
    //gpio_set_dir(_resetPin, GPIO_IN);
    // Per datasheet, wait 5ms after reset
    _delay(5);
    // Not sure if this is really needed:
    _delay(250);

    // Check the radio version to make sure things are connected
    // properly.
    uint8_t ver = spi_read(0x42);
    if (ver == 0) {
        _log.error("No radio found");
        return -1;
    } else if (ver < SX1276_EXPECTED_VERSION) {
        _log.error("Version mismatch: %d", ver);
        return -1;
    }

    // Switch into Sleep mode, LoRa mode
    spi_write(0x01, 0x80);

    // Wait for sleep mode 
    _delay(10); 

    // Make sure we are actually in sleep mode
    if (spi_read(0x01) != 0x80) {
        return -2; 
    }

    // Setup the FIFO pointers
    // TX base:
    spi_write(0x0e, 0);
    // RX base:
    spi_write(0x0f, 0);

    // Set frequency
    set_frequency(STATION_FREQUENCY);

    // Set LNA boost
    spi_write(0x0c, spi_read(0x0c) | 0x03);

    // AgcAutoOn=LNA gain set by AGC
    spi_write(0x26, 0x04);

    // DAC enable (adds 3dB)
    spi_write(0x4d, 0x87);

    // Turn on PA and set power to +20dB
    // PaSelect=1
    // OutputPower=17 (20 - 3dB from DAC)
    spi_write(0x09, 0x80 | ((20 - 3) - 2));

    // Set OCP to 140 (as per the Sandeep Mistry library)
    // #### TODO: INVESTIGATE THIS SETTING
    set_ocp(140);

    // Go into stand-by
    set_mode_STDBY();

    // Configure the LoRa channel parameters
    uint8_t reg = 0;

    // 7-4:   0111  (125k BW)
    // 3-1:   001   (4/5 coding rate)
    // 0:     0     (Explicit header mode)
    reg = 0b01110010;
    spi_write(0x1d, reg);

    // 7-4:   1001  (512 chips/symbol, spreading factor 9)
    // 3:     0     (TX continuous mode normal)
    // 2:     1     (CRC mode on)
    // 1-0:   00    (RX timeout MSB) 
    reg = 0b10010100;
    spi_write(0x1e, reg);

    _setLowDataRateOptimize();

    start_Idle();

    //_log.info("Radio initialized");

    return 0;  
}

/**
 * @brief Sets the Over Current Protection register.
 * 
 * @param current_ma 
 */
void SX1276Driver::set_ocp(uint8_t current_ma) {

    uint8_t trim = 27;
    if (current_ma <= 120) {
        trim = (current_ma - 45) / 5;
    } else if (current_ma <= 240) {
        trim = (current_ma + 30) / 10;
    }
    spi_write(0x0b, 0x20 | (0x1F & trim));
}

/**
 * Called after changing LoRa channel parameters.
 * 
 * Semtech modem design guide AN1200.13 (page 7-8) says:
 * 
 * "To avoid issues surrounding drift of the crystal reference oscillator due
 * to either temperature change or motion, the low data rate optimization bit 
 * is used. Specifically for 125 kHz bandwidth and SF=11 and 12,  
 * this adds a small overhead to increase robustness to reference frequency 
 * variations over the timescale of the LoRa packet."
 * 
 * In the Semtech SX1276 datasheet this bit is called the "LowDataRateOptimize" bit
 * and the instructions state that it is mandatory when the symbol length exceeds 16ms.
 */
void SX1276Driver::_setLowDataRateOptimize() {

    // Read current value for BW and SF from radio
    uint8_t bw_idx = spi_read(0x1d) >> 4;
    uint8_t sf = spi_read(0x1e) >> 4;
    const uint32_t bw_tab[] = { 7800, 10400, 15600, 20800, 31250, 41700, 62500, 125000, 
                                250000, 500000 };
    if (bw_idx > 9)
        return;
    uint32_t bw = bw_tab[bw_idx];
    // Calculate symbol period per Semtech AN1200.22 section 4, page 9-10.
    uint32_t symbol_period_ms = (1000 * (1 << sf)) / bw;

    //_log.info("Bandwidth %u, sf %d, symbol ms %u", bw, (int)sf, symbol_period_ms);

    uint8_t reg_modem_config_3 = spi_read(0x26);
    if (symbol_period_ms > 16.0)
        spi_write(0x26, reg_modem_config_3 | 0x08);
    else
        spi_write(0x26, reg_modem_config_3 & ~0x08);   
}

// ----- SPI Glue Code --------------------------------------------------------

uint8_t SX1276Driver::spi_read(uint8_t reg) {
    // Setup the address in the first outbound byte, second outbound is 
    // meaningless.  Make sure the R/W bit is 0.
    const uint8_t out_workarea[2] = { (uint8_t)(reg & ~0x80), 0 };
    uint8_t in_workarea[2];
    gpio_put(_csPin, 0);
    spi_write_read_blocking(_spi, out_workarea, in_workarea, 2);
    gpio_put(_csPin, 1);
    return in_workarea[1];
}

void SX1276Driver::spi_read_multi(uint8_t reg, uint8_t* buf, uint8_t len) {
    assert(len <= 255);
    // Setup the address in the first outbound byte.  Make sure the R/W bit is 0.
    // The rest of the oubound data is meaningless.
    const uint8_t out_workarea[256] = { (uint8_t)(reg & ~0x80) };
    uint8_t in_workarea[256];
    gpio_put(_csPin, 0);
    spi_write_read_blocking(_spi, out_workarea, in_workarea, len + 1);
    gpio_put(_csPin, 1);
    memcpy(buf, in_workarea + 1, len);
}

uint8_t SX1276Driver::spi_write(uint8_t reg, uint8_t val) {
    // Setup the address in the first outbound byte, and the data in 
    // the second outbound. Make sure the R/W bit is 1.
    const uint8_t out_workarea[2] = { (uint8_t)(reg | 0x80), val };
    uint8_t in_workarea[2];
    gpio_put(_csPin, 0);
    spi_write_read_blocking(_spi, out_workarea, in_workarea, 2);
    gpio_put(_csPin, 1);
    return in_workarea[1];
}

uint8_t SX1276Driver::spi_write_multi(uint8_t reg, const uint8_t* buf, uint8_t len) {
    assert(len <= 255);
    // Setup the address in the first outbound byte.  Make sure the R/W bit is 0.
    // The rest of the oubound data is meaningless.
    uint8_t out_workarea[256] = { (uint8_t)(reg | 0x80) };
    memcpy(out_workarea + 1, buf, len);
    uint8_t in_workarea[256];
    gpio_put(_csPin, 0);
    spi_write_read_blocking(_spi, out_workarea, in_workarea, len + 1);
    gpio_put(_csPin, 1);
    return in_workarea[1];
}

// ----- Other Glue -----------------------------------------------------------

void SX1276Driver::_delay(int ms) {
    sleep_ms(ms);
}

}
