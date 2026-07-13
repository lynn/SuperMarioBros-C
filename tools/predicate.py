"""Search the console's own RAM for a condition that fires exactly on the frames
where it lags.

    SMB_REF=fceux.bin python3 tools/predicate.py

The engine has no notion of cycles and so cannot know when a frame's work would
have overrun the NMI handler. But if some byte of game state takes a particular
value on the lagging frames and only on those, the engine can test for it and skip
a frame of the movie there, which is as good as having predicted the lag.

This is how SMBEngine::isLagFrame() was arrived at: it reported that
ScreenRoutineTask == 0 fires on 51 of the movie's 53 lag frames with no false
positives at all across 71,378 iterations, which is far too precise to be a
coincidence -- the handler overruns while the game is drawing a newly loaded area.

Run it again if the rule ever needs revisiting, or against a different movie. Bear
in mind that it only searches for a single byte compared against a single value; a
condition over two bytes would need more than this.
"""
import os
import sys
from collections import defaultdict

import smbram
from smbram import FRAME

data = smbram.reference()
index_path = os.environ['SMB_REF'] + '.idx'
frames = [int(line) for line in open(index_path) if line.strip()]

count = min(len(frames), len(data) // FRAME)
print("%d iterations" % count)


def ram(i):
    """RAM recorded at the entry of iteration i, which is the state at the end of
    iteration i - 1."""
    return data[i * FRAME:(i + 1) * FRAME]


# The console lagged on iteration i if it skipped a frame before the next NMI. The
# state the engine gets to test is the state at the END of iteration i, which is
# the RAM recorded at the entry of iteration i + 1.
lagging = {i for i in range(count - 1) if frames[i + 1] - frames[i] > 1}
print("lagging iterations: %d" % len(lagging))
print("  first few:", sorted(lagging)[:8])

matched_lagging = defaultdict(int)
matched_any = defaultdict(int)
for i in range(count - 1):
    state = ram(i + 1)
    for a in range(FRAME):
        key = (a, state[a])
        matched_any[key] += 1
        if i in lagging:
            matched_lagging[key] += 1

print("\nconditions firing on most of the lagging iterations, by precision:")
candidates = []
for key, hits in matched_lagging.items():
    if hits >= len(lagging) * 0.5:
        address, value = key
        false_positives = matched_any[key] - hits
        candidates.append((hits - false_positives, hits, false_positives, address, value))

for _, hits, false_positives, address, value in sorted(candidates, reverse=True)[:15]:
    print("  $%04x == %-3d  %-22s fires on %d/%d lag iterations, %d false positives"
          % (address, value, smbram.name(address), hits, len(lagging), false_positives))
