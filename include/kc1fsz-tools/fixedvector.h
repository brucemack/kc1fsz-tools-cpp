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
#pragma once

#include <cassert>
#include <functional>

namespace kc1fsz {

/**
 * A vector structure that stores its members in a fixed
 * user-provided array. No heap allocations are used.
 */
template <typename T> class fixedvector {
public:

    fixedvector(T* store, unsigned size) 
    :   _data(store), 
        _size(size) {
    }

    unsigned size() const { return _size; }

    T& at(unsigned i) {
        assert(i < _size);
        return _data[i];
    }

    const T& at(unsigned i) const {
        assert(i < _size);
        return _data[i];
    }

    /**
     * Visitor with predicate
     * @param visitor Called for each item that passes the predicate. Should
     * return true to continue the visitation and false to stop it.
     * @param predicate Called for each item to decide if it should be visited.
     */
    void visitIf(std::function<bool(T&)> visitor,
        std::function<bool(const T&)> predicate) {
        bool keepGoing = true;
        for (unsigned i = 0; i < size() && keepGoing; i++)
            if (predicate == nullptr || predicate(at(i)))
                keepGoing = visitor(at(i));
    }

    void visitIf(std::function<bool(const T&)> visitor,
        std::function<bool(const T&)> predicate) const {
        bool keepGoing = true;
        for (unsigned i = 0; i < size() && keepGoing; i++)
            if (predicate == nullptr || predicate(at(i)))
                keepGoing = visitor(at(i));
    }

    void visitAll(std::function<bool(T&)> visitor) {
        visitIf(visitor, nullptr);
    }

    void visitAll(std::function<bool(const T&)> visitor) const {
        visitIf(visitor, nullptr);
    }

    /**
     * @returns Index of first item that satisfies the predicate, or 
     * -1 if none are found.
     */
    int firstIndex(std::function<bool(const T&)> predicate) const {
        for (unsigned i = 0; i < size(); i++)
            if (predicate(at(i)))
                return i;
        return -1;
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
   
private:

    T* _data;
    unsigned _size = 0;
};

}
