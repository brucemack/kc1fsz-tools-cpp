/**
 * Copyright (C) 2026, Bruce MacKinnon KC1FSZ
 *
 * THIS IS PROPRIETARY/COMMERCIAL SOFTWARE.
 */
#include <cstring>

#include "kc1fsz-tools/W5500Driver.h"
#include "kc1fsz-tools/Log.h"

namespace kc1fsz {

W5500Driver::W5500Driver(Log& log, 
    const uint8_t* mac,
    writePinCb writeResetPin, 
    writePinCb writeSelectPin,
    txRxDmaStartCb txRxDmaStart,
    txDmaStartCb txDmaStart,
    uint8_t* rxDmaBuffer, unsigned rxDmaBufferSize,
    uint8_t* txDmaBuffer, unsigned txDmaBufferSize)  
:   _log(log),
    _mac(mac),
    _writeResetPin(writeResetPin), _writeSelectPin(writeSelectPin),
    _txRxDmaStart(txRxDmaStart),
    _txDmaStart(txDmaStart),
    _rxDmaBuffer(rxDmaBuffer), _rxDmaBufferSize(rxDmaBufferSize),
    _txDmaBuffer(txDmaBuffer), _txDmaBufferSize(txDmaBufferSize) {
}

void W5500Driver::reset() {
    _dmaState = DmaState::DMASTATE_IDLE;
    // Chip select high
    _writeSelectPin(true);
    // Reset the chip
    _writeResetPin(true);
    _writeResetPin(false);
    _writeResetPin(true);

    // Read the version
    _txDmaBuffer[0] = 0;
    _txDmaBuffer[1] = 0x39;
    // Block 0, R (0), Fixed 1 byte (01)
    _txDmaBuffer[2] = 0b00000001;
    // Dummy
    _txDmaBuffer[3] = 0;
    memset(_rxDmaBuffer, 0, 4);

    // Chip select 
    _writeSelectPin(false);
    _txRxDmaStart(_txDmaBuffer, _rxDmaBuffer, 4);

    _state = State::STATE_VERSION_CHECK;
    _dmaState = DmaState::DMASTATE_TXRX_RUNNING;

    _Sn_TX_WR = 0;
}

void W5500Driver::run() {
    if (_state == State::STATE_VERSION_CHECK) {
        if (_dmaState == DmaState::DMASTATE_TXRX_COMPLETE) {

            _dmaState = DmaState::DMASTATE_IDLE;
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

                // Chip select 
                _writeSelectPin(false);
                _txDmaStart(_txDmaBuffer, len);

                _state = State::STATE_OPENING;
                _dmaState = DmaState::DMASTATE_TX_RUNNING;
            } 
            else {
                _log.info("Version check bad");
                _state = State::STATE_FAULT;
            }
        }
    }
    else if (_state == State::STATE_OPENING) {
        if (_dmaState == DmaState::DMASTATE_TX_COMPLETE) {

            _dmaState = DmaState::DMASTATE_IDLE;
            _writeSelectPin(true);

            // Ask for status
            // Sn_SR
            _txDmaBuffer[0] = 0;
            _txDmaBuffer[1] = 0x03;
            // Block 00001, R (0), Fixed 1 byte (01)
            _txDmaBuffer[2] = 0b00001001;
            // DUMMY
            _txDmaBuffer[3] = 0;
            memset(_rxDmaBuffer, 0, 4);

            // Chip select 
            _writeSelectPin(false);
            _txRxDmaStart(_txDmaBuffer, _rxDmaBuffer, 4);

            _dmaState = DmaState::DMASTATE_TXRX_RUNNING;
            _state = State::STATE_STATUS_CHECK;
        }
    }
    else if (_state == State::STATE_STATUS_CHECK) {
        if (_dmaState == DmaState::DMASTATE_TXRX_COMPLETE) {

            _dmaState = DmaState::DMASTATE_IDLE;
            _writeSelectPin(true);

            if (_rxDmaBuffer[3] == 0x42) {
                _state = State::STATE_RUNNING;
            }
            else {
                _log.info("Status check bad");
                _state = State::STATE_FAULT;
            }
        }
    }
    else if (_state == State::STATE_SEND_TRANSFERING) {
        if (_dmaState == DmaState::DMASTATE_TX_COMPLETE) {

            _dmaState = DmaState::DMASTATE_IDLE;

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

            // Chip select 
            _writeSelectPin(false);
            _txDmaStart(_txDmaBuffer, 9);

            _state = State::STATE_SENDING;
            _dmaState = DmaState::DMASTATE_TX_RUNNING;
        }
    }
    else if (_state == State::STATE_SENDING) {
        if (_dmaState == DmaState::DMASTATE_TX_COMPLETE) {

            _dmaState = DmaState::DMASTATE_IDLE;
            _writeSelectPin(true);

            _log.info("Sent");

            // Data pointer back to beginning
            _Sn_TX_WR = 0;
            _state = State::STATE_RUNNING;
        }
    }
}

void W5500Driver::txDmaComplete() {    
    if (_dmaState == DmaState::DMASTATE_TX_RUNNING) 
        _dmaState = DmaState::DMASTATE_TX_COMPLETE; 
}

void W5500Driver::txRxDmaComplete() {   
    if (_dmaState == DmaState::DMASTATE_TXRX_RUNNING) 
        _dmaState = DmaState::DMASTATE_TXRX_COMPLETE; 
}

void W5500Driver::sendPacket(const uint8_t* packet, unsigned packetLen) {

    if (_state != State::STATE_RUNNING)
        return;

    // 1. Write data to the last Sn_TX_WR location.
    // 2. Move the Sn_TX_WR forward past the new data.
    // 3. Request a SEND command

    unsigned len = 0;

    // Sn_TX_WR
    _txDmaBuffer[len++] = 0;
    _txDmaBuffer[len++] = 0x24;
    // Block 00001, W (1), Fixed 2 bytes (10)
    _txDmaBuffer[len++] = 0b00001110;
    // Big endian
    _txDmaBuffer[len++] = (_Sn_TX_WR >> 8) & 0xff;
    _txDmaBuffer[len++] = (_Sn_TX_WR & 0xff);

    // Data write into TX buffer
    _txDmaBuffer[len++] = (_Sn_TX_WR >> 8) & 0xff;
    _txDmaBuffer[len++] = (_Sn_TX_WR & 0xff);
    // Block 00010, W (1), Variable Length (00)
    _txDmaBuffer[len++] = 0b00010100;
    // The content itself
    memcpy(_txDmaBuffer + len, packet, packetLen);
    len += packetLen;

    // Chip select 
    _writeSelectPin(false);
    _txDmaStart(_txDmaBuffer, len);

    _Sn_TX_WR += packetLen;
    _state = State::STATE_SEND_TRANSFERING;
    _dmaState = DmaState::DMASTATE_TX_RUNNING;
}

}




