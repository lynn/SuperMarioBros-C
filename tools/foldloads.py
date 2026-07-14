"""Fold a register load into the statement that reads it, when it has only one.

    python3 tools/foldloads.py source/SMB/SMB.cpp [--dry-run]

A 6502 instruction cannot read memory and act on it in one go, so the disassembly
is full of loads whose only job is to carry a byte to the instruction below them:

    a = M(0xec);         // check for value set here
    if (a != 0x04)
        goto CheckForHammerBro;

In C the read can happen where the value is used, and the load goes away, leaving
its comment behind:

    // check for value set here
    if (M(0xec) != 0x04)
        goto CheckForHammerBro;

The reader is not always a statement with a slot to put the expression in. Where it
is an operation on the register itself, the operation comes to the load instead, which
amounts to the same thing -- the byte is worked on where it is read:

    a = M(Mirror_PPU_CTRL_REG1);   ->   a = M(Mirror_PPU_CTRL_REG1) & 0b01111111;
    a &= 0b01111111;

and a run of shifts by one is one shift by as many, since each of them only feeds the
next: `a = M(f); a >>= 1; a >>= 1; a >>= 1; a >>= 1;` is `a = M(f) >> 4;`. Only the
operations whose result still fits in the register merge this way (see WIDTH_KEEPING).

Whether any of this is safe is a question about the whole file, not about two lines:
it holds only if nothing else reads A afterwards, and the branch means "afterwards"
includes the code at CheckForHammerBro. So the pass builds the control flow graph
of the file and runs reaching definitions over it. Every jump here is written down
-- a branch is an if, a jump table is a switch, and even RTS is a `goto Return`
into a switch that dispatches back to the Return_N label that JSR leaves after each
call -- so the graph is complete, and every read of a register can be traced to the
loads it might be reading.

A load folds into a read when all of the following hold, and stays where it is
otherwise. The refusals are as much the point as the folds:

  * The read is the only one that load reaches. A load that several statements read
    is holding a value, not carrying one, and holding it once under a name reads
    better than reading memory again at every rung of a compare ladder.
  * That read reaches no other load, so the value it sees is this one's whatever
    path it arrived by.
  * The value does not cross a subroutine call in either direction. A register live
    across a JSR is the ROM passing an argument, and one live out of a routine is it
    handing back a result; moving the read into the callee, or out into every caller,
    would hide that. (A return is treated the same way, since a, x and y outlive the
    call to code().)
  * Nothing writes memory between the load and the read -- no writeData, no ++M, no
    pha, no readData, and no subroutine that might do any of them -- because the byte
    at that address may no longer be the one the load fetched.
  * No register in the load's address has been written since, so that M(Enemy_ID + x)
    still means what it did at the load.
  * The read is a plain read of the register: `a &= 0x0f` and `++x` read the register
    into their own new value, and pha reads it without naming it, so there is nowhere
    to put the expression.
  * The read is a value and not an address. Folding into an index would nest one
    memory read inside another, and M(Enemy_ID + M(ObjectOffset)) is not an
    improvement on the two lines it came from.

Loads of a constant are left to whoever wrote them, and to the pass before this one.
This is about the ones that read memory, where the fold moves a read and has to
prove it can.
"""
import argparse
import collections
import re
import sys

from smbcfg import (EXIT, REGS, RE_ASSIGN, RE_INCDEC, RE_LOAD, RE_OPASSIGN,
                    Graph, Source, parse, registers_in)

UNKNOWN = '?'   # a definition from outside the file: the registers survive between calls

# The operations whose result is as wide as the register they are applied to, so that
# doing them where the byte is loaded needs no cast back down. Add, subtract and shift
# left can carry a bit out of the byte, which `a += 1` drops on the way back into a and
# `a = M(f) + 1` would not, so those stay where the ROM put them.
WIDTH_KEEPING = ('&', '|', '^', '>>')

# What a register may hold at a point in the graph.
#
#   loads       the loads whose byte it may still be carrying (UNKNOWN: something else)
#   fresh       no memory has been written since, so a read would see the same byte
#   clobbered   the registers written since, whose value an address may depend on
#   here        it has not been passed to a subroutine, or handed back out of one
State = collections.namedtuple('State', 'loads fresh clobbered here')

TOP = State(frozenset({UNKNOWN}), False, frozenset(REGS), False)


def merge(one, other):
    if one is None or other is None:
        return one or other
    return State(one.loads | other.loads,
                 one.fresh and other.fresh,
                 one.clobbered | other.clobbered,
                 one.here and other.here)


def reaching_loads(graph):
    """For each statement, which load each register may be carrying when it runs."""
    state = {graph.entry: {r: TOP for r in REGS}}
    work = collections.deque([graph.entry])
    while work:
        i = work.popleft()
        statement = graph.statements[i]
        out = dict(state[i])
        if statement.writes_memory:
            out = {r: s._replace(fresh=False) for r, s in out.items()}
        if statement.leaves_routine:
            out = {r: s._replace(here=False) for r, s in out.items()}
        for r in statement.defines:
            for other in REGS:
                if other != r:
                    out[other] = out[other]._replace(clobbered=out[other].clobbered | {r})
            out[r] = (State(frozenset({i}), True, frozenset(), True)
                      if statement.load else TOP)
        for j in graph.successors[i]:
            if j == EXIT:
                continue
            was = state.get(j)
            now = {r: merge(was[r] if was else None, out[r]) for r in REGS}
            if now != was:
                state[j] = now
                work.append(j)
    return state


def read_positions(statement, register):
    """Where the statement reads the register as a value, if it does so plainly."""
    code = statement.code
    if RE_OPASSIGN.match(code) or RE_INCDEC.match(code.rstrip(';')) or code == 'pha();':
        return None                     # reads it into its own new value, or unnamed
    if statement.returns:
        return None
    spans = [m.span() for m in re.finditer(r'(?<!\w)%s(?!\w)' % register, code)]
    assignment = RE_ASSIGN.match(code)
    if assignment and assignment.group(1) == register:
        spans = [s for s in spans if s[0] >= code.index('=')]   # the left side is a write
    return spans or None


def is_value(code, position):
    """Is the offset a value, or is it part of an address (an M/W index, writeData's first)?"""
    calls = []
    for i, character in enumerate(code):
        if i == position:
            break
        if character == '(':
            name = re.search(r'(\w+)$', code[:i])
            calls.append(name.group(1) if name else '')
        elif character == ')':
            calls.pop()
    if any(call in ('M', 'W', 'readData') for call in calls):
        return False
    if calls and calls[-1] == 'writeData':
        opened = code.rindex('writeData(') + len('writeData(')
        depth = 0
        for i in range(opened, position):
            depth += (code[i] == '(') - (code[i] == ')')
            if code[i] == ',' and depth == 0:
                return True             # past the address, so this is the value written
        return False
    return True


def shift_run(graph, reader):
    """The `a >>= 1` statements that carry straight on from this one, shifting further."""
    register, operator, operand = graph.statements[reader].operation
    run, at = [], reader
    while operator in ('>>', '<<') and re.match(r'^(0x[0-9a-f]+|\d+)$', operand):
        following = graph.successors[at][0]
        if graph.successors[at] != [following] or graph.predecessors[following] != [at]:
            break                       # something else can reach it, so it is not just ours
        operation = graph.statements[following].operation
        if not operation or operation[0] != register or operation[1] != operator:
            break
        if not re.match(r'^(0x[0-9a-f]+|\d+)$', operation[2]):
            break
        run.append(following)
        at = following
    return run


def find_folds(src, graph, state):
    """The loads whose one reader can take them, and why the others cannot."""
    readers = collections.defaultdict(set)
    foldable = {}                            # load -> fold
    left = {}                                # load -> why not

    def refuse(load, reason):
        left.setdefault(load, reason)

    for i in sorted(graph.statements):
        if i not in state:
            continue                         # SMB's residual routines: nothing reaches them
        statement = graph.statements[i]
        for register in statement.reads:
            carried = state[i][register]
            for load in carried.loads:
                if load == UNKNOWN:
                    continue
                readers[load].add(i)
                if len(carried.loads) > 1:
                    refuse(load, 'more than one load reaches the read')
                elif not carried.here:
                    refuse(load, 'the value crosses a subroutine call')
                elif not carried.fresh:
                    refuse(load, 'memory is written between the load and the read')
                elif registers_in(graph.statements[load].load) & carried.clobbered:
                    refuse(load, 'the address is indexed by a register written since')
                elif not (src.one_line(load) and src.one_line(i)):
                    refuse(load, 'the load or the read is spread over several lines')
                elif statement.operation and statement.operation[0] == register:
                    # The read is the left operand of a compound assignment, which has
                    # nowhere to put an expression -- but the operation can come to the
                    # load instead: `a = M(f); a &= 3;` is `a = M(f) & 3;`.
                    operator = statement.operation[1]
                    if operator not in WIDTH_KEEPING:
                        refuse(load, 'merging the operation would need a narrowing cast')
                    else:
                        foldable[load] = ('merge', i, shift_run(graph, i))
                else:
                    spans = read_positions(statement, register)
                    if not spans:
                        refuse(load, 'the read is not a plain read of the register')
                    elif not all(is_value(statement.code, at) for at, _ in spans):
                        refuse(load, 'the read is an address, not a value')
                    else:
                        foldable[load] = ('substitute', i, spans)

    folds = {}
    for load, fold in foldable.items():
        if load in left:
            continue
        if len(readers[load]) > 1:
            left[load] = 'more than one statement reads the load'
        else:
            folds[load] = fold
    for load, statement in graph.statements.items():
        if statement.load and load not in folds and load not in left:
            left[load] = 'nothing reads the load'
    return folds, left


def merged(graph, load, reader, run):
    """`a = M(f); a >>= 1; a >>= 1;` said in one line: `a = M(f) >> 2;`."""
    register, operator, operand = graph.statements[reader].operation
    if run:
        shifted = sum(int(graph.statements[at].operation[2], 0) for at in [reader] + run)
        operand = str(shifted)
    return '%s = %s %s %s;' % (register, graph.statements[load].load, operator, operand)


def rewrite(src, graph, folds):
    """Put each load's expression where its reader is, and take the load away."""
    lines = list(src.lines)
    gone = []
    substitutions = collections.defaultdict(list)

    taken = []
    for load, (kind, reader, rest) in folds.items():
        if kind == 'substitute':
            for span in rest:
                substitutions[reader].append((span, graph.statements[load].load))
        else:
            # The shifts of a run are one shift now, and what was written on each of
            # them usually reads as one sentence, so say it on the line that is left.
            said = [src.comment(at)[2:].strip() for at in [reader] + rest if src.comment(at)]
            comment = '// ' + ' '.join(said) if said else ''
            lines[reader] = (src.indent(reader) + merged(graph, load, reader, rest)
                             + (' ' + comment if comment else ''))
            taken.extend(rest)
        gone.append(load)

    for reader, spans in substitutions.items():
        code = graph.statements[reader].code
        for (start, stop), expression in sorted(spans, reverse=True):
            code = code[:start] + expression + code[stop:]
        comment = src.comment(reader)
        lines[reader] = src.indent(reader) + code + (' ' + comment if comment else '')

    for i in gone:
        comment = src.comment(i)         # the line goes, but what it was for stays
        lines[i] = src.indent(i) + comment if comment else None
    for i in taken:
        lines[i] = None                  # its comment is on the line that replaced it

    return [line for line in lines if line is not None]


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument('file', help='the file to rewrite, source/SMB/SMB.cpp')
    parser.add_argument('--dry-run', action='store_true',
                        help='print the diff instead of writing the file')
    arguments = parser.parse_args()

    src = Source(arguments.file)
    items, _ = parse(src, src.start, src.end)
    graph = Graph(src, items)
    state = reaching_loads(graph)
    folds, left = find_folds(src, graph, state)

    substituted = sum(1 for kind, _, _ in folds.values() if kind == 'substitute')
    print('%s: %d loads folded into the statement that reads them (%d into a read, '
          '%d into an operation)' % (arguments.file, len(folds), substituted,
                                     len(folds) - substituted))
    for reason, count in collections.Counter(left.values()).most_common():
        print('  left alone: %-50s %d' % (reason, count))

    lines = rewrite(src, graph, folds)
    if arguments.dry_run:
        sys.stdout.writelines(__import__('difflib').unified_diff(
            [line + '\n' for line in src.lines], [line + '\n' for line in lines],
            arguments.file, arguments.file + ' (rewritten)'))
    elif folds:
        with open(arguments.file, 'w') as handle:
            handle.write('\n'.join(lines))


if __name__ == '__main__':
    main()
