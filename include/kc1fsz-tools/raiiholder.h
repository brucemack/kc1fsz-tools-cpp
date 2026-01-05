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
 * A generic RAII utility that will fire a callback when the holder is going out of scope.
 * This would typically be used to free a resource of some kind.
 */
template <typename T> class raiiholder {
public:

    raiiholder(T* o, std::function<void(T*)> f) :  _o(o), _f(f) { }
    ~raiiholder() { _f(_o); }

private:

    T* _o;
    std::function<void(T*)> _f;
};

}
