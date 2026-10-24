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
	src/common/util.h
	src/debug/bios_call_tracer.c
	src/debug/bios_call_tracer.h
	src/debug/disasm.c
	src/debug/disasm.h
	src/debug/fixed_string.c
	src/debug/fixed_string.h
	src/debug/log.c
	src/debug/log.h
	src/lib/lifo.h
	src/lib/ring_buffer.h
	src/peripherals/input/digital_pad.c
	src/peripherals/storage/memory_card.c
	src/system/cdrom_drive/cdrom_drive.c
	src/system/cdrom_drive/cdrom_drive.h
	src/system/cdrom_drive/defs.h
	src/system/cpu/backend/interpreter.c
	src/system/cpu/backend/interpreter.h
	src/system/cpu/defs.c
	src/system/cpu/defs.h
	src/system/dmac/defs.h
	src/system/dmac/dmac.c
	src/system/dmac/dmac.h
	src/system/gpu/backend/software/software.c
	src/system/gpu/backend/software/software.h
	src/system/gpu/defs.h
	src/system/gpu/gpu.c
	src/system/gpu/gpu.h
	src/system/intc/defs.h
	src/system/intc/intc.c
	src/system/intc/intc.h
	src/system/sbus/defs.h
	src/system/sbus/sbus.c
	src/system/sbus/sbus.h
	src/system/sio0/defs.h
	src/system/sio0/peripheral.c
	src/system/sio0/sio0.c
	src/system/sio0/sio0.h
	src/system/sio1/defs.h
	src/system/sio1/sio1.c
	src/system/sio1/sio1.h
	src/system/spu/defs.h
	src/system/spu/spu.c
	src/system/spu/spu.h
	src/system/timers/defs.h
	src/system/timers/timers.c
	src/system/timers/timers.h
	src/system/sched.c
	src/system/sched.h
	src/ctx.c
	src/exe_loader.c
	src/exe_loader.h
)

target_include_directories(psycho PUBLIC include)
target_include_directories(psycho PRIVATE src)

set_source_files_properties(
	src/cpu/interpreter.c PROPERTIES
	COMPILE_OPTIONS "-fno-gcse;-fno-crossjumping"
)
