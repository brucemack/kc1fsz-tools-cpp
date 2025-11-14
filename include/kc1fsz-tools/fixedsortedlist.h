#pragma once

#include <cassert>
#include <functional>
#include <iostream>

using namespace std;

namespace kc1fsz {

/**
 * A sorted list that works from fixed storage. NO DYNAMIC MEMORY IS USED.
 */
template <typename T> class fixedsortedlist {
public:

    /**
     * @param comparator. Should return -1 if a < b, 0 if a == b, and 1 if a > b.
     */
    fixedsortedlist(T* objSpace, unsigned* ptrSpace, unsigned spaceSize,
        std::function<int(const T& a, const T& b)> comparator) 
    :   _objSpace(objSpace), 
        _ptrSpace(ptrSpace),
        _spaceSize(spaceSize),
        _comparator(comparator),
        _firstPtr(-1),
        _freePtr(0) {
        clear();
    }

    void clear() {
        _firstPtr = -1;
        _freePtr = 0;
        // Get the free list linked
        for (unsigned i = 0; i < _spaceSize - 1; i++)            
            _ptrSpace[i] = i + 1;
        _ptrSpace[_spaceSize - 1] = -1;
    }

    bool empty() const { return _firstPtr == -1; }
    bool hasCapacity() const { return _freePtr != -1; }

    /**
     * Not fast, so use empty() if that's the goal.
     * @returns The number of elements. 
     */
    unsigned size() const {
        unsigned total = 0;
        visitAll([&total](const T&) { total++; return true; });
        return total;
    }

    /**
     * Walks across the items while the visitor says to keep going.
     */
    void visitAll(std::function<bool(const T&)> visitor) const {

        int slot = _firstPtr;
        bool keepGoing = true;

        while (slot != -1 && keepGoing) {
            keepGoing = visitor(_objSpace[slot]);
            slot = _ptrSpace[slot];
        }
    }

    /**
     * Walks across the list (in order) and visits anything that 
     * satisfies the predicate while the visitor says keep going. 
     * After successful visitation the item is removed from the list.
     */
    void visitIfAndRemove(std::function<bool(const T&)> predicate,
        std::function<bool(const T&)> visitor) {
        
        int previousSlot = -1; 
        int slot = _firstPtr;
        bool keepGoing = true;

        while (slot != -1 && keepGoing) {

            // Keep track of where we are going next
            int nextSlot = _ptrSpace[slot];

            if (predicate(_objSpace[slot])) {
                
                keepGoing = visitor(_objSpace[slot]);

                // Remove the current slot, taking into account the special
                // case of removing the first element.
                if (slot == _firstPtr) {
                    _firstPtr = _ptrSpace[slot];
                    previousSlot = -1;
                } else {
                    _ptrSpace[previousSlot] = nextSlot;
                    // In this case the previous slot remains unchanged!
                }
                // Put the slot back on the free list
                _ptrSpace[slot] = _freePtr;
                _freePtr = slot;
            } else {
                previousSlot = slot;
            }
            slot = nextSlot;
        }
    }

    /**
     * Inserts an item at the right place (sequentially) in the list.
     * @returns true on success, false if max capacity has been reached.
     */
    bool insert(T proposedObj) {
        // All full?
        if (_freePtr == -1)
            return false;
        // Take (and remove) a new slot from the start of the free list 
        unsigned allocatedSlot = _freePtr;
        _freePtr = _ptrSpace[allocatedSlot];
        // If the list was empty then create the new first entry and we're done.
        if (_firstPtr == -1) {
            _firstPtr = allocatedSlot;
            _objSpace[allocatedSlot] = proposedObj;
            _ptrSpace[allocatedSlot] = -1;
            return true;
        }
        // Otherwise, scan through the chain looking for our place in the sequence.
        // Look for the first item that is *larger* than the proposed one and 
        // insert just before that point in the chain.
        else {
            int previousSlot = -1;
            int slot = _firstPtr;
            while (slot != -1) {
                // This value is -1 when the slot is older, +1 when slot is newer.
                int compare = _comparator(_objSpace[slot], proposedObj);
                if (compare > 0) {
                    // We've found the right place, insert
                    _objSpace[allocatedSlot] = proposedObj;
                    _ptrSpace[allocatedSlot] = slot;
                    // Special handling for insertion at the start of the list
                    if (previousSlot == -1)
                        _firstPtr = allocatedSlot;
                    // Otherwise, link the previous slot to the newly inserted
                    else 
                        _ptrSpace[previousSlot] = allocatedSlot;
                    return true;
                }
                // Keep walking the chain
                previousSlot = slot;
                slot = _ptrSpace[slot];
            }
            // If we reach this point then there was nothing larger in the chain, 
            // so we add at the end - we are the largest thing.
            _objSpace[allocatedSlot] = proposedObj;
            _ptrSpace[allocatedSlot] = -1;
            _ptrSpace[previousSlot] = allocatedSlot;
            return true;
        }
    }

private:

    T* _objSpace;
    unsigned* _ptrSpace;
    unsigned _spaceSize;
    std::function<int(const T&, const T&)> _comparator;

    int _firstPtr;
    int _freePtr;
};

}

