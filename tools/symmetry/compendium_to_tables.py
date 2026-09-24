"""Convert the 5KE195 compendium's character tables into ECCE's format.

Parsed rather than retyped: forty-two tables is far too much hand
transcription to trust, and the source is the authoritative one Andy
teaches from.  The orthogonality checks in tests/symmetry then verify
every table independently of this parser.
"""
import re, sys, math

SRC = '/home/andy/pCloudDrive/5KE195_compendium/appendix.tex'
text = open(SRC).read()

#  Strip LaTeX comments FIRST.  Several tables carry a "% 10 character
#  columns" note on the line before the header row, and without this the
#  comment is read as the group name and the whole table is dropped --
#  silently, which is how D4h, D5h and others went missing on the first
#  run with no entry in either the parsed or the skipped list.
text = re.sub(r'(?m)(?<!\\)%.*$', '', text)

def clean(s):
    """LaTeX -> a plain token."""
    s = s.strip()
    s = s.replace('\\,', ' ').replace('\;', ' ').replace('\\!', '')
    s = re.sub(r'\\t?frac\{1\}\{2\}', 'HALF', s)
    s = s.replace('\\sigma', 's').replace('\\eps', 'E').replace('\\epsilon', 'E')
    s = s.replace('\\sqrt', 'sqrt')
    s = re.sub(r'[{}$]', '', s)
    s = s.replace('\\', '')
    return s.strip()

def value(tok):
    """A character as a float, or None if it is not numeric."""
    t = clean(tok).replace(' ', '')
    if t in ('', '-'): return None
    try:
        return float(t)
    except ValueError:
        pass
    #  HALF(1+sqrt5) and friends
    m = re.match(r'^HALF\(([-+]?\d*)([-+])sqrt(\d+)\)$', t)
    if m:
        a = float(m.group(1) or 1)
        r = math.sqrt(float(m.group(3)))
        return 0.5*(a + (r if m.group(2) == '+' else -r))
    m = re.match(r'^([-+]?)sqrt(\d+)$', t)
    if m:
        v = math.sqrt(float(m.group(2)))
        return -v if m.group(1) == '-' else v
    #  '2cos72^circ' and friends -- the five-fold groups and the
    #  icosahedral ones are full of these.
    m = re.match(r'^([-+]?[\d.]*)cos(\d+)(\^?circ)?$', t)
    if m:
        a = m.group(1)
        a = float(a) if a not in ('', '+', '-') else (-1.0 if a == '-' else 1.0)
        return a*math.cos(math.radians(float(m.group(2))))
    return None

def classInfo(tok):
    """'2C_6' -> ('C6', 2);  '3s_d' -> ('sd', 3);  'E' -> ('E', 1)."""
    t = clean(tok)
    m = re.match(r'^(\d*)\s*(.*)$', t)
    n = int(m.group(1)) if m.group(1) else 1
    name = m.group(2).replace('_', '').replace('^', 'p').replace(' ', '')
    #  Drop an explanatory parenthetical: the table writes the principal
    #  two-fold of D4 and Oh as "C_2(=C_4^2)".  Keeping it made the name
    #  "C2(=C4p2)", whose digits joined to order 242 -- so that class's
    #  Cartesian character came out +3 instead of -1 and the whole
    #  reduction went fractional.
    name = re.sub(r'\(.*?\)', '', name)
    return name, n

blocks = re.findall(r'\\begin\{array\}\{[^}]*\}(.*?)\\end\{array\}', text, re.S)
out = []
skipped = []

for b in blocks:
    if '\\hline' not in b:
        continue
    head, _, body = b.partition('\\hline')
    headCells = [c for c in head.split('&')]
    group = clean(headCells[0]).replace('_', '').replace(' ', '').upper()
    if not group or not re.match(r'^[A-Z][A-Z0-9]*$', group):
        continue

    classes, counts = [], []
    for c in headCells[1:]:
        name, n = classInfo(c)
        if not name or name in ('',):
            continue
        #  The trailing basis-function columns are empty in the header.
        classes.append(name); counts.append(n)

    rows, order = [], []
    complexPair = False
    for line in body.split('\\\\'):
        cells = line.split('&')
        if len(cells) < 2: continue
        label = clean(cells[0]).replace('_', '').replace(' ', '')
        if not label or not re.match(r'^[A-Za-z]', label): continue
        if 'matrix' in line: complexPair = True
        vals = []
        for c in cells[1:1+len(classes)]:
            vals.append(value(c))
        if len(vals) != len(classes) or any(v is None for v in vals):
            continue
        rows.append((label, vals)); order.append(label)
        #  Basis functions, for the translations line.
        rest = ' '.join(cells[1+len(classes):])
        rows[-1] = (label, vals, clean(rest))

    if not rows or not classes:
        skipped.append((group, 'unparsed rows' if not rows else 'no classes'))
        continue
    h = sum(counts)
    if len(rows) != len(classes):
        skipped.append((group, '%d irreps for %d classes%s'
                        % (len(rows), len(classes),
                           ' (complex pairs)' if complexPair else '')))
        continue
    out.append((group, h, classes, counts, rows))

print("parsed  : %d groups" % len(out))
print("skipped : %d" % len(skipped))
for g, why in skipped: print("   %-6s %s" % (g, why))
print()
print("groups  :", " ".join(g for g, *_ in out))

# ---- emit -------------------------------------------------------------
def fmt(v):
    r = round(v)
    return str(int(r)) if abs(v - r) < 1e-9 else ("%.9f" % v).rstrip('0')

def translations(rows):
    """Which irreps x, y and z span, from the basis-function column."""
    got = []
    for label, vals, basis in rows:
        b = ' ' + basis + ' '
        n = 0
        #  A row carries (x,y) as a pair, or x / y / z singly.
        if re.search(r'\(x,y\)', b): n += 2
        else:
            for ax in ('x', 'y'):
                if re.search(r'[\s,(]%s[\s,)]' % ax, b): n += 1
        #  Whitespace counts as a delimiter: the basis cells are joined
        #  with spaces, so a bare "z" sits between a space and a space,
        #  and a comma-only rule missed it in every Cnv group.
        if re.search(r'[\s,(]z[\s,)]', b): n += 1
        #  n counts BASIS FUNCTIONS; the multiplicity is that divided by
        #  the irrep's dimension.  (x,y) sitting in a two-dimensional E
        #  is ONE E, not two -- which is what "A2u + 2E1u" was saying.
        if n:
            dim = int(round(vals[0]))
            got.append((label, n // dim if dim else n))
    return " + ".join(l if n == 1 else "%d%s" % (n, l) for l, n in got)

lines = []
for group, h, classes, counts, rows in out:
    lines.append("[%s] h=%d" % (group, h))
    lines.append("classes: " + " ".join(classes))
    lines.append("counts:  " + " ".join(str(c) for c in counts))
    for label, vals, _ in rows:
        lines.append("%s: %s" % (label, " ".join(fmt(v) for v in vals)))
    t = translations(rows)
    #  C1's table has no basis-function column to read, being one row of
    #  one character; x, y and z all span its only irrep.
    if not t and group == 'C1':
        t = "3A"
    if t:
        lines.append("translations: " + t)
    lines.append("")

open(sys.argv[1] if len(sys.argv) > 1 else '/dev/stdout', 'w').write("\n".join(lines))
