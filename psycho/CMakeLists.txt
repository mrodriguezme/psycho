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

add_library(psycho STATIC)
target_link_libraries(psycho PRIVATE psycho_cfg_base_c)

target_sources(
	psycho PRIVATE
	include/psycho/bus.h
	include/psycho/cpu-defs.h
	include/psycho/cpu.h
	include/psycho/ctx.h
	include/psycho/disasm.h
	include/psycho/log.h
	src/bus.c
	src/bus.h
	src/cpu-defs.h
	src/cpu.c
	src/cpu.h
	src/ctx.c
	src/disasm.c
	src/log.c
	src/log.h
)

target_include_directories(psycho PUBLIC include)
