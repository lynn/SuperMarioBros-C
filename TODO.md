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

## Possible follow-ups

- Delete JumpEngine and `s` entirely (they are unreachable) if cross-reference
  value is judged low.
- The out-of-bounds table reads (see memory: fix one table at a time).
- General readability passes now that the register ABI is gone.
