#include <cassert>
#include "kc1fsz-tools/Common.h"
#include "kc1fsz-tools/fixedqueue.h"
#include "kc1fsz-tools/fixedstring.h"

using namespace kc1fsz;

int main(int,const char**) {
    {
        const char* test0 = "this is a test";
        fixedstring c0[4];
        fixedqueue<fixedstring> q0(c0, 4);
        assert(tokenize(test0, ' ', q0) == 0);
        assert(q0.size() == 4);
    }
    {
        // Extra spaces are ignored
        const char* test0 = " this  is a  test ";
        fixedstring c0[4];
        fixedqueue<fixedstring> q0(c0, 4);
        assert(tokenize(test0, ' ', q0) == 0);
        assert(q0.size() == 4);
    }
    {
        // Capacity management
        const char* test0 = " this  is a  test ";
        fixedstring c0[3];
        fixedqueue<fixedstring> q0(c0, 3);
        assert(tokenize(test0, ' ', q0) == -1);
    }
    return 0;
}