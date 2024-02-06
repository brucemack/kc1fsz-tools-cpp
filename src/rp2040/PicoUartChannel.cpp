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
    uint8_t* writeBuffer, uint32_t writeBufferSize)
:   _uart(uart),
    _irq((_uart == uart0) ? UART0_IRQ : UART1_IRQ),
    _readBuffer(readBuffer),
    _readBufferSize(readBufferSize),
    _readBufferUsed(0),
    _writeBuffer(writeBuffer),
    _writeBufferSize(writeBufferSize),
    _writeBufferUsed(0),
    _isrCountRead(0),
    _isrCountWrite(0),
    _isrLoopWrite(0),
    _readBytesLost(0) {      

    critical_section_init_with_lock_num(&_sectRead, 0);
    critical_section_init_with_lock_num(&_sectWrite, 1);

    // Install handler
    _INSTANCE = this;
    irq_set_exclusive_handler(_irq, _ISR);
    // We always want the write to be enabled at least
    uart_set_irq_enables(_uart, true, false);
    irq_set_enabled(_irq, true);
    _writeIntEnabled = false;
}

PicoUartChannel::~PicoUartChannel() {
    critical_section_deinit(&_sectRead);
    critical_section_deinit(&_sectWrite);
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
    const_cast<PicoUartChannel*>(this)->_lockWrite();
    uint32_t r = _writeBufferSize - _writeBufferUsed;
    const_cast<PicoUartChannel*>(this)->_unlockWrite();
    return r;
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
        if (moveSize < _readBufferUsed)
            memcpy(_readBuffer, _readBuffer + moveSize, _readBufferUsed);
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

    // This has the effect of disabling interrupts
    _lockWrite();

    // Memory sync
    __dsb();

    // Figure out how much we can take
    uint32_t moveSize = std::min(bufLen, _writeBufferSize - _writeBufferUsed);
    if (moveSize > 0) {
        // Take the caller's data
        memcpy(_writeBuffer + _writeBufferUsed, buf, moveSize);
        _writeBufferUsed += moveSize;
    }

    // Memory sync
    __dsb();

    // Try to make immediate progress on the send
    _writeISR();

    // Read is always enabled, but figure out if the write 
    // needs to be scheduled as well
    //_checkISRStatus();

    // This has the effect of re-enabling interrupts
    _unlockWrite();

    return moveSize;
}

bool PicoUartChannel::poll() {
    _lockWrite();
    _checkISRStatus();
    _unlockWrite();
    return true;
}

void PicoUartChannel::_checkISRStatus() {

    uint32_t used = _writeBufferUsed;

    // Read is always enabled, but figure out if the write 
    // needs to be scheduled as well
    if (used > 0) {
        uart_set_irq_enables(_uart, true, true);
        _writeIntEnabled = true;
    } else {
        uart_set_irq_enables(_uart, true, false);
        _writeIntEnabled = false;
    }
}

void PicoUartChannel::_ISR() {
    _INSTANCE->_readISR();
    _INSTANCE->_writeISR();
}

// IMPORTANT: There is an assumption here that interrupts are disable
// while working inside of the ISR.
void PicoUartChannel::_readISR() {

    // Keep reading until we can't
    while (uart_is_readable(_uart)) {

        // When the ISR fires, the only way to clear it is to consume
        // the byte out of the UART/FIFO.
        char c = uart_getc(_uart);

        _isrCountRead++;

        // Check to see if we're at capacity on the read buffer.
        // If the read buffer is full then we can assume that 
        // there's a good chance that we are losing bytes here.
        if (_readBufferUsed == _readBufferSize) {
            _readBytesLost++;
            break;
        } 
        else {
            _readBuffer[_readBufferUsed++] = c;
        }

    }
}

// IMPORTANT: There is an assumption here that interrupts are disable
// while working inside of the ISR.
void PicoUartChannel::_writeISR() {

    // Memory sync
    __dsb();
    
    _isrCountWrite++;

    uint32_t moveSize = 0;

    while (moveSize < _writeBufferUsed) {
        if (uart_is_writable(_uart)) {
            _isrLoopWrite++;
            char c = (char)_writeBuffer[moveSize++];
            uart_putc(_uart, c);
        } else {
            break;
        }
    }

    _writeBufferUsed -= moveSize;

    if (moveSize > 0) {
        // Shift remaining data (if any) down to the start of the buffer
        if (_writeBufferUsed > 0) {
            for (unsigned int i = 0; i < _writeBufferUsed; i++) 
                _writeBuffer[i] = _writeBuffer[i + moveSize];
            // DANGER: OVERLAPPING!
            //memcpy(_writeBuffer, _writeBuffer + moveSize, _writeBufferUsed);
        }
    }

    // Memory sync
    __dsb();
}

void PicoUartChannel::_lockRead() {
    critical_section_enter_blocking(&_sectRead);
}

void PicoUartChannel::_unlockRead() {
    critical_section_exit(&_sectRead);
}

void PicoUartChannel::_lockWrite() {
    critical_section_enter_blocking(&_sectWrite);
}

void PicoUartChannel::_unlockWrite() {
    critical_section_exit(&_sectWrite);
}

}
