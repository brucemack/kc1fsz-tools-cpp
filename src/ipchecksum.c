#include "kc1fsz-tools/ipchecksum.h"

static uint32_t accumulateChecksum(const uint8_t* data, unsigned len) {
    uint32_t sum = 0;
    while (len > 1)  {
        // Big endian treatment!
        unsigned short v = (data[0] << 8) | data[1];
        sum += v;
        data += 2;
        len -= 2;
    }
    // Add left-over byte, if any
    if (len > 0) {
        // Big endian treatment!
        // Pretend the last byte is a zero (pseudo pad)
        unsigned short v = (data[0] << 8);
        sum += v;
    }
    return sum;
}

uint16_t ipChecksum3(const uint8_t* data0, unsigned len0, const uint8_t* data1, unsigned len1,
    const uint8_t* data2, unsigned len2) {
    uint32_t sum = accumulateChecksum(data0, len0);
    sum += accumulateChecksum(data1, len1);
    sum += accumulateChecksum(data2, len2);
    // Fold 32-bit sum to 16 bits
    while (sum >> 16)
        sum = (sum & 0xffff) + (sum >> 16);
    return ~sum;
}
