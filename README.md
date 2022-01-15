# PS4-P8

An emulator to run pico-8 cartridges on a PS4.

Project based on [OpenOrbis PS4 Toolchain](https://github.com/OpenOrbis/OpenOrbis-PS4-Toolchain)

Pico-8 is a Fantasy Programmable Console made by Lexaloffle Games. This emulator is compatible with built pico-8 cartridges in PNG format, to build your own games you will need the original [Pico-8 console](https://www.lexaloffle.com/pico-8.php)

## Status

Not finished - Currently it's complete just enough so that it can run very simple games, but most of them don't fully work just yet.

It comes with one simple game which fully works, Minewalker, which is a game inspired by minesweeper. The cartridge is included in assets/misc/minewalker.p8.png:

![minewalker.p8.png](https://github.com/voliva/ps4-p8/blob/main/assets/misc/minewalker.p8.png?raw=true)

More .p8.png cartridges can be bundled with the .pkg file by adding them into `/assets/misc` folder, or they will be loaded in runtime from PS4's `/data/p8-cartridges` folder.

- `print`: It prints text on the screen, but it's missing some P8SCII control codes, non-ascii characters.
- Sprites: Done
- Sfx: Done
- Music: Done
- Menu: WIP.
- Memory manipulation: Almost complete.
- Maps: Done
- Custom P8-Lua: WIP

## Building

The project is cross-platform for Windows + PS4.

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

## License

### External code used

Special thanks to:

- Bucanero - The code under http.cpp is based from code on [apolo-ps4](https://github.com/bucanero/apollo-ps4) GPL v3

### External tools and dependencies used

- [OpenOrbis PS4 Toolchain](https://github.com/OpenOrbis/OpenOrbis-PS4-Toolchain) GPL v3
- [Lua](https://www.lua.org/) MIT
- [STB](https://github.com/nothings/stb) MIT
- [EFLA](http://www.edepot.com/algorithm.html)
- [dirent](https://github.com/tronkko/dirent) MIT
- [SDL2](https://www.libsdl.org/) ZLib
- [Pico-8 Font + Palette](https://www.lexaloffle.com/pico-8.php?page=faq) CC-0
- [ConcurrentQueue](https://stackoverflow.com/a/26491017/1026619)

### PS4-P8

MIT

Copyright 2021 Victor Oliva

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
