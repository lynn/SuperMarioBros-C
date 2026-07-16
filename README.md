SuperMarioBros-C
================

An attempt to translate the original Super Mario Bros. for the NES to readable C/C++.

Forked from [Mitchell Sternke's repo](https://github.com/MitchellSternke/SuperMarioBros-C), which is based on [doppelganger's disassembly](6502disassembly.com/nes-smb/SuperMarioBros.html).

Building
--------

Install SDL2, flex, bison, and CMake. Dear ImGui, which the debugging interface is built on, comes in as a submodule, so fetch it with `git submodule update --init`. Then run `(mkdir build; cd build; cmake ..; make)`.

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

The game runs as one view on a workspace, alongside views for debugging it, which the `View` menu opens:

* `Game` — the screen, as the PPU renders it.
* `Nametables` — all four of the PPU's nametables, which is the whole background it holds rather than the part of it the scroll registers put on screen. The scrolled region is outlined in red, and wraps around the edges the way the PPU wraps it.
* `RAM` — the 2KB of NES RAM, live, as 64 rows of 32 hex bytes.

The workspace is scaled for the density of the display it is on, so that it stays readable on a HiDPI monitor. Pass `--scale 2` to set the factor yourself.

Pass `--lag 123,124,125` to simulate lag frames to synchronize movie playback. You can use `tools/lagframes.lua` to produce a list of lag frames using FCEUX's movie playback:

```
./smbc movie smb-allitems.fm2 --lag "$(paste -sd, smb-allitems.lag)"
```

