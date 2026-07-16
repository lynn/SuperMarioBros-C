The goal of this project is to transform source/SMB/*.cpp into readable code that is RAM-compatible with SMB1, but need not conform to the restrictions of the 6502.

Aim to preserve function/subroutine names for ease of cross-reference with the disassembly.

When making any changes, use `bash tools/check.sh` to make sure behavior is still RAM-exact.
