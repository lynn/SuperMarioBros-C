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

prints the first iteration whose game state differs, and which cells differ. That
is the whole loop: run it, look at what it says, fix the bug, run it again and
watch the number go up.

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
