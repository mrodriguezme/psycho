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
	include/psycho/debug/bios_func_tracer.h
	include/psycho/debug/disasm.h
	include/psycho/debug/fixed_string.h
	include/psycho/debug/log.h
	include/psycho/devices/digital_pad.h
	include/psycho/system/cpu/defs.h
	include/psycho/system/cpu/interpreter.h
	include/psycho/system/cpu/ops.h
	include/psycho/system/bus.h
	include/psycho/system/dma.h
	include/psycho/system/gpu.h
	include/psycho/system/interrupt_controller.h
	include/psycho/system/sio0_device.h
	include/psycho/system/sio0.h
	include/psycho/compiler_support.h
	include/psycho/ctx.h
	include/psycho/scheduler.h
	src/debug/bios_func_tracer.c
	src/debug/bios_func_tracer.h
	src/debug/disasm.c
	src/debug/disasm.h
	src/debug/fixed_string.c
	src/debug/fixed_string.h
	src/debug/log.c
	src/debug/log.h
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
	src/system/interrupt_controller.c
	src/system/interrupt_controller.h
	src/system/sio0_device.c
	src/system/sio0.c
	src/system/sio0.h
	src/ctx.c
	src/exe_loader.c
	src/exe_loader.h
	src/scheduler.c
	src/scheduler.h
	src/util.h
)

target_include_directories(psycho PUBLIC include)
target_include_directories(psycho PRIVATE src)
