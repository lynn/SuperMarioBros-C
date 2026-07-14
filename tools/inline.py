"""Move a constant table out of SMBData.cpp and into the function that reads it.

    python3 tools/inline.py --report            # what could move, and what could not
    python3 tools/inline.py --audit  [--apply]  # instrument the reads, to measure their range
    python3 tools/inline.py --inline [--apply]  # move the tables that are safe to move

SMBData.cpp writes 343 tables into a 32KB image of the ROM's constant data, and the game reads
them back out of it with M(Table + y) -- addresses, because on the console that is all they were.
A table that only one function ever reads has no reason to be there: it can be a plain C array,
declared in that function, indexed with [] like an array. The function becomes readable on its
own terms, the address goes away, and so does the entry in SMBDataPointers.hpp.

What makes it more than a text substitution is that the image is contiguous and the game knows
it. Several tables are read *past their end* on purpose, and land in the bytes of whatever table
was assembled next; a few are read from before their start (M(DemoTimingData - 1 + x)). Those
reads are the reason SMBData.cpp is one flat image rather than 343 arrays, and a table on the
end of one cannot leave: an array does not have neighbours, and C would be reading the stack.

So this measures rather than assumes. --audit rewrites every read as a probe that records the
index it was given and then does the read exactly as before; play the movie under it and the
range of every table is known, from the game rather than from an argument about the game. A
table whose reads all landed inside it is safe to move, and --inline moves it. One that ran off
the end stays where it is, and gets named, because that is a bug worth its own fix.

Nothing is written without --apply.
"""
import argparse
import collections
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from split import blocks

SOURCE = 'source/SMB'
DATA = os.path.join(SOURCE, 'SMBData.cpp')
POINTERS = os.path.join(SOURCE, 'SMBDataPointers.hpp')

# The table's definition in SMBData.cpp: the comment above it, the array, and the write.
TABLE = re.compile(r'^    const uint8_t (\w+)_data\[\] = \{$')


def tables():
    """Every table SMBData.cpp defines: its text, its values, and where they are."""
    lines = open(DATA).read().split('\n')
    found = collections.OrderedDict()
    for i, line in enumerate(lines):
        match = TABLE.match(line)
        if not match:
            continue
        name = match.group(1)
        heading = i
        while heading > 0 and lines[heading - 1].strip().startswith('//'):
            heading -= 1
        end = next(k for k in range(i, len(lines))
                   if ', %s_data, sizeof(%s_data));' % (name, name) in lines[k])
        body = '\n'.join(lines[i:end])               # the array, up to and not incl. the write
        size = len([v for v in re.search(r'\{(.*)\};', body, re.S).group(1).split(',')
                    if v.strip()])
        # A table written anywhere but at its own address is one the game reads off the end of,
        # and it has been padded by hand with the ROM's neighbouring bytes to keep those reads
        # honest (ClimbAdder, written at ClimbXPosAdder - 1). The padding is the point: it only
        # works while the table is at an address, so the table stays.
        found[name] = dict(heading=heading, end=end, size=size,
                           padded=not lines[end].startswith('    writeData(%s,' % name),
                           text='\n'.join(lines[heading:i]),  # the comment
                           array=body)
    return found, lines


def reads(names):
    """Every read of a table in the game: M(Table), M(Table + i), M(Table - k + i)."""
    out = collections.defaultdict(list)
    for f in sorted(os.listdir(SOURCE)):
        if not f.endswith('.cpp') or f == 'SMBData.cpp':
            continue
        path = os.path.join(SOURCE, f)
        text = open(path).read().split('\n')
        where = {}
        for routine, (heading, end, _) in blocks(path).items():
            for i in range(heading, end + 1):
                where[i] = routine
        for i, line in enumerate(text):
            for start, inside in scan(line):
                name = re.match(r'\s*(\w+)', inside)
                if not name or name.group(1) not in names:
                    continue
                out[name.group(1)].append(
                    dict(file=f, line=i, routine=where.get(i), inside=inside, start=start))
    return out


def scan(line):
    """Each M(...) in the line, as the offset it starts at and the text inside its parens."""
    for match in re.finditer(r'\bM\(', line):
        depth, i = 1, match.end()
        while i < len(line) and depth:
            depth += (line[i] == '(') - (line[i] == ')')
            i += 1
        yield match.start(), line[match.end():i - 1]


def index(inside, name):
    """The index a read is really using: M(T - 8 + x) is T[x - 8], M(T) is T[0]."""
    rest = inside[len(re.match(r'\s*%s' % name, inside).group(0)):].strip()
    if not rest:
        return '0'
    bias = re.match(r'^-\s*(\w+)\s*\+\s*(.+)$', rest)      # M(T - 8 + x)
    if bias:
        return '%s - %s' % (bias.group(2).strip(), bias.group(1))
    plus = re.match(r'^\+\s*(.+)$', rest)                  # M(T + x)
    if plus:
        return plus.group(1).strip()
    return None                                            # something else; leave it alone


def candidates():
    """The tables one function reads, in ways this can rewrite. Plus why the others cannot."""
    defined, _ = tables()
    seen = reads(set(defined))

    # a name used anywhere but in a read -- a pointer stashed in the zero page, an address in
    # another table -- is an address the game needs, and the table has to keep having one.
    everywhere = collections.Counter()
    for f in sorted(os.listdir(SOURCE)):
        if not f.endswith(('.cpp', '.hpp')) or f in ('SMBData.cpp', 'SMBDataPointers.hpp'):
            continue
        for line in open(os.path.join(SOURCE, f)).read().split('\n'):
            for name in re.findall(r'\b(\w+)\b', re.sub(r'//.*', '', line)):
                if name in defined:
                    everywhere[name] += 1

    good, bad = collections.OrderedDict(), collections.OrderedDict()
    for name, table in defined.items():
        sites = seen.get(name, [])
        homes = {(s['file'], s['routine']) for s in sites}
        if table['padded']:
            bad[name] = 'padded by hand: the game reads off the end of it'
        elif not sites:
            bad[name] = 'never read by name: reached through its address'
        elif len(homes) > 1:
            bad[name] = 'read by %d functions' % len(homes)
        elif None in {s['routine'] for s in sites}:
            bad[name] = 'read outside any function'
        elif everywhere[name] != len(sites):
            bad[name] = 'used %d times but only read %d: its address is taken' % (
                everywhere[name], len(sites))
        elif any(index(s['inside'], name) is None for s in sites):
            bad[name] = 'read in a way this cannot rewrite'
        else:
            table['home'] = homes.pop()
            table['sites'] = sites
            good[name] = table
    return good, bad


def rewrite(good, audit):
    """Rewrite the reads, and put each table in the function that reads it."""
    edited = collections.defaultdict(dict)      # file -> line number -> new text
    for name, table in good.items():
        for site in table['sites']:
            line = edited[site['file']].get(site['line'])
            if line is None:
                line = open(os.path.join(SOURCE, site['file'])).read().split('\n')[site['line']]
            i = index(site['inside'], name)
            was = 'M(%s)' % site['inside']
            now = ('TBL(%s, %d, %s)' % (name, table['size'], i) if audit
                   else '%s_data[%s]' % (name, i))
            edited[site['file']][site['line']] = line.replace(was, now, 1)

    for f, changes in edited.items():
        path = os.path.join(SOURCE, f)
        lines = open(path).read().split('\n')
        for i, line in changes.items():
            lines[i] = line
        if not audit:                            # declare the tables inside their functions
            where = blocks(path)
            mine = [(n, t) for n, t in good.items() if t['home'][0] == f]
            # bottom of the file upwards, so that inserting into one function does not move the
            # next one out from under the line numbers we looked up
            for name, table in sorted(mine, key=lambda kv: -where[kv[1]['home'][1]][0]):
                heading, _, _ = where[table['home'][1]]
                brace = next(k for k in range(heading, len(lines)) if lines[k] == '{')
                lines[brace + 1:brace + 1] = (table['array'] + '\n').split('\n')
        open(path, 'w').write('\n'.join(lines))


def strip(good):
    """Take the tables out of SMBData.cpp, and their addresses out of SMBDataPointers.hpp."""
    defined, lines = tables()
    cut = set()
    for name in good:
        table = defined[name]
        cut.update(range(table['heading'], table['end'] + 2))   # + the blank line after it
    kept = [line for i, line in enumerate(lines) if i not in cut]
    open(DATA, 'w').write('\n'.join(kept))

    out = []
    for line in open(POINTERS).read().split('\n'):
        name = re.match(r'\s*(?:uint16_t (\w+)_ptr;|this->(\w+)_ptr = 0x|#define (\w+) )', line)
        if name:
            hit = re.match(r'\s*uint16_t (\w+)_ptr;', line) or \
                  re.match(r'\s*this->(\w+)_ptr = ', line) or \
                  re.match(r'#define (\w+) \(dataPointers', line)
            if hit and hit.group(1) in good:
                continue
        out.append(line)
    open(POINTERS, 'w').write('\n'.join(out))


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument('--report', action='store_true', help='what could move, and what could not')
    parser.add_argument('--audit', action='store_true', help='instrument the reads')
    parser.add_argument('--inline', action='store_true', help='move the tables')
    parser.add_argument('--safe', help='file of table names the audit cleared')
    parser.add_argument('--apply', action='store_true', help='write the files')
    arguments = parser.parse_args()

    good, bad = candidates()

    if arguments.safe:
        safe = set(open(arguments.safe).read().split())
        for name in list(good):
            if name not in safe:
                bad[name] = 'the audit saw it read out of bounds'
                del good[name]

    print('%d tables can move, %d cannot' % (len(good), len(bad)))
    if arguments.report:
        for name, table in good.items():
            print('  %-26s %3d bytes  -> %s (%s)' % (name, table['size'],
                                                     table['home'][1], table['home'][0]))
        print()
        why = collections.Counter(bad.values())
        for reason, n in why.most_common():
            print('  %3d %s' % (n, reason))
        return

    if not arguments.apply:
        print('(nothing written: pass --apply)')
        return

    rewrite(good, audit=arguments.audit)
    if arguments.inline:
        strip(good)
        print('  moved them, and took their addresses out of SMBDataPointers.hpp')
    else:
        print('  instrumented %d reads; the tables stay where they are'
              % sum(len(t['sites']) for t in good.values()))


if __name__ == '__main__':
    main()
