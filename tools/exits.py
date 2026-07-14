"""Say the return where the return is.

    python3 tools/exits.py source/SMB/SMB.cpp [--dry-run] [--verbose]

A 6502 routine leaves by branching to its RTS, and the disassembly gives that RTS a
label so that the branches have something to name:

    BNE ExDBub
    ...
  ExDBub: RTS

which the port writes down as it stands -- a label, and a goto to it from every branch
that wanted to leave:

    if (y != 0)
        goto ExDBub;
    ...
ExDBub: // leave
    return;

The goto is a return. It is written as a jump to a place where a return is, because on
the 6502 a branch is all there is and the return has to be somewhere; C can put the
return where the branch is and say the same thing:

    if (y != 0)
        return;

So this threads every jump to an exit through to the exit itself, and takes the label
away. Nothing about the file's meaning changes -- a jump to a return and a return are
the same instructions -- but a reader who meets `goto ExDBub;` has to go and find out
what ExDBub is, and a reader who meets `return;` is already done.

An exit is a statement that ends the routine and does nothing else. In a routine that
tools/subroutines.py has lifted out that is `return;`, or `return jumpspringFound;` in
one that answers its caller. Inside code(), which is still one function, it is
`goto Return;`: the RTS is a jump into a switch that dispatches on the index the call
pushed, and until code() is cut up too that switch is where its returns go. Both are one
statement that leaves, which is the whole of what this needs to know about them.

A label is threaded when:

  * The statement it stands in front of is an exit, and it stands directly in front of
    it -- so `Exit: return;` and not `Exit: y = M(0x02); return;`, which is a routine's
    tail and not its exit, and would have to be copied to every jump rather than threaded.
  * Nothing calls it. A JSR to a label whose body is a return is a call to a routine with
    nothing to do, and threading it would return from the caller instead of from the
    routine that made the call, which is a different program.

The label goes, and the exit statement it labelled goes with it unless the code above
falls into it, which is the usual case: the last branch of a routine leaves by running
off the end into the same RTS the earlier ones jumped to. The label's name and comment
are kept on the exit when it stays, the way tools/degoto.py keeps a label's name on the
brace that replaced it, so that the code can still be read against docs/smbdis.asm.

Passes are repeated until nothing changes, which threads the chains. A label in front of
a jump to an exit -- `EndExitOne: goto Return;` -- is not an exit itself, so the first
pass leaves it alone; but the pass rewrites its jump into a return, and then it is one,
and the next pass takes it too.

Run this before tools/subroutines.py rather than after. An exit label shared between two
routines is a tail they share, and a routine that ends by jumping into the middle of
another one cannot be lifted out of code() while it does; threading the jump unpicks the
tail and lets both of them go.
"""
import argparse
import re
import sys

from smbcfg import EXIT, RE_LABEL, Graph, Source, functions, parse

RE_RETURN = re.compile(r'^return\b.*;$')


def is_exit(statement):
    """Does this statement end the routine, and do nothing else?"""
    return bool(RE_RETURN.match(statement.code)) or statement.goto == 'Return'


def written(src, first, last):
    """The labels the text of a routine actually spells, by name.

    graph.labels holds more than these: JSR(Sub, n) expands to a jump and a `Return_n`
    label after it for the RTS to come back to, and the graph synthesises that label
    rather than reading it, because it is inside the macro. Those are not lines of the
    file and there is nothing to take away, so a pass that rewrites text wants only the
    labels that are in the text.
    """
    return {RE_LABEL.match(src.code(i).strip()).group(1): i
            for i in range(first, last)
            if RE_LABEL.match(src.code(i).strip())}


def threadable(src, graph, labels):
    """The labels of this routine that stand in front of nothing but an exit."""
    found = {}
    called = {statement.jsr[0] for statement in graph.statements.values() if statement.jsr}
    for label in labels:
        following = graph.successors[graph.labels[label]][0]
        if following == EXIT or following in labels.values():
            continue                      # the end of the routine, or a second label
        if not is_exit(graph.statements[following]) or not src.one_line(following):
            continue
        if label in called:
            continue                      # a call, not a jump: see the docstring
        found[label] = following
    return found


SEPARATOR = '//---'


def said(comment):
    return comment.lstrip('/').strip()


def empty(src, i):
    """Is this line nothing but whitespace?

    Not Source.blank(), which strips the comments off first and so calls a line that is all
    comment an empty one. That is what parsing wants and the opposite of what laying the text
    out wants: the rule above a routine is a comment, and it is not whitespace to be closed up.
    """
    return not src.lines[i].strip()


def describe(src, at, exit_at):
    """The label's name and everything written about it, as the exit's trailing comment.

    The name has to survive somewhere: it is what docs/smbdis.asm calls this place, and the
    comments on the branches to it say things like "branch to leave" that only mean anything
    if the reader can still tell what was left. So it goes on the line that replaced the line
    it was written on, which is what tools/degoto.py does with the labels it takes away too.
    """
    name = RE_LABEL.match(src.code(at).strip()).group(1)
    about = ', '.join(text for text in (said(src.comment(at)), said(src.comment(exit_at)))
                      if text)
    return '// %s%s' % (name, ': ' + about if about else '')


def heading(src, at):
    """The first line of what is written above a label: its comments, and the rule above them."""
    first = at
    while first > 0:
        above = src.lines[first - 1].strip()
        if above and not above.startswith('//'):
            break
        first -= 1
        if above.startswith(SEPARATOR):
            break
    return first


def thread(src, lines, graph, label, exit_at):
    """Rewrite every jump to this label as the exit it jumps to, and take the label away.

    The exit stays if the code above falls into it, which is the usual case: the last branch
    of a routine leaves by running off the end into the same RTS the earlier ones jumped to.
    Every edge into a label is either one of the jumps this is removing or the line above
    running on into it, so if none of the edges are of the second kind then nothing is left
    that can reach the exit, and it goes -- along with the rule and the comments above it,
    which were a heading for a piece of the file that is no longer there.
    """
    at = graph.labels[label]
    exit_code = src.code(exit_at).strip()
    jumps = [i for i, statement in graph.statements.items() if statement.goto == label]
    for i in jumps:
        comment = src.comment(i)
        lines[i] = src.indent(i) + exit_code + (' ' + comment if comment else '')

    falls_in = any(graph.statements[j].goto != label for j in graph.predecessors[at])
    if falls_in:
        lines[at] = None
        lines[exit_at] = '%s%s %s' % (src.indent(exit_at), exit_code,
                                      describe(src, at, exit_at))
        return jumps, True

    first, last = heading(src, at), exit_at
    while (last + 1 < len(lines) and empty(src, last + 1)
           and (first == 0 or empty(src, first - 1))):
        last += 1                         # do not leave the blank lines either side to run together
    for i in range(first, last + 1):
        lines[i] = None
    return jumps, False


def sweep(src):
    """Thread every exit label in the file, once. Returns the new lines, and what it did."""
    lines = list(src.lines)
    done = []
    for name, (first, last) in functions(src).items():
        graph = Graph(src, parse(src, first, last)[0])
        labels = written(src, first, last)
        for label, exit_at in threadable(src, graph, labels).items():
            jumps, falls_in = thread(src, lines, graph, label, exit_at)
            done.append((name, label, src.code(exit_at).strip(), len(jumps), falls_in))
    return [line for line in lines if line is not None], done


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument('file', help='the file to rewrite, source/SMB/SMB.cpp')
    parser.add_argument('-n', '--dry-run', action='store_true',
                        help='report what would change, write nothing')
    parser.add_argument('--verbose', action='store_true',
                        help='name every label threaded, not just the counts')
    arguments = parser.parse_args()

    original = Source(arguments.file).lines
    lines, done = original, []
    while True:                           # to a fixed point: a jump to an exit becomes one
        lines, found = sweep(Source(arguments.file, lines))
        done += found
        if not found:
            break

    jumps = sum(count for _, _, _, count, _ in done)
    kept = sum(1 for _, _, _, _, falls_in in done if falls_in)
    print('%s: %d exit labels threaded, %d jumps became the return they jumped to'
          % (arguments.file, len(done), jumps))
    print('  %d of the labelled returns are fallen into and stay; %d were unreachable and go'
          % (kept, len(done) - kept))

    if arguments.verbose:
        for name, label, code, count, falls_in in done:
            print('    %-26s %-20s %-24s %2d jump%s %s'
                  % (name, label, code, count, ' ' if count == 1 else 's',
                     '' if falls_in else '(nothing fell into it)'))

    if arguments.dry_run:
        sys.stdout.writelines(__import__('difflib').unified_diff(
            [line + '\n' for line in original], [line + '\n' for line in lines],
            arguments.file, arguments.file + ' (rewritten)'))
    elif done:
        with open(arguments.file, 'w') as handle:
            handle.write('\n'.join(lines))


if __name__ == '__main__':
    main()
