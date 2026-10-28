# psycho - Sony PlayStation 1 emulator

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

## Introduction

psycho is an in-development [Sony PlayStation 1](https://en.wikipedia.org/wiki/PlayStation_%28console%29)
emulator written in the [C programming language](https://en.wikipedia.org/wiki/C_%28programming_language%29)
targeting the [C17 ISO/IEC 9899:2018](https://en.wikipedia.org/wiki/C17_%28C_standard_revision%29) standard.

This is not suitable for production usage in any way, shape, or form at this
time. If you want a production quality PS1 emulator, use [DuckStation](https://www.duckstation.org/).

## Why the name psycho?

Commercial games for the PlayStation 1 were developed with an SDK provided by Sony called PsyQ, which was created as a
result of a joint venture between [SN Systems Limited](https://en.wikipedia.org/wiki/SN_Systems) and [Psygnosis](https://en.wikipedia.org/wiki/Psygnosis).

> Pedantics incoming: PsyQ is really just the compiler toolchain, and the actual runtime libraries for use on the
> PlayStation 1 were developed by Sony themselves. However, the rest of the world calls the official SDK "PsyQ", so to
> avoid maximum confusion we'll also keep doing that, although it's not quite accurate.

Fast forward some 30+ years later, armed with newfound knowledge and reverse engineering efforts over that timeframe,
great strides have been made in our understanding of both PsyQ and the PlayStation 1's hardware.

Back in the PlayStation 1's heyday, Sony required commercial developers to use PsyQ when developing their games.
These days you'd have to be psycho to use it as we have modern SDK alternatives like [psyqo](https://github.com/pcsx-redux/nugget/tree/main/psyqo)
or hell, you can even [go bare-metal](https://github.com/spicyjpeg/ps1-bare-metal/tree/main) and do it all yourself;
either way will be much more efficient than PsyQ itself, much less the environment pains you have to get through to use
it at all.

Given the names of PsyQ, psyqo, and psycho, it is expected that the reader recognizes the pattern here.

> Note: This project is not affiliated with, endorsed by, or associated in any way with any of the projects or SDKs
> mentioned here.
