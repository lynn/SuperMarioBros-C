"""Find the labels that are already subroutines, and could be said as functions.

    python3 tools/subroutines.py source/SMB/SMB.cpp [--verbose]

SMB.cpp is one function of 16,000 lines because that is what the disassembly is: the
ROM's subroutines are labels in it, a call is JSR(Sub, n) -- `goto Sub`, with a label
left behind to come back to -- and RTS is `goto Return`, a jump into a switch that
dispatches on the index the call pushed. The subroutines are all there; C just cannot
see them.

Some of them can be lifted out as real functions, and this says which. A label is a
function when the graph says control can only get in by calling it and can only get
out by returning:

  * Something calls it. A label nothing calls is not a subroutine, it is a place.
  * Nothing else gets in: every way into every statement of the body comes from inside
    the body, or is one of the calls. A label that is also branched to, or that the
    code above simply falls into -- which is how most of this file is written -- is a
    piece of a routine and not a routine.
  * Nothing else gets out: every exit is an RTS. A body that ends by jumping to a label
    outside it shares a tail with something, and moving it would break the jump.
  * It does not `return`, which leaves code() rather than the routine, and would mean
    something quite different once the routine is a function of its own.
  * Every call it makes is to a routine that is itself a function. A JSR in the body
    is a `goto` out of the body and the RTS at the far end comes back through the
    dispatch switch, so a routine cannot be lifted out while it still calls one that
    has not been. Leaves come out first, and their callers become leaves in turn, so
    this is run to a fixed point and reports the waves it finds.

The registers are members of SMBEngine and need no passing. The locals of code() do:
`wide` and `carry` and the named results the carry-flag rewrites left behind live in
the function being cut up, so a body that uses one needs it either declared inside the
new function (if nothing else uses it) or promoted to a member (if something does).
Each routine is reported with what it would need.

This only reports. Cutting the file up is the rewrite that follows it.
"""
import argparse
import collections
import re

from smbcfg import RE_ASSIGN, RE_OPASSIGN, Graph, Source, parse

# The locals of code(), which a lifted-out function would no longer be inside.
LOCALS = ('shiftedBit', 'wide', 'borrow', 'carry', 'miscSlotSearched', 'blooberCarry',
          'allEnemySlotsFull', 'bumpedBlockFound', 'climbMTileFound', 'coinMTileFound',
          'collisionFound', 'demoOver', 'enemyRightOfPlayer', 'endGame',
          'enemyYPosInRange', 'hammerSpawned', 'jumpspringFound', 'lrgObjJustStarted',
          'playerVerticalOutOfRange', 'sidePipeShaftDrawn', 'solidMTileFound')


def is_rts(statement):
    return statement.goto == 'Return'


class Program:
    """The file's routines, and what stands between each of them and being a function."""

    def __init__(self, src):
        self.src = src
        self.graph = Graph(src, parse(src, src.start, src.end)[0])
        self.callers = collections.defaultdict(list)
        for i, statement in self.graph.statements.items():
            if statement.jsr:
                self.callers[statement.jsr[0]].append(i)
        self.lifted = {}                      # routine -> its statements, once it is a function

    def returns_to(self, call):
        """Where a call comes back to: the label JSR leaves after itself."""
        return self.graph.labels['Return_%d' % self.graph.statements[call].jsr[1]]

    def successors(self, i):
        """Where control goes, once the routines already lifted out are calls and returns."""
        statement = self.graph.statements[i]
        if is_rts(statement):
            return []                         # a return, which leaves the routine
        if statement.jsr and statement.jsr[0] in self.lifted:
            return [self.returns_to(i)]       # a call, which comes back
        return self.graph.successors[i]

    def entrances(self):
        """Every edge in the file, as it now stands, so that a body can be checked for leaks."""
        into = collections.defaultdict(set)
        for i in self.graph.statements:
            for j in self.successors(i):
                if j != -1:
                    into[j].add(i)
        # The dispatch switch only comes back to the calls that are still JSR macros.
        dispatch = next(i for i, s in self.graph.statements.items()
                        if s.code.startswith('switch (popReturnIndex'))
        for j in list(into):
            if dispatch in into[j]:
                into[j].discard(dispatch)
        for routine, calls in self.callers.items():
            if routine not in self.lifted:
                for call in calls:
                    into[self.returns_to(call)].add(dispatch)
        return into

    def body(self, label):
        """The statements a call to this label can reach, or why it is not a routine."""
        start = self.graph.labels[label]
        seen, edge = {start}, [start]
        while edge:
            i = edge.pop()
            statement = self.graph.statements[i]
            if statement.returns:
                return None, 'it returns from code(), not from a routine'
            if statement.jsr and statement.jsr[0] not in self.lifted:
                return None, 'it calls %s, which is not a function yet' % statement.jsr[0]
            for j in self.successors(i):
                if j == -1:
                    return None, 'it runs off the end of the file'
                if j not in seen:
                    seen.add(j)
                    edge.append(j)
        return seen, None

    def routine(self, label):
        """Can this label be lifted out as a function, and if not, what stops it?"""
        calls = self.callers.get(label)
        if not calls:
            return None, 'nothing calls it'
        statements, why = self.body(label)
        if why:
            return None, why

        start = self.graph.labels[label]
        into = self.entrances()
        for i in statements:
            outside = into[i] - statements
            if i == start:
                outside -= set(calls)         # the calls are how a routine is meant to be entered
            if outside:
                if i == start and any(self.graph.statements[j].goto for j in outside):
                    return None, 'it is branched to as well as called'
                if i == start:
                    return None, 'the code above falls into it'
                return None, 'something outside jumps into the middle of it'

        if not any(is_rts(self.graph.statements[i]) for i in statements):
            return None, 'it never returns'
        return statements, None

    def sweep(self):
        """Lift out every routine that can be, then look again: its callers may now be able to."""
        waves = []
        left = {}
        while True:
            found = {}
            for label in self.graph.labels:
                if label.startswith('Return_') or label in self.lifted:
                    continue
                statements, why = self.routine(label)
                if statements:
                    found[label] = statements
                else:
                    left[label] = why
            if not found:
                return waves, left
            self.lifted.update(found)
            waves.append(found)
            for label in found:
                left.pop(label, None)


def writes_local(statement, name):
    match = RE_ASSIGN.match(statement.code) or RE_OPASSIGN.match(statement.code)
    return bool(match) and match.group(1) == name


def reads_local(statement, name):
    if not re.search(r'(?<!\w)%s(?!\w)' % name, statement.code):
        return False
    match = RE_ASSIGN.match(statement.code)
    if match and match.group(1) == name:
        return bool(re.search(r'(?<!\w)%s(?!\w)' % name, match.group(2)))
    return True                          # a condition, an argument, or `wide += ...`


def first_touch(program, start, name, within=None):
    """Walking on from here, is the local read before anything writes it?

    The walk stops where the code it is walking returns. Following an RTS means going
    through the dispatch switch, which fans out to every call site in the file and so
    would find a reader of `wide` no matter where it started from.
    """
    seen, edge = {start}, [start]
    while edge:
        i = edge.pop()
        statement = program.graph.statements[i]
        if reads_local(statement, name):
            return True
        if writes_local(statement, name) or is_rts(statement):
            continue                     # written before it was read, or the caller is done
        for j in (program.successors(i) if within else program.graph.successors[i]):
            if j == -1 or (within is not None and j not in within):
                continue
            if j not in seen:
                seen.add(j)
                edge.append(j)
    return False


def locals_used(program, label, statements):
    """The locals of code() this routine touches, and how the value gets in or out."""
    needs = []
    for name in LOCALS:
        if not any(reads_local(program.graph.statements[i], name)
                   or writes_local(program.graph.statements[i], name) for i in statements):
            continue
        start = program.graph.labels[label]
        arrives = first_touch(program, start, name, within=statements)
        leaves = any(first_touch(program, program.returns_to(call), name)
                     for call in program.callers[label])
        if arrives and leaves:
            needs.append('%s (in and out)' % name)
        elif arrives:
            needs.append('%s (from its caller)' % name)
        elif leaves:
            needs.append('%s (back to its caller)' % name)
        else:
            needs.append('%s (scratch)' % name)
    return needs


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument('file', help='the file to read, source/SMB/SMB.cpp')
    parser.add_argument('--verbose', action='store_true',
                        help='list every routine found, not just the count per wave')
    arguments = parser.parse_args()

    src = Source(arguments.file)
    program = Program(src)
    waves, left = program.sweep()

    total = sum(len(wave) for wave in waves)
    called = sum(1 for label in program.graph.labels
                 if not label.startswith('Return_') and program.callers.get(label))
    print('%s: %d of the %d labels that are called can be lifted out as functions'
          % (arguments.file, total, called))

    for number, wave in enumerate(waves, 1):
        lines = sum(len(wave[label]) for label in wave)
        print('\n  wave %d: %d routines, %d statements'
              % (number, len(wave), lines))
        if not arguments.verbose:
            continue
        for label in sorted(wave, key=lambda l: -len(wave[l])):
            statements = wave[label]
            needs = locals_used(program, label, statements)
            print('    %-28s %4d statements  %2d call%s  %s'
                  % (label, len(statements), len(program.callers[label]),
                     ' ' if len(program.callers[label]) == 1 else 's',
                     'needs ' + ', '.join(needs) if needs else ''))

    print('\n  the rest, and what stands in the way:')
    for why, count in collections.Counter(left.values()).most_common(12):
        print('    %-52s %d' % (why[:52], count))


if __name__ == '__main__':
    main()
