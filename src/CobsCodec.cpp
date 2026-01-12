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
#include <cassert>

#include "kc1fsz-tools/CobsCodec.h"

namespace kc1fsz {

int cobsEncode(const uint8_t *data, unsigned length, 
    uint8_t *buffer, unsigned bufferCapacity)
{
	assert(data && buffer);

    uint8_t* maxOut = buffer + bufferCapacity;
	uint8_t* encode = buffer;
	uint8_t* codep = encode++; // Output code pointer
	uint8_t code = 1; // Code value

	for (const uint8_t *byte = (const uint8_t *)data; length--; ++byte) {
        // Byte not zero, write it
		if (*byte) {
            if (maxOut == encode)
                return -1;
			*encode++ = *byte, ++code;
        }
        // Input is zero, restart
		if (!*byte) {
            if (maxOut == codep)
                return -1;
			*codep = code, code = 1, codep = encode;
			if (!*byte || length)
				++encode;
		}
	}
    if (maxOut == codep)
        return -1;
     // Write final code value
	*codep = code;

	return (int)(encode - buffer);
}

int cobsDecode(const uint8_t *buffer, unsigned length, 
    uint8_t* data, unsigned dataCapacity)
{
	assert(buffer && data);

    uint8_t* maxOut = data + dataCapacity;
	const uint8_t *byte = buffer; 
	uint8_t *decode = data; 

	for (uint8_t code = 0xff, block = 0; byte < buffer + length; --block) {
        // Decode block byte
		if (block) {
            if (maxOut == decode)
                return -1;
			*decode++ = *byte++;
        } 
        else {
            // Fetch the next block length
			block = *byte++;             
            // Encoded zero, write it unless it's delimiter.
			if (block && (code != 0xff)) {
                if (maxOut == decode)
                    return -1;
				*decode++ = 0;
            }
			code = block;
			if (!code) // Delimiter code found
				break;
		}
	}

	return (int)(decode - data);
}

}

