# SPDX-License-Identifier: MIT
#
# Copyright 2026 Michael Rodriguez
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the “Software”), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

# The FATAL_ERROR option is accepted but ignored by CMake 2.6 and higher. It
# should be specified so CMake versions 2.4 and lower fail with an error instead
# of just a warning.
cmake_minimum_required(VERSION 3.22.1 FATAL_ERROR)

include(CheckCCompilerFlag)
include(CheckLinkerFlag)

project(
	psycho
	VERSION 0.1.0
	DESCRIPTION "Sony PlayStation 1 emulator"
	HOMEPAGE_URL "https://github.com/mrodriguezme/psycho"
	LANGUAGES C
)

foreach(lang C ASM)
	set(CMAKE_${lang}_FLAGS "")

	foreach(cfg DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
		set(CMAKE_${lang}_FLAGS_${cfg} "")
	endforeach()
endforeach()

if (NOT PSYCHO_USING_CMAKE_PRESET)
	message(FATAL_ERROR
		"You must use a CMake preset to configure this project; run "
		"\"cmake --list-presets\" to list the available presets, then "
		"run \"cmake --preset <name>\" to select it."
	)
endif()

option(PSYCHO_ENABLE_SANITIZERS "Enable ASan and UBSan if possible" OFF)
option(PSYCHO_WARNINGS_ARE_ERRORS "Treat all warnings as errors" OFF)

toolchain_setup_c_base()

add_subdirectory(psycho)
add_subdirectory(app)
