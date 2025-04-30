#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cassert>

#include "kc1fsz-tools/SWDUtils.h"

namespace kc1fsz {

/**
 * Makes a 16-bit RP2040 ROM table code from the two 
 * ASCII characters. 
 */
static uint16_t rom_table_code(char c1, char c2) {
    return (c2 << 8) | c1;
}      

/**
 * Calls a function via the PICO debug_trampoline() helper function.
 * 
 * IMPORTANT: We are assuming the target is already in debug/halt mode 
 * when this function is called.  On exit, this target will still be 
 * in debug/halt mode.
 */
std::expected<uint32_t, int> call_function(SWDDriver& swd, 
    uint32_t trampoline_addr, uint32_t func_addr, 
    uint32_t a0, uint32_t a1, uint32_t a2, uint32_t a3) {

    printf("Calling %08X ( %08X, %08X, %08X, %08X)\n", func_addr, a0, a1, a2, a3);

    // Write the target function address in the r7 register
    if (const auto r = swd.writeWordViaAP(ARM_DCRDR, func_addr); r != 0)
        return std::unexpected(-1);
    // [16] is 1 (write), [6:0] 0b0000111 means r7
    if (const auto r = swd.writeWordViaAP(ARM_DCRSR, 0x00010007); r != 0) 
        return std::unexpected(-2);
    // Poll to find out if the write is done
    if (swd.pollREGRDY() != 0)
        return std::unexpected(-3);

    // Write the arguments in the r0-r3 registers

    if (const auto r = swd.writeWordViaAP(ARM_DCRDR, a0); r != 0)
        return std::unexpected(-4);
    if (const auto r = swd.writeWordViaAP(ARM_DCRSR, 0x00010000); r != 0) 
        return std::unexpected(-5);
    if (swd.pollREGRDY() != 0)
        return std::unexpected(-6);

    if (const auto r = swd.writeWordViaAP(ARM_DCRDR, a1); r != 0)
        return std::unexpected(-4);
    if (const auto r = swd.writeWordViaAP(ARM_DCRSR, 0x00010001); r != 0) 
        return std::unexpected(-5);
    if (swd.pollREGRDY() != 0)
        return std::unexpected(-6);

    if (const auto r = swd.writeWordViaAP(ARM_DCRDR, a2); r != 0)
        return std::unexpected(-4);
    if (const auto r = swd.writeWordViaAP(ARM_DCRSR, 0x00010002); r != 0) 
        return std::unexpected(-5);
    if (swd.pollREGRDY() != 0)
        return std::unexpected(-6);

    if (const auto r = swd.writeWordViaAP(ARM_DCRDR, a3); r != 0)
        return std::unexpected(-4);
    if (const auto r = swd.writeWordViaAP(ARM_DCRSR, 0x00010003); r != 0) 
        return std::unexpected(-5);
    if (swd.pollREGRDY() != 0)
        return std::unexpected(-6);

    // Write a value in the MSP register
    if (const auto r = swd.writeWordViaAP(ARM_DCRDR, 0x20000080); r != 0)
        return std::unexpected(-7);
    // [16] is 1 (write), [6:0] 0b0001101 means MSP
    if (const auto r = swd.writeWordViaAP(ARM_DCRSR, 0x0001000d); r != 0) 
        return std::unexpected(-8);
    // Poll to find out if the write is done
    if (swd.pollREGRDY() != 0)
        return std::unexpected(-9);

    // Set xPSR to 0x01000000 (ESPR.T=1).
    // NOTE: I DON'T UNDERSTANT THIS.  IS IT NEEDED?
    if (const auto r = swd.writeWordViaAP(ARM_DCRDR, 0x01000000); r != 0)
        return std::unexpected(-10);
    if (const auto r = swd.writeWordViaAP(ARM_DCRSR, 0x00010010); r != 0) 
        return std::unexpected(-11);
    // Poll to find out if the write is done
    if (swd.pollREGRDY() != 0)
        return std::unexpected(-12);

    // Write a value into the debug return address value. 
    if (const auto r = swd.writeWordViaAP(ARM_DCRDR, trampoline_addr); r != 0)
        return std::unexpected(-10);
    // [16] is 1 (write), [6:0] 0b0001111 
    if (const auto r = swd.writeWordViaAP(ARM_DCRSR, 0x0001000f); r != 0) 
        return std::unexpected(-11);
    // Poll to find out if the write is done
    if (swd.pollREGRDY() != 0)
        return std::unexpected(-12);

    // Clear DFSR
    uint32_t dfsr = 0;
    if (const auto r = swd.readWordViaAP(0xE000ED30); !r.has_value()) {
        return std::unexpected(-17);
    }
    else {
        dfsr = *r;
    }
    if (const auto r = swd.writeWordViaAP(0xE000ED30, dfsr); r != 0)
        return std::unexpected(-13);

    // Remove halt, keep MASKINT and DEBUGEN.  We should run until the BKPT #0 instruction
    if (const auto r = swd.writeWordViaAP(0xe000edf0, 0xa05f0000 | 0b001 | 0b1000); r != 0)
        return std::unexpected(-13);

    // Poll DHCSR, looking for the debug halt status
    while (true) {
        if (const auto r = swd.readWordViaAP(0xe000edf0); !r.has_value()) {
            return std::unexpected(-14);
        } else {
            if (*r == 0x0003000B)
                break;
        }
    }

    // Read back from the r0 register
    // [16] is 0 (read), [6:0] 0b0000000 means r0
    if (const auto r = swd.writeWordViaAP(ARM_DCRSR, 0x00000000); r != 0)
        return std::unexpected(-15);
    // Poll to find out if the read is done
    if (swd.pollREGRDY() != 0)
        return std::unexpected(-16);
    if (const auto r = swd.readWordViaAP(ARM_DCRDR); !r.has_value()) {
        return std::unexpected(-17);
    } else {
        return *r;
    }
}

/**
 * NOTES:
 * - It is assumed that the SWD initialization is complete
 * - It is assumed that the AP is selected (bank 0).
 */
int reset(SWDDriver& swd) {

    // Clear DEMCR.VC_CORERESET so that we don't enter debug mode
    // on the next reset
    if (const auto r = swd.writeWordViaAP(0xE000EDFC, 0x00000000); r != 0)
        return -13;

    // Finalize any last writes
    swd.writeBitPattern("00000000");

    // Leave debug
    if (const auto r = swd.writeWordViaAP(0xe000edf0, 0xa05f0000); r != 0) 
        return -14;

    // Trigger a reset by writing SYSRESETREQ to NVIC.AIRCR.
    if (const auto r = swd.writeWordViaAP(0xe000ed0c, 0x05fa0004); r != 0) 
        return -15;

    // Finalize any last writes
    swd.writeBitPattern("00000000");

    return 0;
}




/**
 * NOTES:
 * - It is assumed that the SWD initialization is complete
 * - It is assumed that the AP is selected (bank 0).
 */
int reset_into_debug(SWDDriver& swd) {

    // Enable debug mode and halt
    if (const auto r = swd.writeWordViaAP(0xe000edf0, 0xa05f0000 | 0b1011); r != 0) 
        return -7;

    // Set DEMCR.VC_CORERESET=1 so that we come up in debug mode after reset
    // (i.e. vector catch)
    if (const auto r = swd.writeWordViaAP(0xE000EDFC, 0x00000001); r != 0)
        return -8;

    // Trigger a reset by writing SYSRESETREQ to NVIC.AIRCR.
    if (const auto r = swd.writeWordViaAP(0xe000ed0c, 0x05fa0004); r != 0)
        return -9;

    // Finalize any last writes
    swd.writeBitPattern("00000000");

    // ???
    // TODO: Figure out what to poll for to avoid this race condition
    swd.delayMs(10);

    return 0;
}

/**
 * Takes a binary image and flashes it at the specified offset.
 * 
 * NOTES:
 * - It is assumed that the SWD is initialize, the correct AP is selected 
 *   and AP bank 0 is selected before calling this function!
 * - The image must be a multiple of 4096 bytes.
 * 
 * @param offset Offset from the start of FLASH.  So 0 means start of flash.
 * @param image_bin_len Must be a multiple of 4096!
 */
int flash_and_verify(SWDDriver& swd, uint32_t offset, const uint8_t* image_bin, uint32_t image_bin_len) {

    assert(image_bin_len % 4096 == 0);

    // Move VTOR to SRAM in preparation 
    if (const auto r = swd.writeWordViaAP(0xe000ed08, 0x20000000); r != 0)
        return -12;

    // ----- Get the ROM function locations -----------------------------------

    // Observation from an RP2040: table pointer is a 0x7a. This is interesting
    // because it is not a multiple of 4, so we need to be careful when searching
    // the lookup table - use half-word accesses.

    // Get the start of the ROM function table
    uint16_t tab_ptr;
    if (const auto r = swd.readHalfWordViaAP(0x00000014); !r.has_value()) {
        return -3;
    } else {
        tab_ptr = *r;
    }

    uint16_t rom_connect_internal_flash_func = 0;
    uint16_t rom_flash_exit_xip_func = 0;
    uint16_t rom_flash_range_erase_func = 0;
    uint16_t rom_flash_range_program_func = 0;
    uint16_t rom_flash_flush_cache_func = 0;
    uint16_t rom_flash_enter_cmd_xip_func = 0;
    uint16_t rom_debug_trampoline_func = 0;
    uint16_t rom_debug_trampoline_end_func = 0;

    // Iterate through the table until we find a null function code
    // Each entry word is two 8-bit codes and a 16-bit 
    // address.  

    while (true) {

        uint16_t func_code;
        if (const auto r = swd.readHalfWordViaAP(tab_ptr); !r.has_value()) {
            return -4;
        } else {
            func_code = *r;
        }

        uint16_t func_addr;
        if (const auto r = swd.readHalfWordViaAP(tab_ptr + 2); !r.has_value()) {
            return -5;
        } else {
            func_addr = *r;
        }

        if (func_code == 0)
            break;

        if (func_code == rom_table_code('I', 'F'))
            rom_connect_internal_flash_func = func_addr;
        else if (func_code == rom_table_code('E', 'X'))
            rom_flash_exit_xip_func = func_addr;
        else if (func_code == rom_table_code('R', 'E'))
            rom_flash_range_erase_func = func_addr;
        else if (func_code == rom_table_code('R', 'P'))
            rom_flash_range_program_func = func_addr;
        else if (func_code == rom_table_code('F', 'C'))
            rom_flash_flush_cache_func = func_addr;
        else if (func_code == rom_table_code('C', 'X'))
            rom_flash_enter_cmd_xip_func = func_addr;
        else if (func_code == rom_table_code('D', 'T'))
            rom_debug_trampoline_func = func_addr;
        else if (func_code == rom_table_code('D', 'E')) 
            rom_debug_trampoline_end_func = func_addr;
        
        tab_ptr += 4;
    }

    // Take size of binary and pad it up to a 4K boundary
    const unsigned int page_size = 4096;
    unsigned int whole_pages = image_bin_len / page_size;
    unsigned int remainder = image_bin_len % page_size;
    // Make sure we are using full pages
    if (remainder)
        whole_pages++;
    // Make a zero buffer large enough and copy in the code
    unsigned int buf_len = whole_pages * page_size;
    unsigned int buf_words = buf_len / 4;
    uint8_t* buf = (uint8_t*)malloc(buf_len); 
    memset(buf, 0, buf_len);
    memcpy(buf, image_bin, image_bin_len);

    const unsigned int ram_workarea = 0x20000100;     
    printf("Programing %u bytes ...\n", buf_len);

    // Get the flash into serial write mode
    if (const auto r = call_function(swd, rom_debug_trampoline_func, rom_connect_internal_flash_func, 0, 0, 0, 0); !r.has_value())
        return -1;
    if (const auto r = call_function(swd, rom_debug_trampoline_func, rom_flash_exit_xip_func, 0, 0, 0, 0); !r.has_value())
        return -1;

    // Erase everything using the largest block size possible
    if (const auto r = call_function(swd, rom_debug_trampoline_func, rom_flash_range_erase_func, offset, buf_len, 1 << 16, 0xd8); !r.has_value())
        return -1;
    if (const auto r = call_function(swd, rom_debug_trampoline_func, rom_flash_flush_cache_func, 0, 0, 0, 0); !r.has_value())
        return -1;

    // Process each page individually
    for (unsigned int page = 0; page < whole_pages; page++) {

        // TODO: Make a complete page

        // Copy the page into RAM 
        if (const auto r = swd.writeMultiWordViaAP(ram_workarea, (const uint32_t*)(buf + (page * page_size)), page_size / 4); r != 0)
            return -1;
        
        // Flash the page from RAM -> FLASH
        if (const auto r = call_function(swd, rom_debug_trampoline_func, rom_flash_range_program_func, 
            offset + page * page_size, ram_workarea, page_size, 0); !r.has_value())
            return -1;
    }

    // Get back into normal flash reading mode
    if (const auto r = call_function(swd, rom_debug_trampoline_func, rom_flash_flush_cache_func, 0, 0, 0, 0); !r.has_value())
        return -1;
    if (const auto r = call_function(swd, rom_debug_trampoline_func, rom_flash_enter_cmd_xip_func, 0, 0, 0, 0); !r.has_value())
        return -1;

    printf("Verifying ...\n");

    // Verify each page individually
    for (unsigned int page = 0; page < whole_pages; page++) {

        // TODO: Make a complete page

        // Copy the page into RAM 
        if (const auto r = swd.writeMultiWordViaAP(ram_workarea, (const uint32_t*)(buf + (page * page_size)), page_size / 4); r != 0)
            return -1;
        
        uint32_t ram_ptr = ram_workarea;
        uint32_t flash_ptr = 0x10000000 + offset + (page * page_size);
        
        for (unsigned int i = 0; i < page_size / 4; i++) {

            // Compare the RAM word to the FLASH word
            uint32_t ram, flash;

            if (const auto r = swd.readWordViaAP(ram_ptr); !r.has_value()) {
                return -1;
            } else {
                ram = *r;
            }
            if (const auto r = swd.readWordViaAP(flash_ptr); !r.has_value()) {
                return -1;
            } else {
                flash = *r;
            }

            if (ram != flash) {
                printf("Verify failure at word %08X RAM: %08X, FLASH: %08X\n", i, ram, flash);
                break;
            }

            ram_ptr += 4;
            flash_ptr += 4;
        }
    }

    return 0;
}

}
