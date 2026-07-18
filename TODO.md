# De-registering / de-gotoing TODO

## Session notes (2026-07-18, third session)

- **SMBPlayer.cpp — DONE (0 tokens, 0 gotos).** Finished the last two functions:
  - `PlayerBGCollision` (130 tokens, 31 gotos) rewritten structured. CheckSideMTiles
    and HandleClimbing/ChkForFlagpole/VineCollision became lambdas (reached from two
    side-check call sites); DoPlayerSideCheck is the natural fall-through tail;
    AwardTouchedCoin inlined as `HandleCoinMetatile(); return;`. KEY subtlety: the
    second `BlockBufferColli_Feet(y)` call received member y ALREADY INCREMENTED by
    the first call's transition write, so the structured version passes
    `footAdder + 1`. Side-check loop advances its adder by 2 per iteration (loop-top
    `++y` acted on BHalf's offset). The flagpole score loop became
    `while (scoreOfs != 0 && playerY < data[scoreOfs]) --scoreOfs;`.
  - After that, both `y = adderOffset` transition writes in
    `BlockBufferColli_Head`/`_Side` proved dead (their only register-based callers
    were inside PlayerBGCollision) — removed.
  - `PlayerPhysicsSub` (58 tokens, 25 gotos): climb offset / jump-physics offset /
    friction offset all became locals with if/else chains; the NoJump/ChkRFast goto
    web collapsed into a `jumpPressed` bool plus a `slowFriction` bool feeding the
    shared ChkRFast block.
- Re-swept SMBObject's 5 LIVE tail restores (CheckRightScreenBBox, UpdateNumber,
  RelWOfs, VariableObjOfsRelPos, GetOffScreenBitsSet) after SMBPlayer went clean:
  all 5 STILL LIVE — their consumers are register-based callers in **SMBGame**.

## Current counts (2026-07-18, end of third session; comment-stripped token count)

| File          | Reg tokens | Gotos |
|---------------|-----------:|------:|
| SMB.cpp       |          0 |     0 |
| SMBArea/Data/EnemyGfx/Sound | 0 |   0 |
| SMBEnemies.cpp|          0 |     0 |
| SMBEngine.cpp |         17 |     0 |
| SMBObject.cpp |         19 |     0 |
| SMBPlayer.cpp |          0 |     0 |
| SMBGame.cpp   |       1290 |   101 |

NEXT UP: SMBGame.cpp (last dirty file; tackle goto removal alongside
de-registering per function). SMBObject's and SMBEngine's remaining tokens are
all blocked on SMBGame callers — re-sweep them as SMBGame functions get cleaned.

