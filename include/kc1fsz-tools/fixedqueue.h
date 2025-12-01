/**
 * Copyright (C) 2025, Bruce MacKinnon KC1FSZ
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef _fixedqueue_h
#define _fixedqueue_h

#include <cassert>
#include <functional>

namespace kc1fsz {

/**
 * A queue structure that stores its members in a fixed
 * user-provided structure. No heap allocations are used.
 */
template <typename T> class fixedqueue {
public:

    fixedqueue(T* store, unsigned maxSize) 
    :   _data(store), 
        MAX_SIZE(maxSize) {
    }

    bool empty() const { return _size == 0; }
    bool isEmpty() const { return empty(); }
    unsigned size() const { return _size; }
    bool hasCapacity() const { return _size < MAX_SIZE; }

    /**
     * Pushes on the back
     */
    void push(const T& t) {
        if (_size == MAX_SIZE) 
            assert(false);
        _data[_size++] = t;
    }

    /**
     * Pops from the front
     */
    void pop() {
        remove(0);
    }

    /** 
     * Removes an arbitrary item
     */
    void remove(unsigned pos) {
        assert(pos < _size);
        if (pos < _size - 1) {
            // Shift left
            for (unsigned i = pos; i < _size - 1; i++)
                _data[i] = _data[i + 1];
        }
        _size--;
    }

    void visitAndRemoveIf(std::function<bool(const T&)> visitor,
        std::function<bool(const T&)> predicate) {
        unsigned pos = 0;
        bool keepGoing = true;
        while (pos < size() && keepGoing) {
            if (predicate(at(pos))) {
                keepGoing = visitor(at(pos));
                remove(pos);
            }
            else
                pos++;
        }
    }

    /**
     * Remove all items for which the predicate is true.
     */
    void removeIf(std::function<bool(const T&)> predicate) {
        unsigned pos = 0;
        while (pos < size()) {
            if (predicate(at(pos))) 
                remove(pos);
            else
                pos++;
        }
    }

    /**
     * Visitor with predicate
     */
    void visitIf(std::function<bool(const T&)> visitor,
        std::function<bool(const T&)> predicate) {
        bool keepGoing = true;
        for (unsigned i = 0; i < size() && keepGoing; i++)
            if (predicate(at(i)))
                keepGoing = visitor(at(i));
    }

    /**
     * Counter with predicate
     */
    unsigned countIf(std::function<bool(const T&)> predicate) {
        unsigned result = 0;;
        for (unsigned i = 0; i < size(); i++)
            if (predicate(at(i)))
                result++;
        return result;
    }

    const T& first() const { return _data[0]; }
    
    const T& at(unsigned pos) const { 
        assert(pos < _size);
        return _data[pos];
    }

    void clear() {
        _size = 0;
    }

private:

    T* _data;
    const unsigned MAX_SIZE;
    unsigned _size = 0;
};

}

#endif
