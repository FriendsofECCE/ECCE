#!/usr/bin/env python3
"""ecce_displacement: Kabsch (proper rotation), per-element displacement and
the CLI on two stored calculations (#701)."""
import json
import math
import os
import subprocess
import sys
import tempfile

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', '..',
                                'scripts'))
import ecce_displacement as D

fails = []


def check(name, cond, info=''):
    print(('ok   ' if cond else 'FAIL ') + name +
          ((' ' + str(info)) if not cond else ''))
    if not cond:
        fails.append(name)


def rot(axis, ang):
    """Rotation matrix about a unit axis by ang radians (Rodrigues)."""
    x, y, z = axis
    c, s = math.cos(ang), math.sin(ang)
    t = 1 - c
    return [[c + x * x * t, x * y * t - z * s, x * z * t + y * s],
            [y * x * t + z * s, c + y * y * t, y * z * t - x * s],
            [z * x * t - y * s, z * y * t + x * s, c + z * z * t]]


def mm(m, v):
    return [sum(m[i][k] * v[k] for k in range(3)) for i in range(3)]


# A chiral (non-planar) set, so a mirror image cannot be a proper rotation.
MOL = [(0.0, 0.0, 0.0), (1.1, 0.2, 0.0), (0.1, 1.0, 0.1), (0.0, 0.1, 0.9)]

# 1. A rotated + translated copy fits to RMSD 0.
R = rot((0.3, 0.8, 0.5), 0.7)  # not a unit axis exactly; normalise below
n = math.sqrt(sum(x * x for x in (0.3, 0.8, 0.5)))
R = rot((0.3 / n, 0.8 / n, 0.5 / n), 0.7)
t = (2.0, -1.0, 0.5)
moved = [tuple(mm(R, v)[k] + t[k] for k in range(3)) for v in MOL]
rmsd, aligned = D.kabsch(MOL, moved)
check('rotated+translated copy: RMSD 0', abs(rmsd) < 1e-8, rmsd)
check('rotated+translated copy: aligned coordinates',
      all(all(abs(aligned[i][k] - MOL[i][k]) < 1e-7 for k in range(3))
          for i in range(len(MOL))), aligned)

# 2. A reflection is not reported as 0 (proper rotation only), and the
# routine returns the optimal proper rotation, not the worst one: the
# no-rotation RMSD (1.105 A) is itself a valid proper rotation, so the
# optimum must beat it (Horn's most-negative eigenvalue, larger in magnitude
# for a mirror, would give the worst rotation, 1.434 A).  The optimum is the
# closed-form Kabsch value sqrt((sum|p|^2 + sum|q|^2 - 2*(s1+s2-s3)) / n).
mirror = [(-v[0], v[1], v[2]) for v in MOL]
rmsd, _ = D.kabsch(MOL, mirror)
no_rot = math.sqrt(sum(sum((MOL[i][k] - mirror[i][k]) ** 2 for k in range(3))
                       for i in range(len(MOL))) / len(MOL))
check('mirror image: RMSD not 0', rmsd > 1e-3, rmsd)
check('mirror image: optimal, not the worst, rotation', rmsd < no_rot,
      (rmsd, no_rot))
check('mirror image: known optimal RMSD', abs(rmsd - 0.562792) < 1e-4, rmsd)

# 3. A known displacement gives the known per-element numbers.
a = [(0.0, 0.0, 0.0), (1.0, 0.0, 0.0)]
b = [(0.0, 0.0, 0.0), (1.0, 0.0, 0.4)]
d = D.per_element(a, b)
check('known per-element displacement', abs(d[0]['displacement']) < 1e-12 and
      abs(d[1]['displacement'] - 0.4) < 1e-12 and
      d[1]['dz'] == 0.4, d)

# 4. Trace statistics: atom 0 goes 5 A then back; atom 1 stays put.
frames = [[(0.0, 0.0, 0.0), (1.0, 1.0, 1.0)],
          [(3.0, 4.0, 0.0), (1.0, 1.0, 1.0)],
          [(0.0, 0.0, 0.0), (1.0, 1.0, 1.0)]]
s = D.trace_stats(frames)
check('trace max/rms', abs(s[0]['max'] - 5.0) < 1e-12 and
      abs(s[0]['rms'] - math.sqrt(25.0 / 2)) < 1e-12 and
      s[1]['max'] == 0.0 and s[1]['rms'] == 0.0, s)


def write_calc(tmp, name, geom, elements):
    calc = os.path.join(tmp, name)
    os.makedirs(os.path.join(calc, 'Props'))
    os.makedirs(os.path.join(calc, 'Parameters'))
    n = len(elements)
    frames = ''.join('<step number="%d">%s</step>\n' % (
        i + 1, ' '.join('%.10e %.10e %.10e' % g for g in fr))
        for i, fr in enumerate(geom))
    open(os.path.join(calc, 'Props', 'GEOMTRACE'), 'w').write(
        '<?xml version="1.0"?>\n<tsvectable columnLabels="X Y Z" columns="3" '
        'name="GEOMTRACE" rowLabel="Geometry Step" rowLabels="%s" rows="%d" '
        'units="Angstrom">\n%s</tsvectable>\n'
        % (' '.join(str(i) for i in range(n)), n, frames))
    open(os.path.join(calc, 'Parameters', 'chemsys.mvm'), 'w').write(
        'type: molecule\nnum_atoms: %d\natom_info: symbol cart\natom_list: \n'
        % n + ''.join('%s 0 0 0\n' % e for e in elements))
    return calc


# 5. The CLI on two stored calculations prints JSON with the expected keys.
tmp = tempfile.mkdtemp(prefix='displacement-')
try:
    g1 = [(0.0, 0.0, 0.0), (1.0, 0.0, 0.0), (0.0, 1.0, 0.0), (0.0, 0.0, 1.0)]
    g2 = [tuple(mm(R, v)[k] + t[k] for k in range(3)) for v in g1]
    c1 = write_calc(tmp, 'a', [g1], ['C', 'H', 'H', 'O'])
    c2 = write_calc(tmp, 'b', [g2], ['C', 'H', 'H', 'O'])
    cli = [sys.executable, os.path.join(os.path.dirname(__file__), '..', '..',
                                        'scripts', 'ecce_displacement.py')]
    r = subprocess.run(cli + [c1, c2, '--json'],
                       capture_output=True, text=True)
    check('CLI two calcs exit 0', r.returncode == 0, r.stderr)
    j = json.loads(r.stdout)
    check('CLI JSON keys', all(k in j for k in ('rmsd', 'n_atoms', 'atoms')),
          list(j))
    check('CLI rmsd is the rotation fit', j['rmsd'] < 1e-8, j['rmsd'])
    check('CLI per-atom rows', len(j['atoms']) == 4 and
          all('element' in x and 'displacement' in x and 'index' in x
              for x in j['atoms']), j['atoms'])
    # --select restricts both the fit and the table.
    r = subprocess.run(cli + [c1, c2, '--json', '--select', '1,4'],
                       capture_output=True, text=True)
    j2 = json.loads(r.stdout)
    check('CLI selection', j2['n_atoms'] == 2 and j2['selection'] == [1, 4],
          j2)
    # one calculation: the geometry trace.
    frames = [g1, [(0.0, 0.0, 0.0), (1.0, 0.0, 0.0), (0.0, 1.0, 0.0),
                   (0.0, 0.0, 0.7)]]
    c3 = write_calc(tmp, 'c', frames, ['C', 'H', 'H', 'O'])
    r = subprocess.run(cli + [c3, '--json'], capture_output=True, text=True)
    j3 = json.loads(r.stdout)
    check('CLI one calc trace keys', all(k in j3 for k in ('frames', 'atoms')) and
          j3['frames'] == 2, j3)
    check('CLI one calc atom 4 moved 0.3',
          any(x['index'] == 4 and abs(x['max'] - 0.3) < 1e-6
              for x in j3['atoms']), j3['atoms'])
finally:
    import shutil
    shutil.rmtree(tmp, ignore_errors=True)

sys.exit(1 if fails else 0)
