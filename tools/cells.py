"""List every RAM cell that differs from FCEUX at any point in a range of
iterations, and when it first did.

    SMB_REF=fceux.bin python3 tools/cells.py ours.bin [lo] [hi]

Unlike itercmp.py this includes the cells that are artifacts of the port rather
than game state, and marks them. It answers a different question: not "where does
it first go wrong" but "is what goes wrong confined to one subsystem". A long list
that is all one subsystem, or all artifacts, is worth knowing before digging in.
"""
import sys

import smbram
from smbram import FRAME, SHIFT

ours = smbram.ours(sys.argv[1])
theirs = smbram.reference()
lo = int(sys.argv[2]) if len(sys.argv) > 2 else 0
hi = int(sys.argv[3]) if len(sys.argv) > 3 else min(len(ours) // FRAME,
                                                    len(theirs) // FRAME - SHIFT)

first_differed = {}
for i in range(lo, hi):
    o, t = i * FRAME, (i + SHIFT) * FRAME
    for a in range(FRAME):
        if ours[o + a] != theirs[t + a]:
            first_differed.setdefault(a, i)

print("cells differing at any point in iterations %d-%d:" % (lo, hi))
for a in sorted(first_differed):
    print("  $%04x  %-28s first differs at iteration %-7d%s"
          % (a, smbram.name(a), first_differed[a],
             "  (port artifact, not game state)" if smbram.is_artifact(a) else ""))
