# Copyright (C) 2024, Bruce MacKinnon KC1FSZ
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# ( at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
#
# NOT FOR COMMERCIAL USE WITHOUT PERMISSION.
#
cmake_minimum_required(VERSION 3.13)
if(DEFINED ENV{PICO_BUILD})
include(pico_sdk_import.cmake)
endif()
project(kc1fsz-tools-cpp C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 23)
file(MAKE_DIRECTORY tmp)

if(DEFINED ENV{PICO_BUILD})
pico_sdk_init()
endif()

if(DEFINED ENV{PICO_BUILD})
message("----- PICO build ---------------------------------------------------------------")
add_compile_options(-Wall -g)
else()
message("Desktop build!")
add_compile_options(-fstack-protector-all -Wall -Wpedantic -g)
endif()

# ------ audio-test-1 ---------------------------------------------------------
# Target: Host

add_executable(audio-test-1
  tests/audio-test-1.cpp
  src/Common.cpp
  src/AudioAnalyzer.cpp
) 

target_include_directories(audio-test-1 PRIVATE src)
target_include_directories(audio-test-1 PRIVATE include)

# ------ uart-test-1 ----------------------------------------------------------
# Target: RP2040 board

add_executable(uart-test-1
  tests/rp2040/uart-test-1.cpp
  src/rp2040/PicoUartChannel.cpp
  src/rp2040/PicoPollTimer.cpp
)

if(DEFINED ENV{PICO_BUILD})
pico_enable_stdio_usb(uart-test-1 1)
pico_enable_stdio_uart(uart-test-1 0)
target_compile_definitions(uart-test-1 PUBLIC PICO_BUILD=1)
target_link_libraries(uart-test-1 pico_stdlib hardware_i2c hardware_adc pico_multicore)
endif()
target_include_directories(uart-test-1 PRIVATE src)
target_include_directories(uart-test-1 PRIVATE include)

# ------ dns--test-1 ---------------------------------------------------------
# Target: Host

add_executable(dns-test-1
  tests/dns-test-1.cpp
  src/Common.cpp
  src/MicroDNS.cpp
  src/NetUtils.cpp
) 

target_include_directories(dns-test-1 PRIVATE src)
target_include_directories(dns-test-1 PRIVATE include)

# ------ gen-test-1 ---------------------------------------------------------
# Target: Host

add_executable(gen-test-1
  tests/gen-test-1.cpp
  src/Common.cpp
) 

target_include_directories(gen-test-1 PRIVATE src)
target_include_directories(gen-test-1 PRIVATE include)
