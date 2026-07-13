Fidelity fixes
==============

This is a record of the places where the translated code did not behave like the
console, why, and what was done about it.

How they were found
-------------------

Each one was found by playing the same FCEUX movie (`smb-allitems.fm2`, a
full-game tool-assisted run) through both FCEUX and this engine, dumping the 2KB
of NES RAM after every frame of the game's logic, and comparing the two. The
first frame whose game state differs points at the code that ran differently on
that frame, and from there it is usually a short step to the instruction.

When the movie first played back, the game state matched the console for 27
frames. It now matches for 50,124 -- about fourteen minutes of play, and as far
as this approach can go (see "The ceiling", below).

The one bug behind most of them
-------------------------------

Super Mario Bros. reads its own code as if it were data, constantly and by
accident. It indexes a data table with a register that is out of range -- a
player facing direction that is zero or three, a leftover index from an unrelated
routine, a nybble that counts to fifteen against a five-entry table -- and finds
whatever the ROM happens to store after the table, which is nearly always the
game's own machine code. The values it finds are stable, so the game's behaviour
depends on them, and some of them are load-bearing: the fast fireball that
speedruns rely on is a read past the end of a two-entry table.

The translated code has no ROM layout. Its data is packed into one blob in a
different order, with no code in between, so every one of these reads finds the
wrong byte. Each fix below spells out the bytes the ROM has around the table so
that the read finds what it finds on the console.

The fixes
---------

### Fireball speed

`FireballXSpdData` has two entries and is indexed with the player's facing
direction minus one, which is only in range for two of the four directions.

- **Facing both ways at once** (turning around on the same frame a fireball is
  thrown) gives an index of 2, one past the table. On the NES that byte is `$86`,
  the first byte of the code for `FireballObjCore`. It makes for a noticeably
  fast fireball, and speedruns rely on it.
- **Facing neither way** gives an index of `$ff`: the `dey` that turns the facing
  direction into an index underflows, so the read is 255 bytes *past* the table,
  not one before it. On the NES that byte is `$a9`, which as a speed sends the
  fireball left at a good clip.

The table is now stored on its own, with the 256 bytes it can be indexed with
kept clear, and both of those bytes spelled out.

### Vines

Three separate problems, all on vines.

- **A vine placed in the level data spawned in the wrong place.** `Setup_Vine`
  copies the vine's position out of the block object buffers using the Y
  register, but nothing on that code path sets Y: it is reached through the
  ROM's `JumpEngine`, which leaves its own indexing behind in Y. See "Jump
  tables" below.
- **Grabbing a vine put the player at the wrong X position.** `ClimbXPosAdder`
  and `ClimbPLocAdder` are indexed with the player's facing direction, and the
  facing direction is zero while holding left and right at once on a vine
  (`ClimbingSub` stores the buttons inverted, so both buttons come out as zero
  rather than as three). An index of zero reads the byte *in front of* the table,
  which on the NES is the last byte of a `jmp RemoveCoin_Axe`, `$8a`.
- The two tables are now stored together, in the order the ROM has them, with
  that `$8a` in front and the byte that follows them behind.

### Air bubbles

On the first frame of every water level the air bubbles took their timer and
their movement force from the wrong bytes.

`BubbleCheck` puts a pseudorandom bit in `$07` and falls through into
`SetupBubble`, which uses it to index `Bubble_MForceData` and `BubbleTimerData`.
But the player entrance code calls `SetupBubble` directly, without setting `$07`,
so on that path the tables are indexed with whatever the last routine left in
`$07` -- which is the high byte of an address left there by `JumpEngine`.

Both tables are now stored as the 258 bytes the ROM has from the start of
`Bubble_MForceData`, which is every byte they can be indexed with.

### Flying cheep-cheeps

`MoveFlyingCheepCheep` indexes `PRandomSubtracter` and `FlyCCBPriority`, both
five entries long, with the high nybble of a movement force, which counts to
fifteen. Ten of the sixteen reads are off the end. The disassembly says so at the
instruction that sets the index: *"note this tends to go into reach of code"*.

It is not residual code. The bytes it finds past the end of the tables are what
decide how the cheep-cheep moves, so getting them wrong changes the motion. Both
tables now carry the bytes the ROM has after them.

### Jump tables

The ROM dispatches eighteen jump tables through a routine called `JumpEngine`,
which reads the address of the routine to jump to out of a table stored after the
call. The translation turns each of these into a `switch`, which gets the control
flow right but throws away everything else `JumpEngine` does on its way:

    $04-$05   the address of the table, less one
    $06-$07   the address of the routine being jumped to
    Y         the offset it read the last byte of that address from
    A         the high byte of that address

None of that is meant to be read back. The game reads it back anyway, because it
leaves registers and zero page uninitialized and takes whatever is there:
`Setup_Vine` indexes the block object buffers with Y, and `SetupBubble` indexes
its data tables with `$07`. So the game's behaviour depends on the addresses its
own routines are stored at -- which the translated code otherwise has no notion
of at all.

`SMBEngine::jumpEngine()` now reproduces all of it, at all eighteen dispatch
sites, using the real jump tables from the ROM. `$04-$07` are now identical to
the console's for the whole movie, which is a good sign the rest of it is right
too.

### Uninitialized data storage

The 32KB of storage the translated data is written into was never initialized, so
anything the game read from a gap between two tables read whatever was on the
heap. It is now zeroed, which at least makes those reads deterministic.

Movie playback
--------------

Two things had to be right before any of the above could be measured, because
both of them desynchronize the movie from the game on their own.

- **Boot.** A movie records input from power-on, but `SMBEngine::reset()` does the
  console's startup instantly. Seven movie frames elapse on the console before the
  game's first frame of logic, so playback starts six frames in
  (`Fm2Movie::BOOT_FRAMES`) and the lag rule below accounts for the seventh.
- **Lag frames.** The game runs inside the NMI handler, and when a frame's work
  overruns the handler the console misses the next NMI and never reads the
  controller for that frame. The engine has no notion of cycles, so it cannot
  know when that happens. `SMBEngine::isLagFrame()` assumes it happens exactly
  while the game is drawing the screen for a newly loaded area, which is where
  nearly all of the game's lag comes from: of the 53 lag frames in the movie, that
  identifies 51, with no false positives across all 71,378 frames.

The ceiling
-----------

The two lag frames the rule misses are in World 5-3, where the console lags
because of the number of enemies on screen rather than because of a screen draw.
The first of them is at iteration 50,117, and it is why playback of this movie
stops matching at 50,125: the missed frame costs the game a controller read, and
the recorded input lands one frame late from there on.

Nothing short of cycle-accurate timing can predict those, so 50,124 iterations is
as far as this movie can go. Every difference in game state before that point has
been fixed.

Known remaining differences
---------------------------

`MusicData` (`$f5-$f6`) and the two noise offsets (`$07b0`, `$07c1`) hold
different values from the console's while the silence "song" is loaded.
`SilenceHdr` is four bytes long and `LoadHeader` reads six, so it reads into the
header that follows and picks up the low byte of a pointer to `CastleMusData` --
a ROM address, so ours differs.

The value is never used: the noise routine is skipped whenever silence is the
loaded header, so it sits frozen until the next real header overwrites it, and no
sound state downstream of it differs. It is the same class of bug as the ones
above and could be fixed the same way.

A note on the generated code
----------------------------

The corrections above live in the four files under `source/SMB` that were
originally generated from `docs/smbdis.asm` by the tool in `codegen`. That tool
knows nothing about them, so regenerating discards them. It no longer runs as
part of the build; see the note in `codegen/CMakeLists.txt`.
