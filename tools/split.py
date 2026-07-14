"""Move a module's routines out of SMB.cpp into a file of their own.

    python3 tools/split.py source/SMB/SMB.cpp SMBSound.cpp --entry SoundEngine
    python3 tools/split.py source/SMB/SMB.cpp SMBObject.cpp --kernel        [--apply]

Every routine of the game is a method of SMBEngine, declared once in SMBEngine.hpp, so which
file a routine's body is written in is of no consequence to anything but us: moving one changes
no declaration, no call, and no behaviour, and the linker checks the arithmetic. That is what
makes this cheap. What it is not is arbitrary -- a routine can only leave with a module if
nothing outside the module needs it -- so this works out what belongs to what rather than being
told.

A module is a subtree of the dominator tree. X dominates Y when every path from code() to Y runs
through X, so Y is reachable only by way of X and can go wherever X goes; the routines a
subsystem dominates are its own, and the ones it merely calls are shared with somebody else and
have to stay behind. `--entry SoundEngine` takes the routines SoundEngine dominates.

The kernel is the other kind of module, and the dominator tree cannot name it: it is not a
subtree but the routines two or more subsystems both reach -- the sprite, collision, gravity and
offscreen primitives every actor in the game is built out of. Nothing dominates them, because
being reached from several places is the whole of what they are. `--kernel` takes those, plus
whatever they need that nobody else owns, so that the file is closed: it calls nothing above it.

Nothing is written without --apply.
"""
import argparse
import collections
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

DEFINITION = re.compile(r'^(void|bool) SMBEngine::(\w+)\(')

# The subsystems, as the routines they hang from. A routine shared by two of these belongs to
# neither and falls to the kernel; one shared by a subsystem and code() alone is the subsystem's,
# and code() may call down into it.
SUBSYSTEMS = ('SoundEngine', 'EnemiesAndLoopsCore', 'AreaParserTaskHandler',
              'GameCoreRoutine', 'PlayerCtrlRoutine', 'EnemyGfxHandler')

HEADER = '''// %s
//
// Moved out of SMB.cpp by tools/split.py. These are methods of SMBEngine like every other
// routine of the game and are declared in SMBEngine.hpp; the file they are written in is the
// only thing that is different, and tools/layers.py is what keeps it meaning something.
//
#include "SMB.hpp"
'''

SUBSYSTEM_BLURB = ('The %s subsystem: everything %s() reaches that nothing\n'
                   '// outside it reaches, and so nothing outside it needs.')
KERNEL_BLURB = ('The object kernel: the sprite, collision, gravity and offscreen primitives that\n'
                '// every actor in the game is built out of. It is not a subsystem -- being reached\n'
                '// from several of them at once is the whole of what it is -- so no subsystem owns\n'
                '// it, and it calls nothing above itself.')


def blocks(path):
    """Each routine of the file, as the run of lines it is written on.

    A routine's text starts at the rule above it -- the separator and whatever comment is
    written under it -- and ends at the brace in the first column that closes it.
    """
    lines = open(path).read().split('\n')
    found = collections.OrderedDict()
    for i, line in enumerate(lines):
        match = DEFINITION.match(line)
        if not match:
            continue
        heading = i
        while heading > 0:                       # take the rule and the comments above it too
            above = lines[heading - 1].strip()
            if above and not above.startswith('//'):
                break
            heading -= 1
            if above.startswith('//---'):
                break
        end = next(k for k in range(i, len(lines)) if lines[k] == '}')
        found[match.group(2)] = (heading, end, lines)
    return found


def calls(path, names):
    """What each routine of the file calls."""
    out = collections.defaultdict(list)
    call = re.compile(r'\b(%s)\s*\(' % '|'.join(sorted(names, key=len, reverse=True)))
    for name, (heading, end, lines) in blocks(path).items():
        for line in lines[heading:end + 1]:
            for callee in call.findall(re.sub(r'//.*', '', line)):
                if callee not in out[name]:
                    out[name].append(callee)
    return out


def dominators(out, root):
    """The immediate dominator of every routine, by Cooper-Harvey-Kennedy."""
    order, seen, edge = [], {root}, [root]
    while edge:
        i = edge.pop()
        order.append(i)
        for j in out[i]:
            if j not in seen:
                seen.add(j)
                edge.append(j)
    rank = {name: k for k, name in enumerate(order)}
    into = collections.defaultdict(list)
    for a in out:
        for b in out[a]:
            if a in rank and b in rank:
                into[b].append(a)
    idom = {root: root}

    def meet(a, b):
        while a != b:
            while rank[a] > rank[b]:
                a = idom[a]
            while rank[b] > rank[a]:
                b = idom[b]
        return a

    moved = True
    while moved:
        moved = False
        for name in order:
            if name == root:
                continue
            fresh = None
            for pred in into[name]:
                if pred in idom:
                    fresh = pred if fresh is None else meet(fresh, pred)
            if fresh is not None and idom.get(name) != fresh:
                idom[name] = fresh
                moved = True
    return idom


def owned(out, idom, root):
    """The routines a subsystem dominates: its own, and nobody else's."""
    kids = collections.defaultdict(list)
    for name, parent in idom.items():
        if name != parent:
            kids[parent].append(name)
    got, edge = {root}, [root]
    while edge:
        for kid in kids[edge.pop()]:
            got.add(kid)
            edge.append(kid)
    return got


def adopted(out, idom, names, mine, root):
    """The routines only this module, and the code above it, ever call.

    Dominance is stricter than it needs to be. A routine that EnemiesAndLoopsCore calls and
    code() also calls is dominated by neither -- code() reaches it without going through the
    subsystem -- so it stays behind in SMB.cpp, and then the subsystem calls up out of its own
    file, which is the one thing the layering forbids. But it can go down with the module all
    the same, because the module is not the only thing allowed to call it: SMB.cpp sits on top
    of every module and may call down into any of them. The only caller that would be a problem
    is a *different* subsystem, and a routine two subsystems share is in the kernel already.

    So: no other subsystem calls it, and this one does. MoveJ_EnemyVertically and BowserGfxHandler
    come home to the enemies, WriteGameText to the area parser, and the modules stop reaching up.
    """
    others = set()
    for other in SUBSYSTEMS:
        if other != root and other in names:
            others |= owned(out, idom, other)
    callers = collections.defaultdict(set)
    for a in out:
        for b in out[a]:
            callers[b].add(a)

    taken = set(mine)
    growing = True
    while growing:                               # taking one may free the routines only it calls
        growing = False
        for name in names:
            if name in taken or name in others or name == 'code':
                continue
            who = callers[name]
            if who & taken and not who & others:
                taken.add(name)
                growing = True
    return taken - set(mine)


def kernel(out, idom, names):
    """The routines more than one subsystem reaches, and whatever those need in turn."""
    mine = {root: owned(out, idom, root) for root in SUBSYSTEMS}
    spoken_for = set().union(*mine.values())
    rest = set(names) - spoken_for - {'code'}

    reached = collections.defaultdict(set)
    for root, members in mine.items():
        for member in members:
            for callee in out[member]:
                if callee in rest:
                    reached[callee].add(root)
    shared = {name for name, roots in reached.items() if len(roots) > 1}
    growing = True
    while growing:                               # close it: a kernel routine's needs are kernel too
        growing = False
        for name in list(shared):
            for callee in out[name]:
                if callee in rest and callee not in shared:
                    shared.add(callee)
                    growing = True
    return shared


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument('file', help='source/SMB/SMB.cpp')
    parser.add_argument('out', help='the file to move them into, e.g. SMBSound.cpp')
    parser.add_argument('--entry', help='the routine the module hangs from')
    parser.add_argument('--kernel', action='store_true', help='the routines the subsystems share')
    parser.add_argument('--apply', action='store_true', help='write the files')
    arguments = parser.parse_args()

    text = blocks(arguments.file)
    names = set(text)
    out = calls(arguments.file, names)
    idom = dominators(out, 'code')

    comes_home = set()
    if arguments.kernel:
        members = kernel(out, idom, names)
        what = KERNEL_BLURB
    else:
        members = owned(out, idom, arguments.entry)
        comes_home = adopted(out, idom, names, members, arguments.entry)
        members = members | comes_home
        what = SUBSYSTEM_BLURB % (arguments.entry, arguments.entry)

    escaping = {c for m in members for c in out[m] if c not in members}
    lines = next(iter(text.values()))[2]

    print('%s: %d routines, %d lines' % (arguments.out, len(members),
                                         sum(text[m][1] - text[m][0] for m in members)))
    if comes_home:
        print('  %d of them only this module and the code above it call, so they come home:'
              % len(comes_home))
        print('    %s' % ', '.join(sorted(comes_home)))
    if escaping:
        print('  calls %d routines it does not own, which stay in %s:'
              % (len(escaping), os.path.basename(arguments.file)))
        print('    %s' % ', '.join(sorted(escaping)))
    else:
        print('  calls nothing outside itself: the file is closed')

    if not arguments.apply:
        return

    taken = sorted(members, key=lambda m: text[m][0])
    moved, cut = [], set()
    for name in taken:
        heading, end, _ = text[name]
        moved.append('\n'.join(lines[heading:end + 1]).strip('\n'))
        cut.update(range(heading, end + 1))

    kept = [line for i, line in enumerate(lines) if i not in cut]
    while kept and not kept[-1].strip():
        kept.pop()
    with open(arguments.file, 'w') as handle:
        handle.write('\n'.join(kept) + '\n')

    path = os.path.join(os.path.dirname(arguments.file), arguments.out)
    with open(path, 'w') as handle:
        handle.write(HEADER % what + '\n' + '\n\n'.join(moved) + '\n')
    print('  wrote %s, took them out of %s' % (path, arguments.file))


if __name__ == '__main__':
    main()
