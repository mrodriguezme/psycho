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

if (CMAKE_CONFIGURATION_TYPES)
	message(FATAL_ERROR
		"This project does not support multi-config generators; please "
		"use a single-config generator such as Ninja or Unix Makefiles."
	)
endif()

option(PSYCHO_ENABLE_SANITIZERS "Enable ASan and UBSan if possible" OFF)
option(PSYCHO_WARNINGS_ARE_ERRORS "Treat all warnings as errors" OFF)

add_library(psycho_cfg_base_c INTERFACE)

function(add_compiler_flags)
	foreach(FLAG IN LISTS ARGN)
		string(MAKE_C_IDENTIFIER "${FLAG}" FLAG_ID)
		set(var_name "HAVE_FLAG_${FLAG_ID}")

		check_c_compiler_flag(${FLAG} ${var_name})

		if(${var_name})
			target_compile_options(
				psycho_cfg_base_c INTERFACE ${FLAG}
			)
		endif()
	endforeach()
endfunction()

add_compiler_flags(
	-std=gnu17
	-Walloc-zero
	-Walloca
	-Wbad-function-cast
	-Wcast-qual
	-Wc++-compat
	-Wdate-time
	-Wdisabled-optimization
	-Wdouble-promotion
	-Wduplicated-branches
	-Wduplicated-cond
	-Wextra
	-Wfloat-equal
	-Wformat-signedness
	-Wformat=2
	-Winit-self
	-Winline
	-Winvalid-pch
	-Wlogical-op
	-Wmissing-declarations
	-Wmissing-include-dirs
	-Wmissing-prototypes
	-Wnested-externs
	-Wno-aggressive-loop-optimizations
	-Wnull-dereference
	-Wold-style-declaration
	-Wold-style-definition
	-Wpacked
	-Wredundant-decls
	-Wshadow
	-Wshift-overflow=2
	-Wstrict-prototypes
	-Wsuggest-attribute=cold
	-Wsuggest-attribute=const
	-Wsuggest-attribute=format
	-Wsuggest-attribute=malloc
	-Wsuggest-attribute=noreturn
	-Wsuggest-attribute=pure
	-Wswitch-default
	-Wswitch-enum
	-Wsync-nand
	-Wtrampolines
	-Wundef
	-Wuninitialized
	-Wunknown-pragmas
	-Wunsafe-loop-optimizations
	-Wunsuffixed-float-constants
	-Wunused-macros
	-Wvla
	-Wwrite-strings
)

if (PSYCHO_ENABLE_SANITIZERS)
	check_linker_flag(
		C
		-fsanitize=address,undefined
		HAVE_LINKER_SANITIZERS
	)

	set(CMAKE_REQUIRED_LINK_OPTIONS "-fsanitize=address,undefined")

	check_c_compiler_flag(
		-fsanitize=address,undefined
		HAVE_COMPILER_SANITIZERS
	)

	if (HAVE_COMPILER_SANITIZERS AND HAVE_LINKER_SANITIZERS)
		target_compile_options(
			psycho_cfg_base_c INTERFACE
			-fsanitize=address,undefined
		)

		target_link_options(
			psycho_cfg_base_c INTERFACE
			-fsanitize=address,undefined
		)
	endif()
endif()

if (PSYCHO_WARNINGS_ARE_ERRORS)
	add_compiler_flags(-Werror)
endif()

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
	add_compiler_flags(-Og)
elseif (CMAKE_BUILD_TYPE STREQUAL "Release")
	add_compiler_flags(-Ofast)
else()
	message(FATAL_ERROR "CMAKE_BUILD_TYPE must be either Debug or Release.")
endif()

if ((CMAKE_C_COMPILER_ID STREQUAL "AppleClang") OR
    (CMAKE_C_COMPILER_ID STREQUAL "Clang"))
	add_compiler_flags(-glldb)
elseif (CMAKE_C_COMPILER_ID STREQUAL "GNU")
	add_compiler_flags(-ggdb3)
else()
	message(
		FATAL_ERROR
		"Your C compiler is not supported by this project; please use "
		"either clang or gcc."
	)
endif()

add_subdirectory(psycho)
add_subdirectory(app)
