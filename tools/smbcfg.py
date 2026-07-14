"""The control flow graph of SMB.cpp, which every rewrite of it needs.

SMB.cpp is one function, and every jump in it is written down: a branch is an if, a
jump table is a switch, a call is JSR(Sub, n) -- which expands to `goto Sub` with a
label after it -- and RTS is `goto Return`, a jump into a switch that dispatches on
the index the call pushed, back to the label after it. So the file can be read as a
graph with no edges left to guess, and a question like "what else reads this register"
or "what else jumps into this label" has an answer.

    src = Source('source/SMB/SMB.cpp')
    graph = Graph(src, parse(src, src.start, src.end)[0])

The graph's nodes are statements, keyed by the line they start on. Statement says what
each one does to the registers and to memory; Graph.successors and Graph.predecessors
say where control goes. tools/foldloads.py and tools/subroutines.py are built on it.
"""
import collections
import re

REGS = ('a', 'x', 'y')

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
        self.operation = None   # (register, operator, operand) of a compound assignment
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
            target, operator, operand = match.groups()
            self.reads = registers_in(operand)
            if target in REGS:
                self.defines = {target}
                self.reads |= {target}   # a &= x reads a into its own new value
                if target not in registers_in(operand) and not self.writes_memory:
                    self.operation = (target, operator, operand)
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

    def __init__(self, path, lines=None):
        self.path = path
        # `lines` is for a pass that runs to a fixed point: it has rewritten the file in
        # memory and wants to read the rewrite back, without a trip through the disk.
        self.lines = open(path).read().split('\n') if lines is None else list(lines)
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


RE_METHOD = re.compile(r'^[\w:<>*& ]+SMBEngine::(\w+)\(')


def functions(src):
    """Every method in the file: name -> (the first line of its body, its closing brace).

    SMB.cpp was one function, and the tools here were written to read that one. It is not
    any more: tools/subroutines.py has been cutting the routines out of code() and writing
    them as functions of their own, so a pass that wants to see the whole file has to look
    at all of them. code() is one of these and the routines lifted out of it are the rest.

    The bounds are what parse() wants: parse(src, *functions(src)['DrawBubble']) is the body
    of that routine and nothing else, since parse stops at the closing brace it is handed.
    """
    found = {}
    for i, line in enumerate(src.lines):
        match = RE_METHOD.match(line)
        if not match:
            continue
        opening = i
        while src.lines[opening].strip() != '{':
            opening += 1
        closing = next(j for j in range(opening + 1, len(src.lines)) if src.lines[j] == '}')
        found[match.group(1)] = (opening + 1, closing)
    return found


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
    """The control flow graph of the file. Every edge in it is written down somewhere.

    `guarded` is the one thing the graph knows that the edges do not say. The 6502 leaves
    a routine two ways: JMP, which goes somewhere else for good, and a branch -- BNE, BCC --
    which is how a routine picks its way through itself. Both are `goto` here, and the
    difference survives only in how the goto is written: a branch became the whole body of
    an if, and a JMP stayed a bare `goto Somewhere;`. tools/subroutines.py reads a bare goto
    into a routine as a tail call and a guarded one as a branch, so it keeps the set.
    """

    def __init__(self, src, items):
        self.src = src
        self.statements = {}
        self.successors = {}
        self.labels = {}
        self.guarded = set()    # the statements a branch instruction was written as: see below
        self.entry = self.wire(items, EXIT)
        self.resolve_jumps()
        self.predecessors = collections.defaultdict(list)
        for i, following in self.successors.items():
            for j in following:
                self.predecessors[j].append(i)

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
                for body in (item[2], item[3]):
                    if len(body) == 1 and body[0][0] == 'stmt':
                        self.guarded.add(body[0][1])
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
