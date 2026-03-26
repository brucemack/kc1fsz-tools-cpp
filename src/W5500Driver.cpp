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
    writePinCb writeResetPin, 
    writePinCb writeSelectPin,
    txRxDmaStartCb txRxDmaStart,
    txDmaStartCb txDmaStart,
    uint8_t* rxDmaBuffer, unsigned rxDmaBufferSize,
    uint8_t* txDmaBuffer, unsigned txDmaBufferSize)  
:   _log(log),
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
}

void W5500Driver::run() {
    if (_state == State::STATE_VERSION_CHECK) {
        if (_dmaState == DmaState::DMASTATE_TXRX_COMPLETE) {

            _dmaState = DmaState::DMASTATE_IDLE;
            _writeSelectPin(true);

            if (_rxDmaBuffer[3] == 0x04) {

                // Next we configure socket 0
                // Sn_MR
                _txDmaBuffer[0] = 0;
                _txDmaBuffer[1] = 0;
                // Block 00001, W (1), Fixed 1 byte (01)
                _txDmaBuffer[2] = 0b00001101;
                // MFEN=1, BCASTB=0, MMB=0, MIP6B=1, P=0100 (MACRAW)
                _txDmaBuffer[3] = 0b10010100;

                // Open socket
                // Sn_CR
                _txDmaBuffer[4 + 0] = 0;
                _txDmaBuffer[4 + 1] = 1;
                // Block 00001, W (1), Fixed 1 byte (01)
                _txDmaBuffer[4 + 2] = 0b00001101;
                // OPEN=0x01
                _txDmaBuffer[4 + 3] = 0x01;

                // Chip select 
                _writeSelectPin(false);
                _txDmaStart(_txDmaBuffer, 8);

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
    // 
}


}




