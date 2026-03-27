#ifndef _ip_checksum_h
#define _ip_checksum_h

#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif

/**
 * Performs an IP checksum calculation on the three buffers, pretending that they are 
 * contiguous. The first two buffers must have an even length!
 */
uint16_t ipChecksum3(const uint8_t* data0, unsigned len0, const uint8_t* data1, unsigned len1,
    const uint8_t* data2, unsigned len2);

#ifdef __cplusplus
}
#endif


#endif 
