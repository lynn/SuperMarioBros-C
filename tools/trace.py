"""Watch a few RAM cells, ours against FCEUX's, over a range of iterations. Prints
only the iterations where one of them changes, so that a long range stays readable.

    SMB_REF=fceux.bin python3 tools/trace.py ours.bin 33,86,1d 9500 9515

Addresses are hexadecimal and comma separated. Useful once itercmp.py has pointed
at an iteration: it shows how the state got there, and how the two runs part.

Careful with the values it prints: they are the state at the END of an iteration.
A cell the game writes and then rewrites within one frame will not show the value
that some intermediate read of it saw. When that matters, print from the engine
at the instruction itself instead of reasoning from these.
"""
import sys

import smbram
from smbram import FRAME, SHIFT

ours = smbram.ours(sys.argv[1])
theirs = smbram.reference()
addresses = [int(x, 16) for x in sys.argv[2].split(',')]
lo, hi = int(sys.argv[3]), int(sys.argv[4])

previous = None
differing = 0

for i in range(lo, hi):
    o, t = i * FRAME, (i + SHIFT) * FRAME
    row = tuple((ours[o + a], theirs[t + a]) for a in addresses)
    differs = any(x != y for x, y in row)

    if row != previous:
        cells = '  '.join("$%04x o=%3d f=%3d" % (a, x, y)
                          for a, (x, y) in zip(addresses, row))
        print("iter %6d  %s%s" % (i, cells, '  <-- DIFFER' if differs else ''))
        previous = row

    if differs:
        differing += 1

print("\n%d of %d iterations differ in these cells" % (differing, hi - lo))
print("names: " + ', '.join("$%04x %s" % (a, smbram.name(a)) for a in addresses))
