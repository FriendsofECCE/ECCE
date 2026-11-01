#!/usr/bin/env python3
"""ecce_contacts on hand-made geometries; the CREST ensemble part runs if
crest and xtb exist (n-butane-free: Na+(H2O)4, GFN-FF, --noreftopo)."""
import json, math, os, random, shutil, subprocess, sys, tempfile
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', '..', 'scripts'))
import ecce_contacts as C

fails = []
def check(name, cond, info=''):
    print(('ok   ' if cond else 'FAIL ') + name + (' ' + str(info) if cond is False else ''))
    if not cond: fails.append(name)

def water(o, dirn, up):
    """O at o, H's 0.96 A away, 104.5 deg, bisector along dirn (unit)."""
    h = math.radians(104.5 / 2)
    # perpendicular to dirn
    t = [up[i] - sum(up[k]*dirn[k] for k in range(3))*dirn[i] for i in range(3)]
    n = math.sqrt(sum(x*x for x in t)); t = [x/n for x in t]
    return [('O',) + tuple(o)] + [('H',) + tuple(o[i] + 0.96*(math.cos(h)*dirn[i] + s*math.sin(h)*t[i]) for i in range(3)) for s in (1, -1)]

def aq(n_dirs, r=2.4):
    # water oxygen on the ion side, H pointing away from the ion
    a = [('Na', 0., 0., 0.)]
    for d in n_dirs:
        a += water([r*x for x in d], d, [0.3, 0.5, 0.8] if abs(d[2]) < .9 else [1, 0, 0])
    return a

OCT = [(1,0,0),(-1,0,0),(0,1,0),(0,-1,0),(0,0,1),(0,0,-1)]
tet = [(1,1,1),(1,-1,-1),(-1,1,-1),(-1,-1,1)]
tet = [tuple(x/math.sqrt(3) for x in v) for v in tet]

a = aq(OCT)
check('Na(H2O)6 key', C.pattern_key(a) == 'Na: 6×O(H2O)', C.pattern_key(a))
sh = C.shells(a)[0]
check('Na(H2O)6 cn', sh['cn'] == 6 and all(f['denticity'] == 1 for f in sh['fragments']))

# Na+ with 4 monodentate EC-like O (as H2O here) + one bidentate ClO4-
def clo4(center, dirn):
    # Cl with O1,O2 toward the ion (bidentate), O3,O4 away; Na-O 2.4
    x = [center[i] for i in range(3)]
    return [('Cl',) + tuple(x),
            ('O', x[0]-1.0, x[1]+1.2, x[2]), ('O', x[0]-1.0, x[1]-1.2, x[2]),
            ('O', x[0]+1.0, x[1], x[2]+1.2), ('O', x[0]+1.0, x[1], x[2]-1.2)]
b = [('Na', 0., 0., 0.)]
for d in [(0,1,0),(0,-1,0),(0,0,1),(0,0,-1)]:
    b += water([2.4*x for x in d], d, [1, 0, 0])
b += clo4([2.4 + 1.0, 0, 0], None)   # O at x=2.4, y=+-1.2: d(Na-O)=2.68 <= 2.82
check('Na 4 O + bidentate ClO4', C.pattern_key(b) == 'Na: 4×O(H2O), 1×O2(ClO4, bidentate)', C.pattern_key(b))

# permuted atom order gives the same key
for atoms in (a, b):
    idx = list(range(len(atoms))); random.Random(1).shuffle(idx)
    check('permutation key', C.pattern_key([atoms[i] for i in idx]) == C.pattern_key(atoms))

# water dimer
d1 = water([0, 0, 0], (1, 0, 0), [0, 1, 0])
d1[1] = ('H', 0.96, 0.0, 0.0)           # H points straight at the acceptor O
d2 = water([2.85, 0, 0], (1, 0, 0), [0, 1, 0])
dimer = d1 + d2
hb = C.hbonds(dimer)
check('dimer: one H-bond', len(hb) == 1, hb)
if hb:
    check('dimer H-bond geometry', abs(hb[0]['dist_HA'] - 1.89) < 0.01 and hb[0]['angle'] > 170, hb[0])
check('dimer key', C.hbond_key(dimer, hb) == '1×O(H2O)>O(H2O)', C.hbond_key(dimer, hb))

# ensemble: hand-made multi-frame xyz, grouping and representative
tmp = tempfile.mkdtemp(prefix='contacts-')
def xyz(frames):
    s = ''
    for atoms, e, r in frames:
        s += '%d\nenergy: %.6f relative: %.3f\n' % (len(atoms), e, r)
        s += ''.join('%s %.5f %.5f %.5f\n' % t for t in atoms)
    return s
c6 = aq(OCT); c4 = aq(tet)
fr = [(c6, -1.0, 2.0), (c4, -1.0, 1.0), (c6[::-1], -1.0, 0.5), (c4, -1.0, 3.0)]
p = os.path.join(tmp, 'ens.xyz'); open(p, 'w').write(xyz(fr))
cli = [sys.executable, os.path.join(os.path.dirname(__file__), '..', '..', 'scripts', 'ecce_contacts.py')]
out = subprocess.run(cli + [p, '--group', '--format', 'json'], capture_output=True, text=True, check=True).stdout
g = json.loads(out)
check('groups', len(g) == 2 and sorted(x['count'] for x in g) == [2, 2], g)
check('representative is lowest energy', {x['pattern']: x['representative'] for x in g}['Na: 6×O(H2O)'] == 3, g)
print(subprocess.run(cli + [p], capture_output=True, text=True).stdout)

# CREST ensemble
if shutil.which('crest') and shutil.which('xtb'):
    w = os.path.join(tmp, 'crest'); os.mkdir(w)
    open(os.path.join(w, 'in.xyz'), 'w').write(xyz([(aq(tet, 2.3), 0, 0)]))
    try:
        subprocess.run(['crest', 'in.xyz', '--gfnff', '--chrg', '1', '--noreftopo', '-T', '2'],
                       cwd=w, capture_output=True, timeout=300, check=True)
        ce = os.path.join(w, 'crest_conformers.xyz')
        check('crest wrote an ensemble', os.path.exists(ce))
        r = subprocess.run(cli + [ce], capture_output=True, text=True)
        print(r.stdout); print(subprocess.run(cli + [ce, '--group'], capture_output=True, text=True).stdout)
        check('crest ensemble analysed', r.returncode == 0 and 'Na:' in r.stdout)
    except subprocess.TimeoutExpired:
        print('crest took over 5 min; skipped')
else:
    print('no crest/xtb: ensemble part skipped')
shutil.rmtree(tmp, ignore_errors=True)
sys.exit(1 if fails else 0)
