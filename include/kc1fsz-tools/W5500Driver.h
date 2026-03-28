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
class Clock;

class W5500Driver : public Runnable {
public:

    using eventCb = std::function<void()>;
    using writePinCb = std::function<void(bool state)>;
    // EX: HAL_SPI_TransmitReceive_DMA and HAL_SPI_TxRxCpltCallback. 
    using txRxDmaStartCb = std::function<void(const uint8_t* txPtr, uint8_t* rxPtr, unsigned len)>;
    // EX: HAL_SPI_Transmit_DMA and HAL_SPI_TxCpltCallback. 
    using packetCb = std::function<void(const uint8_t* txPtr, unsigned len)>;

    /**
     * @param mac A 6-byte hardware address (binary form)
     * @param rxPacket A callback fired when a valid packet is received. This will only
     * be triggered during a call to ::run(), so it will not happen inside of an ISR.
     * @param flushDCache A callback that can be used to flush data from the data-cache. This
     * is importnat when 
     */
    W5500Driver(Log& log, 
        Clock& clock,
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
        uint8_t* txDmaBuffer, unsigned txDmaBufferSize);

    void reset();
    
    void txDmaComplete();
    void txRxDmaComplete();
    void rxInt();

    bool isFaulted() const { return _state == State::STATE_FAULT; }

    bool isUp() const { return _state == State::STATE_RUNNING || _state == State::STATE_SEND_TRANSFERING || 
        _state == State::STATE_SENDING || _state == State::STATE_RECEIVE_CHECK || 
        _state == State::STATE_RECEIVE_TRANSFERRING || _state == State::STATE_RECEIVING; }

    bool isDmaRunning() const { return _dmaRunning; }

    /**
     * Sends a packet to the W5500 chip if possible. There is no internal buffering at this
     * layer so it's possible that sends will fail due to other activity going on. The caller
     * should be prepared to try again shortly.

     * @returns True if packet was accepted, false is not due being busy, in the wrong state, etc.
     */
    bool sendPacketIfPossible(const uint8_t* packet, unsigned packetLen);

    /** 
     * Can be installed as a callback.
     */
    static void txRxDmaComplete(void* data) {
        reinterpret_cast<W5500Driver*>(data)->txRxDmaComplete();
    }

    static void txDmaComplete(void* data) {
        reinterpret_cast<W5500Driver*>(data)->txDmaComplete();
    }

    static void rxInt(void* data) {
        reinterpret_cast<W5500Driver*>(data)->rxInt();
    }

    // ----- From Runnable ---------------------------------------------------

    /**
     * Performs any background activity. Importantly, will fire the receive callback 
     * if a packet is received.
     */
    void run();

private:

    enum State {
        STATE_IDLE,
        STATE_FAULT,
        // 2
        STATE_RUNNING,
        STATE_VERSION_CHECK,
        STATE_OPENING,
        STATE_STATUS_CHECK,
        STATE_SEND_TRANSFERING,
        STATE_SENDING,
        // 8
        STATE_RECEIVE_CHECK,
        STATE_RECEIVE_TRANSFERRING,
        // 10
        STATE_RECEIVING
    };

    State _state = State::STATE_IDLE;
    volatile bool _dmaRunning = false;
    volatile bool _rxPending = false;
    uint16_t _rxTransferSize = 0;
    uint64_t _lastRxCheckMs = 0;
    uint64_t _lastPollMs = 0;

    Log& _log;
    Clock& _clock;
    const uint8_t* _mac;
    eventCb _enableIrq;
    eventCb _disableIrq;
    writePinCb _writeResetPin;
    writePinCb _writeSelectPin;
    txRxDmaStartCb _txRxDmaStart;
    packetCb _txDmaStart;
    packetCb _rxEvent;
    packetCb _flushDCache;

    uint8_t* _rxDmaBuffer;
    const unsigned _rxDmaBufferSize;
    uint8_t* _txDmaBuffer;
    const unsigned _txDmaBufferSize;

    // This accumulator is used to assemble full Ethernet frames 
    static const unsigned _rxAccSize = 2048;
    uint8_t _rxAcc[_rxAccSize];
    unsigned _rxAccUsed = 0;

    // IMPORTANT: Shadow is same width as real pointer in the chip
    uint16_t _Sn_TX_WR;
    uint16_t _Sn_RX_RD;
};

}
