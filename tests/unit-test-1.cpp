#include <iostream>
#include <cassert>
#include <cstring>

#include "kc1fsz-tools/CircularQueuePointers.h"
#include "kc1fsz-tools/CircularQueueWithTrigger.h"

using namespace std;
using namespace kc1fsz;

int main(int,const char**) {
    
    CircularQueuePointers ptrs(4);
    assert(ptrs.getDepth() == 0);
    assert(ptrs.getFree() == 3);
    ptrs.push();
    ptrs.push();
    ptrs.push();
    assert(ptrs.isFull());
    ptrs.push();
    assert(ptrs.isFault());
    ptrs.reset();

    ptrs.push(3);
    assert(ptrs.isFull());
    assert(!ptrs.isFault());

    // Overflow
    ptrs.reset();
    ptrs.push(4);
    assert(ptrs.isFull());
    assert(ptrs.isFault());

    // Make sure the push works with a wrap
    ptrs.reset();
    ptrs.push();
    ptrs.push();
    ptrs.pop();
    ptrs.pop();
    assert(!ptrs.isFull());
    assert(!ptrs.isFault());
    ptrs.push(3);
    assert(ptrs.isFull());
    assert(!ptrs.isFault());

    // Test the run size
    ptrs.reset();
    assert(ptrs.getMaxContiguousPushLength() == 3);
    ptrs.push();
    ptrs.push();
    assert(ptrs.getMaxContiguousPushLength() == 2);
    ptrs.pop();
    ptrs.pop();
    assert(ptrs.getMaxContiguousPushLength() == 2);
    ptrs.push();
    ptrs.pop();
    assert(ptrs.getMaxContiguousPushLength() == 1);

    int32_t space[4];
    CircularQueueWithTrigger<int32_t> q(space, 4, 1);
    int32_t temp[5] = { 0, 1, 2, 3, 4 };
    q.pushInt32(temp, 2);
    assert(q.getDepth() == 2);

    int32_t temp2[4];
    q.tryPop(temp2, 2);
    assert(q.getDepth() == 0);

    // This will cause two writes because of the wrap
    q.pushInt32(temp + 2, 3);
    assert(q.getDepth() == 3);
    assert(space[2] == 2);
    assert(space[3] == 3);
    assert(space[0] == 4);

    // This will cause two reads because of the wrap
    q.tryPopInt32(temp2, 3);
    assert(temp2[0] == 2);
    assert(temp2[1] == 3);
    assert(temp2[2] == 4);
    assert(q.getDepth() == 0);

    // Repeat the operation to makes sure all pointers are right
    q.pushInt32(temp + 2, 3);
    assert(q.getDepth() == 3);
    // Notice placement is different
    assert(space[1] == 2);
    assert(space[2] == 3);
    assert(space[3] == 4);

    q.tryPopInt32(temp2, 3);
    assert(temp2[0] == 2);
    assert(temp2[1] == 3);
    assert(temp2[2] == 4);
    assert(q.isEmpty());

    // Read tests
    ptrs.reset();
    assert(ptrs.getMaxContiguousPopLength() == 0);
    ptrs.push(3);
    assert(ptrs.getMaxContiguousPopLength() == 3);


}
