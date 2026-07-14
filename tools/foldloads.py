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

Whether that is safe is a question about the whole file, not about the two lines:
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

REGS = ('a', 'x', 'y')

UNKNOWN = '?'   # a definition from outside the file: the registers survive between calls

RE_LABEL = re.compile(r'^([A-Za-z_]\w*):$')
RE_CASE = re.compile(r'^case (\w+):$')
RE_ASSIGN = re.compile(r'^(\w+) = (.*);$')
RE_OPASSIGN = re.compile(r'^(\w+) (\+|-|&|\||\^|>>|<<)= (.*);$')
RE_INCDEC = re.compile(r'^(\+\+|--)([axy])$')
RE_GOTO = re.compile(r'^goto (\w+);$')
RE_JSR = re.compile(r'^JSR\((\w+), (\d+)\);$')
RE_LOAD = re.compile(r'^M\([^()]*(\([^()]*\))?[^()]*\)$')   # M(Foo), M(Foo + x), M(W(0x00) + y)
RE_RTS = re.compile(r'^switch \(popReturnIndex')

# Everything that can change what a memory read would see. A JSR is not here: the
# graph walks into the subroutine, so whatever it writes is seen on the way through.
WRITES_MEMORY = re.compile(r'\bwriteData\(|\+\+M\(|--M\(|\bpha\(\)|\breadData\(|\bloadConstantData\(')

EXIT = -1


def registers_in(expression):
    return {r for r in REGS if re.search(r'(?<!\w)%s(?!\w)' % r, expression)}


class Statement:
    """One statement of the file, and what it does to the registers and to memory."""

    def __init__(self, line, code):
        self.line = line
        self.code = code
        self.defines = set()
        self.reads = set()
        self.load = None        # the M(...) expression this line loads into a register
        self.goto = None
        self.jsr = None
        self.returns = False
        self.writes_memory = bool(WRITES_MEMORY.search(code))
        self.leaves_routine = bool(RE_RTS.match(code))
        self.classify()

    def classify(self):
        code = self.code
        match = RE_GOTO.match(code)
        if match:
            self.goto = match.group(1)
            return
        match = RE_JSR.match(code)
        if match:
            self.jsr = (match.group(1), int(match.group(2)))
            self.leaves_routine = True
            return
        if code == 'return;':
            self.returns = True
            self.reads = set(REGS)  # what is in the registers outlives the call to code()
            return
        if code == 'pha();':
            self.reads = {'a'}      # read without naming it: nowhere to put an expression
            return
        if code == 'pla();':
            self.defines = {'a'}
            return
        match = RE_INCDEC.match(code.rstrip(';'))
        if match:
            self.defines = self.reads = {match.group(2)}
            return
        match = RE_OPASSIGN.match(code)
        if match:
            target = match.group(1)
            self.reads = registers_in(match.group(3))
            if target in REGS:
                self.defines = {target}
                self.reads |= {target}   # a &= x reads a into its own new value
            return
        match = RE_ASSIGN.match(code)
        if match:
            target, value = match.group(1), match.group(2)
            self.reads = registers_in(value)
            if target in REGS:
                self.defines = {target}
                if target not in self.reads and not self.writes_memory and RE_LOAD.match(value):
                    self.load = value
            return
        self.reads = registers_in(code)   # a condition, or anything else that only reads


class Source:
    """The lines of SMB.cpp, and the statements they spell."""

    def __init__(self, path):
        self.path = path
        self.lines = open(path).read().split('\n')
        self.span = {}      # first line of a statement -> its last line
        self.text = {}      # first line of a statement -> its code, joined up
        self.start = next(i for i, l in enumerate(self.lines)
                          if l.startswith('void SMBEngine::code')) + 2
        self.end = max(i for i, l in enumerate(self.lines) if l == '}')

    def code(self, i):
        return re.sub(r'/\*.*?\*/', '', re.sub(r'//.*$', '', self.lines[i])).rstrip()

    def comment(self, i):
        match = re.search(r'//.*$', self.lines[i])
        return match.group(0) if match else ''

    def indent(self, i):
        return re.match(r'\s*', self.lines[i]).group(0)

    def blank(self, i):
        return self.code(i).strip() == ''

    def skip_blank(self, i, end=None):
        end = self.end if end is None else end
        while i < end and self.blank(i):
            i += 1
        return i

    def join(self, i, semicolon):
        """A statement or a condition may run over several lines. Read it as one."""
        text = self.code(i).strip()
        last = i
        while last + 1 < self.end and (text.count('(') != text.count(')')
                                       or (semicolon and not text.endswith(';'))):
            last += 1
            text += ' ' + self.code(last).strip()
        self.span[i] = last
        self.text[i] = text
        return text, last

    def one_line(self, i):
        return self.span.get(i, i) == i


def parse(src, i, end, single=False):
    """Parse a block into ('stmt' | 'label' | 'if' | 'do' | 'switch') items."""
    items = []
    while i < end:
        if single and items:
            return items, i
        code = src.code(i).strip()
        if code == '':
            i += 1
        elif code == '{':
            i += 1
        elif code == '}' or code.startswith(('} while', '} else')):
            return items, i
        elif RE_CASE.match(code):
            return items, i                      # the switch parser wants to see this
        elif RE_LABEL.match(code):
            items.append(('label', i, RE_LABEL.match(code).group(1)))
            i += 1
        elif code.startswith('if ('):
            condition = i
            _, i = src.join(i, semicolon=False)
            consequent, i = parse_body(src, i + 1, end)
            alternative = []
            j = src.skip_blank(i, end) if i < end else i
            if j < end and src.code(j).strip() in ('else', '} else'):
                alternative, i = parse_body(src, j + 1, end)
            items.append(('if', condition, consequent, alternative))
        elif code == 'do':
            body, i = parse_body(src, i + 1, end)
            assert src.code(i).strip().startswith('} while'), (i, src.lines[i])
            items.append(('do', body, i))
            _, i = src.join(i, semicolon=True)
            i += 1
        elif code.startswith('switch ('):
            head = i
            _, i = src.join(i, semicolon=False)
            i = src.skip_blank(i + 1, end)
            assert src.code(i).strip() == '{', (i, src.lines[i])
            i += 1
            cases = []
            while True:
                i = src.skip_blank(i, end)
                if src.code(i).strip() == '}':
                    i += 1
                    break
                assert RE_CASE.match(src.code(i).strip()), (i, src.lines[i])
                i += 1                            # several cases may share one body
                while RE_CASE.match(src.code(src.skip_blank(i, end)).strip()):
                    i = src.skip_blank(i, end) + 1
                body, i = parse(src, i, end)
                cases.append(body)
            items.append(('switch', head, cases))
        else:
            text, last = src.join(i, semicolon=True)
            assert text.endswith(';'), (i, src.lines[i])
            items.append(('stmt', i))
            i = last + 1
    return items, i


def parse_body(src, i, end):
    """The body of an if, an else or a do: a braced block, or a single statement."""
    i = src.skip_blank(i, end)
    if src.code(i).strip() == '{':
        items, i = parse(src, i + 1, end)
        if src.code(i).strip() == '}':
            i += 1
        return items, i
    return parse(src, i, end, single=True)


class Graph:
    """The control flow graph of the file. Every edge in it is written down somewhere."""

    def __init__(self, src, items):
        self.src = src
        self.statements = {}
        self.successors = {}
        self.labels = {}
        self.entry = self.wire(items, EXIT)
        self.resolve_jumps()

    def node(self, i):
        if i not in self.statements:
            self.statements[i] = Statement(i, self.src.text.get(i, self.src.code(i).strip()))
        return i

    def wire(self, items, following):
        """Wire items back to front; `following` is where the block falls through to."""
        entry = following
        for item in reversed(items):
            follow = entry
            if item[0] == 'label':
                entry = self.node(item[1])
                self.labels[item[2]] = entry
                self.successors[entry] = [follow]
            elif item[0] == 'stmt':
                entry = self.node(item[1])
                self.successors[entry] = [follow]     # jumps are patched in resolve_jumps
            elif item[0] == 'if':
                entry = self.node(item[1])
                consequent = self.wire(item[2], follow)
                alternative = self.wire(item[3], follow) if item[3] else follow
                self.successors[entry] = [consequent, alternative]
            elif item[0] == 'do':
                condition = self.node(item[2])
                body = self.wire(item[1], condition)
                self.successors[condition] = [body, follow]
                entry = body
            elif item[0] == 'switch':
                entry = self.node(item[1])
                bodies = [self.wire(body, follow) for body in item[2]]
                # no default: a value no case matches falls out of the switch
                self.successors[entry] = bodies + [follow]
        return entry

    def resolve_jumps(self):
        # JSR(Sub, n) expands to `pushReturnIndex(n); goto Sub; Return_n:`, so the
        # statement after the call is a label, and RTS is what jumps to it.
        for i, statement in self.statements.items():
            if statement.jsr:
                self.labels['Return_%d' % statement.jsr[1]] = self.successors[i][0]
        for i, statement in self.statements.items():
            if statement.goto:
                self.successors[i] = [self.labels[statement.goto]]
            elif statement.jsr:
                self.successors[i] = [self.labels[statement.jsr[0]]]
            elif statement.returns:
                self.successors[i] = [EXIT]


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


def find_folds(src, graph, state):
    """The loads whose one reader can take them, and why the others cannot."""
    readers = collections.defaultdict(set)
    foldable = {}                            # load -> (reader, [spans])
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
                else:
                    spans = read_positions(statement, register)
                    if not spans:
                        refuse(load, 'the read is not a plain read of the register')
                    elif not all(is_value(statement.code, at) for at, _ in spans):
                        refuse(load, 'the read is an address, not a value')
                    else:
                        foldable[load] = (i, spans)

    folds = {}
    for load, (reader, spans) in foldable.items():
        if load in left:
            continue
        if len(readers[load]) > 1:
            left[load] = 'more than one statement reads the load'
        else:
            folds[load] = (reader, spans)
    for load, statement in graph.statements.items():
        if statement.load and load not in folds and load not in left:
            left[load] = 'nothing reads the load'
    return folds, left


def rewrite(src, graph, folds):
    """Put each load's expression where its reader is, and take the load away."""
    lines = list(src.lines)
    into = collections.defaultdict(list)
    for load, (reader, spans) in folds.items():
        for span in spans:
            into[reader].append((span, graph.statements[load].load))

    for reader, substitutions in into.items():
        code = graph.statements[reader].code
        for (start, stop), expression in sorted(substitutions, reverse=True):
            code = code[:start] + expression + code[stop:]
        comment = src.comment(reader)
        lines[reader] = src.indent(reader) + code + (' ' + comment if comment else '')

    for load in folds:
        comment = src.comment(load)      # the load goes, but what it was for stays
        lines[load] = src.indent(load) + comment if comment else None

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

    reads = sum(len(spans) for _, spans in folds.values())
    print('%s: %d loads folded into %d reads' % (arguments.file, len(folds), reads))
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
