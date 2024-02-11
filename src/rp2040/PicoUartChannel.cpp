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

int PicoUartChannel::traceLevel = 0;

PicoUartChannel* PicoUartChannel::_INSTANCE = 0;

PicoUartChannel::PicoUartChannel(uart_inst_t* uart,
    uint8_t* readBuffer, uint32_t readBufferSize, 
    uint8_t* txBuffer, uint32_t txBufferSize)
:   _uart(uart),
    _irq((_uart == uart0) ? UART0_IRQ : UART1_IRQ),
    _readBuffer(readBuffer),
    _readBufferSize(readBufferSize),
    _readBufferUsed(0),
    _txBuffer(txBuffer),
    _txBufferSize(txBufferSize),
    _txBufferWriteCount(0),
    _txBufferSentCount(0) {

    critical_section_init_with_lock_num(&_sectRead, 0);

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
    critical_section_deinit(&_sectRead);
}

void PicoUartChannel::resetCounters() {
    _isrCountRead = 0;
    _bytesReceived = 0;
    _readBytesLost= 0;      
}

bool PicoUartChannel::PicoUartChannel::isReadable() const {
    return (bytesReadable() > 0);
}

bool PicoUartChannel::PicoUartChannel::isWritable() const {
    return (bytesWritable() > 0);
}

uint32_t PicoUartChannel::bytesReadable() const {
    const_cast<PicoUartChannel*>(this)->_lockRead();
    uint32_t r = _readBufferUsed;
    const_cast<PicoUartChannel*>(this)->_unlockRead();
    return r;
}

uint32_t PicoUartChannel::bytesWritable() const {
    // NOTE: Wrap-around case is addressed on each increment
    return _txBufferWriteCount - _txBufferSentCount;
}

uint32_t PicoUartChannel::read(uint8_t* buf, uint32_t bufCapacity) {

    _lockRead();

    // Memory sync
    __dsb();

    // Figure out how much we can give
    uint32_t moveSize = std::min(bufCapacity, _readBufferUsed);
    if (moveSize > 0) {
        // Give the caller their data
        memcpy(buf, _readBuffer, moveSize);
        _readBufferUsed -= moveSize;
        // Shift any remaining data back to the start (should not be the norm)
        if (moveSize < _readBufferUsed) {
            //memcpy(_readBuffer, _readBuffer + moveSize, _readBufferUsed);
            for (uint32_t i = 0; i < _readBufferUsed; i++) {
                _readBuffer[i] = _readBuffer[moveSize + i];
            }
        }
    }

    _unlockRead();

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
        panic("Logic error 1");
    }

    // We immediately buffer all outbound data, regardless of the UART 
    // status.  The actual sending happens from poll() in all cases.
    // Figure out how much space is available in the TX buffer
    uint32_t used = _txBufferWriteCount - _txBufferSentCount;
    // Sanity check
    if (used >= _txBufferSize) {
        panic("Logic error 2");
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
        // get away from this tricky edge case.
        if (_txBufferWriteCount == 0) {
            _txBufferWriteCount += (_txBufferSize * 2);
            _txBufferSentCount += (_txBufferSize * 2);
        }
    }

    return i;
}

bool PicoUartChannel::poll() {

    // Santity check - the sending should never get ahead
    if (_txBufferWriteCount < _txBufferSentCount) {
        panic("Logic error 1");
    }

    // Write as much as possible
    uint32_t moveSize = 0;
    while (_txBufferWriteCount > _txBufferSentCount && 
        uart_is_writable(_uart)) {
        // TODO: Switch to mask
        uint32_t slot = _txBufferSentCount % _txBufferSize;
        uart_putc(_uart, (char)_txBuffer[slot]);
        moveSize++;
        _txBufferSentCount++;
        // EDGE CASE: If the counter just wrapped to zero then 
        // shift both the write counter and the send counter to 
        // get away from this tricky edge case.
        if (_txBufferSentCount == 0) {
            _txBufferWriteCount += (_txBufferSize * 2);
            _txBufferSentCount += (_txBufferSize * 2);
        }
    }

    return moveSize > 0;
}

// TODO: __not_in_flash_func()
void PicoUartChannel::_ISR() {
    _INSTANCE->_readISR();
}

// IMPORTANT: There is an assumption here that interrupts are disabled
// while working inside of the ISR.
void PicoUartChannel::_readISR() {

    _isrCountRead++;

    // Keep reading until we can't
    while (uart_is_readable(_uart)) {

        // When the ISR fires, the only way to clear it is to consume
        // the byte out of the UART/FIFO.
        char c = uart_getc(_uart);

        // Check to see if we're at capacity on the read buffer.
        // If the read buffer is full then we can assume that 
        // there's a good chance that we are losing bytes here.
        if (_readBufferUsed == _readBufferSize) {
            _readBytesLost++;
            break;
        } 
        else {
            _bytesReceived++;
            _readBuffer[_readBufferUsed++] = c;
        }
    }
}

void PicoUartChannel::_lockRead() {
    critical_section_enter_blocking(&_sectRead);
}

void PicoUartChannel::_unlockRead() {
    critical_section_exit(&_sectRead);
}

}
