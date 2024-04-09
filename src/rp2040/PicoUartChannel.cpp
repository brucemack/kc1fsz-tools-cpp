/**
 * Copyright (C) 2024, Bruce MacKinnon KC1FSZ
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
#include <iostream>
#include <cstring>

#include <pico/stdlib.h>
#include <hardware/uart.h>
#include <hardware/irq.h>
#include <hardware/sync.h>

#include "kc1fsz-tools/Common.h"
#include "kc1fsz-tools/rp2040/PicoUartChannel.h"

using namespace std;

namespace kc1fsz {

static const char* LOGIC_ERROR = "LOGIC ERROR";

int PicoUartChannel::traceLevel = 0;

PicoUartChannel* PicoUartChannel::_INSTANCE = 0;

PicoUartChannel::PicoUartChannel(uart_inst_t* uart,
    uint8_t* readBuffer, uint32_t readBufferSize, 
    uint8_t* txBuffer, uint32_t txBufferSize)
:   _uart(uart),
    _irq((_uart == uart0) ? UART0_IRQ : UART1_IRQ),
    _rxBuffer(readBuffer),
    _rxBufferSize(readBufferSize),
    _rxBufferRecvCount(0),
    _rxBufferReadCount(0),
    _txBuffer(txBuffer),
    _txBufferSize(txBufferSize),
    _txBufferWriteCount(0),
    _txBufferSentCount(0) {

    // Install handler
    _INSTANCE = this;
    irq_set_exclusive_handler(_irq, _ISR);

    // We always want the read to be enabled.  The TX interrupt
    // is not used.
    uart_set_irq_enables(_uart, true, false);
    irq_set_enabled(_irq, true);

    resetCounters();
}

PicoUartChannel::~PicoUartChannel() {
}

void PicoUartChannel::resetCounters() {
    _isrCountRead = 0;
    _rxBytesLost= 0;      
}

bool PicoUartChannel::PicoUartChannel::isReadable() const {
    return (bytesReadable() > 0);
}

bool PicoUartChannel::PicoUartChannel::isWritable() const {
    return (bytesWritable() > 0);
}

uint32_t PicoUartChannel::bytesReadable() const {
    // NOTE: Wrap-around case is addressed on each increment
    uint32_t used = _rxBufferRecvCount - _rxBufferReadCount;
    // Sanity check
    if (used >= _rxBufferSize) {
        panic(LOGIC_ERROR);
    }
    return used;
}

uint32_t PicoUartChannel::bytesWritable() const {
    // NOTE: Wrap-around case is addressed on each increment
    uint32_t used = _txBufferWriteCount - _txBufferSentCount;
    // Sanity check
    if (used >= _txBufferSize) {
        panic(LOGIC_ERROR);
    }
    uint32_t available = _txBufferSize - used - 1;
    return available;
}

uint32_t PicoUartChannel::read(uint8_t* buf, uint32_t bufCapacity) {

    // Memory sync
    __dsb();

    uint32_t startRxBufferRecvCount = _rxBufferRecvCount;
    uint32_t moveSize = 0;

    while (moveSize < bufCapacity && startRxBufferRecvCount > _rxBufferReadCount) {
        // TODO: Switch to mask
        uint32_t slot = _rxBufferReadCount % _rxBufferSize;
        buf[moveSize++] = _rxBuffer[slot];
        _rxBufferReadCount++;
        // Look for wrap
        if (_rxBufferReadCount == 0) {
            // TODO: CONSIDER A CRTIICAL SECTION SINCE BOTH OF THESE
            // NEED TO MOVE IN TANDEM
            _rxBufferReadCount += (_rxBufferSize * 2);
            _rxBufferRecvCount += (_rxBufferSize * 2);
        }
    }

    if (traceLevel > 0) {
        cout << "PicoUartChannel::read()" << endl;
        prettyHexDump(buf, moveSize, cout);
    }

    return moveSize;
}

uint32_t PicoUartChannel::write(const uint8_t* buf, uint32_t bufLen) {

    if (traceLevel > 0) {
        cout << "PicoUartChannel::write()" << endl;
        prettyHexDump(buf, bufLen, cout);
    }

    // Santity check - the sending should never get ahead
    if (_txBufferWriteCount < _txBufferSentCount) {
        panic(LOGIC_ERROR);
    }

    // We immediately buffer all outbound data, regardless of the UART 
    // status.  The actual sending happens from poll() in all cases.
    // Figure out how much space is available in the TX buffer
    uint32_t used = _txBufferWriteCount - _txBufferSentCount;
    // Sanity check
    if (used >= _txBufferSize) {
        panic(LOGIC_ERROR);
    }
    uint32_t available = _txBufferSize - used - 1;
    // Move as much as possible
    uint32_t i;
    for (i = 0; i < bufLen && i < available; i++) {
        // TODO: Switch to mask
        uint32_t slot = _txBufferWriteCount % _txBufferSize;
        _txBuffer[slot] = buf[i];
        _txBufferWriteCount++;
        // EDGE CASE: If the counter just wrapped to zero then 
        // shift both the write counter and the sent counter to 
        // get away from this tricky edge case. (rare)
        if (_txBufferWriteCount == 0) {
            // TODO: CONSIDER CRITICAL SECTION?
            _txBufferWriteCount += (_txBufferSize * 2);
            _txBufferSentCount += (_txBufferSize * 2);
        }
    }

    return i;
}

void PicoUartChannel::run() {

    // Santity check - the sending should never get ahead
    if (_txBufferWriteCount < _txBufferSentCount) {
        panic(LOGIC_ERROR);
    }

    // Send as much as possible
    while (_txBufferWriteCount > _txBufferSentCount && 
        uart_is_writable(_uart)) {
        // TODO: Switch to mask
        uint32_t slot = _txBufferSentCount % _txBufferSize;
        uart_putc(_uart, (char)_txBuffer[slot]);
        _txBufferSentCount++;
        // EDGE CASE: If the counter just wrapped to zero then 
        // shift both the write counter and the send counter to 
        // get away from this tricky edge case. (rare)
        if (_txBufferSentCount == 0) {
            // TODOD: CONSIDER CRITICAL SECTION TO KEEP THESE IN SYNC
            _txBufferWriteCount += (_txBufferSize * 2);
            _txBufferSentCount += (_txBufferSize * 2);
        }
    }
}

// TODO: __not_in_flash_func()
void PicoUartChannel::_ISR() {
    _INSTANCE->_readISR();
}

// IMPORTANT: There is an assumption here that interrupts are disabled
// while working inside of the ISR. So nothing else is changing.
void PicoUartChannel::_readISR() {

    // Memory sync
    __dsb();

    _isrCountRead++;

    // We are very conservative here and only grab one byte at a time.  This
    // is done to yield back as agressively as possible.
    //if (uart_is_readable(_uart)) {
    // Keep reading until we can't
    while (uart_is_readable(_uart)) {

        // When the ISR fires, the only way to clear it is to consume
        // the byte out of the UART/FIFO.
        char c = uart_getc(_uart);

        // Check to see if we're at capacity on the read buffer.
        // If the read buffer is full then we can assume that 
        // there's a good chance that we are losing bytes here.

        uint32_t used = _rxBufferRecvCount - _rxBufferReadCount;
        // Sanity check
        if (used >= _rxBufferSize) {
            panic(LOGIC_ERROR);
        }

        uint32_t available = _rxBufferSize - used - 1;

        if (available == 0) {
            _rxBytesLost++;
            return;
        } 

        // TODO: Switch to mask for speed
        uint32_t slot = _rxBufferRecvCount % _rxBufferSize;
        _rxBuffer[slot] = c;
        _rxBufferRecvCount++;

        // Look for wrap-around of the counter (rare)
        if (_rxBufferRecvCount == 0) {
            // No lock needed since we are in an ISR
            _rxBufferReadCount += (_rxBufferSize * 2);
            _rxBufferRecvCount += (_rxBufferSize * 2);
        }
    }
}

}
