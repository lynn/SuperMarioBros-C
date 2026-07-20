# De-registering / de-gotoing TODO

## DONE (2026-07-19, fifth session) — all files at 0 register tokens, 0 gotos

SMBGame.cpp's remaining 413 tokens / 41 gotos were removed in 7 committed
batches (the reverted batch-F splice was redone in small pieces, one check
between each). After that, every register-based consumer in the codebase was
gone, so:

- SMBObject.cpp's 17 remaining tail restores (all previously LIVE, consumed
  by SMBGame/SMBPlayer callers) were deleted, and the stale "Outputs: x/y/a"
  comments rewritten.
- The a/x/y members, their save-state fields, and the uncalled pha()/pla()
  were removed from SMBEngine. The `s` stack register remains solely for
  JumpEngine, which is itself unreachable (kept for cross-reference with the
  disassembly).

Signature changes made along the way: ChgAreaPipe(mode),
CyclePlayerPalette(bits), FlagpoleGfxHandler(slot), BulletBillHandler(slot);
GameRoutines' goto targets became PlayerChangeSize / PlayerInjuryBlink /
InitChangeSize; GetPlayerColors returns its leftover VRAM buffer offset
(consumed by Entrance_GameTimerSetup as the residual SetupBubble slot).

| File          | Reg tokens | Gotos |
|---------------|-----------:|------:|
| all           |          0 |     0 |

## NEXT: zero-page pseudo-registers $00-$07

They deserve the same treatment as a/x/y: replace with locals, parameters and
return values. Survey as of 2026-07-19 (start of the work):

| addr | writes | reads | ++/-- | heaviest files | pre-write readers |
|------|-------:|------:|------:|----------------------------------------|---:|
| $00  |     92 |     88 |   14 | SMBEnemies 73, SMBObject 37, SMBGame 32, SMBArea 28 | 17 |
| $01  |     32 |     33 |    1 | SMBEnemies 20, SMBObject 17, SMBGame 13 |  8 |
| $02  |     33 |     29 |    8 | SMBEnemies 22, SMBObject 19, SMBEnemyGfx 11 | 12 |
| $03  |     17 |     17 |    1 | SMBObject 10, SMBArea 9                 |  2 |
| $04  |     17 |     10 |    2 | SMBObject 9, spread thin                |  4 |
| $05  |     15 |     12 |    1 | SMBEnemies 11, SMBObject 6              |  3 |
| $06  |     18 |     13 |    1 | SMBArea 10, SMBEnemies 10               |  8 |
| $07  |     29 |     34 |    3 | SMBArea 30, SMBObject 12                | 16 |

137 functions touch at least one (SMBEnemies 37, SMBObject 30, SMBArea 24,
SMBGame 23, SMBPlayer 12, SMB 8, SMBEnemyGfx 3). "Pre-write readers" are
functions whose first touch of the byte is a read -- they consume a value a
caller staged there, the zero-page analog of a register leak.

Facts that shape the work:

- **The harness masks $00-$07** (`smbram.is_artifact()`, along with $e7-$ea
  and the stack page), so converting a temp to a local cannot fail check.sh
  directly; only a mis-transcribed *value* flowing into real RAM fails. Same
  oracle dynamics as the registers.
- **The cross-function reads cluster into recognizable ABIs:**
  - `DrawSpriteObject` reads $00-$05: a six-byte parameter block (tile
    numbers row in $00/$01/$02 sharing space with y-coord staging, attributes
    $04, flip $03, x/y coords) staged by every drawing loop. Biggest single
    ABI; wants to be a struct or parameters, and unlocks most $00-$05 sites
    in the drawing code.
  - **$07 is the area parser's register**: 11 of its 16 pre-write readers are
    SMBArea object handlers (TreeLedge, MushroomLedge, VerticalPipe, NormObj,
    ...) sharing a temp staged by the parser dispatch loop. Plus the known
    SetupBubble residual (the `writeData(0x07, 145)` LYNN HACK becomes an
    explicit parameter once converted).
  - Documented input contracts: ImposeGravity reads $00 (movement amount) and
    $02 (max speed); DividePDiff reads $06 (pixel threshold);
    PutBlockMetatile/RemBridge round-trip $00/$01; the firebar cluster
    (FirebarCollision/DrawFirebar_Collision) reads nearly all of them and is
    the gnarliest consumer.
  - The easy majority: `writeData(0x00, 0x03); do {...; --M(0x00);} while`
    loop counters and same-function scratch. Mechanical conversions.

Planned order: **$03 -> $04 -> $05** (fewest cross-function readers), then
$06, $01, $02, then $07 (mostly SMBArea) and $00 last. DrawSpriteObject's
parameter block is worth one early dedicated batch since it spans $00-$05.

### Progress (2026-07-19, session 2): $03/$04/$05 effectively DONE

Six committed RAM-exact batches. Counts now: $03 = 7, $04 = 7, $05 = 4, all
in two deliberately deferred spots (see below). What landed:

- Same-function locals: RenderAreaGraphics ($03/$04/$05), OutputNumbers'
  digit counter, OffscreenBoundsCheck's extended-edge 16-bit pair, the enemy
  group placer's page:x pair, MoveSwimmingCheepCheep, BlockBufferCollision
  internals, RenderSidewaysPipe's dead $05 write.
- RemBridge(metatileGroupOfs4, vramOffset, nameTableLow, nameTableHigh);
  stagers PutBlockMetatile and BridgeCollapse pass values.
- DrawSpriteObject/DrawOneSpriteRow(+DrawPlayerLoop) take (flipBits,
  attributeBits, xPos). Stagers converted: RenderPlayerSub, DrawBlock,
  FlagpoleGfxHandler, DrawPowerUp.
- BlockBufferCollision returns {metatile, coordinate nybble}; the whole
  wrapper family (Skip_9, Colli_Head/Feet/Side, BBChk_E, Chk_Enemy,
  ChkUnderEnemy) returns the pair; consumers: PlayerBGCollision's
  head/foot/side nybble checks (lambdas take it as a parameter) and
  EnemyToBGCollisionDet's landing check. BumpBlock(collidedMetatile).
  DEAD register-era residuals deleted: GetX/GetYOffscreenBits' $04 saves,
  DividePDiff's $05 save.
- Firebar: GetFirebarPosition *returns* the mirror data ($03);
  DrawFirebar_Collision(mirrorData) shifts a local ($05); FirebarCollision's
  counter ($05) and modX ($04) are locals.

Remaining $03-$05 sites, deferred on purpose:
1. **EnemyGfxHandler subsystem** (SMBEnemyGfx.cpp): stages $02-$05 (and
   $eb-$ef) at the top of EnemyGfxHandler with mutations along the way;
   DrawEnemyObjRow passes M(0x03)/M(0x04)/M(0x05) at call time for now.
   Convert together with $02 and the $eb-$ef pseudo-registers as one
   subsystem batch (a small staging struct may be the right shape).
2. **DrawPlayer_Intermediate** (SMB.cpp): stages $02-$07 from a data table
   in a loop; passes M() reads to DrawPlayerLoop for now. Falls out
   naturally with the $02/$07 pass.
3. **JumpEngine** (SMB.cpp): unreachable; $04/$05 writes go away if/when it
   is deleted.

### Progress (2026-07-20, session 3): $00-$02 started

Three committed RAM-exact batches. Writes now: $00 = 72 (was 92), $01 = 23
(was 32), $02 = 17 (was 33). What landed:

- **UpdateScreen(bufferAddr)**: the $00/$01 indirect became a uint16_t
  parameter the packet loop advances directly; NonMaskableInterrupt composes
  it from the VRAM address tables. Also in SMB.cpp: DrawTitleScreen's $0300
  pointer is a local page/offset pair, SpriteShuffler's $28 preset a local
  constant, ReadPortBits' $00 write was dead.
- **The sprite-drawing parameter block is done.** DrawSpriteObject and
  DrawOneSpriteRow take both tile numbers ($00/$01) as leading parameters,
  and the $02 y coordinate as a `uint8_t&` in/out (each row reads it and
  writes back +8 so the loop walks down the object). DrawPlayerLoop forwards
  it. Stagers converted: DrawPowerUp, DrawBlock, RenderPlayerSub, the
  flagpole floatey number.
- **ImposeGravity(mode, offset, downAmount, upAmount, maxSpeed)**: the
  documented $00/$01/$02 input contract, threaded through the
  ImposeGravitySprObj / SetXMoveAmt / RedPTroopaGrav wrappers to all nine
  stagers. Its internal $07 scratch became two locals.

Still deferred, unchanged from the $03-$05 notes: the EnemyGfxHandler
subsystem (DrawEnemyObjRow now round-trips $02 through a local, since the
handler keeps mutating it between rows -- convert with $eb-$ef as one batch)
and DrawPlayer_Intermediate's $02-$07 table staging.

Remaining $00-$02 sites by file: SMBEnemies 89, SMBObject 49, SMBArea 37,
SMBGame 35, SMBPlayer 22, SMBEnemyGfx 12, SMB 1. The known clusters left are
PutBlockMetatile's $00/$01 round-trip, the firebar cluster, ImpedePlayerMove's
$00 collision-side input, the block-buffer $02 vertical high nybble in
SMBPlayer, and a long tail of loop counters and same-function scratch.

### Progress (2026-07-20, session 4): SMBGame and SMBPlayer nearly clear

Five more committed RAM-exact batches. Writes now: $00 = 50, $01 = 18,
$02 = 16. Remaining sites by file: SMBEnemies 87, SMBObject 46, SMBArea 38,
SMBEnemyGfx 13, SMBPlayer 8, SMB 1. What landed:

- **PutBlockMetatile(metatileGroupSelector, vramOffset)**: its $00 write was
  a register-era save of the control bit that nothing reads any more, so the
  control bit stopped being threaded at all -- WriteBlockMetatile(metatile)
  and DestroyBlockMetatile() lost their parameter.
- **SMBGame is done**: the area palette copy counter, the whirlpool right
  extent and center 16-bit pairs, CyclePlayerPalette's masked bits,
  AnimationControl's upper extent (already a parameter), PlayerOffscreenChk's
  shifted bits, DrawBrickChunks' palette bits and original relative X, and
  the fireball loop's enemy slot. HurtBowser(slot, scoreSlot) -- the score
  slot is the fireball loop's own offset, which differs from the
  linked-bowser slot it already received.
- **PlayerEnemyDiff returns the low byte** of the difference as well as the
  high byte it already returned; three consumers (PlayerLakituDiff's distance
  clamp, the piranha plant's player-near-pipe check, BulletBillHandler's
  cannon distance) each work on their own copy now.
- **ImpedePlayerMove(side)**: the player side-check loop's $00 counter also
  served as ImpedePlayerMove's collision-side input, so the two had to move
  together -- the counter is a local, the side a parameter threaded through
  StopPlayerMove and the CheckSideMTiles lambda. The platform collision in
  SMBEnemies and the foot check pass theirs directly.
- Also HandlePipeEntry(rightFootMetatile, leftFootMetatile), X_Physics'
  friction index, MoveOnVine's sign-extension adder.

Clusters left: the firebar family, PlayerLakituDiff's three caller-staged
values at $01-$03, the SMBPlayer/SMBObject $02 block-buffer vertical high
nybble (BlockBufferCollision writes it; CheckTopOfBlock, ErACM,
PlayerHeadCollision and PutBlockMetatile read it -- wants to join
BlockBufferCollision's returned pair), SMBArea's parser temps, and the long
tail in SMBEnemies. Still deferred: the EnemyGfxHandler subsystem (with
$eb-$ef) and DrawPlayer_Intermediate's table staging.

### Progress (2026-07-20, session 5): $00-$02 down to two deferred clusters

Seven more committed RAM-exact batches. **SMBEnemies, SMBObject, SMBGame
and SMBPlayer are entirely clear of $00-$02.** What is left: SMBArea 9 (the
area parser's object id, which pairs with $07), SMBEnemyGfx 12 and SMB 1
(both previously deferred). What landed:

- **BlockBufferCollision returns the block row** it found -- the $02
  vertical high nybble offset -- as a third value through BBChk_E, Skip_9,
  BlockBufferColli_Head/Feet/Side, BlockBufferChk_Enemy and ChkUnderEnemy.
  Every consumer takes it as a parameter: PlayerHeadCollision, ErACM,
  HandleCoinMetatile, RemoveCoin_Axe, PutBlockMetatile, SetupJumpCoin, and
  the vine and enemy blank-metatile checks in SMBEnemies. CheckTopOfBlock
  takes it by reference, since BrickShatter and BumpBlock reach it with the
  row already moved up a row.
- **The firebar cluster**: GetFirebarPosition takes the firebar part number
  and returns the horizontal and vertical adders alongside the mirroring
  data; DrawFirebar_Collision takes both. With the loop counter a local,
  FirebarCollision's save/restore of $00 across InjurePlayer went dead.
- **SetupPlatformRope returns the name table address** instead of leaving it
  at $00/$01. ProcLPlatCollisions takes the value its callers staged (the
  small platform search's bounding box counter, or the large platform's
  vertical coordinate). MoveWithXMCntrs returns the signed adder
  PositionPlayerOnHPlat wants. SetHJ takes the jump length bitmask.
  SixSpriteStacker returns the OAM offset it was given.
- **RunOffscrBitsSubs returns the horizontal nybble** it left at $00
  alongside the vertical bits; GetMaskedOffScrBits takes the onscreen
  bitmask its two callers staged; PlayerEnemyDiff (session 4) the same for
  the low byte. VariableObjOfsRelPos and SetOffscrBitsOffset were adding
  their own parameter back to itself through $00.
- Plus a long tail of same-function locals in SMBArea's renderers,
  SMBObject's bounding box and horizontal movement, and SMBEnemies'
  cheep-cheep frenzy, bowser's flame and enemy group placer.

The two clusters left are both the ones already deferred: **EnemyGfxHandler**
(SMBEnemyGfx stages $02-$05 with mutations along the way -- convert with
$eb-$ef as one subsystem batch) and **DrawPlayer_Intermediate** (SMB.cpp,
$02-$07 from a data table). SMBArea's nine $00 sites are the area parser's
object id, which wants to move together with $07.

### Progress (2026-07-20, session 6): $06 done bar the SMBArea parser

Five more committed RAM-exact batches. $06 is down from 32 sites to 10, all
of them SMBArea's parser temp (which pairs with $07) plus one in the
unreachable JumpEngine. What landed:

- **DividePDiff(pixelDiff, threshold, ...)**: the documented $06 threshold
  and the $07 pixel difference both became parameters; GetXOffscreenBits and
  GetYOffscreenBits pass their own locals.
- **BlockBufferResult** replaces the anonymous three-tuple that the whole
  BlockBufferCollision wrapper family returned (BBChk_E, Skip_9,
  BlockBufferColli_Head/Feet/Side, BlockBufferChk_Enemy, ChkUnderEnemy), with
  named `metatile` / `coordinate` / `row` fields.
- **The block buffer address is the struct's fourth field.**
  GetBlockBufferAddr returns a uint16_t instead of leaving $06/$07 set, and
  every consumer takes it as a parameter: PutBlockMetatile,
  WriteBlockMetatile, DestroyBlockMetatile, ReplaceBlockMetatile,
  RemoveCoin_Axe, ErACM, HandleCoinMetatile, CheckTopOfBlock, BumpBlock,
  BrickShatter, SetupJumpCoin, PlayerHeadCollision and PutPlayerOnVine (plus
  the checkSideMTiles/handleClimbing lambdas). The self-contained pointers
  became locals: SMBArea's metatile graphics table and block buffer fill, and
  SMBGame's block object updater, which composes $0500 | Block_BBuf_Low.
- **The firebar's $06/$07**: FirebarCollision(oamOffset, segmentX, segmentY)
  returns the next OAM offset, and DrawFirebar_Collision takes and returns it
  too, so FirebarGfxHandler's loop threads the offset directly.
- Plus locals for the enemy parser's extended right boundary, the bounding
  box collision counter, and InitializeMemory's page base.

**Remaining: $07, 46 sites, and it is one cluster.** SMBArea's area parser
uses $07 as the row/column register shared between the dispatch loop and the
object handlers (TreeLedge, MushroomLedge, VerticalPipe, NormObj, ...), with
$06 as the length temp beside it and $00 as the object id. GetLrgObjAttrib
writes $07 for DrawRow. Also here: the SetupBubble residual in SMBGame
(`writeData(0x07, 145)` LYNN HACK, which wants to become an explicit
parameter) and DrawPlayer_Intermediate's $02-$07 table staging, still
deferred alongside the EnemyGfxHandler/$eb-$ef subsystem.

Counts now: $00 = 9, $01 = 0, $02 = 14, $03 = 7, $04 = 9, $05 = 4, $06 = 10,
$07 = 46. By file: SMBArea 46, SMBEnemyGfx 21, SMBGame 7, SMBEnemies 6,
SMB 5, SMBPlayer 3.

## Possible follow-ups

- Delete JumpEngine and `s` entirely (they are unreachable) if cross-reference
  value is judged low.
- The out-of-bounds table reads (see memory: fix one table at a time).
- General readability passes now that the register ABI is gone.
