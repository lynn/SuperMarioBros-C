"""Shared pieces of the RAM comparison tools. See README.md in this directory."""
import os
import re
import sys
import time

FRAME = 0x800

_STARTED = time.time()


def log(message):
    """Say what phase we are in. Goes to stderr, so a tool's report stays on stdout
    and can still be piped somewhere."""
    tool = os.path.basename(sys.argv[0]).replace('.py', '') or 'smbram'
    print("[%s %5.1fs] %s" % (tool, time.time() - _STARTED, message), file=sys.stderr)

# FCEUX records RAM at the entry of the NMI, which is the state at the END of the
# previous iteration of the game's logic. "smbc ram" records it after update(),
# which is the state at the end of THIS one. So our record i lines up with theirs
# at i + SHIFT. Getting this wrong makes everything look broken.
SHIFT = 1


def reference():
    """The FCEUX dump, from $SMB_REF."""
    path = os.environ.get('SMB_REF')
    if not path:
        sys.exit("set SMB_REF to the FCEUX dump (see tools/README.md)")
    return _read(path, 'fceux')


def ours(path):
    return _read(path, 'ours')


def _read(path, what):
    data = open(path, 'rb').read()
    log("%s: %s, %d iterations (%.0f MB)"
        % (what, path, len(data) // FRAME, len(data) / 1e6))
    return data


def iterations(a, b):
    """How many iterations the two dumps can be compared over."""
    return min(len(a), len(b) // 1) // FRAME - SHIFT


# Differences in these cells are artifacts of the port, not differences in the
# game's state:
#
#   $00-$07    scratch temporaries. JumpEngine leaves ROM addresses in $04-$07,
#              which SMBEngine::jumpEngine() reproduces, so these do match -- but
#              they are not game state, so a difference here is not a bug in
#              itself. It is worth looking at when something downstream diverges.
#   $e7-$ea    pointers to ROM data. Synthetic addresses in the port.
#   $f5-$f6    MusicData, likewise a pointer to ROM data.
#   $100-$1ff  the 6502 stack page. The port has its own call stack.
#   $07b0      MusicOffset_Noise and NoiseDataLoopbackOfs. While the silence
#   $07c1      header is loaded these hold a byte read past the end of it, which
#              on the NES is the low byte of a pointer to CastleMusData. The noise
#              routine is gated off whenever silence is loaded, so the value is
#              never used. See "Known remaining differences" in FIXES.md.
_ARTIFACTS = {0xf5, 0xf6, 0x7b0, 0x7c1}


def is_artifact(address):
    return (address <= 0x07
            or 0xe7 <= address <= 0xea
            or 0x100 <= address <= 0x1ff
            or address in _ARTIFACTS)


def game_state_cells():
    return [a for a in range(FRAME) if not is_artifact(a)]


def game_state_runs():
    """The game state cells as contiguous [lo, hi) runs. Comparing an iteration is
    then half a dozen memcmps rather than seventeen hundred byte compares in Python,
    which is the difference between scanning the whole movie in a third of a second
    and in a couple of minutes."""
    runs, start = [], None
    for a in range(FRAME + 1):
        artifact = a == FRAME or is_artifact(a)
        if artifact and start is not None:
            runs.append((start, a))
            start = None
        elif not artifact and start is None:
            start = a
    return runs


def first_difference(ours, theirs, start, count):
    """The first iteration in [start, count) whose game state differs, or None."""
    runs = game_state_runs()
    o_ram, t_ram = memoryview(ours), memoryview(theirs)
    for i in range(start, count):
        o, t = i * FRAME, (i + SHIFT) * FRAME
        for lo, hi in runs:
            if o_ram[o + lo:o + hi] != t_ram[t + lo:t + hi]:
                return i
    return None


def _load_labels():
    labels = {}
    try:
        source = open('docs/smbdis.asm', errors='ignore')
    except IOError:
        log("no docs/smbdis.asm here; addresses will be unnamed (run from the repository root)")
        return labels
    for line in source:
        match = re.match(r'^([A-Za-z_0-9]+)\s*=\s*\$([0-9a-fA-F]+)\s*$', line.strip())
        if match:
            address = int(match.group(2), 16)
            if address < FRAME and address not in labels:
                labels[address] = match.group(1)
    return labels


_LABELS = _load_labels()


def name(address):
    """The disassembly's name for a RAM address, with an offset if it is inside an
    array. Empty if run from somewhere that cannot see docs/smbdis.asm."""
    for base in range(address, max(address - 64, -1), -1):
        if base in _LABELS:
            label = _LABELS[base]
            return label if base == address else '%s+%d' % (label, address - base)
    return '?'
