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

## Possible follow-ups

- Delete JumpEngine and `s` entirely (they are unreachable) if cross-reference
  value is judged low.
- The out-of-bounds table reads (see memory: fix one table at a time).
- General readability passes now that the register ABI is gone.
