"""Rewrite the disassembly's branches as the ifs, elses and loops they were.

    python3 tools/degoto.py source/SMB/SMB.cpp [--dry-run] [--max-body N]
                                               [--no-else] [--no-loops]

A 6502 conditional branch that jumps over a piece of code comes out of codegen as
a branch to a label just past it:

    if (a == 0)
        goto ChkJH;          // if not set, go ahead with code
    goto MoveDefeatedEnemy;  // otherwise jump to something else

ChkJH:                       // check jump timer

which is an if-block with the sense of the test reversed. Invert the condition,
turn the `goto` into the opening brace and the label into the closing one:

    if (!z)
    {                        // if not set, go ahead with code
        goto MoveDefeatedEnemy; // otherwise jump to something else
    } // ChkJH: check jump timer

The comments are moved verbatim onto the lines that replaced the ones they were
written on, and the label's name is kept in the comment on the closing brace, so
that the code can still be read against docs/smbdis.asm. Both are worth a skim
afterwards: a comment on a branch usually describes the branch being *taken*, and
the sense of that is now reversed.

A branch is only rewritten when all of this holds, and is left alone otherwise:

  * The label is referenced exactly once in the whole file, by this goto. It is
    not enough to count `goto`s -- JSR(Sub, n) expands to `goto Sub`, so a label
    that is also a subroutine entry point is referenced by every call to it, and
    deleting it would not compile.
  * The label is below the goto (a skip forward, not a loop) with at least one
    line in between, so that the block is not empty.
  * The condition is a flag (z, c, n, possibly negated) or a simple comparison,
    both of which invert exactly.
  * The lines in between are brace-balanced. Without this an if-block whose goto
    lands inside a block some other rewrite created, but whose label lands past
    the end of it, would emit crossed braces.

Passes are repeated until nothing changes, so a skip-branch nested inside the
body of another is rewritten too, on a later pass.

Once there is an if-block, an else can be seen behind it. A block that ends by
jumping over the code that follows it is the then-half of an if/else, and the
code it jumps over is the other half:

    if (z)
    {
        foo;
        goto b;
    }
    bar;
b:

    ->  if (z) { foo; } else { bar; }

with the same conditions on the label as above, plus one that is easy to get
wrong: the `goto` must be a statement of the block and not the body of an
unbraced `if` that happens to be the last thing in it. Lifting a conditional
jump out as if it were the block's exit both leaves the `if` dangling and moves
code that was only reached sometimes. See is_consequent().

A branch backwards is a loop. If it is the last thing the loop does, the loop is
a do-while and the label is its top (find_do). If instead the label is followed
directly by an if-block that ends by jumping back to it -- which is what a loop
that tests at the top looks like once its exit branch has been rewritten -- the
loop is a while (find_while). Neither condition is inverted: the branch is taken
to go round again, which is what both loops do while their condition holds.

The label's single reference matters as much here. A label that something else
also jumps to is an entry into the middle of the loop, which no C loop can spell,
and a `goto` in the body that leaves it is left as a `goto`: there is no `break`
to be recovered when the jump goes somewhere other than just past the loop.
"""
import argparse
import re
import sys
from collections import Counter

IF = re.compile(r'^(?P<indent>[ \t]*)if \((?P<cond>.+)\)[ \t]*(?P<comment>//.*)?$')
GOTO = re.compile(r'^[ \t]*goto (?P<label>\w+);[ \t]*(?P<comment>//.*)?$')
OPEN = re.compile(r'^(?P<indent>[ \t]*)\{[ \t]*(?P<comment>//.*)?$')
HEADER = re.compile(r'^[ \t]*(if \(.*\)|else)[ \t]*(//.*)?$')  # takes the next statement
LABEL = re.compile(r'^(?P<label>[A-Za-z_]\w*):')
IDENT = re.compile(r'[A-Za-z_]\w*')
COMMENT = re.compile(r'//.*|/\*.*?\*/')

FLAGS = ('z', 'c', 'n')
OPPOSITE = {'<': '>=', '>=': '<', '>': '<=', '<=': '>', '==': '!=', '!=': '=='}
COMPARISON = re.compile(r'^([^<>=!&|]+?)\s*(<=|>=|==|!=|<|>)\s*([^<>=!&|]+)$')


def invert(condition):
    """The negation of a condition, or None if it does not invert exactly."""
    condition = condition.strip()
    if condition in FLAGS:
        return '!' + condition
    if condition.startswith('!') and condition[1:] in FLAGS:
        return condition[1:]
    comparison = COMPARISON.match(condition)
    if comparison:
        left, operator, right = comparison.groups()
        return '%s %s %s' % (left.strip(), OPPOSITE[operator], right.strip())
    return None


def code_only(line):
    return COMMENT.sub('', line)


def reference_counts(lines):
    """How often each name is used, not counting the labels' own definitions."""
    counts = Counter()
    for line in lines:
        line = code_only(line)
        definition = LABEL.match(line)
        if definition:
            line = line[definition.end():]
        counts.update(IDENT.findall(line))
    return counts


def label_lines(lines):
    return {LABEL.match(line)['label']: i
            for i, line in enumerate(lines) if LABEL.match(line)}


def balanced(lines):
    """True if the lines open and close every brace they contain, in that order."""
    depth = 0
    for line in lines:
        for character in code_only(line):
            depth += (character == '{') - (character == '}')
            if depth < 0:
                return False
    return depth == 0


def close_of(lines, opening):
    """The index of the brace that closes the one on the `opening` line."""
    depth = 0
    for i in range(opening, len(lines)):
        for character in code_only(lines[i]):
            depth += (character == '{') - (character == '}')
        if depth == 0:
            return i
    return None


def last_statement(lines, first, last):
    """The index of the final non-blank line in lines[first:last], if any."""
    while last > first and not lines[last - 1].strip():
        last -= 1
    return last - 1 if last > first else None


def is_consequent(lines, i):
    """True if the statement on line i is the unbraced body of the if above it.

    Such a statement is conditional, however unconditional it looks in isolation,
    and it cannot be moved or taken to be the last thing a block does. A `goto`
    ending a block is only the block's exit if it is a statement of the block:

        if (!z)
        {
            compare(a, A_Button + Start_Button);
            if (a != A_Button + Start_Button)
                goto ChkSelect;  // the block's last line, but not its exit
        }
    """
    previous = last_statement(lines, 0, i)
    return previous is not None and HEADER.match(lines[previous]) is not None


def comments(*lines_or_none):
    """The comments on some lines, run together, as one trailing comment."""
    texts = []
    for line in lines_or_none:
        if line:
            match = re.search(r'//(.*)$', line)
            if match and match.group(1).strip():
                texts.append(match.group(1).strip())
    return ' // ' + ', '.join(texts) if texts else ''


def find_do(lines, max_body, skipped):
    """The backward branches, which are the bottom of a do-while, as indices.

        Loop:                       do // Loop:
            body            ->      {
            if (c)                      body
                goto Loop;          } while (c);
    """
    labels = label_lines(lines)
    references = reference_counts(lines)
    found = []
    previous = -1
    i = 1
    while i < len(lines):
        jump = GOTO.match(lines[i])
        branch = IF.match(lines[i - 1]) if jump else None
        top = labels.get(jump['label']) if jump else None
        if not branch or top is None or top >= i - 1:
            i += 1
            continue
        label = jump['label']
        reason = None
        if top <= previous:
            # Matches are found by their end, so the inner loop of a nest comes
            # first and the one around it would overlap what was just taken. Let
            # the next pass have it, once this one is a do-while and balanced.
            reason = 'do: overlaps the loop just taken'
        elif is_consequent(lines, i - 1):
            reason = 'do: the branch is the body of another if'
        elif references[label] != 1:
            reason = 'do: label is used %d times' % references[label]
        elif not balanced(lines[top + 1:i - 1]):
            reason = 'do: body is not brace-balanced'
        elif max_body and i - top > max_body:
            reason = 'do: body is longer than %d lines' % max_body
        if reason:
            skipped[reason] += 1
            i += 1
            continue
        found.append((top, i - 1, i))
        previous = i
        i += 1
    return found


def rewrite_do(lines, found):
    out = []
    at = 0
    for top, branch, jump in found:
        out.extend(lines[at:top])
        header = IF.match(lines[branch])
        indent = header['indent']
        body = lines[top + 1:branch]
        while body and not body[-1].strip():
            body.pop()

        out.append('%sdo // %s' % (indent, describe(lines[top])))
        out.append('%s{' % indent)
        out.extend(reindent(body))
        out.append('%s} while (%s);%s' % (indent, header['cond'],
                                          comments(lines[branch], lines[jump])))
        at = jump + 1
    out.extend(lines[at:])
    return out


def find_while(lines, max_body, skipped):
    """The loops that test at the top, left as a label and an if-block by find().

        Loop:                       while (c) // Loop:
            if (c)          ->      {
            {                           body
                body                }
                goto Loop;
            }
    """
    labels = label_lines(lines)
    references = reference_counts(lines)
    found = []
    previous = -1
    for label, top in sorted(labels.items(), key=lambda item: item[1]):
        head = next((i for i in range(top + 1, len(lines)) if lines[i].strip()), None)
        if head is None or head + 1 >= len(lines) or top <= previous:
            continue
        branch, opening = IF.match(lines[head]), OPEN.match(lines[head + 1])
        if not (branch and opening and branch['indent'] == opening['indent']):
            continue
        close = close_of(lines, head + 1)
        jump = last_statement(lines, head + 2, close) if close is not None else None
        if jump is None or not GOTO.match(lines[jump]):
            continue
        if GOTO.match(lines[jump])['label'] != label:
            continue
        reason = None
        if references[label] != 1:
            reason = 'while: label is used %d times' % references[label]
        elif is_consequent(lines, jump):
            reason = 'while: the block does not end by jumping'
        elif max_body and close - head > max_body:
            reason = 'while: body is longer than %d lines' % max_body
        if reason:
            skipped[reason] += 1
            continue
        found.append((top, head, jump, close))
        previous = close
    return found


def rewrite_while(lines, found):
    out = []
    at = 0
    for top, head, jump, close in found:
        out.extend(lines[at:top])
        branch = IF.match(lines[head])
        body = lines[head + 2:jump]
        while body and not body[-1].strip():
            body.pop()

        out.append('%swhile (%s) // %s' % (branch['indent'], branch['cond'],
                                           describe(lines[top])))
        out.append(lines[head + 1])  # the opening brace, with its comment
        out.extend(body)
        out.append(lines[close])
        at = close + 1
    out.extend(lines[at:])
    return out


def find_else(lines, max_body, skipped):
    """The if-blocks that jump over the code after them, as index tuples."""
    labels = label_lines(lines)
    references = reference_counts(lines)
    found = []
    i = 0
    while i + 2 < len(lines):
        branch = IF.match(lines[i])
        opening = OPEN.match(lines[i + 1]) if branch else None
        if not opening or opening['indent'] != branch['indent']:
            i += 1
            continue
        close = close_of(lines, i + 1)
        jump = last_statement(lines, i + 2, close) if close is not None else None
        if jump is None or not GOTO.match(lines[jump]):
            i += 1
            continue
        label = GOTO.match(lines[jump])['label']
        end = labels.get(label)
        reason = None
        if is_consequent(lines, jump):
            reason = 'else: the block does not end by jumping'
        elif close + 1 < len(lines) and lines[close + 1].strip().startswith('else'):
            reason = 'else: there is one already'
        elif end is None:
            reason = 'else: label is not in this file'
        elif references[label] != 1:
            reason = 'else: label is used %d times' % references[label]
        elif end <= close:
            reason = 'else: jumps backwards'
        elif not lines[close + 1:end] or not any(
                line.strip() for line in lines[close + 1:end]):
            reason = 'else: nothing to put in it'
        elif not balanced(lines[close + 1:end]):
            reason = 'else: body is not brace-balanced'
        elif max_body and end - close > max_body:
            reason = 'else: body is longer than %d lines' % max_body
        if reason:
            skipped[reason] += 1
            i += 1
            continue
        found.append((i, jump, close, end))
        i = end + 1
    return found


def rewrite_else(lines, found):
    out = []
    at = 0
    for start, jump, close, end in found:
        indent = IF.match(lines[start])['indent']
        body = lines[at:jump]  # the if line, the brace, and all but the goto
        while body and not body[-1].strip():
            body.pop()
        otherwise = lines[close + 1:end]
        while otherwise and not otherwise[0].strip():
            otherwise.pop(0)
        while otherwise and not otherwise[-1].strip():
            otherwise.pop()

        out.extend(body)
        out.append(lines[close])
        out.append('%selse%s' % (indent, trailing(GOTO.match(lines[jump])['comment'])))
        out.append('%s{' % indent)
        out.extend(reindent(otherwise))
        out.append('%s} // %s' % (indent, describe(lines[end])))
        at = end + 1
    out.extend(lines[at:])
    return out


def find(lines, max_body, skipped):
    """The non-overlapping skip-branches in lines, as (if, goto, label) indices."""
    labels = label_lines(lines)
    references = reference_counts(lines)
    found = []
    i = 0
    while i + 1 < len(lines):
        branch, jump = IF.match(lines[i]), GOTO.match(lines[i + 1])
        if not (branch and jump):
            i += 1
            continue
        label = jump['label']
        end = labels.get(label)
        reason = None
        if is_consequent(lines, i):
            reason = 'branch is the body of another if'
        elif end is None:
            reason = 'label is not in this file'
        elif references[label] != 1:
            reason = 'label is used %d times' % references[label]
        elif end < i:
            reason = 'jumps backwards'
        elif end == i + 2:
            reason = 'body is empty'
        elif invert(branch['cond']) is None:
            reason = 'condition does not invert'
        elif not balanced(lines[i + 2:end]):
            reason = 'body is not brace-balanced'
        elif max_body and end - (i + 2) > max_body:
            reason = 'body is longer than %d lines' % max_body
        if reason:
            skipped[reason] += 1
            i += 1
            continue
        found.append((i, i + 1, end))
        i = end + 1  # a nested skip-branch is picked up on a later pass
    return found


def trailing(comment):
    return ' ' + comment if comment else ''


def reindent(body):
    """Body lines, moved one level in. Labels stay in column 0, both to keep them
    findable on the next pass and because that is where the rest of the file puts
    them."""
    return [line if not line.strip() or LABEL.match(line) else '    ' + line
            for line in body]


def describe(label):
    """The label line, as the text of the comment that will replace it."""
    definition = LABEL.match(label)
    comment = label[definition.end():].strip()
    if comment.startswith('//'):
        comment = comment[2:].strip()
    return definition['label'] + (': ' + comment if comment else '')


def rewrite(lines, found):
    out = []
    at = 0
    for start, jump, end in found:
        out.extend(lines[at:start])
        branch = IF.match(lines[start])
        indent = branch['indent']
        body = lines[jump + 1:end]
        while body and not body[-1].strip():
            body.pop()

        out.append('%sif (%s)%s' % (indent, invert(branch['cond']),
                                    trailing(branch['comment'])))
        out.append('%s{%s' % (indent, trailing(GOTO.match(lines[jump])['comment'])))
        out.extend(reindent(body))
        out.append('%s} // %s' % (indent, describe(lines[end])))
        at = end + 1
    out.extend(lines[at:])
    return out


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument('file')
    parser.add_argument('-n', '--dry-run', action='store_true',
                        help='report what would change, write nothing')
    parser.add_argument('--max-body', type=int, default=0, metavar='N',
                        help='leave branches skipping over more than N lines alone')
    parser.add_argument('--no-else', action='store_true',
                        help='do not recover elses')
    parser.add_argument('--no-loops', action='store_true',
                        help='do not recover while and do-while loops')
    arguments = parser.parse_args()

    with open(arguments.file) as handle:
        lines = handle.read().split('\n')

    # Order matters. An else, and a loop that tests at the top, can only be seen
    # once the skip-branch that hid the if-block it is made of has been rewritten.
    # So each kind is looked for only when the ones before it have run out, and
    # then everything is looked for again, which is also what nests them.
    passes = [('skip-branches', find, rewrite)]
    if not arguments.no_else:
        passes.append(('elses', find_else, rewrite_else))
    if not arguments.no_loops:
        passes.append(('whiles', find_while, rewrite_while))
        passes.append(('do-whiles', find_do, rewrite_do))

    original = list(lines)
    counts = Counter()
    while True:
        for name, look_for, apply_to in passes:
            found = look_for(lines, arguments.max_body, Counter())
            if found:
                counts[name] += len(found)
                lines = apply_to(lines, found)
                break
        else:
            break

    skipped = Counter()  # what the finished file still holds, and why
    for _, look_for, _ in passes:
        look_for(lines, arguments.max_body, skipped)

    print('%s: %s' % (arguments.file, ', '.join(
        '%d %s' % (counts[name], name) for name, _, _ in passes)))
    for reason, count in skipped.most_common():
        print('  left alone: %-36s %d' % (reason, count))

    if arguments.dry_run:
        sys.stdout.writelines(__import__('difflib').unified_diff(
            [line + '\n' for line in original], [line + '\n' for line in lines],
            arguments.file, arguments.file + ' (rewritten)'))
    elif counts:
        with open(arguments.file, 'w') as handle:
            handle.write('\n'.join(lines))


if __name__ == '__main__':
    main()
