# PS4-P8

An emulator to run pico-8 cartridges on a PS4.

Project based on [OpenOrbis PS4 Toolchain](https://github.com/OpenOrbis/OpenOrbis-PS4-Toolchain)

Pico-8 is a Fantasy Programmable Console made by Lexaloffe Games. This emulator is compatible with built pico-8 cartridges in PNG format, to build your own games you will need the original [Pico-8 console](https://www.lexaloffle.com/pico-8.php)

## Status

Not finished - Currently it's complete just enough so that it can run one simple game I made, Minewalker, which is a game inspired by minesweeper. The cartridge is included in assets/misc/minewalker.p8.png:

![minewalker.p8.png](https://github.com/voliva/ps4-p8/blob/audio/assets/misc/minewalker.p8.png?raw=true)

- `print`: It prints text on the screen, but it's missing some P8SCII control codes, non-ascii characters, and autoscroll.
- Sprites: It draws them correctly.
- Sfx: Plays sound effects on one channel, a bit buggy if you try to play more than one.
- Menu: not implemented yet.
- Memory manipulation: not implemented yet.
- Maps: not implemented yet.
- Music: not implemented yet.

When this is finished, one future idea is to have a cartridge explorer where you can just play any cartridge published on the internet.

## Building

The project is cross-platform for Windows + PS4.

You'll need to add the external libraries in `${SolutionDir}/lib`:

- lib/include/stb/stb_image.h: Grab this file from https://github.com/nothings/stb
- lib/include/SDL2: SDL2 headers (needed for Windows)
- lib/lib/SDL2.lib: SDL2 lib (needed for Windows)

### Windows

Run with visual studio. First build `${SolutionDir}/lua` project, then `${SolutionDir}/pico8_ps4`.

### PS4

Run `./build.bat` - First of `${SolutionDir}/lua` project, then `${SolutionDir}/pico8_ps4`.

## License

### External dependencies used

- [OpenOrbis PS4 Toolchain](https://github.com/OpenOrbis/OpenOrbis-PS4-Toolchain)
- [Lua](https://www.lua.org/)
- [STB](https://github.com/nothings/stb)
- SDL2

### PS4-P8

MIT

Copyright 2021 Victor Oliva

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
