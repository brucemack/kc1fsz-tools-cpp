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
#ifndef _PicoUartChannel_h
#define _PicoUartChannel_h

#include "hardware/uart.h"

#include "kc1fsz-tools/AsyncChannel.h"

namespace kc1fsz {

/**
 * Wraps the UART with some read/write buffers that are managed via
 * the interrupt mechanism.
 * 
 * No internal dynamic memory allocation.
 * No internal thread synchronization.
 * 
 * IMPORTANT
 * ---------
 * Please make sure this UART setup has already happened:
 * 1. UART init w/ speed (ex):
 *    
 *    uart_init(UART_ID, 115200);
 * 
 * 2. Pins mapping (ex):
 * 
 *    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
 *    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
 * 
 * 3. Flow control (ex):
 * 
 *    uart_set_hw_flow(UART_ID, false, false);
 * 
 * 4. Data format (ex):
 * 
 *    uart_set_format(UART_ID, DATA_BITS, STOP_BITS, PARITY);
 * 
 * 5. Turn on FIFO (ex):
 * 
 *    uart_set_fifo_enabled(UART_ID, true);
*/
class PicoUartChannel {
public:

    PicoUartChannel(uart_inst_t* uart,
        uint8_t* readBuffer, uint32_t readBufferSize, 
        uint8_t* writeBuffer, uint32_t writeBufferSize);
    virtual ~PicoUartChannel();

    /**
     * Call this from the event loop to let the channel do its thing.
     * @return true if anything happens
    */
    bool poll();

    /**
     * @return true if read() can be called productively.
    */
    virtual bool isReadable() const;

    /**
     * @return true if write() can be called productively.
    */
    virtual bool isWritable() const;

    virtual uint32_t bytesReadable() const;

    virtual uint32_t bytesWritable() const;

    /**
     * @return The number of bytes that were actually read.  <= bufCapacity
    */
    virtual uint32_t read(uint8_t* buf, uint32_t bufCapacity);

    /**
     * @return The number of bytes that were actually written.  <= bufLen, 
     *   so be ready to hold some un-written piece if necessary.
    */
    virtual uint32_t write(const uint8_t* buf, uint32_t bufLen);

private:

    static PicoUartChannel* _INSTANCE;
    static void _ISR();

    void _readISR();
    void _writeISR();
    void _lock();
    void _unlock();
    
    uart_inst_t* _uart;
    int _irq;
    uint8_t* _readBuffer;
    uint32_t _readBufferSize;
    uint32_t _readBufferUsed;
    uint8_t* _writeBuffer;
    uint32_t _writeBufferSize;
    uint32_t _writeBufferUsed;
};

}

#endif
