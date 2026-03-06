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

void packNtpTime(uint64_t us, uint8_t* packet) {
    uint32_t s = us >> 32;
    uint32_t fraction = us;
    pack_uint32_be(s, packet);
    pack_uint32_be(fraction, packet + 4);
}

int64_t calcOffset(const uint8_t* packet, uint64_t now) {
    int64_t t1 = unpackNtpTime(packet + 24);
    int64_t t2 = unpackNtpTime(packet + 32);
    int64_t t3 = unpackNtpTime(packet + 40);
    int64_t t4 = now;
    return ((t2 - t1) + (t3 - t4)) / 2;
}

uint64_t calcRoundTrip(const uint8_t* packet, uint64_t now) {
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

int query_1() {

    Log log;
    StdClock clock;

    const unsigned PACKET_SIZE = 48;
    uint8_t packet[PACKET_SIZE] = { 0 };
    int len = 48;

    packet[0] = 0x23;

    uint64_t now = usToNtpTime(clock.timeUs());
    printf("%016lX\n", now);

    packNtpTime(now, packet + 24);
    // RFC5905 page 29: "A packet is bogus if the origin timestamp T1 in the packet
    // does not match the xmt state variable T1.
    packNtpTime(now, packet + 40);

    log.infoDump("Send", packet, len);

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

    // Send the query to a DNS server
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

    // RX loop

    cout << "receive loop" << endl;

    while (true) {
        // Check for input
        const unsigned readBufferSize = 512;
        uint8_t readBuffer[readBufferSize];
        struct sockaddr_in peerAddr;
        socklen_t peerAddrLen = sizeof(peerAddr);
        int rc = recvfrom(sockFd, readBuffer, readBufferSize, 0,
            (struct sockaddr *)&peerAddr, &peerAddrLen);
        if (rc > 0) {
            // Immediately record T4
            now = usToNtpTime(clock.timeUs());
            log.infoDump("Receive", readBuffer, rc);

            //uint32_t recS = unpack_uint32_be(readBuffer + 32);
            //log.info("ReceiveTimestamp %u", recS);

            // Do the offset calc
            double offset = calcOffset(readBuffer, now);
            offset = offset / (double)(1LL << 32);
            cout << "Offset " << offset << endl;
            double rt = calcRoundTrip(readBuffer, now);
            rt = rt / (double)(1LL << 32);
            cout << "Rt " << rt << endl;
        }
    }
}

int main(int, const char**) {
    query_1();
}
