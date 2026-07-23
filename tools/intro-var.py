#!/usr/bin/env python3
"""Introduce a named reference for a single RAM cell, and use it everywhere.

    python3 tools/intro-var.py TimerControl

For a constant NAME that is the address of a RAM cell, this binds a member reference

    uint8_t& name;   // camelCase of NAME

to ram[NAME] and rewrites every M(NAME) and ram[NAME] in the SMB sources to that
reference. M(NAME) is getMemory(NAME), which for a RAM address is ram[NAME & 0x7ff]
== ram[NAME], so the reference names the very same byte and the change is RAM-exact.

The reference is declared on SMBEngine (SMBEngine.hpp) and bound in the constructor's
initializer list (SMBEngine.cpp); the whole-token forms M(NAME) and ram[NAME] are
replaced across source/SMB/*.cpp. Offset reads like M(NAME + i) are left alone -- a
scalar alias has no index -- and so is IntervalTimerControl and the like, since the
enclosing (...) / [...] pin the match to the exact name.

Run tools/check.sh afterwards to confirm the RAM stays identical.
"""
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SMB = ROOT / "source" / "SMB"
CONSTANTS = SMB / "SMBConstants.hpp"
HEADER = SMB / "SMBEngine.hpp"
ENGINE = SMB / "SMBEngine.cpp"
RAM_SIZE = 0x800


def var_name(name):
    """TimerControl -> timerControl."""
    return name[0].lower() + name[1:]


def constant_address(name):
    """The value of `#define NAME 0x....` in SMBConstants.hpp, or None."""
    m = re.search(rf"^#define\s+{re.escape(name)}\s+(0x[0-9a-fA-F]+|\d+)\b",
                  CONSTANTS.read_text(), re.MULTILINE)
    return int(m.group(1), 0) if m else None


def fail(msg):
    sys.exit(f"intro-var: {msg}")


def main():
    if len(sys.argv) != 2:
        fail("usage: tools/intro-var.py NAME")
    name = sys.argv[1]
    var = var_name(name)

    address = constant_address(name)
    if address is None:
        fail(f"no `#define {name} ...` in {CONSTANTS.name}")
    if address >= RAM_SIZE:
        fail(f"{name} = {address:#06x} is not a RAM address (< {RAM_SIZE:#x}); "
             "the reference would not alias ram[] RAM-exactly")

    header = HEADER.read_text()
    if re.search(rf"\buint8_t&\s+{re.escape(var)}\b", header):
        fail(f"{var} is already declared in {HEADER.name}")

    # 1. Rewrite the usages first, before the constructor gains a `ram[NAME]` of
    #    its own that we must not touch.
    total = 0
    read = re.compile(rf"\bM\({re.escape(name)}\)")
    cell = re.compile(rf"\bram\[{re.escape(name)}\]")
    for path in sorted(SMB.glob("*.cpp")):
        text = path.read_text()
        text, n1 = read.subn(var, text)
        text, n2 = cell.subn(var, text)
        if n1 or n2:
            path.write_text(text)
            total += n1 + n2
            print(f"  {path.name}: {n1 + n2} site(s)")

    # 2. Declare the reference member, right after `chr`.
    decl = f"\n\n    uint8_t& {var};       /**< Alias for ram[{name}]. */"
    header, n = re.subn(r"(uint8_t\* chr;.*)", rf"\1{decl}", header, count=1)
    if n != 1:
        fail(f"could not find the `uint8_t* chr;` anchor in {HEADER.name}")
    HEADER.write_text(header)

    # 3. Bind it in the constructor's initializer list.
    engine = ENGINE.read_text()
    engine, n = re.subn(r"(audioEnabled\(enableAudio\),)",
                        rf"\1 {var}(ram[{name}]),", engine, count=1)
    if n != 1:
        fail(f"could not find the initializer-list anchor in {ENGINE.name}")
    ENGINE.write_text(engine)

    print(f"Introduced `{var}` -> ram[{name}] ({address:#06x}); "
          f"rewrote {total} usage(s). Now run tools/check.sh.")


if __name__ == "__main__":
    main()
