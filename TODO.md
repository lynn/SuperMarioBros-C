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

## Session notes (2026-07-18, fourth session) — SMBGame 1290→413 tokens, 101→41 gotos

Five batches landed (one commit each), all RAM-exact:
1. Player action/animation/gfx cluster: the GetOffsetFromAnimCtrl chain became
   return-value functions; ProcessPlayerAction/HandleChangeSize return the gfx
   offset; DrawPlayerLoop consumes DrawOneSpriteRow's returned pair.
2. Obj-offset wrappers (GetProperObjOffset et al), bubbles, fireball
   positioning. KEY: FBallB's second call consumes BoundingBoxCore's *returned*
   boundBoxIdx (its tail `y =` transition write), not relPosIdx.
3. Block objects cluster (BlockObjectsCore's pha/pla state → `savedState`
   local; DrawBlock loop on the returned pair; ChkLeftCo(offscreenBits, oamSlot)).
4. FloateyNumbersRoutine(slot) (GameCoreRoutine's loop transition `x = i`
   dropped), DrawHammer, JCoinGfxHandler (goto-into-do-while → plain if/else),
   ProcHammerObj, MiscObjectsCore, PlayerHammerCollision.
5. Fireball/enemy collision cluster + HurtBowser(slot). KEY BUG FOUND: facing
   dir 0 makes `FireballXSpdData + (PlayerFacingDir - 1)` an out-of-bounds
   +0xff read — must wrap as uint8_t, not go to address −1.

**Batch F (the remaining ~29 functions) was spliced whole, diverged, and was
REVERTED — the scratch replacement text was
`scratchpad/batchF.cpp` (session-local, gone next session). Redo it in small
pieces with a check between each.** What it covered: ColorRotation,
GetAreaMusic, TransposePlayers, ProcessWhirlpools, LoadAreaPointer,
CyclePlayerPalette(bits), ResetPalStar, DonePlayerTask, PlayerFireFlower,
FlagpoleRoutine, FlagpoleGfxHandler(slot), PlayerLoseLife, ContinueGame,
Entrance_GameTimerSetup, Vine_AutoClimb, VerticalPipeEntry, SideExitPipeEntry,
ChgAreaPipe(mode), EnterSidePipe, PlayerDeath, FlagpoleSlide, PlayerEntrance,
PlayerEndLevel, GameRoutines, UpdScrollVar, RunGameTimer, ProcessCannons,
BulletBillHandler(slot), GameCoreRoutine. Known facts from the attempt:
- GameCoreRoutine star-palette shifts: timer **< 8** gets the extra `>>= 2`
  (cycle every eighth frame); this was transcribed inverted once and fixed.
- BulletBillHandler: `(diff & 0x80) == 0` → ++movingDir (transcribed inverted
  once in drafting; the fixed version was in the spliced text).
- After both fixes the remaining first divergence was iteration 12136:
  Cannon_Timer ours=0 fceux=1, SprObject_YMF_Dummy/Y_MoveForce and player X/Y
  off — suspects are ProcessWhirlpools and/or ProcessCannons transcription;
  bisect those two against the originals first.
- Entrance_GameTimerSetup's `SetupBubble(x)` residual: x there is
  **GetPlayerColors' leftover** (= old VRAM_Buffer1_Offset) on the normal
  path, or 0x05 after the JoypadOverride vine path. The attempt changed
  GetPlayerColors (SMBObject.cpp) to *return* that offset and dropped its
  `x = offset` write — re-do that part too (it was reverted with the rest).
- FlagpoleRoutine's FPGfx x is 5 on every path (AddToScore→UpdateNumber
  restores x = ObjectOffset = 5 after the digit write clobbers it).

