"""Check that the modules of the game only call downwards.

    python3 tools/layers.py            # exit 0 if the layering holds, 1 if it does not
    python3 tools/layers.py --report   # print what calls what, module by module

Splitting SMB.cpp into files is cheap, and so is undoing it by accident: every routine is a
method of SMBEngine, declared in one header, so any of them can call any other and the compiler
will never object. The files are a claim about the shape of the program, and nothing but this
enforces it. Without it the layering is a comment.

The claim is that the modules form a stack, and a call only ever goes down it:

    SMB.cpp     code(), the NMI, and the mode dispatch that runs the game
    SMBSound    the APU driver: music and sound effects
    SMBObject   the kernel: sprites, collision, gravity, the offscreen checks

A module may call anything in a layer below it, and anything in itself. It may not call up.
SMBObject is at the bottom and calls nothing but itself, which is what let it be a file at all;
SMBSound is beside it and does the same. Should SMBObject ever call the game, the kernel is not
a kernel any more, and this says so before the file is committed rather than a year later.

Add a module by giving it a rank: lower numbers are lower down, and equal numbers are siblings
that may not call each other.
"""
import argparse
import collections
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from split import DEFINITION, blocks

SOURCE = 'source/SMB'

# The stack, bottom first. A module may call down and into itself, never up, and never sideways.
RANK = {
    'SMBObject.cpp': 0,     # the kernel: sprites, collision, gravity, offscreen
    'SMBSound.cpp': 0,      # the APU driver, which calls nothing at all
    'SMBEnemyGfx.cpp': 1,   # drawing an enemy, which the enemies and the game both do
    'SMBPlayer.cpp': 1,     # the player's own control, movement and collision
    'SMBArea.cpp': 1,       # the area parser: level data into the block buffer
    'SMBEnemies.cpp': 2,    # every enemy, platform and loop command in the game
    'SMBGame.cpp': 3,       # the game core, which drives all of the above
    'SMB.cpp': 9,           # code(), the NMI, and the mode dispatch
}


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument('--report', action='store_true', help='print what calls what')
    arguments = parser.parse_args()

    home, text = {}, {}
    for name in sorted(RANK):
        for routine, block in blocks(os.path.join(SOURCE, name)).items():
            home[routine] = name
            text[routine] = block

    call = re.compile(r'\b(%s)\s*\(' % '|'.join(sorted(home, key=len, reverse=True)))
    out = collections.defaultdict(list)
    for routine, (heading, end, lines) in text.items():
        for line in lines[heading:end + 1]:
            for callee in call.findall(re.sub(r'//.*', '', line)):
                out[routine].append(callee)

    edges = collections.defaultdict(collections.Counter)
    wrong = []
    for routine, callees in out.items():
        for callee in callees:
            here, there = home[routine], home[callee]
            if here == there:
                continue
            edges[here][there] += 1
            if RANK[there] >= RANK[here]:
                wrong.append((routine, here, callee, there))

    if arguments.report:
        print('%-16s %5s %6s   %s' % ('MODULE', 'RANK', 'FNS', 'calls into'))
        print('-' * 66)
        for name in sorted(RANK, key=lambda n: (RANK[n], n)):
            mine = [r for r in home if home[r] == name]
            into = ', '.join('%s(%d)' % (os.path.splitext(m)[0], n)
                             for m, n in edges[name].most_common())
            print('%-16s %5d %6d   %s' % (name, RANK[name], len(mine), into or '--'))
        print()

    if not wrong:
        print('the layering holds: %d modules, no call goes up' % len(RANK))
        return 0

    print('%d calls go the wrong way:\n' % len(wrong))
    for routine, here, callee, there in sorted(wrong):
        print('  %s (%s) calls %s (%s)' % (routine, here, callee, there))
    print('\nEither the callee belongs lower down, or the caller belongs higher up.')
    return 1


if __name__ == '__main__':
    sys.exit(main())
