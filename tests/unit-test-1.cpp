#include <gtest/gtest.h>

#include <iostream>
#include <cassert>
#include <cstring>

#include "kc1fsz-tools/Log.h"
#include "kc1fsz-tools/CircularQueuePointers.h"
#include "kc1fsz-tools/CircularQueueWithTrigger.h"
#include "kc1fsz-tools/GPSUtils.h"

using namespace std;
using namespace kc1fsz;

static void assertClose(float a, float b, float tol) {
    assert(abs(a - b) < tol);
}

TEST(UnitTest1,math1) {
    {
        // q15 times q15
        int16_t a = 0.25 * 32767.0;
        int16_t b = 0.25 * 32767.0;
        int16_t c = ((int32_t)a * (int32_t)b) >> 15;
        assertClose((float)c / 32767.0f, 0.0625, 0.001);
    }

    {
        // q15 times q13
        // q15 value
        int16_t a = 0.25 * 32767.0;
        // q13 value
        int16_t b = 2.0 * (32767.0 / 4.0);
        // Notice here we shift right by 15 - 2 = 13
        int16_t c = ((int32_t)a * (int32_t)b) >> 13;
        assertClose((float)c / 32767.0f, 0.5, 0.1);
    }

    {
        // q15 times q10
        // q15 value
        int16_t a = 0.25 * 32767.0;
        // q10 value
        int16_t b = 2.0 * (32767.0 / 32.0);
        // Notice here we shift right by 15 - 5 = 10
        int16_t c = ((int32_t)a * (int32_t)b) >> 10;
        assertClose((float)c / 32767.0f, 0.5, 0.1);
    }

    {
        // q15 times q11
        // q15 value
        int16_t a = 0.0625 * 32767.0;
        // q11 value
        int16_t b = 16.0 * (32767.0 / 16.0);
        // Notice here we shift right by 15 - 4 = 11
        int16_t c = ((int32_t)a * (int32_t)b) >> 11;
        assertClose((float)c / 32767.0f, 1.0, 0.1);
    }

    {
        // q15 times q11
        // q15 value
        int16_t a = 0.0625 * 32767.0;
        // q11 value
        int16_t b = 1 * (32767.0 / 16.0);
        assert(b == 2047);
        // Notice here we shift right by 15 - 4 = 11
        int16_t c = ((int32_t)a * (int32_t)b) >> 11;
        assertClose((float)c / 32767.0f, 0.0625, 0.001);
    }
}

TEST(UnitTest1, queue1) {

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

TEST(UnitTest1, gps1) {

    const char* sent1 = "$GNRMC,183722.000,A,4218.20250,N,07118.07083,W,0.00,152.45,240326,,,A,V*1A";
    char tokens[10][16];
    int tokenCount = tokenizeNMEASentence(sent1, tokens, 10, 16);
    ASSERT_EQ(tokenCount, 10);

    struct tm tm_out;
    int rc = parseTimeFromGPRMC(tokens, tokenCount, &tm_out);
    ASSERT_EQ(rc, 0);
    ASSERT_EQ(tm_out.tm_year, 126);

    // IMPORTANT: mktime() operates on local time, not UTC!
    //time_t t = mktime(&tm_out);
    // This is the GMT equivalent
    time_t t = timegm(&tm_out);
    ASSERT_EQ(t, 1774377442L);
}

// Demonstrate some basic assertions.
TEST(UnitTest1, BasicAssertions) {
  // Expect two strings not to be equal.
  EXPECT_STRNE("hello", "world");
  // Expect equality.
  EXPECT_EQ(7 * 6, 42);
}
