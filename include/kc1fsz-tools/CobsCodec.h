/* 
 * Copyright (C) 2025 Bruce MacKinnon
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
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

#include <cstdint>

namespace kc1fsz {

/** 
 * COBS encode data to buffer.
 * 
 * @param data Pointer to input data to encode
 * @param length Number of bytes to encode
 * @param buffer Pointer to encoded output buffer
 * @return Encoded buffer length in bytes, or -1 if the output
 * buffer was too small.
*/
int cobsEncode(const uint8_t *data, unsigned length, 
    uint8_t *buffer, unsigned bufferCapacity);

/** COBS decode data from buffer
 * @param buffer Pointer to encoded input bytes
 * @param length Number of bytes to decode
 * @param data Pointer to decoded output data
 * @return Number of bytes successfully decoded
 */
int cobsDecode(const uint8_t *buffer, unsigned length, 
    uint8_t* data, unsigned dataCapacity);
}
