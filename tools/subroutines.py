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
A routine's result comes back as a result: the flag its caller reads after the call is
written on every path to every return, so `bool CheckForCoinMTiles()` says exactly what
the local said, and the caller assigns it. Scratch the routine writes before it reads --
wide, shiftedBit -- it declares for itself, and the one flag that arrives from a caller
comes in as an argument.

    python3 tools/subroutines.py source/SMB/SMB.cpp --apply

cuts them out, rewrites the calls, drops the return-dispatch cases that no longer have
anything to come back to, and declares them all in SMBEngine.hpp.
"""
import argparse
import collections
import os
import re

from smbcfg import RE_ASSIGN, RE_OPASSIGN, Graph, Source, parse

# The locals of code(), which a lifted-out function would no longer be inside.
LOCALS = ('shiftedBit', 'wide', 'borrow', 'carry', 'miscSlotSearched', 'blooberCarry',
          'allEnemySlotsFull', 'bumpedBlockFound', 'climbMTileFound', 'coinMTileFound',
          'collisionFound', 'demoOver', 'enemyRightOfPlayer', 'endGame',
          'enemyYPosInRange', 'hammerSpawned', 'jumpspringFound', 'lrgObjJustStarted',
          'playerVerticalOutOfRange', 'sidePipeShaftDrawn', 'solidMTileFound')


SEPARATOR = '//' + '-' * 72


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


DECLARATION = {'wide': 'uint32_t %s = 0;', 'shiftedBit': 'bool %s = false;'}


class Signature:
    """What a lifted routine has to say to its caller, beyond the registers."""

    def __init__(self, label):
        self.label = label
        self.takes = []       # locals it reads before writing: they come in as arguments
        self.gives = []       # locals its caller reads after the call: they go back as the result
        self.scratch = []     # locals it only ever writes first: it can declare its own

    @property
    def result(self):
        return 'bool' if self.gives else 'void'

    def declaration(self):
        return '%s %s(%s);' % (self.result, self.label,
                               ', '.join('bool ' + name for name in self.takes))

    def definition(self):
        return '%s SMBEngine::%s(%s)' % (self.result, self.label,
                                         ', '.join('bool ' + name for name in self.takes))

    def call(self, indent, comment):
        arguments = ', '.join(self.takes)
        call = '%s(%s);' % (self.label, arguments)
        if self.gives:
            call = '%s = %s' % (self.gives[0], call)
        return indent + call + (' ' + comment if comment else '')

    def locals(self, indent):
        lines = [indent + DECLARATION[name] % name for name in self.scratch]
        lines += [indent + 'bool %s = false;' % name for name in self.gives]
        return lines + ([''] if lines else [])

    def returns(self, indent, comment):
        statement = 'return %s;' % self.gives[0] if self.gives else 'return;'
        return indent + statement + (' ' + comment if comment else '')


def signature(program, label, statements):
    """Which of code()'s locals this routine needs, and which way each of them crosses."""
    sign = Signature(label)
    for name in LOCALS:
        if not any(reads_local(program.graph.statements[i], name)
                   or writes_local(program.graph.statements[i], name) for i in statements):
            continue
        start = program.graph.labels[label]
        arrives = first_touch(program, start, name, within=statements)
        leaves = any(first_touch(program, program.returns_to(call), name)
                     for call in program.callers[label])
        if arrives:
            sign.takes.append(name)   # in and out cannot happen here: see must_write below
        elif leaves:
            sign.gives.append(name)
        else:
            sign.scratch.append(name)
    return sign


def written_on_every_path(program, statements, entry, name):
    """Is the local written before every return, so that returning it says the same thing?

    If it is not, a path exists on which the caller keeps the value it had before the
    call, which a result handed back cannot say and a member variable could.
    """
    written = dict.fromkeys(statements, False)
    for _ in range(len(statements) + 1):
        stable = True
        for i in sorted(statements):
            statement = program.graph.statements[i]
            if writes_local(statement, name):
                now = True
            elif i == entry:
                now = False
            else:
                arriving = [j for j in statements if i in program.successors(j)]
                now = bool(arriving) and all(written[j] for j in arriving)
            if now != written[i]:
                written[i] = now
                stable = False
        if stable:
            break
    return all(written[i] for i in statements if is_rts(program.graph.statements[i]))


def extent(src, program, label, statements):
    """The lines of the file the routine is written on: its label, down to its last brace."""
    first = program.graph.labels[label]
    last = max(src.span.get(i, i) for i in statements)
    while True:                          # a body can end inside a block: take its braces too
        text = '\n'.join(src.code(i) for i in range(first, last + 1))
        if text.count('{') <= text.count('}'):
            break
        last += 1
    heading = first                      # the separator and comments written above the label
    while heading > 0:
        above = src.lines[heading - 1].strip()
        if above and not above.startswith('//'):
            break
        heading -= 1
        if above.startswith('//---'):
            break
    return heading, first, last


def lift(src, program, routines):
    """Cut each routine out of code() and write it as a function of its own."""
    signatures = {label: signature(program, label, body) for label, body in routines.items()}
    for label, sign in signatures.items():
        for name in sign.gives:
            assert written_on_every_path(program, routines[label],
                                         program.graph.labels[label], name), label

    cut = {}                             # line -> the routine whose text starts here
    removed = set()
    for label, body in routines.items():
        heading, first, last = extent(src, program, label, body)
        cut[heading] = (label, first, last)
        removed.update(range(heading, last + 1))

    functions = []
    for heading in sorted(cut):
        label, first, last = cut[heading]
        sign = signatures[label]
        text = list(src.lines[heading:first])
        while text and not text[0].strip():
            text.pop(0)
        if not any(line.startswith('//---') for line in text):
            text = [SEPARATOR, ''] + text        # every routine gets the rule above it
        comment = src.comment(first)     # anything written on the label itself
        if comment:
            text.append(comment)
        text.append(sign.definition())
        text.append('{')
        text.extend(sign.locals('    '))
        for i in range(first + 1, last + 1):
            statement = program.graph.statements.get(i)
            if statement and is_rts(statement):
                text.append(sign.returns(src.indent(i), src.comment(i)))
            else:
                text.append(src.lines[i])
        text.append('}')
        functions.append('\n'.join(text))

    # The calls, and the dispatch switch that no longer has to bring them back.
    lines = list(src.lines)
    dropped = set()
    for i, statement in program.graph.statements.items():
        if statement.jsr and statement.jsr[0] in routines:
            label, index = statement.jsr
            lines[i] = signatures[label].call(src.indent(i), src.comment(i))
            dropped.add(index)
    for i, line in enumerate(lines):
        match = re.match(r'^\s*case (\d+):$', src.code(i))
        if match and int(match.group(1)) in dropped and 'goto Return_' in src.code(i + 1):
            lines[i] = lines[i + 1] = None

    kept = [line for i, line in enumerate(lines)
            if i not in removed and line is not None]
    while kept and not kept[-1].strip():
        kept.pop()                       # code() ends here; the routines follow it

    text = '\n'.join(kept) + '\n\n' + '\n\n'.join(functions) + '\n'
    return text, signatures


def declare(header, signatures):
    """Give the lifted routines their declarations, next to the code() they came out of."""
    lines = open(header).read().split('\n')
    at = lines.index('    void code(int mode);') + 1
    block = ['',
             '    /**',
             '     * The routines of the game that are routines in C too: a call is the only',
             '     * way in and a return the only way out. The rest of them are still labels',
             '     * inside code(), which is where these were lifted from.',
             '     *',
             '     * See SMB.cpp for implementations.',
             '     */']
    block += ['    ' + signatures[label].declaration() for label in sorted(signatures)]
    return '\n'.join(lines[:at] + block + lines[at:])


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument('file', help='the file to read, source/SMB/SMB.cpp')
    parser.add_argument('--verbose', action='store_true',
                        help='list every routine found, not just the count per wave')
    parser.add_argument('--apply', action='store_true',
                        help='lift the routines out, rewriting the file and the header')
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

    if not arguments.apply:
        return
    routines = {label: body for wave in waves for label, body in wave.items()}
    text, signatures = lift(src, program, routines)
    header = os.path.join(os.path.dirname(arguments.file), 'SMBEngine.hpp')
    declarations = declare(header, signatures)
    with open(arguments.file, 'w') as handle:
        handle.write(text)
    with open(header, 'w') as handle:
        handle.write(declarations)
    print('\n  lifted %d routines out of %s, declared them in %s'
          % (len(routines), arguments.file, header))


if __name__ == '__main__':
    main()
