#include <cassert>
#include <iostream>

#include "kc1fsz-tools/Common.h"
#include "kc1fsz-tools/fixedqueue.h"
#include "kc1fsz-tools/fixedstring.h"
#include "kc1fsz-tools/fixedsortedlist.h"

using namespace std;
using namespace kc1fsz;

void test_1() {
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
}

void test_2() {

    int objSpace[4];
    unsigned ptrSpace[4];
    fixedsortedlist<int> list(objSpace, ptrSpace, 4,
        [](const int& a, const int& b) {
            if (a < b) {
                return -1;
            } else if (a > b) {
                return 1;
            } else {
                return 0;
            }
        }
    );
    
    list.insert(2);
    list.insert(1);
    list.insert(4);
    assert(!list.empty());
    assert(list.size() == 3);
    assert(list.hasCapacity());
    assert(list.first() == 1);

    int count = 0;
    list.visitAll([&count](const int& a) {
            cout << "Visited " << a << endl;
            count++;
            return true;
        }
    );   
    assert(count == 3);

    // Selective visit
    count = 0;
    list.visitAll(
        [&count](const int& a) {
            cout << "Visited " << a << endl;
            count++;
            return true;
        },
        [](const int& a) {
            if (a == 4)
                return true;
            else    
                return false;
        }
    );   
    assert(count == 1);

    // Stress test
    assert(list.insert(3));
    assert(list.size() == 4);
    assert(!list.hasCapacity());

    // No more room
    assert(!list.insert(5));
    assert(list.size() == 4);
    assert(!list.hasCapacity());

    // Remove things selectively
    list.visitIfAndRemove(
        [](const int& a) {
            return true;
        },
        [](const int& a) {
            if (a == 3)
                return true;
            else    
                return false;
        }
    );
    assert(list.size() == 3);

    // Special case - remove first thing
    list.visitIfAndRemove(
        [](const int& a) {
            return true;
        },
        [](const int& a) {
            if (a == 1)
                return true;
            else    
                return false;
        }
    );
    assert(list.size() == 2);
    assert(list.first() == 2);
}

int main(int,const char**) {
    test_1();
    test_2();
}
