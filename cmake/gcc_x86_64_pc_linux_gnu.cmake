# SPDX-License-Identifier: MIT
#
# Copyright 2026 Michael Rodriguez
#
# Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
# documentation files (the "Software"), to deal in the Software without restriction, including without limitation the
# rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit
# persons to whom the Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all copies or substantial portions of the
# Software.
#
# THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
# WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
# COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
# OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

include_guard(GLOBAL)

include(CheckCCompilerFlag)
include(CheckLinkerFlag)

set(CMAKE_SYSTEM_NAME Linux)

set(CMAKE_C_FLAGS_DEBUG_INIT "")
set(CMAKE_ASM_FLAGS_DEBUG_INIT "")

find_program(
	CMAKE_C_COMPILER
	NAMES gcc
	REQUIRED
)

find_program(
	CMAKE_ASM_COMPILER
	NAMES gcc
	REQUIRED
)

find_program(
	CMAKE_AR
	NAMES ar
	REQUIRED
)

find_program(
	CMAKE_RANLIB
	NAMES ranlib
	REQUIRED
)

find_program(
	CMAKE_OBJCOPY
	NAMES objcopy
	REQUIRED
)

find_program(
	CMAKE_SIZE_UTIL
	NAMES size
	REQUIRED
)

execute_process(
        COMMAND ${CMAKE_C_COMPILER} -print-sysroot
        OUTPUT_VARIABLE TOOLCHAIN_SYSROOT
        OUTPUT_STRIP_TRAILING_WHITESPACE
)

set(CMAKE_SYSROOT ${TOOLCHAIN_SYSROOT})

add_library(psycho_c_base INTERFACE)

function(add_compiler_flags)
	cmake_parse_arguments(PARSE_ARGV 0 ARG "REQUIRED;LINK" "" "")

	foreach(FLAG IN LISTS ARG_UNPARSED_ARGUMENTS)
		string(MAKE_C_IDENTIFIER "${FLAG}" FLAG_ID)

		if (ARG_LINK)
			set(var_name "HAVE_LINK_FLAG_${FLAG_ID}")
			set(CMAKE_REQUIRED_LINK_OPTIONS ${FLAG})
		else()
			set(var_name "HAVE_FLAG_${FLAG_ID}")
		endif()

		check_c_compiler_flag(${FLAG} ${var_name})

		if (${var_name})
			target_compile_options(psycho_c_base INTERFACE "$<$<COMPILE_LANGUAGE:C>:${FLAG}>")

			if (ARG_LINK)
				target_link_options(psycho_c_base INTERFACE ${FLAG})
			endif()
		elseif (ARG_REQUIRED)
			message(FATAL_ERROR
				"Required compiler flag '${FLAG}' is not supported by ${CMAKE_C_COMPILER} "
				"(${CMAKE_C_COMPILER_ID} ${CMAKE_C_COMPILER_VERSION})")
		endif()
	endforeach()
endfunction()

function(toolchain_setup_c_base)
	add_compiler_flags(
		REQUIRED
		-ggdb3
		-std=gnu23
	)

	add_compiler_flags(
		-Walloc-zero
		-Walloca
		-Wbad-function-cast
		-Wcast-qual
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
		-Wunused-macros
		-Wvla
		-Wwrite-strings
	)

	if (PSYCHO_ENABLE_SANITIZERS)
		add_compiler_flags(REQUIRED LINK -fsanitize=address,undefined)
	endif()

	if (PSYCHO_WARNINGS_ARE_ERRORS)
		add_compiler_flags(
			REQUIRED
			-Werror
		)
	endif()

	if (CMAKE_BUILD_TYPE STREQUAL "Debug")
		add_compiler_flags(
			REQUIRED
			-Og
		)
	elseif (CMAKE_BUILD_TYPE STREQUAL "Release")
		add_compiler_flags(
			REQUIRED
			-Ofast
		)
		target_compile_definitions(psycho_c_base INTERFACE "$<$<COMPILE_LANGUAGE:C>:NDEBUG>")
	endif()
endfunction()
