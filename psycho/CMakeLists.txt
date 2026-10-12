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
	include/psycho/common/compiler_support.h
	include/psycho/common/sched.h
	include/psycho/debug/bios_trace.h
	include/psycho/debug/disasm.h
	include/psycho/debug/log.h
	include/psycho/debug/str.h
	include/psycho/devices/digital_pad.h
	include/psycho/system/cpu/defs.h
	include/psycho/system/cpu/interpreter.h
	include/psycho/system/cpu/ops.h
	include/psycho/system/bus.h
	include/psycho/system/dmac.h
	include/psycho/system/gpu.h
	include/psycho/system/intctrl.h
	include/psycho/system/sio0_dev.h
	include/psycho/system/sio0.h
	include/psycho/ctx.h
	src/common/sched.c
	src/common/sched.h
	src/common/util.h
	src/debug/bios_trace.c
	src/debug/bios_trace.h
	src/debug/disasm.c
	src/debug/disasm.h
	src/debug/log.c
	src/debug/log.h
	src/debug/str.c
	src/debug/str.h
	src/devices/digital_pad.c
	src/system/cpu/defs.c
	src/system/cpu/defs.h
	src/system/cpu/interpreter.c
	src/system/cpu/interpreter.h
	src/system/gpu/gpu.c
	src/system/gpu/gpu.h
	src/system/gpu/sw.c
	src/system/gpu/sw.h
	src/system/bus.c
	src/system/bus.h
	src/system/intctrl.c
	src/system/intctrl.h
	src/system/sio0_dev.c
	src/system/sio0.c
	src/system/sio0.h
	src/ctx.c
	src/exe_loader.c
	src/exe_loader.h
)

target_include_directories(psycho PUBLIC include)
target_include_directories(psycho PRIVATE src)
