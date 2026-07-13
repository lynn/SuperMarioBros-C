SuperMarioBros-C
================

An attempt to translate the original Super Mario Bros. for the NES to readable C/C++.

I've taken the `smbdis.asm` disassembly of Super Mario Bros. and successfully converted it to C++ using an automated codegen program I wrote (which you can be found in the `codegen/` subdirectory of the repo). Right now, it looks very similar to the original disassembly and is fairly dense code, but it works! Check out `source/SMB/SMB.cpp` if you're curious.

Many thanks to doppelganger (doppelheathen@gmail.com), who wrote the original comprehensive Super Mario Bros. disassembly. This can be found in the `docs/` folder of the repo.

![Demo gif](https://github.com/MitchellSternke/SuperMarioBros-C/raw/master/demo.gif)

*looks and plays just like the original*

Building
--------

**Dependencies**
- C++14 compiler
- SDL2
- Flex
- Bison
- CMake

From the root of the repo, execute:
```
mkdir build
cd build
cmake ..
make
```

This should create the executable `smbc` in the `build` directory.

Running
-------

This requires an *unmodified* copy of the `Super Mario Bros. (JU) (PRG0) [!].nes` ROM to run. Without this, the game won't have any graphics, since the CHR data is used for rendering. The program looks for it in the working directory.

The program takes a command:

```
./smbc interactive [--mute]                      play the game with the keyboard
./smbc movie <movie.fm2> [--mute]                play back an FCEUX movie
./smbc screenshot <movie.fm2> <frame> [out.png]  save the screen on a frame of a movie
./smbc ram <movie.fm2> <frame> [out.bin]         save the NES RAM of every frame up to one
```

With no command, it plays interactively. `--mute` turns the sound off.

Movies
------

An FCEUX movie (`.fm2`) can be played back instead of playing the game yourself:

```
./smbc movie smb-allitems.fm2
```

The movie plays from power-on; pressing `R` restarts it, and once it runs out, control returns to the keyboard.

By default, playback is not perfectly faithful to the NES, and a movie that depends on frame-perfect inputs (such as a tool-assisted speedrun) will eventually drift out of sync with the recording. A movie records one frame of input per frame of the *console*, but the game only reads the controller once per frame of *its own* logic, and the two are not the same: the game runs entirely inside the NMI handler, and when a frame's work overruns the handler, the console misses the next NMI and never reads the controller for that frame (a "lag frame"). The engine runs the game as compiled C++ and has no notion of cycles, so it cannot tell when the handler would have overrun. It instead assumes the handler overruns exactly when it is drawing the screen for a newly loaded area, which is where nearly all of the game's lag comes from. Frames that lag for other reasons -- an unusual number of enemies on screen, for example -- are not accounted for, and each one costs a frame of drift.

Tell it where the lag frames are, and none of that applies: the movie plays back in step with the recording for the whole of it. `--lag` takes the frames the console lagged on, as a comma-separated list, and playback ignores the movie's input on each of them, exactly as the console did.

```
./smbc movie smb-allitems.fm2 --lag "$(paste -sd, smb-allitems.lag)"
```

Only the console can say where its lag frames are, so they are asked of an emulator once and kept: `smb-allitems.lag` is the list for the movie above, and `tools/lagframes.lua` is what produced it (see `tools/README.md`). With it, the engine plays this movie exactly as the console does: the NES RAM matches FCEUX's, byte for byte, on every one of its 71,377 frames.

Playing a movie back is also how the engine is checked against the console, by comparing the NES RAM of both after every frame. That found a number of places where the translated code did not behave like the ROM, mostly because the game reads its own code as if it were data; `FIXES.md` describes them, and `tools/README.md` describes how to look for more.

Capturing a movie
-----------------

The `screenshot` and `ram` commands play a movie back with no window, no sound, and no real-time delay, and save what the game did:

```
./smbc screenshot smb-allitems.fm2 5000    # the screen on frame 5000, as a 256x240 PNG
./smbc ram smb-allitems.fm2 5000           # the 2KB of NES RAM after each of frames 1-5000
```

They default to writing `smbc-screenshot.png` and `smbc-ram.bin`, and both take an output path as an optional last argument. Frames are counted in frames of the game's logic, from power-on, exactly as movie playback counts them.

`ram` writes one 2KB record per frame rather than only the frame asked for, because a whole run is what the engine is compared against the console with: FCEUX records the same thing on its side, and the first record that differs is the frame the engine got something wrong on. `tools/README.md` describes how that is done. To look at a single frame, take the last record (`tail -c 2048`).

Architecture
------------

The game consists of a few parts:
- The decompiled original Super Mario Bros. source code in C++
- An emulation layer, consisting of
  - Core NES CPU functionality (RAM, CPU registers, call stack, and emulation of unique 6502 instructions that don't have C++ equivalents)
  - Picture Processing Unit (PPU) emulation (for video)
  - Audio Processing Unit (APU) emulation (for sound/music)
  - Controller emulation
- SDL2 library for cross-platform video/audio/input

Essentially, the game is a statically recompiled version of Super Mario Bros. for modern platforms. The only part of the NES that doesn't have to be emulated is the CPU, since most instructions are now native C++ code.

The plan is to eventually ditch all of the emulation layer and convert code that relies upon it. Once that's done, this will be a true cross-platform version of Super Mario Bros. which behaves identically to the original. It could then be easily modified and extended with new features!

License
-------

TODO
