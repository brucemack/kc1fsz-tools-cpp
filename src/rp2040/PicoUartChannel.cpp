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

#include "kc1fsz-tools/rp2040/PicoUartChannel.h"

using namespace std;

namespace kc1fsz {

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
    _isrCount(0) {      
    // Install handler
    _INSTANCE = this;
    irq_set_exclusive_handler(_irq, _ISR);
    irq_set_enabled(_irq, true);
}

PicoUartChannel::~PicoUartChannel() {
}

bool PicoUartChannel::PicoUartChannel::isReadable() const {
    return (bytesReadable() > 0);
}

bool PicoUartChannel::PicoUartChannel::isWritable() const {
    return (bytesWritable() > 0);
}

uint32_t PicoUartChannel::bytesReadable() const {
    const_cast<PicoUartChannel*>(this)->_lock();
    uint32_t r = _readBufferUsed;
    const_cast<PicoUartChannel*>(this)->_unlock();
    return r;
}

uint32_t PicoUartChannel::bytesWritable() const {
    const_cast<PicoUartChannel*>(this)->_lock();
    uint32_t r = _writeBufferSize - _writeBufferUsed;
    const_cast<PicoUartChannel*>(this)->_unlock();
    return r;
}

uint32_t PicoUartChannel::read(uint8_t* buf, uint32_t bufCapacity) {
    _lock();
    // Figure out how much we can give
    uint32_t moveSize = std::min(bufCapacity, _readBufferUsed);
    if (moveSize > 0) {
        // Give the caller their data
        memcpy(buf, _readBuffer, moveSize);
        _readBufferUsed -= moveSize;
        // Shift any remaining data back to the start (should not be the norm)
        if (moveSize < _readBufferUsed)
            memcpy(buf, buf + moveSize, _readBufferUsed);
    }
    _unlock();
    return moveSize;
}

uint32_t PicoUartChannel::write(const uint8_t* buf, uint32_t bufLen) {
    _lock();
    // Figure out how much we can take
    uint32_t moveSize = std::min(bufLen, _writeBufferSize - _writeBufferUsed);
    if (moveSize > 0) {
        // Take the caller's data
        memcpy(_writeBuffer + _writeBufferUsed, buf, moveSize);
        _writeBufferUsed += moveSize;
    }
    _unlock();
    return moveSize;
}

bool PicoUartChannel::poll() {
    // Read is always enabled, but figure out if the write 
    // needs to be scheduled as well
    _lock();
    if (_writeBufferUsed > 0) {
        uart_set_irq_enables(_uart, true, true);
    } else {
        uart_set_irq_enables(_uart, true, false);
    }
    _unlock();
    return true;
}

void PicoUartChannel::_ISR() {
    _INSTANCE->_readISR();
    _INSTANCE->_writeISR();
}

// IMPORTANT: There is an assumption here that interrupts are disable
// while working inside of the ISR.
void PicoUartChannel::_readISR() {
    _isrCount++;
    // Keep reading until we can't
    while (uart_is_readable(_uart) && _readBufferUsed < _readBufferSize)
        _readBuffer[_readBufferUsed++] = uart_getc(_uart);
}

// IMPORTANT: There is an assumption here that interrupts are disable
// while working inside of the ISR.
void PicoUartChannel::_writeISR() {
    _isrCount++;
    uint32_t moveSize = 0;
    while (uart_is_writable(_uart) && moveSize < _writeBufferUsed) {
        uart_putc(_uart, (char)_writeBuffer[moveSize++]);
    }
    _writeBufferUsed -= moveSize;
    // Shift remaining data (if any) down to the start of the buffer
    if (_writeBufferUsed > 0)
        memcpy(_writeBuffer, _writeBuffer + moveSize, _writeBufferUsed);
    
}

void PicoUartChannel::_lock() {
    irq_set_enabled(_irq, false);
}

void PicoUartChannel::_unlock() {
    irq_set_enabled(_irq, true);
}

}
