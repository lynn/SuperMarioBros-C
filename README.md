SuperMarioBros-C
================

An attempt to translate the original Super Mario Bros. for the NES to readable C/C++.

Forked from [Mitchell Sternke's repo](https://github.com/MitchellSternke/SuperMarioBros-C), which is based on [doppelganger's disassembly](6502disassembly.com/nes-smb/SuperMarioBros.html).

Building
--------

Install SDL2, flex, bison, and CMake, then run `(mkdir build; cd build; cmake ..; make)`.

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

Pass `--lag 123,124,125` to simulate lag frames to synchronize movie playback. You can use `tools/lagframes.lua` to produce a list of lag frames using FCEUX's movie playback:

```
./smbc movie smb-allitems.fm2 --lag "$(paste -sd, smb-allitems.lag)"
```

