#include <iostream>
#include <cassert>
#include <cstring>

#include "kc1fsz-tools/NetUtils.h"

using namespace std;
using namespace kc1fsz;

static void net_test_1() {
    const char* params0 = "a=1&b=2%202++4%21";
    char value[32];
    assert(extractQueryParam(params0, "a", value, sizeof(value)) == 0);
    assert(strcmp(value, "1") == 0);
    assert(extractQueryParam(params0, "c", value, sizeof(value)) == -1);
    assert(extractQueryParam(params0, "b", value, sizeof(value)) == 0);
    assert(strcmp(value, "2 2  4!") == 0);
}

static net_test_2() {
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

int main(int,const char**) {
    net_test_1();
    net_test_2();
}

