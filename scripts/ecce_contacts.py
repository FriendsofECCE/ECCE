#!/usr/bin/env python3
"""Coordination shells, hydrogen bonds and pattern keys for a structure or an
ensemble (#693, #708).  Wx-free: the panels and the CLI only display this.

Usage: ecce_contacts.py [--format text|csv|json] [--hbonds] [--group]
                        [--cutoff-extra A] FILE.xyz|FILE.pdb
"""
import argparse
import csv
import io
import json
import math
import re
import sys
from collections import Counter, defaultdict

# Cordero et al. 2008 covalent radii (Angstrom).
RCOV = {
    'H': .31, 'He': .28, 'Li': 1.28, 'Be': .96, 'B': .84, 'C': .76, 'N': .71,
    'O': .66, 'F': .57, 'Ne': .58, 'Na': 1.66, 'Mg': 1.41, 'Al': 1.21,
    'Si': 1.11, 'P': 1.07, 'S': 1.05, 'Cl': 1.02, 'Ar': 1.06, 'K': 2.03,
    'Ca': 1.76, 'Sc': 1.70, 'Ti': 1.60, 'V': 1.53, 'Cr': 1.39, 'Mn': 1.39,
    'Fe': 1.32, 'Co': 1.26, 'Ni': 1.24, 'Cu': 1.32, 'Zn': 1.22, 'Ga': 1.22,
    'Ge': 1.20, 'As': 1.19, 'Se': 1.20, 'Br': 1.20, 'Kr': 1.16, 'Rb': 2.20,
    'Sr': 1.95, 'Y': 1.90, 'Zr': 1.75, 'Mo': 1.54, 'Ru': 1.46, 'Rh': 1.42,
    'Pd': 1.39, 'Ag': 1.45, 'Cd': 1.44, 'In': 1.42, 'Sn': 1.39, 'Sb': 1.39,
    'Te': 1.38, 'I': 1.39, 'Xe': 1.40, 'Cs': 2.44, 'Ba': 2.15, 'La': 2.07,
    'Pt': 1.36, 'Au': 1.36, 'Hg': 1.32, 'Pb': 1.46, 'Bi': 1.48,
}
METALS = {'Li', 'Be', 'Na', 'Mg', 'Al', 'K', 'Ca', 'Sc', 'Ti', 'V', 'Cr', 'Mn',
          'Fe', 'Co', 'Ni', 'Cu', 'Zn', 'Ga', 'Rb', 'Sr', 'Y', 'Zr', 'Mo', 'Ru',
          'Rh', 'Pd', 'Ag', 'Cd', 'In', 'Sn', 'Cs', 'Ba', 'La', 'Pt', 'Au',
          'Hg', 'Pb', 'Bi'}
DONORS = {'N', 'O', 'F', 'P', 'S', 'Cl', 'Se', 'Br', 'I'}
HB_DONORS = {'N', 'O', 'F', 'S'}
HB_ACCEPTORS = {'N', 'O', 'F', 'S', 'Cl', 'Br'}

# Ion-donor cutoff = r_cov(M) + r_cov(X) + 0.5.  The sum alone is 0.2-0.4 A
# short of real ion-water distances (Na-O 2.3-2.5 vs 2.32); 0.5 keeps the
# first shell of Li..Cs and stays below the second shell (about 4 A).
SHELL_TOL = 0.5
# Covalent bonds inside a ligand: r_i + r_j + 0.4 (usual perception rule).
BOND_TOL = 0.4
# H...A <= 2.01 A is the Builder's limit (HydrogenBondsCmd: 2.01 A for O).
# Its angle test (>= 60 deg at H) accepts nearly everything, so the angle
# criterion here is D-H...A >= 120 deg.
HB_MAX_HA = 2.01
HB_MIN_ANGLE = 120.0


def _el(s):
    s = re.sub(r'[^A-Za-z]', '', s)
    s = s[:1].upper() + s[1:2].lower()
    if s not in RCOV and s[:1] in RCOV:
        s = s[:1]
    return s


def read_frames(path):
    """Return a list of frames: {'atoms': [(el, x, y, z)], 'energy', 'rel', 'comment'}."""
    text = open(path).read()
    if path.lower().endswith('.pdb'):
        return _read_pdb(text)
    lines = text.splitlines()
    frames, i = [], 0
    while i < len(lines):
        if not lines[i].strip():
            i += 1
            continue
        n = int(lines[i].split()[0])
        comment = lines[i + 1] if i + 1 < len(lines) else ''
        atoms = []
        for ln in lines[i + 2:i + 2 + n]:
            f = ln.split()
            atoms.append((_el(f[0]), float(f[1]), float(f[2]), float(f[3])))
        energy = rel = None
        m = re.search(r'energy:\s*(\S+)\s+relative:\s*(\S+)', comment)
        if m:
            energy, rel = float(m.group(1)), float(m.group(2))
        else:
            m = re.match(r'\s*(-?\d+\.\d+)', comment)
            if m:
                energy = float(m.group(1))
        frames.append({'atoms': atoms, 'energy': energy, 'rel': rel,
                       'comment': comment.strip()})
        i += 2 + n
    # Relative energy (kcal/mol) from Eh where the file gave only energies.
    es = [f['energy'] for f in frames if f['energy'] is not None]
    if es:
        e0 = min(es)
        for f in frames:
            if f['rel'] is None and f['energy'] is not None:
                f['rel'] = (f['energy'] - e0) * 627.5095
    return frames


def _read_pdb(text):
    frames, cur = [], []
    for ln in text.splitlines():
        if ln.startswith(('ATOM', 'HETATM')):
            el = ln[76:78].strip() or ln[12:16]
            cur.append((_el(el), float(ln[30:38]), float(ln[38:46]),
                        float(ln[46:54])))
        elif ln.startswith('ENDMDL') and cur:
            frames.append({'atoms': cur, 'energy': None, 'rel': None,
                           'comment': ''})
            cur = []
    if cur:
        frames.append({'atoms': cur, 'energy': None, 'rel': None, 'comment': ''})
    return frames


def _dist(a, b):
    return math.dist(a[1:], b[1:])


def _angle(a, b, c):
    """Angle a-b-c in degrees."""
    u = [a[k] - b[k] for k in (1, 2, 3)]
    v = [c[k] - b[k] for k in (1, 2, 3)]
    nu, nv = math.hypot(*u), math.hypot(*v)
    cs = sum(x * y for x, y in zip(u, v)) / (nu * nv)
    return math.degrees(math.acos(max(-1.0, min(1.0, cs))))


def _formula(atoms, idx):
    c = Counter(atoms[i][0] for i in idx)
    order = (['C', 'H'] if 'C' in c else []) + sorted(
        e for e in c if e not in ('C', 'H') or 'C' not in c)
    return ''.join(e + (str(c[e]) if c[e] > 1 else '') for e in order)


def fragments(atoms):
    """Connected components of the non-metal atoms (covalent bonds only)."""
    n = len(atoms)
    ids = [i for i in range(n) if atoms[i][0] not in METALS]
    seen, out = set(), []
    for s in ids:
        if s in seen:
            continue
        comp, stack = [], [s]
        seen.add(s)
        while stack:
            i = stack.pop()
            comp.append(i)
            for j in ids:
                if j not in seen and _dist(atoms[i], atoms[j]) <= (
                        RCOV.get(atoms[i][0], 1.5) + RCOV.get(atoms[j][0], 1.5)
                        + BOND_TOL):
                    seen.add(j)
                    stack.append(j)
        out.append(sorted(comp))
    return out


def shells(atoms, extra=0.0):
    """First shell of every metal atom.  Returns a list of dicts:
    ion, ion_index, cn, fragments=[{formula, donors=[(idx, el, d)], denticity}]."""
    frags = fragments(atoms)
    frag_of = {i: k for k, f in enumerate(frags) for i in f}
    res = []
    for m, a in enumerate(atoms):
        if a[0] not in METALS:
            continue
        per = defaultdict(list)
        for j, b in enumerate(atoms):
            if b[0] not in DONORS:
                continue
            d = _dist(a, b)
            if d <= RCOV[a[0]] + RCOV.get(b[0], 1.5) + SHELL_TOL + extra:
                per[frag_of[j]].append((j, b[0], round(d, 3)))
        fl = []
        for k, dn in sorted(per.items()):
            dn.sort(key=lambda t: t[2])
            fl.append({'formula': _formula(atoms, frags[k]), 'donors': dn,
                       'denticity': len(dn)})
        res.append({'ion': a[0], 'ion_index': m,
                    'cn': sum(f['denticity'] for f in fl), 'fragments': fl})
    return res


_DENT = {1: '', 2: 'bidentate', 3: 'tridentate', 4: 'tetradentate'}


def shell_key(sh):
    """"Na: 4×O(H2O), 2×O(ClO4, bidentate)": n×donor elements(ligand[, denticity]),
    where n counts ligand molecules of that kind; independent of atom order."""
    c = Counter()
    for f in sh['fragments']:
        els = Counter(e for _, e, _ in f['donors'])
        don = ','.join(e + (str(n) if n > 1 else '') for e, n in sorted(els.items()))
        c[(don, f['formula'], f['denticity'])] += 1
    toks = []
    for (don, form, dent), n in sorted(c.items()):
        extra = _DENT.get(dent, '%d-dentate' % dent)
        toks.append('%d×%s(%s%s)' % (n, don, form, ', ' + extra if extra else ''))
    return '%s: %s' % (sh['ion'], ', '.join(toks) if toks else 'no contacts')


def hbonds(atoms):
    """D-H...A list: dict(donor, h, acceptor, dist_HA, dist_DA, angle).
    Each H belongs to its nearest heavy atom within covalent range."""
    out = []
    heavy = [i for i, a in enumerate(atoms) if a[0] != 'H' and a[0] not in METALS]
    for h, a in enumerate(atoms):
        if a[0] != 'H':
            continue
        par = [i for i in heavy if _dist(a, atoms[i]) <= RCOV['H'] +
               RCOV.get(atoms[i][0], 1.5) + BOND_TOL]
        if not par:
            continue
        dn = min(par, key=lambda i: _dist(a, atoms[i]))
        if atoms[dn][0] not in HB_DONORS:
            continue
        for acc in heavy:
            if acc == dn or atoms[acc][0] not in HB_ACCEPTORS:
                continue
            d = _dist(a, atoms[acc])
            if d > HB_MAX_HA:
                continue
            ang = _angle(atoms[dn], a, atoms[acc])
            if ang < HB_MIN_ANGLE:
                continue
            # Skip the two other atoms of the same ligand (H2O's own O).
            out.append({'donor': dn, 'h': h, 'acceptor': acc,
                        'dist_HA': round(d, 3),
                        'dist_DA': round(_dist(atoms[dn], atoms[acc]), 3),
                        'angle': round(ang, 1)})
    return out


def hbond_key(atoms, hb):
    frags = fragments(atoms)
    frag_of = {i: k for k, f in enumerate(frags) for i in f}
    lab = [_formula(atoms, f) for f in frags]
    c = Counter()
    for b in hb:
        kd, ka = frag_of[b['donor']], frag_of[b['acceptor']]
        if kd == ka:
            continue  # intramolecular contacts are not a ligand pattern
        c[(lab[kd], atoms[b['donor']][0], lab[ka], atoms[b['acceptor']][0])] += 1
    return ', '.join('%d×%s(%s)>%s(%s)' % (n, a, b, c_, d)
                     for (b, a, d, c_), n in sorted(c.items()))


def pattern_key(atoms, with_hbonds=False):
    shs = shells(atoms)
    key = '; '.join(sorted(shell_key(s) for s in shs)) if shs else 'no metal'
    if with_hbonds:
        hk = hbond_key(atoms, hbonds(atoms))
        key += ' | HB ' + (hk or 'none')
    return key


def analyse(frames, with_hbonds=False):
    rows = []
    for i, f in enumerate(frames):
        rows.append({'index': i + 1, 'energy_Eh': f['energy'],
                     'relative_kcal': None if f['rel'] is None else round(f['rel'], 3),
                     'pattern': pattern_key(f['atoms'], with_hbonds)})
    return rows


def group(rows):
    """One row per pattern: lowest energy member (first one if no energies),
    plus the count and the member indices."""
    by = defaultdict(list)
    for r in rows:
        by[r['pattern']].append(r)
    out = []
    for p, rs in by.items():
        rep = min(rs, key=lambda r: (r['relative_kcal'] is None,
                                     r['relative_kcal'] or 0, r['index']))
        out.append({'pattern': p, 'count': len(rs), 'representative': rep['index'],
                    'relative_kcal': rep['relative_kcal'],
                    'members': [r['index'] for r in rs]})
    out.sort(key=lambda g: (g['relative_kcal'] is None, g['relative_kcal'] or 0))
    return out


def _fmt(rows, fmt, cols):
    if fmt == 'json':
        return json.dumps(rows, indent=1)
    if fmt == 'csv':
        s = io.StringIO()
        w = csv.writer(s, lineterminator='\n')
        w.writerow(cols)
        for r in rows:
            w.writerow(['' if r[c] is None else
                        (' '.join(map(str, r[c])) if isinstance(r[c], list) else r[c])
                        for c in cols])
        return s.getvalue().rstrip('\n')
    lines = ['\t'.join(cols)]
    for r in rows:
        lines.append('\t'.join('' if r[c] is None else
                               (','.join(map(str, r[c])) if isinstance(r[c], list)
                                else str(r[c])) for c in cols))
    return '\n'.join(lines)


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('file')
    ap.add_argument('--format', choices=('text', 'csv', 'json'), default='text')
    ap.add_argument('--hbonds', action='store_true',
                    help='list hydrogen bonds; in an ensemble add them to the key')
    ap.add_argument('--group', action='store_true',
                    help='ensemble: one representative per pattern with its count')
    ap.add_argument('--cutoff-extra', type=float, default=0.0,
                    help='Angstrom added to the ion-donor cutoff')
    a = ap.parse_args(argv)
    frames = read_frames(a.file)
    if not frames:
        sys.exit('no structure in %s' % a.file)
    if len(frames) > 1 or a.group:
        rows = analyse(frames, a.hbonds)
        if a.group:
            print(_fmt(group(rows), a.format,
                       ['pattern', 'count', 'representative', 'relative_kcal',
                        'members']))
        else:
            print(_fmt(rows, a.format,
                       ['index', 'energy_Eh', 'relative_kcal', 'pattern']))
        return 0
    atoms = frames[0]['atoms']
    rows = []
    for s in shells(atoms, a.cutoff_extra):
        for f in s['fragments']:
            for idx, el, d in f['donors']:
                rows.append({'ion': '%s%d' % (s['ion'], s['ion_index'] + 1),
                             'ligand': f['formula'], 'denticity': f['denticity'],
                             'donor': '%s%d' % (el, idx + 1), 'distance': d,
                             'pattern': shell_key(s)})
    if a.format == 'json':
        out = {'shells': rows, 'pattern': pattern_key(atoms)}
        if a.hbonds:
            out['hbonds'] = hbonds(atoms)
        print(json.dumps(out, indent=1))
        return 0
    print(_fmt(rows, a.format, ['ion', 'ligand', 'denticity', 'donor',
                                'distance', 'pattern']))
    if a.hbonds:
        hb = [{'donor': '%s%d' % (atoms[b['donor']][0], b['donor'] + 1),
               'h': 'H%d' % (b['h'] + 1),
               'acceptor': '%s%d' % (atoms[b['acceptor']][0], b['acceptor'] + 1),
               'dist_HA': b['dist_HA'], 'dist_DA': b['dist_DA'],
               'angle': b['angle']} for b in hbonds(atoms)]
        print()
        print(_fmt(hb, a.format, ['donor', 'h', 'acceptor', 'dist_HA',
                                  'dist_DA', 'angle']))
    return 0


if __name__ == '__main__':
    sys.exit(main())
