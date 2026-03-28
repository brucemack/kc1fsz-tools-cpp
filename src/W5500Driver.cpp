/**
 * Copyright (C) 2026, Bruce MacKinnon KC1FSZ
 *
 * THIS IS PROPRIETARY/COMMERCIAL SOFTWARE.
 */
#include <cstring>

#include "kc1fsz-tools/W5500Driver.h"
#include "kc1fsz-tools/Log.h"
#include "kc1fsz-tools/Clock.h"

// The maximum amount of time that can pass without proactively checking
// for receive activity in the chip.
#define RX_CHECK_INTERVAL_MS (250)

namespace kc1fsz {

W5500Driver::W5500Driver(Log& log, Clock& clock,
    const uint8_t* mac,
    eventCb enableIrq,
    eventCb disableIrq,
    writePinCb writeResetPin, 
    writePinCb writeSelectPin,
    txRxDmaStartCb txRxDmaStart,
    packetCb txDmaStart,
    packetCb rxEvent,
    packetCb flushDCache,
    uint8_t* rxDmaBuffer, unsigned rxDmaBufferSize,
    uint8_t* txDmaBuffer, unsigned txDmaBufferSize)  
:   _log(log),
    _clock(clock),
    _mac(mac),
    _enableIrq(enableIrq),
    _disableIrq(disableIrq),
    _writeResetPin(writeResetPin), _writeSelectPin(writeSelectPin),
    _txRxDmaStart(txRxDmaStart),
    _txDmaStart(txDmaStart),
    _rxEvent(rxEvent),
    _flushDCache(flushDCache),
    _rxDmaBuffer(rxDmaBuffer), _rxDmaBufferSize(rxDmaBufferSize),
    _txDmaBuffer(txDmaBuffer), _txDmaBufferSize(txDmaBufferSize) {
}

void W5500Driver::reset() {

    _state = State::STATE_IDLE;
    _dmaRunning = false;
    _rxPending = false;
    _rxTransferSize = 0;
    _lastRxCheckMs = 0;
    _rxAccUsed = 0;

    // Chip select high
    _writeSelectPin(true);

    // Reset the chip
    _writeResetPin(true);
    _writeResetPin(false);
    _writeResetPin(true);

    // Reset the shadow registers back to zero
    _Sn_TX_WR = 0;
    _Sn_RX_RD = 0;

    // Read the version
    _txDmaBuffer[0] = 0;
    _txDmaBuffer[1] = 0x39;
    // Block 0, R (0), Fixed 1 byte (01)
    _txDmaBuffer[2] = 0b00000001;
    // Dummy
    _txDmaBuffer[3] = 0;

    // Chip select 
    _writeSelectPin(false);
    _txRxDmaStart(_txDmaBuffer, _rxDmaBuffer, 4);
    _dmaRunning = true;

    _state = State::STATE_VERSION_CHECK;
}

void W5500Driver::run() {

    // Diagnostic stuff
    if (_clock.isPastWindow(_lastPollMs, 5000)) {
        _lastPollMs = _clock.timeMs();
        _log.info("State %d dmaRunning %d Sn_TX_WR %04X Sn_RX_RD %04X", 
            _state, _dmaRunning, _Sn_TX_WR, _Sn_RX_RD);
    }

    if (_state == State::STATE_VERSION_CHECK) {
        if (!_dmaRunning) {

            _writeSelectPin(true);

            if (_rxDmaBuffer[3] == 0x04) {

                unsigned len = 0;

                // Reset the PHY
                // PHYCFGR
                _txDmaBuffer[len++] = 0;
                _txDmaBuffer[len++] = 0x2e;
                // Block 00000, W (1), Fixed 1 byte (01)
                _txDmaBuffer[len++] = 0b00000101;
                // RST=1
                _txDmaBuffer[len++] = 0x80;

                // Set the MAC in three two-byte parts

                // SHAR
                _txDmaBuffer[len++] = 0;
                _txDmaBuffer[len++] = 0x09;
                // Block 00000, W (1), Fixed 2 bytes (10)
                _txDmaBuffer[len++] = 0b00000110;
                _txDmaBuffer[len++] = _mac[0];
                _txDmaBuffer[len++] = _mac[1];

                // SHAR+2
                _txDmaBuffer[len++] = 0;
                _txDmaBuffer[len++] = 0x0b;
                // Block 00000, W (1), Fixed 2 bytes(10)
                _txDmaBuffer[len++] = 0b00000110;
                _txDmaBuffer[len++] = _mac[2];
                _txDmaBuffer[len++] = _mac[3];

                // SHAR+4
                _txDmaBuffer[len++] = 0;
                _txDmaBuffer[len++] = 0x0d;
                // Block 00000, W (1), Fixed 2 bytes (10)
                _txDmaBuffer[len++] = 0b00000110;
                _txDmaBuffer[len++] = _mac[4];
                _txDmaBuffer[len++] = _mac[5];

                // Next we configure socket 0
                // Sn_MR
                _txDmaBuffer[len++] = 0;
                _txDmaBuffer[len++] = 0;
                // Block 00001, W (1), Fixed 1 byte (01)
                _txDmaBuffer[len++] = 0b00001101;
                // MFEN=1, BCASTB=0, MMB=0, MIP6B=1, P=0100 (MACRAW)
                _txDmaBuffer[len++] = 0b10010100;

                // Open socket
                // Sn_CR
                _txDmaBuffer[len++] = 0;
                _txDmaBuffer[len++] = 1;
                // Block 00001, W (1), Fixed 1 byte (01)
                _txDmaBuffer[len++] = 0b00001101;
                // OPEN=0x01
                _txDmaBuffer[len++] = 0x01;

                // Socket Interrupt Mask 
                // Common Register SIMR
                _txDmaBuffer[len++] = 0;
                _txDmaBuffer[len++] = 0x18;
                // Block 00000, W (1), Fixed 1 byte (01)
                _txDmaBuffer[len++] = 0b00000101;
                // S0_IMR=1
                _txDmaBuffer[len++] = 0x01;

                // Socket-level interrupt mask
                // Socket Register Sn_IMR
                _txDmaBuffer[len++] = 0;
                _txDmaBuffer[len++] = 0x2c;
                // Block 00001, W (1), Fixed 1 byte (01)
                _txDmaBuffer[len++] = 0b00001101;
                // RECV=0b100
                _txDmaBuffer[len++] = 0b00000100;

                // Push an initializing reset on the socket interrupt register 
                _txDmaBuffer[len++] = 0;
                _txDmaBuffer[len++] = 0x0002;
                // Block 00001, W (1), Fixed 1 byte (01)
                _txDmaBuffer[len++] = 0b00001101;
                // Reset RECV interrupt
                _txDmaBuffer[len++] = 0b00000100;

                _flushDCache(_txDmaBuffer, len);
                _writeSelectPin(false);
                _disableIrq();
                _txDmaStart(_txDmaBuffer, len);
                _dmaRunning = true;
                _enableIrq();

                _state = State::STATE_OPENING;
            } 
            else {
                _state = State::STATE_FAULT;
            }
        }
    }
    else if (_state == State::STATE_OPENING) {
        if (!_dmaRunning) {

            _writeSelectPin(true);

            // Ask for status
            // Sn_SR
            _txDmaBuffer[0] = 0;
            _txDmaBuffer[1] = 0x03;
            // Block 00001, R (0), Fixed 1 byte (01)
            _txDmaBuffer[2] = 0b00001001;
            // DUMMY
            _txDmaBuffer[3] = 0;

            _flushDCache(_txDmaBuffer, 4);
            _writeSelectPin(false);
            _disableIrq();
            _txRxDmaStart(_txDmaBuffer, _rxDmaBuffer, 4);
            _dmaRunning = true;
            _enableIrq();

            _state = State::STATE_STATUS_CHECK;
        }
    }
    else if (_state == State::STATE_STATUS_CHECK) {
        if (!_dmaRunning) {
            
            _writeSelectPin(true);
            _flushDCache(_rxDmaBuffer, 4);

            if (_rxDmaBuffer[3] == 0x42) {
                _state = State::STATE_RUNNING;
            }
            else {
                _state = State::STATE_FAULT;
            }
        }
    }
    else if (_state == State::STATE_SEND_TRANSFERING) {
        if (!_dmaRunning) {

            // Very important! This terminates the variable length transfer
            _writeSelectPin(true);
            
            // Move write pointer forward past new data
            // Sn_TX_WR
            _txDmaBuffer[0] = 0;
            _txDmaBuffer[1] = 0x24;
            // Block 00001, W (1), Fixed 2 bytes (10)
            _txDmaBuffer[2] = 0b00001110;
            // Big endian
            _txDmaBuffer[3] = (_Sn_TX_WR >> 8) & 0xff;
            _txDmaBuffer[4] = (_Sn_TX_WR & 0xff);

            // Command send
            // Sn_CR
            _txDmaBuffer[5 + 0] = 0;
            _txDmaBuffer[5 + 1] = 1;
            // Block 00001, W (1), Fixed 1 byte (01)
            _txDmaBuffer[5 + 2] = 0b00001101;
            // SEND=0x20
            _txDmaBuffer[5 + 3] = 0x20;

            _flushDCache(_rxDmaBuffer, 9);
            _writeSelectPin(false);
            _disableIrq();
            _txDmaStart(_txDmaBuffer, 9);
            _dmaRunning = true;
            _enableIrq();

            _state = State::STATE_SENDING;
        }
    }
    else if (_state == State::STATE_SENDING) {
        if (!_dmaRunning) {
            _writeSelectPin(true);
            _state = State::STATE_RUNNING;
        }
    }
    else if (_state == State::STATE_RECEIVE_CHECK) {
        if (!_dmaRunning) {

            _writeSelectPin(true);
            _flushDCache(_rxDmaBuffer, 16);

            // How many bytes are pending in the chip?
            // The request included some other things
            unsigned discard = 4 + 4 + 3;
            // The data is in big-endian format
            uint16_t rxRsr = ((uint16_t)_rxDmaBuffer[discard] << 8) | _rxDmaBuffer[discard + 1];
            if (rxRsr > 0) {

                _rxTransferSize = rxRsr;

                // Schedule the transfer of the new data from the W5500 back into 
                // the local DMA buffer.
                unsigned len = 0;
                // Read data from RX buffer
                _txDmaBuffer[len++] = (_Sn_RX_RD >> 8) & 0xff;
                _txDmaBuffer[len++] = (_Sn_RX_RD & 0xff);
                // Block 00011, R (0), Variable Length (00)
                _txDmaBuffer[len++] = 0b00011000;
                // Assume space for the data itself
                len += rxRsr;

                _flushDCache(_rxDmaBuffer, len);
                _writeSelectPin(false);
                _disableIrq();
                _txRxDmaStart(_txDmaBuffer, _rxDmaBuffer, len);
                _dmaRunning = true;
                _enableIrq();
                _state = State::STATE_RECEIVE_TRANSFERRING;
            }
            else {
                _rxPending = false;
                _state = State::STATE_RUNNING;
            }
        }
    }
    else if (_state == State::STATE_RECEIVE_TRANSFERRING) {
        if (!_dmaRunning) {

            // Very important to stop the variable length read: Chip deselect 
            _writeSelectPin(true);

            // Assume the pointer has been advanced in the chip, so we also
            // advance the shadow copy accordingly. Per the documentation, the chip will 
            // wrap at 0xffff so the shadow does as well.
            _Sn_RX_RD += _rxTransferSize;

            // Update the read pointer and issue the RECV command
            unsigned len = 0;

            // Move read pointer forward past new data
            // Sn_RX_RD
            _txDmaBuffer[len++] = 0;
            _txDmaBuffer[len++] = 0x28;
            // Block 00001, W (1), Fixed 2 bytes (10)
            _txDmaBuffer[len++] = 0b00001110;
            // Big endian
            _txDmaBuffer[len++] = (_Sn_RX_RD >> 8) & 0xff;
            _txDmaBuffer[len++] = (_Sn_RX_RD & 0xff);

            // Command RECV
            // Sn_CR
            _txDmaBuffer[len++] = 0;
            _txDmaBuffer[len++] = 1;
            // Block 00001, W (1), Fixed 1 byte (01)
            _txDmaBuffer[len++] = 0b00001101;
            // RECV=0x40
            _txDmaBuffer[len++] = 0x40;

            _flushDCache(_txDmaBuffer, len);
            _writeSelectPin(false);
            _disableIrq();
            _txDmaStart(_txDmaBuffer, len);
            _dmaRunning = true;
            _enableIrq();
            _state = State::STATE_RECEIVING;
        }
    }
    else if (_state == State::STATE_RECEIVING) {
        if (!_dmaRunning) {

            _writeSelectPin(true);
            _flushDCache(_rxDmaBuffer, _rxTransferSize + 8);

            // Append the newly received data to the accumulator.
            // Keep in mind that the first three bytes in the DMA buffer are useless placeholders 
            // that result from the way the TX/RX SPI operation works.
            memcpy(_rxAcc + _rxAccUsed, _rxDmaBuffer + 3, _rxTransferSize);
            _rxAccUsed += _rxTransferSize;

            // In MACRAW mode each packet is prefixed with a two-byte length (in big endian format)
            // that describes the length of the packet INCLUSIVE OF THE TWO LENGTH BYTES.
            //
            // It's possible that we have more than one packet in the buffer so this sets up a loop
            // that will keep going until we don't have enough to work with.  Do we have enough in the 
            // DMA buffer to at least check the length?
            while (_rxAccUsed > 2) {

                // Look at the first two bytes of the accumulator to see how long the current packet is.
                const uint16_t packetLen = unpack_uint16_be(_rxAcc);

                // Do we have at least a full packet?
                if (_rxAccUsed >= packetLen) {

                    // Fire the receive callback. We wait until all of the bookkeeping 
                    // in the W5500 has been completed in case this callback generates an
                    // immediate send. The callback doesn't receive the length bytes.
                    _rxEvent(_rxAcc + 2, packetLen - 2);

                    // Show the destination 
                    //_log.info("Dest: %02X:%02X:%02X:%02X:%02X:%02X", 
                    //    packet[0], packet[1], packet[2],
                    //    packet[3], packet[4], packet[5]);

                    // Shift left to eliminate the consumed data (overlapping move)
                    if (_rxAccUsed > packetLen) 
                        memmove(_rxAcc, _rxAcc + packetLen, _rxAccUsed - packetLen);
                    _rxAccUsed -= packetLen;
                }
                else {
                    break;
                }
            }

            _state = State::STATE_RUNNING;
        }
    }
    else if (_state == State::STATE_RUNNING) {

        // If we've been told about a pending receive then start to pull the packet
        // out of the chip. There is also a timeout here to make sure that 
        // we check once in awhile regardless of the interrupt status so 
        // that nothing is accidentally stranded in the receive buffer.

        if (_rxPending || _clock.isPastWindow(_lastRxCheckMs, RX_CHECK_INTERVAL_MS)) {

            _lastRxCheckMs = _clock.timeMs();

            unsigned len = 0;

            // Pull the value of the socket interrupt register to find out what kind
            // of interrupt was raised.
            // Socket: Sn_IR
            _txDmaBuffer[len++] = 0;
            _txDmaBuffer[len++] = 0x0002;
            // Block 00001, R (0), Fixed 1 byte (01)
            _txDmaBuffer[len++] = 0b00001001;
            // Dummy
            _txDmaBuffer[len++] = 0;

            // Push a reset on the socket interrupt register 
            _txDmaBuffer[len++] = 0;
            _txDmaBuffer[len++] = 0x0002;
            // Block 00001, W (1), Fixed 1 byte (01)
            _txDmaBuffer[len++] = 0b00001101;
            // Reset RECV interrupt
            _txDmaBuffer[len++] = 0b00000100;

            // Ask for size of the latest packet received.
            // Sn_RX_RSR
            _txDmaBuffer[len++] = 0;
            _txDmaBuffer[len++] = 0x26;
            // Block 00001, R (0), Fixed 2 bytes (10)
            _txDmaBuffer[len++] = 0b00001010;
            // Dummy
            _txDmaBuffer[len++] = 0;
            _txDmaBuffer[len++] = 0;

            _flushDCache(_rxDmaBuffer, len);
            _writeSelectPin(false);
            _disableIrq();
            _txRxDmaStart(_txDmaBuffer, _rxDmaBuffer, len);
            _dmaRunning = true;
            _enableIrq();
            _state = State::STATE_RECEIVE_CHECK;
        }
    }
}

void W5500Driver::txDmaComplete() {    
    _dmaRunning = false;
}

void W5500Driver::txRxDmaComplete() {   
    _dmaRunning = false;
}

void W5500Driver::rxInt() {
    _rxPending = true;
}

bool W5500Driver::sendPacketIfPossible(const uint8_t* packet, unsigned packetLen) {

    if (_state != State::STATE_RUNNING || _dmaRunning)
        return false;

    // 1. Write data to the last Sn_TX_WR location.
    // 2. Move the Sn_TX_WR forward past the new data.
    // 3. Request a SEND command

    unsigned len = 0;

    // Data write into TX buffer
    _txDmaBuffer[len++] = (_Sn_TX_WR >> 8) & 0xff;
    _txDmaBuffer[len++] = (_Sn_TX_WR & 0xff);
    // Block 00010, W (1), Variable Length (00)
    _txDmaBuffer[len++] = 0b00010100;
    // The content itself
    memcpy(_txDmaBuffer + len, packet, packetLen);
    len += packetLen;

    _flushDCache(_txDmaBuffer, len);
    _disableIrq();
    _writeSelectPin(false);
    _txDmaStart(_txDmaBuffer, len);
    _dmaRunning = true;
    _enableIrq();

    // IMPORTANT: Per the W5500 datasheet, the real Sn_TX_WR will wrap at 0xffff so we 
    // do the same with the shadow pointer.
    _Sn_TX_WR += packetLen;
    _state = State::STATE_SEND_TRANSFERING;

    return true;
}

}




