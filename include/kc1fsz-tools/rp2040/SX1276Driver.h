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
#ifndef _SX1276Driver_h
#define _SX1276Driver_h

#include <stdint.h>
#include "hardware/spi.h"

#include <kc1fsz-tools/Log.h>
#include <kc1fsz-tools/CircularBuffer.h>
#include <kc1fsz-tools/Clock.h>

namespace kc1fsz {

class SX1276Driver {

public:

    SX1276Driver(Log& log, Clock& clock, int reset_pin, int cs_pin, spi_inst_t* spi);

    // Called by interrupt pin
    void event_int();
    // This should be called every 50-100ms.
    void event_tick();
    // This should be called as quickly/often as possible
    void event_poll();

    /** 
     * Sets the radio frequency from a decimal value that is quoted in MHz.
     */
    void set_frequency(float freq_mhz);

    /**
     * Puts something on the send queue
     * @returns true if the message was queued, false if not (because of capacity issues)
     */ 
    bool send(const uint8_t* msg, uint32_t msg_len);

    bool isSendQueueEmpty() const { return _txBuffer.isEmpty(); }

    /**
     * @returns true if something was availble to be received.
     */
    bool popReceiveIfNotEmpty(void* oobBuf, void* buf, unsigned int* len);

private:

    void start_Tx();
    void start_Rx();
    void start_Cad();
    void start_Idle();
    void start_Restart();

    void event_TxDone(uint8_t irqFlags);
    void event_RxDone(uint8_t irqFlags);
    void event_CadDone(uint8_t irqFlags);
    void event_tick_Idle();
    void event_tick_Tx();
    void event_tick_Rx();
    void event_tick_Cad();
    int reset_radio();

    void set_mode_SLEEP();
    void set_mode_STDBY();
    void set_mode_TX();    
    void set_mode_RXCONTINUOUS();
    void set_mode_RXSINGLE();
    void set_mode_CAD();
    
    void enable_interrupt_TxDone();    
    void enable_interrupt_RxDone();
    void enable_interrupt_CadDone();
    void write_message(uint8_t* data, uint8_t len);
    void set_ocp(uint8_t current_ma);

    /**
     * @brief Should be called whenever LoRa parameters change.
     */
    void _setLowDataRateOptimize();

    uint8_t spi_read(uint8_t reg);

    /**
     * @param len must be <= 255 bytes!
     */
    void spi_read_multi(uint8_t reg, uint8_t* buf, uint8_t len);

    /**
     * @returns Whatever comes back during the second byte transfer.
     */
    uint8_t spi_write(uint8_t reg, uint8_t val);

    /**
     * @param len must be <= 255 bytes!
     * @returns Whatever comes back during the second byte transfer.
     */
    uint8_t spi_write_multi(uint8_t reg, const uint8_t* buf, uint8_t len);

    uint32_t _random(uint32_t from, uint32_t to);

    /**
     * Causes a sleep
     * @param sleep_ms The delay time in milliseconds
     */
    void _delay(int ms);

    Log& _log;
    Clock& _mainClock;
    int _resetPin;
    int _csPin;

    // The states of the state machine
    enum State { RESTART_STATE, IDLE_STATE, RX_STATE, TX_STATE, CAD_STATE };
    volatile State _state = State::RESTART_STATE;

    // This is volatile because it is set inside of the ISR context
    volatile bool _isrHit = false;

    // The time when the last receive was started.  Used to manage
    // timeouts.
    volatile uint32_t _startRxTime = 0;
    // The time when we started the last transmission.  This is needed 
    // to create a timeout on transmissions so we don't accidentally get 
    // stuck in a transmission.
    volatile uint32_t _startTxTime = 0;
    // The time when we started the last channel activity detection.
    // This is needed to manage timeouts.
    volatile uint32_t _startCadTime = 0;
    // The time when the last channel activity was seen.. Used to avoid
    // transmission when there is receive activity going on.
    volatile uint32_t _lastActivityTime = 0;

    // We keep a pretty small TX buffer because the main area where we keep 
    // outbound packets is in the MessageProcessor.
    CircularBufferImpl<256> _txBuffer;
    // There is a two-byte OOB allocation here for the RSSI data on receive
    CircularBufferImpl<2048> _rxBuffer;

    spi_inst_t* _spi;
};

}

#endif
