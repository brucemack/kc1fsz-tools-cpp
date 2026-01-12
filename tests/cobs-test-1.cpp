#include <stddef.h>
#include <stdint.h>
#include <assert.h>

#include <iostream>

#include "kc1fsz-tools/Common.h"
#include "kc1fsz-tools/CobsCodec.h"

using namespace std;
using namespace kc1fsz;

int main(int, const char**) {
    {
        uint8_t inmsg[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };
        uint8_t outmsg[16];
        uint8_t outmsg2[16];
        int rc = cobsEncode(inmsg, 8, outmsg, 16);
        assert(rc == 9);
        //cout << "Encoded Len " << rc << endl;    
        //prettyHexDump(outmsg, rc, cout);
        unsigned rc2 = cobsDecode(outmsg, rc, outmsg2, 16);
        //cout << "Decoded Len " << rc2 << endl;    
        //prettyHexDump(outmsg2, rc2, cout);
        assert(rc2 == 8);
        assert(memcmp(inmsg, outmsg2, 8) == 0);
    }
    {
        uint8_t inmsg[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };
        uint8_t outmsg[8];
        int rc = cobsEncode(inmsg, 8, outmsg, 8);
        assert(rc < 0);
    }
    {
        uint8_t inmsg[8] = { 0, 2, 3, 0xff, 5, 6, 7, 0 };
        uint8_t outmsg[16];
        uint8_t outmsg2[16];
        int rc = cobsEncode(inmsg, 8, outmsg, 16);
        assert(rc == 9);
        unsigned rc2 = cobsDecode(outmsg, rc, outmsg2, 16);
        assert(rc2 == 8);
        assert(memcmp(inmsg, outmsg2, 8) == 0);
    }
    {
        uint8_t inmsg[8] = { 0, 2, 3, 0xff, 5, 6, 7, 0xff };
        uint8_t outmsg[16];
        uint8_t outmsg2[16];
        int rc = cobsEncode(inmsg, 8, outmsg, 16);
        assert(rc == 9);
        unsigned rc2 = cobsDecode(outmsg, rc, outmsg2, 16);
        assert(rc2 == 8);
        assert(memcmp(inmsg, outmsg2, 8) == 0);
    }
    {
        uint8_t inmsg[8] = { 1, 2, 3, 0xff, 5, 6, 7, 1 };
        uint8_t outmsg[16];
        uint8_t outmsg2[16];
        int rc = cobsEncode(inmsg, 8, outmsg, 16);
        assert(rc == 9);
        unsigned rc2 = cobsDecode(outmsg, rc, outmsg2, 16);
        assert(rc2 == 8);
        assert(memcmp(inmsg, outmsg2, 8) == 0);
    }
}
