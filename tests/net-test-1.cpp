#include <iostream>
#include <cassert>
#include <cstring>

#include "kc1fsz-tools/NetUtils.h"

using namespace std;
using namespace kc1fsz;

int main(int,const char**) {
    sockaddr_storage addr;
    char addrStr[64];

    assert(parseIPAddrAndPort("[fe80::987e:49de:e95d:90e]:99", addr) == 0);
    // Convert back to get address
    formatIPAddr((const sockaddr&)addr, addrStr, 64);
    assert(strcmp(addrStr, "fe80::987e:49de:e95d:90e") == 0);
    assert(getIPPort((const sockaddr&)addr) == 99);

    assert(parseIPAddrAndPort("10.11.12.13:99", addr) == 0);
    formatIPAddr((const sockaddr&)addr, addrStr, 64);
    assert(strcmp(addrStr, "10.11.12.13") == 0);
    assert(getIPPort((const sockaddr&)addr) == 99);

    assert(parseIPAddrAndPort("10.11.12.13:01234567", addr) == -2);
}
