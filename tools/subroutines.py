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
  * Nothing else gets out: every exit is an RTS, or a tail call. A body that ends by
    jumping into the middle of something else shares a tail with it, and moving it
    would break the jump.
  * It does not `return`, which leaves code() rather than the routine, and would mean
    something quite different once the routine is a function of its own.
  * Every call it makes is to a routine that is itself a function. A JSR in the body
    is a `goto` out of the body and the RTS at the far end comes back through the
    dispatch switch, so a routine cannot be lifted out while it still calls one that
    has not been. Leaves come out first, and their callers become leaves in turn, so
    this is run to a fixed point and reports the waves it finds.

A routine that ends `goto Sub;`, where Sub only ever returns, is not sharing a tail with
Sub: it is calling it. Sub runs, reaches its RTS, and the dispatch switch hands control to
whoever called the routine -- never back to the jump. That is what `Sub(); goto Return;`
says, in two statements instead of one jump that hides both, and the two are the same
instructions in the same order. So a bare `goto Sub;` is read here as a call, and it counts
as one from both ends: the routine that does it can stop there, and Sub can be a function
even when no JSR ever names it. DrawSpriteObject is only ever reached this way. The ROM has
been tail-calling all along -- a JMP to a subroutine is a call whose return the callee does
-- and this only writes it down.

A goto that is the whole body of an if is a branch instruction, BNE or BCC, and not a call:
the routine is picking its way through itself, and the label it lands on is a part of it.
Only a bare `goto Sub;`, which is a JMP, is a tail call. See smbcfg.Graph.guarded.

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


def dispatch(label):
    """The return-dispatch switch, and the labels the calls leave behind for it to land on.

    Jumping to one of these is how the file returns, not how it calls, and there is no
    routine there to lift: `Return` is the switch itself and every `Return_n` is the far
    side of a JSR. See smbcfg for the shape of it.
    """
    return label == 'Return' or label.startswith('Return_')


class Program:
    """The file's routines, and what stands between each of them and being a function."""

    def __init__(self, src):
        self.src = src
        self.graph = Graph(src, parse(src, src.start, src.end)[0])
        self.jsrs = collections.defaultdict(list)     # routine -> the JSRs that call it
        self.jmps = collections.defaultdict(list)     # routine -> the bare gotos into it
        for i, statement in self.graph.statements.items():
            if statement.jsr:
                self.jsrs[statement.jsr[0]].append(i)
            elif statement.goto and not dispatch(statement.goto) and i not in self.graph.guarded:
                self.jmps[statement.goto].append(i)   # a JMP, which may turn out to be a call
        self.called = {statement.jsr[1]: statement.jsr[0]
                       for statement in self.graph.statements.values() if statement.jsr}
        self.targets = [label for label in self.graph.labels
                        if not dispatch(label) and (self.jsrs[label] or self.jmps[label])]
        self.lifted = {}                      # routine -> its statements, once it is a function
        self.signatures = {}                  # routine -> what it says to its caller
        self.reached = {}                     # label -> what a call to it runs, cached

    def returns_to(self, call):
        """Where a call comes back to: the label JSR leaves after itself."""
        return self.graph.labels['Return_%d' % self.graph.statements[call].jsr[1]]

    def tail_call(self, i):
        """Is this jump a call: a bare `goto Sub;` into a routine that only ever returns?

        Sub returns to whoever called the routine doing the jumping, not to the jump, so
        the jump is a call and a return -- but only once Sub is a function, and only from
        outside Sub, where a jump to its own label is a loop and not a call.
        """
        statement = self.graph.statements[i]
        return (bool(statement.goto) and statement.goto in self.lifted
                and i in self.jmps[statement.goto] and i not in self.lifted[statement.goto])

    def exits(self, i):
        """Does control leave the routine here -- by returning, or by tail calling?"""
        return is_rts(self.graph.statements[i]) or self.tail_call(i)

    def successors(self, i):
        """Where control goes, once the routines already lifted out are calls and returns."""
        statement = self.graph.statements[i]
        if self.exits(i):
            return []                         # a return, which leaves the routine
        if statement.jsr and statement.jsr[0] in self.lifted:
            return [self.returns_to(i)]       # a call, which comes back
        if statement.goto and statement.goto.startswith('Return_'):
            # A case of the dispatch switch. Once the call it comes back from is a call,
            # its index is never pushed and the case goes: this edge is not there any more.
            index = int(statement.goto[len('Return_'):])
            if self.called.get(index) in self.lifted:
                return []
        return self.graph.successors[i]

    def entrances(self):
        """Every edge in the file, as it now stands, so that a body can be checked for leaks."""
        into = collections.defaultdict(set)
        for i in self.graph.statements:
            for j in self.successors(i):
                if j != -1:
                    into[j].add(i)
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

    def reach(self, label):
        """Everything a call to this label runs before control comes back to the caller.

        Unlike body(), this asks nothing and refuses nothing: every JSR is a call that
        comes back, lifted or not, and a jump is followed wherever it goes. It is how a
        statement is placed -- which routines were running when it ran -- rather than a
        judgement about whether the label is one.
        """
        if label not in self.reached:
            start = self.graph.labels[label]
            seen, edge = {start}, [start]
            while edge:
                i = edge.pop()
                statement = self.graph.statements[i]
                if is_rts(statement):
                    continue                  # returns to the caller, wherever that is
                following = ([self.returns_to(i)] if statement.jsr
                             else self.graph.successors[i])
                for j in following:
                    if j != -1 and j not in seen:
                        seen.add(j)
                        edge.append(j)
            self.reached[label] = seen
        return self.reached[label]

    def calls(self, label, statements):
        """Every call to this label: the JSRs, and the jumps that turn out to be tail calls."""
        return set(self.jsrs[label]) | {i for i in self.jmps[label] if i not in statements}

    def resumes(self, label, seen=None):
        """Where control ends up once this routine has returned.

        A JSR comes back to the label it left behind, and that is the whole answer for a
        routine the ROM calls. A tail call does not come back: the routine jumped to returns
        to whoever called the routine that jumped, so its resume points are its caller's, and
        this walks back up the jumps until it reaches calls that do come back. Which routine
        a jump was made from is what reach() answers, and a jump inside a loop can reach
        itself, so `seen` stops the walk going round.

        signature() needs this to see what a caller reads after the call. Get it wrong for a
        tail-called routine and a local it writes looks like scratch, and the write is lost.
        """
        seen = set() if seen is None else seen
        if label in seen:
            return set()
        seen.add(label)
        points = {self.returns_to(call) for call in self.jsrs[label]}
        for i in self.jmps[label]:
            if i in self.reach(label):
                continue                      # a jump back into the routine, not a call to it
            for caller in self.targets:
                if i in self.reach(caller):
                    points |= self.resumes(caller, seen)
        return points

    def routine(self, label):
        """Can this label be lifted out as a function, and if not, what stops it?"""
        if label == 'UpdateScreen':
            return None, 'it is UpdateScreen'
        if not self.jsrs[label] and not self.jmps[label]:
            return None, 'nothing calls it'
        statements, why = self.body(label)
        if why:
            return None, why
        calls = self.calls(label, statements)
        if not calls:
            return None, 'nothing calls it'   # every jump into it was one of its own

        start = self.graph.labels[label]
        into = self.entrances()
        for i in statements:
            outside = into[i] - statements
            if i == start:
                outside -= calls              # the calls are how a routine is meant to be entered
            if outside:
                if i == start and any(self.graph.statements[j].goto for j in outside):
                    return None, 'it is branched to as well as called'
                if i == start:
                    return None, 'the code above falls into it'
                return None, 'something outside jumps into the middle of it'

        if not any(self.exits(i) for i in statements):
            return None, 'it never returns'
        if all(i == start or is_rts(self.graph.statements[i]) for i in statements):
            # A jump table case for a thing with nothing to do -- an enemy with no init code --
            # is a label on an RTS. There is no routine there to lift, only a way of returning,
            # and a function of it would be an empty one that its caller gains nothing by calling.
            return None, 'it is nothing but a return'
        strays = [i for i in self.graph.statements
                  if start < i <= max(self.src.span.get(j, j) for j in statements)
                  and i not in statements]
        if strays:
            return None, 'its text is not in one piece'
        return statements, None

    def sweep(self):
        """Lift out every routine that can be, then look again: its callers may now be able to."""
        waves = []
        left = {}
        while True:
            found = {}
            for label in self.graph.labels:
                if dispatch(label) or label in self.lifted:
                    continue
                statements, why = self.routine(label)
                if statements:
                    found[label] = statements
                else:
                    left[label] = why
            # A routine found in the same wave as one it tail calls has swallowed it: its body
            # was walked before the jump was a call, so it ran on into the routine it jumps to
            # and took the whole of it in. Leave it for the next wave, where the jump is a call
            # and the body stops at it. This is the fixed point doing its job, one turn late.
            starts = {self.graph.labels[label]: label for label in found}
            found = {label: body for label, body in found.items()
                     if not any(start in body and inner != label
                                for start, inner in starts.items())}
            if not found:
                return waves, left
            # A routine's signature is settled the moment it is one, and everything found after
            # it needs it: a caller now has a call in its body that reads the arguments and
            # writes the result, and neither is written anywhere in the JSR or the jump it was.
            for label in found:
                self.signatures[label] = signature(self, label, found[label])
                self.lifted[label] = found[label]
                left.pop(label, None)
            waves.append(found)


def crossing(program, i):
    """The signature of the routine this statement calls, if it calls one that is a function.

    A JSR says nothing about the locals and neither does a jump, but the call each of them
    is about to be rewritten into says everything: it passes the arguments and it assigns
    the result. Read the statement as the call it is going to be, or a routine that gets its
    answer by calling another will look as though it never wrote one.
    """
    statement = program.graph.statements[i]
    if statement.jsr and statement.jsr[0] in program.signatures:
        return program.signatures[statement.jsr[0]]
    if program.tail_call(i):
        return program.signatures[statement.goto]
    return None


def writes_local(program, i, name):
    sign = crossing(program, i)
    if sign:
        return name in sign.gives        # the result the call assigns
    code = program.graph.statements[i].code
    match = RE_ASSIGN.match(code) or RE_OPASSIGN.match(code)
    return bool(match) and match.group(1) == name


def reads_local(program, i, name):
    sign = crossing(program, i)
    if sign:
        return name in sign.takes        # the argument the call passes
    code = program.graph.statements[i].code
    if not re.search(r'(?<!\w)%s(?!\w)' % name, code):
        return False
    match = RE_ASSIGN.match(code)
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
        if reads_local(program, i, name):
            return True
        if writes_local(program, i, name) or program.exits(i):
            continue                     # written before it was read, or the caller is done
        for j in (program.successors(i) if within else program.graph.successors[i]):
            if j == -1 or (within is not None and j not in within):
                continue
            if j not in seen:
                seen.add(j)
                edge.append(j)
    return False


# The locals are all flags but `wide`, including the results a routine now calls another
# routine for and keeps to itself.
DECLARATION = collections.defaultdict(lambda: 'bool %s = false;', wide='uint32_t %s = 0;')


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
    start = program.graph.labels[label]
    resumes = program.resumes(label)
    for name in LOCALS:
        if not any(reads_local(program, i, name) or writes_local(program, i, name)
                   for i in statements):
            continue
        arrives = first_touch(program, start, name, within=statements)
        leaves = any(first_touch(program, point, name) for point in resumes)
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
            if writes_local(program, i, name):
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
    return all(written[i] for i in statements if program.exits(i))


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
    signatures = program.signatures
    for label, sign in signatures.items():
        for name in sign.gives:
            assert written_on_every_path(program, routines[label],
                                         program.graph.labels[label], name), label
    home = {i: label for label, body in routines.items() for i in body}

    # The calls become calls, and the dispatch switch loses the cases that were only
    # there to come back from them. A routine that calls another routine is lifted with
    # the call already rewritten, so this is done before anything is cut out.
    lines = list(src.lines)
    dropped = set()
    for i, statement in program.graph.statements.items():
        if statement.jsr and statement.jsr[0] in routines:
            label, index = statement.jsr
            lines[i] = signatures[label].call(src.indent(i), src.comment(i))
            dropped.add(index)
        elif program.tail_call(i):
            # A jump that is a call: say the call, and then say the return it was doing
            # silently. Inside a routine that is a return; in code() it is still an RTS,
            # and the index it pops is the one the jump was going to leave it to pop.
            inside = signatures.get(home.get(i))
            back = (inside.returns(src.indent(i), '') if inside
                    else src.indent(i) + 'goto Return;')
            lines[i] = '%s\n%s' % (signatures[statement.goto].call(src.indent(i),
                                                                   src.comment(i)), back)
    for i, line in enumerate(lines):
        match = re.match(r'^\s*case (\d+):$', src.code(i))
        if match and int(match.group(1)) in dropped and 'goto Return_' in src.code(i + 1):
            lines[i] = lines[i + 1] = None

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
        text = [line for line in lines[heading:first] if line is not None]
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
            elif lines[i] is not None:
                text.append(lines[i])
        text.append('}')
        functions.append('\n'.join(text))

    kept = [line for i, line in enumerate(lines)
            if i not in removed and line is not None]
    while kept and not kept[-1].strip():
        kept.pop()                       # code() ends here; the routines follow it

    text = '\n'.join(kept) + '\n\n' + '\n\n'.join(functions) + '\n'
    return text, signatures


HEADING = '     * The routines of the game that are routines in C too: a call is the only'


def declare(header, signatures):
    """Give the lifted routines their declarations, next to the code() they came out of.

    A run of this only ever sees the labels still inside code(), so the routines lifted by
    the runs before it are not in `signatures` -- they are functions already, and the header
    is where their declarations are. Take those back out, add the new ones, and write the one
    list. Appending a block per run, as this used to, leaves a header of identical blocks.
    """
    lines = open(header).read().split('\n')
    declarations = ['    ' + signatures[label].declaration() for label in signatures]
    while HEADING in lines:
        heading = lines.index(HEADING)
        at = heading - 1                                   # the /** the heading is written in
        if at and not lines[at - 1].strip():
            at -= 1                                        # and the blank line above that
        end = next(i for i in range(heading, len(lines)) if not lines[i].strip())
        declarations += [line for line in lines[heading:end] if not line.startswith('     *')]
        lines[at:end] = []                                 # down to the blank after the last one

    at = lines.index('    void code(int mode);') + 1
    block = ['',
             '    /**',
             HEADING,
             '     * way in and a return the only way out. The rest of them are still labels',
             '     * inside code(), which is where these were lifted from. A call may be a',
             '     * JSR, or a JMP that was a tail call all along.',
             '     *',
             '     * See SMB.cpp for implementations.',
             '     */']
    block += sorted(declarations, key=lambda line: line.split()[1])
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
    print('%s: %d of the %d labels that are called can be lifted out as functions'
          % (arguments.file, total, len(program.targets)))

    for number, wave in enumerate(waves, 1):
        lines = sum(len(wave[label]) for label in wave)
        print('\n  wave %d: %d routines, %d statements'
              % (number, len(wave), lines))
        if not arguments.verbose:
            continue
        for label in sorted(wave, key=lambda l: -len(wave[l])):
            statements = wave[label]
            sign = program.signatures[label]
            calls = program.calls(label, statements)
            tails = sum(1 for i in calls if i in program.jmps[label])
            needs = (['%s (from its caller)' % name for name in sign.takes]
                     + ['%s (back to its caller)' % name for name in sign.gives]
                     + ['%s (scratch)' % name for name in sign.scratch])
            print('    %-26s %4d statements  %2d call%s %-12s %s'
                  % (label, len(statements), len(calls),
                     ' ' if len(calls) == 1 else 's',
                     '(%d a tail call)' % tails if tails else '',
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
