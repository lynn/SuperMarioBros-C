"""Find the first iteration whose game state differs from FCEUX's, and say what
differs. This is the tool that finds the bugs; the others help explain them.

    SMB_REF=fceux.bin python3 tools/itercmp.py ours.bin [start]

Run from the root of the repository, so that RAM addresses can be named.
"""
import sys

import smbram
from smbram import FRAME, SHIFT

ours = smbram.ours(sys.argv[1])
theirs = smbram.reference()
start = int(sys.argv[2]) if len(sys.argv) > 2 else 0

count = min(len(ours) // FRAME, len(theirs) // FRAME - SHIFT)
cells = smbram.game_state_cells()


def differences(i):
    o, t = i * FRAME, (i + SHIFT) * FRAME
    return [a for a in cells if ours[o + a] != theirs[t + a]]


first = next((i for i in range(start, count) if differences(i)), None)
if first is None:
    print("game state identical for all %d iterations" % count)
    sys.exit()

print("identical through iteration %d; diverges at iteration %d (of %d compared)\n"
      % (first - 1, first, count))

for i in range(first, min(first + 3, count)):
    d = differences(i)
    print(" iteration %d: %d cells" % (i, len(d)))
    for a in d[:16]:
        print("   $%04x  %-28s ours=%3d fceux=%3d"
              % (a, smbram.name(a), ours[i * FRAME + a], theirs[(i + SHIFT) * FRAME + a]))
    if len(d) > 16:
        print("   ... and %d more" % (len(d) - 16))
