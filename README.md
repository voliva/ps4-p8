# PS4-P8

An emulator to run pico-8 cartridges on a PS4 (or Switch).

Project based on [OpenOrbis PS4 Toolchain](https://github.com/OpenOrbis/OpenOrbis-PS4-Toolchain) (or [devkitPro](https://devkitpro.org))

Pico-8 is a Fantasy Programmable Console made by Lexaloffle Games. This emulator is compatible with built pico-8 cartridges in PNG format, to build your own games you will need the original [Pico-8 console](https://www.lexaloffle.com/pico-8.php)

![screenshot](https://user-images.githubusercontent.com/5365487/149617734-24f0ad26-4d66-4d49-85c7-a7fd0bbe2765.png)

## Status

Not finished - Compatibility is still limited, but there are many cartridges it can run already.

### Features

- Modified lua interpreter compatible with pico8's lua.
- 16-bit fixed point decimal arithemtic just like pico8.
- Save states (at the moment simplistic, one save state per cartridge).
- The most used predefined pico8 functions are implemented: 90 functions implemented out of 105
- Console options
    - CRT/DOT matrix filters
    - Volume control
    - Invert controls

It comes with a set of bundled games which have been used to improve the compatibility.

More .p8.png cartridges can be bundled with the .pkg file by adding them into `/assets/misc` folder, or they will be loaded in runtime from console's `/data/p8-cartridges` folder for PS4, and `/switch/switch-p8/cartridges` on the SD card for switch.

- `print`: It prints text on the screen, but it's missing some P8SCII control codes, non-ascii characters.
- Sprites: Done
- Sfx: Playable. Room of improvement for sound quality, and sound filters are missing (damp, buzz, reverb, etc.)
- Music: Playable.
- Menu: WIP.
- Memory manipulation: Almost complete.
- Maps: Done
- Custom P8-Lua: Some edge cases missing

A list of some playable cartridges can be found [here](https://github.com/voliva/ps4-p8/blob/main/PlayableCartridges.md)

## Building

The project is cross-platform for Windows + MacOS + PS4 + Switch.

You'll need [OpenOrbis PS4 Toolchain](https://github.com/OpenOrbis/OpenOrbis-PS4-Toolchain) installed with its dependencies (.NET Core) and clang++.

You'll need to add the external libraries in `${SolutionDir}/lib`:

- lib/include/stb/stb_image.h: Grab this file from https://github.com/nothings/stb

Additionally, to run it on windows, you will need:

- lib/include/SDL2: SDL2 headers
- lib/include/dirent.h: Dirent port for windows, grab from https://github.com/tronkko/dirent
- lib/lib/SDL2.lib: SDL2 lib
- libcurl: Install via [vcpkg](https://curl.se/docs/install.html)

### Windows

Run with visual studio. First build `${SolutionDir}/lua` project, then `${SolutionDir}/pico8_ps4`.

### PS4

Run `./build.bat` - First of `${SolutionDir}/lua` project, then `${SolutionDir}/pico8_ps4`.

### Switch

Run `make`

## License

### External code used

Special thanks to:

- Bucanero - The code under http.cpp is based from code on [apolo-ps4](https://github.com/bucanero/apollo-ps4) GPL v3

### External tools and dependencies used

- [OpenOrbis PS4 Toolchain](https://github.com/OpenOrbis/OpenOrbis-PS4-Toolchain) GPL v3
- [devkitPro](https://devkitpro.org) GPL v2
- [Lua](https://www.lua.org/) MIT
- [Eris](https://github.com/fnuecke/eris) MIT
- [libfixmath](https://code.google.com/archive/p/libfixmath/) MIT
- [plusequals powerpatch](http://lua-users.org/wiki/LuaPowerPatches)
- [STB](https://github.com/nothings/stb) MIT
- [EFLA](http://www.edepot.com/algorithm.html)
- [dirent](https://github.com/tronkko/dirent) MIT
- [SDL2](https://www.libsdl.org/) ZLib
- [Pico-8 Font + Palette](https://www.lexaloffle.com/pico-8.php?page=faq) CC-0
- [ConcurrentQueue](https://stackoverflow.com/a/26491017/1026619)

### Bundled cartridges

- [Mine Walker](https://www.lexaloffle.com/bbs/?tid=42395) CC4-BY-NC-SA by voliva
- [Celeste](https://www.lexaloffle.com/bbs/?tid=2145) by noel+maddy
- [To a Starling](https://www.lexaloffle.com/bbs/?tid=45958) CC4-BY-NC-SA by Peteksi+Gruber
- [Flip Knight](https://www.lexaloffle.com/bbs/?tid=40906) CC4-BY-NC-SA by st33d
- [Harold's Bad Day](https://www.lexaloffle.com/bbs/?pid=100973) CC4-BY-NC-SA by biovoid
- [PakPok](https://www.lexaloffle.com/bbs/?tid=35326) CC4-BY-NC-SA by st33d
- [Phoenix](https://www.lexaloffle.com/bbs/?tid=44727) CC4-BY-NC-SA by pahammond
- [Tiny golf puzzles](https://www.lexaloffle.com/bbs/?pid=79680) by beepyeah

### PS4-P8

MIT

Copyright 2021-2025 Victor Oliva

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
