/**
 * Copyright (C) 2026, Bruce MacKinnon KC1FSZ
 *
 * THIS IS PROPRIETARY/COMMERCIAL SOFTWARE.
 */
#pragma once

#include <cstdint>
#include <functional>

#include "kc1fsz-tools/Runnable.h"

namespace kc1fsz {

class Log;

class W5500Driver : public Runnable {
public:

    using writePinCb = std::function<void(bool state)>;
    // EX: HAL_SPI_TransmitReceive_DMA and HAL_SPI_TxRxCpltCallback. 
    using txRxDmaStartCb = std::function<void(uint8_t* txPtr, uint8_t* rxPtr, unsigned len)>;
    // EX: HAL_SPI_Transmit_DMA and HAL_SPI_TxCpltCallback. 
    using txDmaStartCb = std::function<void(uint8_t* txPtr, unsigned len)>;

    W5500Driver(Log& log, 
        writePinCb writeResetPin, writePinCb writeSelectPin,
        txRxDmaStartCb txRxDmaStart,
        txDmaStartCb txDmaStart,
        uint8_t* rxDmaBuffer, unsigned rxDmaBufferSize,
        uint8_t* txDmaBuffer, unsigned txDmaBufferSize);

    void reset();
    
    void txDmaComplete();
    void txRxDmaComplete();

    bool isFaulted() const { return _state == State::STATE_FAULT; }

    bool isUp() const { return _state == State::STATE_RUNNING; }

    bool isDmaRunning() const { return _dmaState == DmaState::DMASTATE_TXRX_RUNNING || 
        _dmaState == DmaState::DMASTATE_TX_RUNNING; }

    void sendPacket(const uint8_t* packet, unsigned packetLen);

    /** 
     * Can be installed as a callback.
     */
    static void txRxDmaComplete(void* data) {
        reinterpret_cast<W5500Driver*>(data)->txRxDmaComplete();
    }

    static void txDmaComplete(void* data) {
        reinterpret_cast<W5500Driver*>(data)->txDmaComplete();
    }

    // ----- From Runnable ---------------------------------------------------

    void run();

private:

    enum State {
        STATE_IDLE,
        STATE_FAULT,
        STATE_RUNNING,
        STATE_VERSION_CHECK,
        STATE_OPENING,
        STATE_STATUS_CHECK
    };

    enum DmaState {
        DMASTATE_IDLE,
        DMASTATE_TX_RUNNING,
        DMASTATE_TX_COMPLETE,
        DMASTATE_TXRX_RUNNING,
        DMASTATE_TXRX_COMPLETE
    };

    State _state = State::STATE_IDLE;
    DmaState _dmaState = DmaState::DMASTATE_IDLE;

    Log& _log;
    writePinCb _writeResetPin;
    writePinCb _writeSelectPin;
    txRxDmaStartCb _txRxDmaStart;
    txDmaStartCb _txDmaStart;
    uint8_t* _rxDmaBuffer;
    unsigned _rxDmaBufferSize;
    uint8_t* _txDmaBuffer;
    unsigned _txDmaBufferSize;
};

}
