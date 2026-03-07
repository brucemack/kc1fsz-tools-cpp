#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <iostream>
#include <cstring>
#include <cassert>

#include "kc1fsz-tools/Common.h"
#include "kc1fsz-tools/Log.h"
#include "kc1fsz-tools/linux/StdClock.h"
#include "kc1fsz-tools/NetUtils.h"

#define NTP_SAMPLE_COUNT (8)

using namespace std;
using namespace kc1fsz;

// https://tf.nist.gov/tf-cgi/servers.cgi
static const char* NTP_IP_ADDR = "129.6.15.28";
static unsigned NTP_IP_PORT = 123;

static uint32_t EpochAdjustment = 2208988800;

// 0x1b 0001 1011 is LI=00, V=011 (version 3), MODE=011 (client)
// 0x23 0010 0011 is LI=00, V=100 (version 4), MODE=011 (client)

// The 64-bit binary fixed-point timestamps used by NTP consist of a 32-bit part for seconds and a 32-bit 
// part for fractional second, giving a time scale that rolls over every 232 seconds (136 years) and a 
// theoretical resolution of 2−32 seconds (233 picoseconds). NTP uses an epoch of January 1, 1900. 
// Therefore, the first rollover occurs on February 7, 2036.

// FILTER: https://www.eecis.udel.edu/%7Emills/ntp/html/warp.html
// MATH: https://www.eecis.udel.edu/~mills/time.html
// NTP Timestamps: https://tickelton.gitlab.io/articles/ntp-timestamps/

// VERY IMPORTANT/RELEVANT
// Clock filter: https://docs.ntpsec.org/latest/filter.html

// How to disable the time synchronization 
// sudo systemctl stop systemd-timesyncd


// The dispersion estimate is increased by 15uS/S as samples age. This is because the dispersion represents 
// the maximum potential error accumulated due to oscillator drift, network jitter, and aging since the last 
// measurement. As time passes without a new sample, uncertainty increases, causing the filter to "age" the 
// data by increasing its dispersion, which is subsequently reduced only when a fresh, lower-dispersion 
// packet arrives.

uint32_t us2ntpfraction(uint64_t usec) {
    // The goal is to compute (usec / 1000000) * 2^32 without losing precision
    return (usec * (1LL << 32)) / 1000000LL;
}

uint64_t ntpfraction2us(uint32_t fraction) {
    // The goal is to compute (fraction * 1000000) / 2^32 without losing precision
    return (fraction * 1e6) / (1LL << 32);
}

uint64_t unpackNtpTime(const uint8_t* packet) {
    uint32_t s = unpack_uint32_be(packet);
    uint32_t fraction = unpack_uint32_be(packet + 4);
    return ((uint64_t)s << 32) | fraction;
}

uint32_t unpackNtpTimeShort(const uint8_t* packet) {
    uint16_t s = unpack_uint16_be(packet);
    uint16_t fraction = unpack_uint16_be(packet + 2);
    return ((uint32_t)s << 16) | fraction;
}

void packNtpTime(uint64_t us, uint8_t* packet) {
    uint32_t s = us >> 32;
    uint32_t fraction = us;
    pack_uint32_be(s, packet);
    pack_uint32_be(fraction, packet + 4);
}

/**
 * @returns the clock offset using the NTP time format. A positive number
 * means that the reference time is ahead. The result is signed.
 */
int64_t calcOffset(const uint8_t* packet, uint64_t now) {
    int64_t t1 = unpackNtpTime(packet + 24);
    int64_t t2 = unpackNtpTime(packet + 32);
    int64_t t3 = unpackNtpTime(packet + 40);
    int64_t t4 = now;
    return ((t2 - t1) + (t3 - t4)) / 2;
}

/**
 * @returns The round-trip of the NTP query using the NTP time format.
 */
int64_t calcDelay(const uint8_t* packet, uint64_t now) {
    uint64_t t1 = unpackNtpTime(packet + 24);
    uint64_t t2 = unpackNtpTime(packet + 32);
    uint64_t t3 = unpackNtpTime(packet + 40);
    uint64_t t4 = now;
    return (t4 - t1) - (t3 - t2);
}

uint64_t usToNtpTime(uint64_t us) {
    uint32_t originS = (us / 1000000) + EpochAdjustment;
    uint64_t originUs = us;
    // Find the remainder
    originUs = originUs % 1000000;
    originUs = us2ntpfraction(originUs);
    return (((uint64_t)originS) << 32) | originUs;
}

struct NTPSample {
    uint64_t offset = 0;
    uint64_t delay = 0;
    // Infinity is 16.0s
    uint32_t dispersion = 16 << 16;
};

static NTPSample Samples[NTP_SAMPLE_COUNT];

void shift(NTPSample* samples, unsigned sampleCount) {
    for (unsigned i = sampleCount - 1; i > 0; i--)
        samples[i] = samples[i-1];
}

void ageDispersions(NTPSample* samples, unsigned sampleCount, uint32_t ageShort) {
    for (unsigned i = 0; i < sampleCount; i++)
        // Don't touch the "infinity" dispersions
        if (samples[i].dispersion != (16 << 16)) 
            samples[i].dispersion += ageShort;
}

int query_1() {

    Log log;
    StdClock clock;

    const unsigned PACKET_SIZE = 48;
    uint8_t packet[PACKET_SIZE] = { 0 };
    int len = 48;

    // Make a UDP socket
    int sockFd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockFd < 0) {
        return -1;
    }

    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    // Bind to all interfaces
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(0);
    if (::bind(sockFd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        cout << "Cant bing" << endl;
        ::close(sockFd);
        return -1;
    }
    // Make non-blocking
    int flags = ::fcntl(sockFd, F_GETFL, 0);
    if (flags == -1) {
        cout << "Non blocking error" << endl;
        ::close(sockFd);
        return -1;
    }
    if (fcntl(sockFd, F_SETFL, flags | O_NONBLOCK) == -1) {
        cout << "Non blocking error" << endl;
        ::close(sockFd);
        return -1;
    }

    uint64_t lastQuery = 0;
    uint64_t lastResponseUs = 0;

    while (true) {

        if (clock.isPastWindow(lastQuery, 8000)) {

            lastQuery = clock.timeMs();

            packet[0] = 0x23;
            uint64_t now = usToNtpTime(clock.timeUs());
            packNtpTime(now, packet + 24);
            // RFC5905 page 29: "A packet is bogus if the origin timestamp T1 in the packet
            // does not match the xmt state variable T1.
            packNtpTime(now, packet + 40);
            log.infoDump("Send", packet, len);

            // Send the query to a NTP server
            struct sockaddr_in dest_addr;
            memset(&dest_addr, 0, sizeof(dest_addr));
            dest_addr.sin_family = AF_INET;
            dest_addr.sin_port = htons(NTP_IP_PORT);
            inet_pton(AF_INET, NTP_IP_ADDR, &dest_addr.sin_addr); 
            int rc = ::sendto(sockFd, packet, len, 0, 
                (struct sockaddr*)&dest_addr, sizeof(dest_addr));
            if (rc < 0) {
                cout << "Send error " << endl;
                return -1;
            }
        }

        // Check for input
        const unsigned readBufferSize = 512;
        uint8_t readBuffer[readBufferSize];
        struct sockaddr_in peerAddr;
        socklen_t peerAddrLen = sizeof(peerAddr);
        int rc = recvfrom(sockFd, readBuffer, readBufferSize, 0,
            (struct sockaddr *)&peerAddr, &peerAddrLen);
        if (rc > 0) {
            // Immediately record T4
            uint64_t nowUs = clock.timeUs();
            uint64_t now = usToNtpTime(nowUs);

            log.infoDump("Receive", readBuffer, rc);

            // The offset (theta) represents the
            // maximum-likelihood time offset of the server clock relative to the
            // system clock.  The delay (delta) represents the round-trip delay
            // between the client and server.  The dispersion (epsilon) represents
            // the maximum error inherent in the measurement. 

            // Do the offset calc
            double offset = calcOffset(readBuffer, now);
            double delay = calcDelay(readBuffer, now);
            offset = offset / (double)(1LL << 32);
            cout << "Offset " << offset << endl;
            delay = delay / (double)(1LL << 32);
            cout << "Delay " << delay << endl;       
            double disp = unpackNtpTimeShort(readBuffer + 8);
            disp /= (double(1 << 16));
            cout << "Server dispersion " << disp << endl;       

            // Age the existing dispersion data
            uint32_t secondsElapsed = (nowUs - lastResponseUs) / 1000000;
            // We age at 15uS per second
            uint32_t usAge = secondsElapsed * 15;
            // Change the microseconds into NTP short units
            uint32_t ageShort = (usAge * (1L << 16)) / 1000000;
            ageDispersions(Samples, NTP_SAMPLE_COUNT, ageShort);

            // Load up the shift register 
            shift(Samples, NTP_SAMPLE_COUNT);
            Samples[0].offset = calcOffset(readBuffer, now);
            Samples[0].delay = calcDelay(readBuffer, now);
            Samples[0].dispersion = unpackNtpTimeShort(readBuffer + 8);

            // Display dispersions
            for (unsigned i = 0; i < NTP_SAMPLE_COUNT; i++)
                printf("%u %08X\n", i, Samples[i].dispersion);

            // Find the smallest delay
            uint64_t minDelay = -1;
            unsigned minIx = 0;
            for (unsigned i = 0; i < NTP_SAMPLE_COUNT; i++) {
                if (Samples[i].dispersion = (16 << 16))
                    break;
                if (Samples[i].delay < minDelay) {
                    minDelay = Samples[i].delay;
                    minIx = i;
                }
            }

            double selectedOffset = Samples[minIxcalcOffset(readBuffer, now);
            double delay = calcDelay(readBuffer, now);
            offset = offset / (double)(1LL << 32);
            cout << "Offset " << offset << endl;


            lastResponseUs = nowUs;
        }
    }
}

int main(int, const char**) {
    query_1();
}
