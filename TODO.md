# De-registering / de-gotoing TODO

## Status update (verified RAM-exact, full 71377-iter check passes in ~1s)

- **SMBSound.cpp — DONE** (0 tokens). `MusicHandler` de-registered with named locals.
  Note: the loopback branches in `HandleSquare2Music`'s `EndOfMusicData` MUST reload
  `sel` (the original leaves the value in A); a stale `sel==0` hangs `FindEventMusicHeader`.
- **SMB.cpp — DONE** (0 tokens). Required converting callees to take params:
  `GetAreaType`, `Skip_6`, `ResJmpM`, `DrawPlayerLoop`, `AutoControlPlayer` (SMBGame);
  `EnemiesAndLoopsCore`, `MoveD_Bowser`, `BowserGfxHandler` (SMBEnemies). Their other
  (dirty) callers now pass globals through — those files stay dirty by design.
- **2026-07-17 session (SMBEnemies + SMBObject platform/collision subtree).**
  Cleaned RunSmallPlatform, RunLargePlatform, RunStarFlagObj, DrawStarFlag,
  DrawSmallPlatform, MoveSmallPlatform, MoveFallingPlatform, MoveJ_EnemyVertically,
  MoveD_EnemyVertically, SetHiMax, GetEnemyOffscreenBits, LargePlatformBoundBox,
  RunRetainerObj, ProcFirebar, RunBowserFlame, RunPUSubs. Key moves:
  (1) `SetHiMax(e, downwardMoveAmt)` — was x/y; de-registers its vertical-move wrappers.
  (2) `GetEnemyOffscreenBits` now sources `M(ObjectOffset)` directly.
  (3) `RelativeEnemyPosition(offset)` is now parameterized — the group offset was
  global x. Clean Run* functions pass `e`; still-register-based callers
  (VineObjectHandler, JumpspringHandler, ProcBowserFlame, RunFireworks,
  RunNormalEnemies, HandleEnemyFBallCol) pass `x` for now (transition trick).
  (4) `GetEnemyOffscreenBits(offset)` parameterized too (BalancePlatform passes
  otherPlatform — a genuine non-ObjectOffset case whose Enemy_OffscreenBits output is
  dead on that path, which is why an earlier M(ObjectOffset) hardcode passed the check
  but was unfaithful). (5) `MoveEnemySlowVert(e)` cleaned MoveD_Bowser. (6) Dead tail
  x-restores dropped in LargePlatformBoundBox, RedPTroopaGrav, DoOtherPlatform; dead
  a-init dropped in Inc2B; MovePiranhaPlant a→local.
  NOTE: `RelativeEnemyPosition`/`GetEnemyOffscreenBits` offsets are NOT always
  ObjectOffset (HandleEnemyFBallCol / BalancePlatform). Tail x-restores confirmed LIVE
  (do not remove): DrawLargePlatform, Inc2B, UpdateNumber. Also FinishFlame's `a=0x00`
  is a live leak a caller consumes (removing it diverges at iter 12138, Enemy_YMF_Dummy);
  its x was the flame's empty slot from ChkNoEn (not ObjectOffset), so it took an `e`
  param. WarpZoneObject/KillEnemyAboveBlock/InitEnemyObject DID act on the current object,
  so M(ObjectOffset) substitution cleaned them with no caller changes. The
  bisect-with-full-check loop (~1s) is the reliable oracle.
  SMBObject now 25 tokens (deep collision-subsystem reg=1 tail restores that need their
  SMBEnemies/SMBGame callers de-registered first); SMBEnemies ~770 (36 dirty funcs);
  SMB/SMBSound both 0.
- **SMBObject.cpp — 41 → 31 tokens.** Removed: dead `a=0` residuals in `InitVStf`,
  `EraseEnemyObject`, `ChgAreaMode` (fixed consumers `HurtBowser`, `NextArea`);
  `BlockBufferCollision`/`MoveObjectHorizontally`/`SetupFloateyNumber` → return values
  (fixed `Skip_9`, `MovePlayerHorizontally`). Remaining 31 are `x = M(ObjectOffset)`
  register-ABI restore leaks that cascade up wrapper chains (e.g. `BBChk_E` →
  `BlockBufferChk_Enemy` → the enemy/block-collision caller tree) into large SMBEnemies
  functions. Finishing SMBObject ≈ de-registering the collision subsystem.

- **2026-07-18 session (SMBEnemies platform-collision cluster + Lakitu).**
  SMBEnemies 36→31 dirty funcs, 770→756 reg tokens. Cleaned fully:
  BalancePlatform (dead ExPF tail x-restore), SmallPlatformCollision (pass
  M(ObjectOffset) straight into ProcLPlatCollisions), ProcLPlatCollisions
  (SetCollisionFlag x→`self` local; NoSideC tail restore dead),
  LargePlatformCollision (both early tail restores dead). Parameterized
  LakituAndSpinyHandler(slot) — its `a` was a local FrenzyEnemyTimer read.
  KEY: ChkForPlayerC_LargeP kept at reg=1 — bisect-with-full-check proved
  ONLY its no-collision exit's `x=M(ObjectOffset)` is a live leak (consumed by
  a register-based caller up the EnemiesAndLoopsCore→switch chain); its other
  two exits and the whole rest of the cluster's 7 tail restores were dead. The
  full-check bisect (~1s) remains the only reliable oracle; a whole-batch
  removal diverged at $041b Enemy_YMF_Dummy+4 and had to be bisected one
  restore at a time. Remaining SMBEnemies reg=1 leaks (DrawLargePlatform, Inc2B,
  FinishFlame, ChkForPlayerC_LargeP) are all confirmed LIVE — need their
  register-based callers de-registered first.
- **2026-07-18 session cont. — EnemiesAndLoopsCore reg 31→2.** The Enemy_Flag
  block indexes off the `enemyOffset` param; RunEnemyObjectsCore's jump index
  (was a/y) and the parameterized `Run*(x)` calls now use a local
  `self = M(ObjectOffset)`. Two member-x writes survive as transition ABI:
  `x = enemyOffset` for ProcLoopCommand (reg=108, still register), and
  `x = self` for the parameterless handlers (RunNormalEnemies, RunFireworks,
  RunBowser, VineObjectHandler, JumpspringHandler) that read member x.
  UNLOCK PATH for the SMBEnemies reg=1 leaks (ChkForPlayerC_LargeP-B,
  DrawLargePlatform, Inc2B, FinishFlame): they stay live because the consumer
  is the `ProcELoop` loop in **GameCoreRoutine (SMBGame.cpp:3251)** —
  `do { writeData(ObjectOffset,x); EnemiesAndLoopsCore(x);
  FloateyNumbersRoutine(); ++x; } while (x != 0x06)`. The `++x` needs member x
  to survive both calls, so EnemiesAndLoopsCore's subtree must leave
  x=ObjectOffset=loop counter. De-register that loop (local counter `i`, set
  member x only for FloateyNumbersRoutine which is still register-based) to
  kill all four leaves at once. Check whether FloateyNumbersRoutine reads
  member x or M(ObjectOffset) first.
- **2026-07-18 session cont. — ProcELoop de-registered (unlock executed).**
  `GameCoreRoutine`'s enemy loop now uses a local counter `i`; member x is set
  only for `FloateyNumbersRoutine` (sole caller, still register-based, reads
  member x). The post-loop `x = 0x06` leak was dead too. Result: `Inc2B` and
  `DrawLargePlatform` tail x-restores are now dead — removed, both reg=0.
  But the unlock did NOT free all four leaves:
  - `ChkForPlayerC_LargeP`'s no-collision `x = M(ObjectOffset)` is STILL live
    (bisected individually; removing it alone diverges at Enemy_YMF_Dummy+4).
    Its consumer is some other register-based caller, not ProcELoop. Marked
    LIVE in-source.
  - `FinishFlame`'s `a = 0x00` is an **`a`** leak, unrelated to the x loop —
    still live, as recorded last session.
  Then: found the ChkForPlayerC_LargeP-B consumer — **`MoveLiftPlatforms`**,
  which indexed Enemy_Y_Position/YMF_Dummy/Y_Speed/Y_MoveForce off member x
  (hence the Enemy_YMF_Dummy divergence signature). It is NOT up the
  ChkForPlayerC_LargeP call chain at all; it is a *sibling* reached later via
  RunLargePlatform → LargePlatformSubroutines. Parameterized
  `MoveLiftPlatforms(e)` and `MoveSmallPlatform(e)` (both callers already had
  `e`), and the ChkForPlayerC_LargeP restore went dead. All four leaves from
  the previous entry are now resolved except FinishFlame's `a = 0x00`, which
  is an `a` leak and still live.
  LESSON: the "Enemy_YMF_Dummy+N" divergence names the *writer* that consumed
  the stale register — grep the diverging RAM cell for member-x indexing to
  find the consumer directly, instead of walking the call tree upward.
- **2026-07-18 session cont. — SMBObject 25 → 19 tokens.** Swept all eleven
  `x = M(ObjectOffset);` tail restores by deleting each one individually and
  running the full check. Six were dead and are gone (BBChk_E,
  MoveEnemyHorizontally, SetXMoveAmt, DrawExplosion_Fireworks,
  ExInjColRoutines, PlayerEnemyCollision); stale "Outputs: x = ..." comments
  updated. Five are still LIVE: CheckRightScreenBBox, UpdateNumber, RelWOfs,
  VariableObjOfsRelPos, GetOffScreenBitsSet.
  The per-line delete-and-check sweep (~1s each) is cheap and worth re-running
  after any batch of caller de-registering — deadness is not monotonic in
  hindsight, restores free up as consumers get parameterized.

- **2026-07-18 session cont. — SMBEnemies DONE (0 tokens, 0 gotos).** Whole file
  de-registered in one session, small-to-large:
  (1) Bowser cluster: RunBowser(e), ChkFireB(e), ProcessBowserHalf(e),
  BowserGfxHandler (full locals, pha/pla → ObjectOffset save/restore),
  InitBowser(e), DuplicateEnemyObj(e), KillAllEnemies (its
  `writeData(EnemyFrenzyBuffer, a)` consumed EraseEnemyObject's original
  `lda #$00` — now a literal 0), MoveLiftPlatforms tail-a dead.
  (2) ChkNoEn(startSlot) + ChkLak spiny-tail via local; both x-restores dead.
  (3) InitBowserFlame(e), PowerUpObjHandler (x=0x05 transition write dead once
  EnemyToBGCollisionDet took a param).
  (4) EnemyToBGCollisionDet(e) — all callees were already parameterized; pure
  x→e + locals.
  (5) EnemiesCollision(e) + ProcEnemyCollisions(first, second): first=x
  =ObjectOffset, second=y=M(0x01); the ECLoop 0x01 writeData must stay
  (RAM-visible). EnemyMovementSubs(e), RunNormalEnemies(e).
  (6) JumpspringHandler(e), RunFireworks(e), HandleGroupEnemies(enemyByte),
  DrawPowerUp via DrawOneSpriteRow's returned pair, VineObjectHandler(e)
  (tail restore dead), FirebarCollision (x=0 before InjurePlayer provably dead:
  InjurePlayer takes no register inputs), ProcBowserFlame(e) (pha/pla bit-shift
  chain → plain offscreenBits bit tests; d0→+12, d1→+8, d2→+4, d3→+0).
  (7) CheckpointEnemyID(e) — KEY FIND: its
  `PutAtRightExtent(...); writeData(Enemy_YMF_Dummy + x, a)` was THE consumer
  of FinishFlame's `a = 0x00` leak (FinishFlame runs inside PutAtRightExtent
  and the original left 0 in A, so the dummy gets 0x00, not the Y position).
  Made the 0x00 literal and FinishFlame's a-write went dead.
  (8) ProcLoopCommand(e) — y was just the EnemyData cursor (local dataOfs),
  FindLoop y → loop index passed into the wrongChk/incMLoop/doLpBack lambdas.
  (9) EnemiesAndLoopsCore: both transition x-writes dropped.
  Also: SprObjectOffscrChk's `x = M(ObjectOffset)` is STILL LIVE (iter 51628
  divergence) — consumer is a register-based caller in SMBGame/SMBPlayer.
  SMBObject's five LIVE tail restores (CheckRightScreenBBox, UpdateNumber,
  RelWOfs, VariableObjOfsRelPos, GetOffScreenBitsSet) re-swept after all this:
  all five STILL LIVE — their consumers are in SMBGame/SMBPlayer.
  LESSON (tooling): when splicing with python, always pass `start` to
  `s.index(end_marker, start)` — an end marker that also occurs earlier in the
  file silently duplicates the span (caused a redefinition mess, recovered via
  git checkout).

## Current counts (2026-07-18, end of second session)

| File          | Dirty funcs | Reg tokens | Gotos |
|---------------|------------:|-----------:|------:|
| SMB.cpp       |           0 |          0 |     1 |
| SMBArea/Data/EnemyGfx/Sound | 0 |      0 |     0 |
| SMBEnemies.cpp|           0 |          0 |     0 |
| SMBEngine.cpp |           4 |         14 |     0 |
| SMBObject.cpp |          12 |         13 |     0 |
| SMBPlayer.cpp |          35 |        583 |    99 |
| SMBGame.cpp   |          81 |       1290 |   101 |

NEXT UP: SMBPlayer or SMBGame (both goto-heavy; tackle goto removal alongside
de-registering per function). SMBObject's last 13 tokens and SMBEngine's 14 are
blocked on their SMBGame/SMBPlayer callers.

---
# (original survey below)


Goal: replace every use of the emulated CPU registers (`a`, `x`, `y` engine
members) in `source/SMB/*.cpp` with local variables, parameters and return
values, and remove `goto`/labels. Behavior must stay RAM-exact
(`bash tools/check.sh`).

Survey generated 2026-07-17 by counting bare `a`/`x`/`y` identifier tokens
(line comments stripped) and `goto` per `SMBEngine::` function.

## Overall status

| File          | Funcs | Clean | Dirty | Reg tokens left | Gotos left |
|---------------|------:|------:|------:|----------------:|-----------:|
| SMBArea.cpp   |    71 |    71 |     0 |               0 |          0 |
| SMBData.cpp   |     1 |     1 |     0 |               0 |          0 |
| SMBEnemyGfx.cpp |   6 |     6 |     0 |               0 |          0 |
| SMBSound.cpp  |    52 |    51 |     1 |              53 |          0 |
| SMB.cpp       |    70 |    62 |     8 |              35 |          1 |
| SMBObject.cpp |    93 |    63 |    30 |              41 |          0 |
| SMBPlayer.cpp |    46 |     9 |    37 |             597 |         99 |
| SMBEnemies.cpp|   157 |   105 |    52 |             794 |          0 |
| SMBGame.cpp   |    87 |     1 |    86 |            1300 |        101 |
| **Total**     | **583** | **369** | **214** |        **2820** |    **201** |

Done (fully clean): **SMBArea, SMBData, SMBEnemyGfx**.
Gotos survive only in **SMBGame**, **SMBPlayer**, and one in **SMB**.

## Remaining work by file (dirty functions: reg / goto)

### SMBSound.cpp — 1 function
- MusicHandler: reg=53

### SMB.cpp — 8 functions
- JumpEngine: reg=13
- ResidualMiscObjectCode: reg=6
- BridgeCollapse: reg=10
- DrawPlayer_Intermediate: reg=2
- GetAreaDataAddrs: reg=1
- ResidualGravityCode: reg=1
- PlayerVictoryWalk: reg=1
- VictoryMode: reg=1
- (plus 1 stray goto to locate)

### SMBObject.cpp — 30 functions (mostly reg=1, low-hanging)
BoundingBoxCore(2), GetObjRelativePosition(1), GetXOffscreenBits(1),
CheckRightScreenBBox(1), DrawSpriteObject(4), InitVStf(1), GetPlayerColors(1),
RemBridge(1), UpdateNumber(1), GetEnemyBoundBoxOfsArg(2), BBChk_E(1),
BlockBufferCollision(2), RelWOfs(1), RelativeEnemyPosition(1),
VariableObjOfsRelPos(1), GetEnemyOffscreenBits(1), GetOffScreenBitsSet(1),
MoveEnemyHorizontally(1), MoveObjectHorizontally(2), SetupFloateyNumber(3),
SprObjectCollisionCore(1), SetHiMax(2), SetXMoveAmt(1), EraseEnemyObject(1),
SprObjectOffscrChk(1), MoveD_EnemyVertically(2), DrawExplosion_Fireworks(1),
ChgAreaMode(1), ExInjColRoutines(1), PlayerEnemyCollision(1)

### SMBPlayer.cpp — 37 functions (goto-heavy)
PlayerBGCollision(129/31), PlayerPhysicsSub(56/25), PlayerCtrlRoutine(53/14),
PlayerHeadCollision(45/3), PlayerMovementSubs(27/8), HandlePipeEntry(26/2),
ClimbingSub(21), ImposeFriction(20/8), SpawnBrickChunks(20),
CheckTopOfBlock(19), PutPlayerOnVine(19), BumpBlock(18),
GetPlayerAnimSpeed(16/4), SetupJumpCoin(16), CoinBlock(12), FindEmptyMiscSlot(9/1),
RemoveCoin_Axe(9), GiveOneCoin(7), JCoinC(7), SetupPowerUp(7),
MovePlayerHorizontally(7), OnGroundStateSub(6), MovePlayerVertically(6),
BrickShatter(6), BlockBumpedChk(5/1), Skip_9(4), VineBlock(4),
DestroyBlockMetatile(3), ErACM(3), BlockBufferColli_Feet(3),
BlockBufferColli_Head(3), BlockBufferColli_Side(3), MushFlowerBlock(2),
StarBlock(2), ExtraLifeMushBlock(2), CheckForCoinMTiles(1/2),
ChkForLandJumpSpring(1)

### SMBEnemies.cpp — 52 functions
ProcLoopCommand(108), CheckpointEnemyID(106), ProcBowserFlame(55),
EnemyToBGCollisionDet(55), ProcEnemyCollisions(50), InitBowserFlame(37),
EnemiesAndLoopsCore(31), EnemiesCollision(30), RunBowser(29),
BowserGfxHandler(28), EnemyMovementSubs(22), FirebarCollision(22),
DuplicateEnemyObj(19), DrawPowerUp(17), VineObjectHandler(15),
HandleGroupEnemies(15), RunFireworks(12), ChkNoEn(12), PowerUpObjHandler(11),
ChkLak(9), JumpspringHandler(9), MoveLiftPlatforms(9), RunNormalEnemies(8),
ProcessBowserHalf(8), ChkFireB(7), KillAllEnemies(6), WarpZoneObject(6),
FinishFlame(6), InitBowser(5), BalancePlatform(4), RunStarFlagObj(4),
RunLargePlatform(4), LakituAndSpinyHandler(4), ChkForPlayerC_LargeP(3),
ProcLPlatCollisions(3), KillEnemyAboveBlock(3), MovePiranhaPlant(2),
SmallPlatformCollision(2), Inc2B(2), LargePlatformCollision(2),
RunSmallPlatform(2), InitEnemyObject(2), and 10 more with reg=1
(RedPTroopaGrav, DrawStarFlag, DoOtherPlatform, MoveSmallPlatform,
DrawSmallPlatform, MoveFallingPlatform, DrawLargePlatform, MoveEnemySlowVert,
MoveJ_EnemyVertically, LargePlatformBoundBox)

### SMBGame.cpp — 86 functions (largest remaining file; goto-heavy)
FloateyNumbersRoutine(72/7), DrawBrickChunks(59), FireballEnemyCollision(47/9),
DrawBlock(46), FlagpoleGfxHandler(46), DrawHammer(46/1), JCoinGfxHandler(45/1),
ProcessCannons(44/8), ProcHammerObj(43/3), BlockObjectsCore(39/5),
ProcessWhirlpools(38/5), FireballObjCore(34), MiscObjectsCore(33/5),
ColorRotation(31), ProcFireball_Bubble(31/6), Entrance_GameTimerSetup(27/4),
BulletBillHandler(27/2), PlayerHammerCollision(25), ProcessPlayerAction(24/11),
ChkForPlayerAttrib(24/3), RunGameTimer(24/2), PlayerLoseLife(22/2),
GameCoreRoutine(21/3), BlockObjMT_Updater(21/2), PlayerEntrance(21/2),
HandleChangeSize(20), PlayerGfxHandler(20), HurtBowser(17), PlayerOffscreenChk(17),
TransposePlayers(15), FireballBGCollision(15/5), RelativeBlockPosition(12),
DrawBubble(12), GetAreaMusic(11/3), PlayerGfxProcessing(12), DrawExplosion_Fireball(12),
GameRoutines(12/3), FlagpoleRoutine(10/5), MoveBubl(10), LoadAreaPointer(9),
VerticalPipeEntry(8), EnterSidePipe(8), UpdScrollVar(8), HandleEnemyFBallCol(19/1),
PlayerEndLevel(7/2), GetGfxOffsetAdder(7), SetupBubble(13), DrawFireball(5),
GetOffsetFromAnimCtrl(5), CyclePlayerPalette(5), GetBlockOffscreenBits(5),
RenderPlayerSub(6), DrawPlayerLoop(6/1), AnimationControl(6),
BlockBufferChk_FBall(6), BubbleCheck(6), GetProperObjOffset(6),
GetFireballBoundBox(6), GetMiscBoundBox(6), RelativeBubblePosition(4),
RelativeFireballPosition(4), RelativeMiscPosition(4), GetFireballOffscreenBits(4),
GetBubbleOffscreenBits(4), GetMiscOffscreenBits(4), ResJmpM(4), Skip_6(4),
FBallB(4), FlagpoleSlide(4), ProcessCannons/others... plus low-count:
ReplaceBlockMetatile(3), GetAreaType(3), ResetPalStar(2), MovePlayerYAxis(2),
NextArea(2), PlayerDeath(2), Vine_AutoClimb(2), and reg=1:
ImposeGravityBlock, GetCurrentAnimOffset, ThreeFrameExtent, DonePlayerTask,
PlayerFireFlower, ContinueGame, AutoControlPlayer, SideExitPipeEntry,
ChgAreaPipe

## Suggested order

1. **SMBObject.cpp** — 30 funcs but nearly all reg=1, no gotos. Fast wins.
2. **SMBSound.cpp** — a single function (MusicHandler).
3. **SMB.cpp** — 8 funcs, mostly small.
4. **SMBEnemies.cpp** — no gotos left; grind the 52 by ascending reg count.
5. **SMBPlayer.cpp** & **SMBGame.cpp** — biggest and only remaining goto work;
   tackle goto removal alongside de-registering per function.
