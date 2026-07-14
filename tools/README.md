Differential debugging against FCEUX
===================================

These are the tools the fidelity fixes in `FIXES.md` were found with. They are not
part of the game and are not built by it.

The idea is simple. Play the same movie through FCEUX and through this engine,
record the 2KB of NES RAM after every iteration of the game's logic on both sides,
and compare. The first iteration whose game state differs is the frame on which
the engine ran something differently from the console, and the cells that differ
say what. That is usually enough to find the instruction, and the instruction is
usually enough to explain the bug.

Setting it up
-------------

Everything below is run from the root of the repository, so that the tools can read
`docs/smbdis.asm` and put names to RAM addresses. `$SMB_REF` is the console's dump,
which every tool reads.

Record the console (about 150MB for the full movie, and slow):

    export SMB_REF=/tmp/fceux.bin
    fceux --loadlua tools/nmifull.lua --playmov smb-allitems.fm2 \
          "build/Super Mario Bros. (JU) (PRG0) [!].nes"

The flag is `--playmov`, not `--playmovie`. FCEUX ignores an unrecognised flag and
starts with no movie loaded, which is not an error: it plays the attract-mode demo
to itself and the dump comes out full of plausible-looking RAM that the movie never
produced. If a dump disagrees with the engine from the very first iterations, check
for `Replay started` in FCEUX's output before believing any of it.

Record the engine. The game does this itself, with the `ram` command, which plays a
movie back with no window and no sound and writes the RAM after every frame of it:

    make -C build smbc
    (cd build && ./smbc ram ../smb-allitems.fm2 71370 /tmp/ours.bin)

The ROM is read from the working directory, which is why that runs from `build`.
It takes a few seconds, and the dump is about 150MB.

Finding a bug
-------------

    python3 tools/itercmp.py /tmp/ours.bin

prints the first iteration whose game state differs, and which cells differ, and
exits non-zero if there is one. That is the whole loop: run it, look at what it
says, fix the bug, run it again and watch the number go up. `tools/check.sh` is
the loop in one command -- it records the console if it has to, builds the engine,
runs the movie through it and compares -- and takes a couple of seconds once the
console's dump is on disk. Both narrate the phase they are in on stderr and keep
the report itself on stdout.

    python3 tools/trace.py /tmp/ours.bin 33,86,1d 9500 9515

follows a few cells across a range of iterations, printing only where something
changes. Once `itercmp.py` has pointed at an iteration, this shows how the state
got there and where the two runs part company.

    python3 tools/cells.py /tmp/ours.bin 0 9600

lists every cell that differs anywhere in a range, and when it first did. It
answers a different question -- whether what is going wrong is confined to one
subsystem -- and it includes the cells that are artifacts of the port rather than
game state, marked as such.

    python3 tools/predicate.py

searches the console's RAM for a condition that fires exactly on the frames it
lags. This is where `SMBEngine::isLagFrame()` came from; see the comment in the
script.

Lag frames
----------

The engine cannot predict the console's lag, but it does not have to: the console
knows, and will say. FCEUX has a flag for it, and

    SMB_LAG=smb-allitems.lag fceux --loadlua tools/lagframes.lua \
        --playmov smb-allitems.fm2 "build/Super Mario Bros. (JU) (PRG0) [!].nes"

writes the frames of the movie it lagged on, one per line. (FCEUX segfaults on the
way out, after the file is written and closed. The file is fine.) `smbc` takes them
as a comma-separated list, and ignores the movie's input on each:

    ./smbc ram smb-allitems.fm2 71377 /tmp/ours.bin --lag "$(paste -sd, smb-allitems.lag)"

which is how the engine is compared against the console over a whole movie rather
than up to the first lag frame it guesses wrong. The list for the movie in this
repository is checked in as `smb-allitems.lag`, so this only has to be done again
for a new movie.

**The lag list and the NMI index are a frame apart.** `predicate.py` calls
iteration `i` a lag frame when the index that `nmifull.lua` writes has no NMI for
frame `i`; `lagframes.lua` calls frame `i` a lag frame when `emu.lagged()` is set
after it runs, which is a frame earlier throughout. FCEUX has already counted the
frame by the time the NMI for it executes, so it is the index that is late, and the
lag list that says which rows of the movie the console actually read. Feed the index
gaps to `--lag` and the engine's input lands one frame early from the first press of
Start.

Two things that will mislead you
--------------------------------

**The two dumps are offset by one record.** FCEUX records at the entry of the NMI,
which is the state at the *end of the previous* iteration. `smbc ram` records after
`update()`, which is the state at the end of *this* one. So our record `i` lines up
with theirs at `i + 1`. The tools handle this (`smbram.SHIFT`), but anything new
written against these dumps has to, and getting it wrong makes the engine look
comprehensively broken rather than subtly wrong.

**The dumps only hold end-of-frame state.** A cell the game writes and rewrites
within a single frame does not show the value that some intermediate read of it
saw, and several of the bugs in `FIXES.md` turned on exactly that -- a scratch cell
or a register that was one thing when a table was indexed with it and another by
the time the frame ended. When it matters, do not reason from these dumps: print
from the engine at the instruction itself. Reasoning from end-of-frame values sent
this investigation down two blind alleys before that lesson stuck.

What is worth comparing
-----------------------

Not all 2KB of it. Some cells differ from the console's for reasons that are not
bugs: the scratch temporaries at `$00-$07`, the pointers into ROM data at
`$e7-$ea` and `$f5-$f6`, which hold synthetic addresses here, and the 6502 stack
page at `$100-$1ff`, which the engine does not use at all. `smbram.is_artifact()`
has the list and says why. `itercmp.py` skips them; `cells.py` shows them.

Shaping the source
------------------

A second set of tools, which change the source rather than measure it. They are safe to
run because the harness above is what says so: every one of these rewrites is meant to
leave the game bit-for-bit identical, and `check.sh` is what proves it did. Run it after
each one, and do not believe a rewrite that has not been through it.

`subroutines.py` lifts the game's routines out of `code()` and makes them functions. It
has finished its job: there are no `JSR`s left, and the RTS jump table it worked against
is gone with them. It still runs, and now finds nothing to do, which is the point.

`split.py` moves a module's routines out of `SMB.cpp` into a file of their own:

    python3 tools/split.py source/SMB/SMB.cpp SMBSound.cpp --entry SoundEngine [--apply]
    python3 tools/split.py source/SMB/SMB.cpp SMBObject.cpp --kernel           [--apply]

Every routine is a method of `SMBEngine` declared in `SMBEngine.hpp`, so which file its
body sits in changes no declaration, no call and no behaviour, and the linker checks the
arithmetic. What it is not is arbitrary: a routine can only leave with a module if nothing
outside the module needs it.

`--entry` takes the routines a subsystem *dominates* -- the ones every path from `code()`
reaches only through it, which are therefore its own and nobody else's. It then takes the
routines that only the subsystem and the code *above* it call. Dominance alone is stricter
than the layering needs: a routine that `EnemiesAndLoopsCore` calls and `code()` also calls
is dominated by neither, so it would stay behind and the subsystem would be left calling up
out of its own file. But `SMB.cpp` sits on top of every module and may call down into any of
them, so the only caller that would really be a problem is a *different* subsystem -- and a
routine two subsystems share is in the kernel already. That is what brings `MoveJ_EnemyVertically`
home to the enemies and `WriteGameText` to the area parser.

`--kernel` takes the other kind of module, the one the dominator tree cannot name because it
is not a subtree: the primitives that two or more subsystems both reach. Nothing dominates
them, because being shared is the whole of what they are. Take the kernel out first, or every
subsystem after it drags a pile of shared code along behind it.

Add the new file to `CMakeLists.txt` and to `RANK` in `layers.py`.

`layers.py` is what stops all of that from quietly coming undone:

    python3 tools/layers.py --report

The files are a claim about the shape of the program, and the compiler will never check it --
any method of `SMBEngine` may call any other, whatever file either is written in. So this
reads the call graph back out of the files and fails if a call goes *up* the stack:

    SMBObject   0   the kernel: sprites, collision, gravity, the offscreen checks
    SMBSound    0   the APU driver, which calls nothing at all
    SMBEnemyGfx 1   drawing an enemy, which the enemies and the game both do
    SMBPlayer   1   the player's own control, movement and collision
    SMBArea     1   the area parser: level data into the block buffer
    SMBEnemies  2   every enemy, platform and loop command in the game
    SMBGame     3   the game core, which drives all of the above
    SMB.cpp     9   code(), the NMI, and the mode dispatch

A module may call anything below it and anything in itself, and nothing else -- not upwards,
and not sideways to a module of the same rank. Should the kernel ever call the game, it is
not a kernel any more, and this says so before it is committed rather than a year later.
Without it the layering is a comment.

