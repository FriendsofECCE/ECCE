#!/usr/bin/env python3
"""H2O/cc-pVQZ: stored density trace and signed MOs against independent Molden.

Run under Slurm after building densityplane_test and installing ECCE. Uses the
real engine and parser pipeline, with an isolated HOME and local calculation.
"""
import math
import os
from pathlib import Path
import subprocess
import sys
import tempfile

REPO = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO / 'tests/modiagram'))
sys.path.insert(0, str(REPO / 'tests/builder'))
import makecalc
import make_densitymap_fixtures as conv
import isolate
import apps

BOHR = 0.52917721092


def harmonic(l, m, xyz):
    """Normalized real spherical harmonic, with positive x/y p components."""
    x, y, z = xyz
    r = math.sqrt(x*x+y*y+z*z)
    if r == 0:
        return 1/math.sqrt(4*math.pi) if l == 0 else 0
    u = z/r
    k = abs(m)
    # Associated Legendre recurrence without the Condon-Shortley phase.
    p = 1.0
    for j in range(1, k+1):
        p *= (2*j-1)*math.sqrt(max(0.0, 1-u*u))
    if l > k:
        prev, p = p, (2*k+1)*u*p
        for j in range(k+2, l+1):
            prev, p = p, ((2*j-1)*u*p-(j+k-1)*prev)/(j-k)
    norm = math.sqrt((2*l+1)/(4*math.pi)*math.factorial(l-k)/math.factorial(l+k))
    angle = math.atan2(y, x)
    angular = 1 if m == 0 else math.sqrt(2)*(math.cos(k*angle) if m > 0 else math.sin(k*angle))
    return r**l*norm*p*angular


def read_molden(path):
    """Read atoms, contracted spherical shells and occupied coefficients only."""
    lines = path.read_text().splitlines()
    assert '[5D]' in lines and '[7F]' in lines and '[9G]' in lines
    atoms = []
    i = next(j for j, s in enumerate(lines) if s.startswith('[Atoms]'))+1
    while not lines[i].startswith('['):
        atoms.append(tuple(float(v)/BOHR for v in lines[i].split()[3:6]))
        i += 1
    shells = []
    i = lines.index('[GTO]')+1
    while lines[i] != '[MO]':
        fields = lines[i].split()
        i += 1
        if not fields:
            continue
        if fields[0].isdigit():
            atom = int(fields[0])-1
            continue
        l = 'spdfghi'.index(fields[0].lower())
        primitives = [tuple(float(v.replace('D', 'E')) for v in line.split()[:2])
                      for line in lines[i:i+int(fields[1])]]
        i += int(fields[1])
        cnorm = sum(c*d*(2*math.sqrt(a*b)/(a+b))**(l+1.5)
                    for a, c in primitives for b, d in primitives)**-0.5
        radial = [(a, c*cnorm*math.sqrt(2*(2*a)**(l+1.5)/math.gamma(l+1.5)))
                  for a, c in primitives]
        # Molden uses x,y,z for p; 0,+1,-1,+2,-2,... for d and higher.
        order = [1, -1, 0] if l == 1 else [m for k in range(l+1) for m in ([0] if k == 0 else [k, -k])]
        shells.append((atoms[atom], l, order, radial))
    orbitals, coefficients, occ = [], [], 0
    for line in lines[i+1:]+['Sym= end']:
        if line.startswith('Sym='):
            if coefficients and occ > 0:
                orbitals.append(coefficients)
            coefficients, occ = [], 0
        elif line.startswith('Occup='):
            occ = float(line.split('=')[1])
        elif line.split() and line.split()[0].isdigit():
            index, value = line.split()
            assert int(index) == len(coefficients)+1
            coefficients.append(float(value))
    assert len(orbitals) == 5
    assert any(l == 4 for _, l, _, _ in shells), 'molecular check must include g'
    return shells, orbitals


def values(shells, orbitals, point):
    basis = []
    for centre, l, order, radial in shells:
        xyz = tuple(p/BOHR-c for p, c in zip(point, centre))
        r2 = sum(x*x for x in xyz)
        contraction = sum(c*math.exp(-a*r2) for a, c in radial)
        basis.extend(contraction*harmonic(l, m, xyz) for m in order)
    assert all(len(c) == len(basis) for c in orbitals)
    return [sum(c*b for c, b in zip(row, basis)) for row in orbitals]


def main():
    build = REPO / 'build-cmake'
    qm = build / 'src/qm/ecce-qm'
    probe = build / 'densityplane_test'
    if not qm.exists() or not probe.exists():
        print('SKIP: build ecce-qm and densityplane_test first')
        return 77
    isolate.apply(apps.INSTALL)
    with tempfile.TemporaryDirectory(prefix='molecular-density-', dir=build) as folder:
        work = Path(folder)
        env = dict(os.environ, HOME=folder, ECCE_REALUSERHOME=folder,
                   ECCE_HOME=str(REPO), ECCE_BASIS_DIR=str(REPO / 'data/admin/basissets'),
                   OMP_NUM_THREADS='2', OPENBLAS_NUM_THREADS='1', TMPDIR=folder)
        os.environ.update(env)
        molden = work / 'water.molden'

        def run_code(code, directory, deck, output):
            subprocess.run([str(qm), '--molden', str(molden), '-o', output, deck],
                           cwd=directory, env=env, check=True, timeout=300,
                           stdout=subprocess.DEVNULL)

        # Reuse the existing engine -> monitor -> parser -> stored-basis tooling.
        makecalc.runCode = run_code
        old_args = sys.argv
        sys.argv = ['makecalc.py', 'ecceqm', 'h2o', '0', str(work / 'raw'),
                    '--basis', 'cc-pVQZ', '--method', 'hf', '--spherical-flag']
        try:
            makecalc.main()
        finally:
            sys.argv = old_args
        conv.SRC = folder
        calc = work / 'water'
        conv.convert('water', 'raw', str(calc))
        shells, orbitals = read_molden(molden)
        # Off-axis points make component permutations and phases observable.
        points = [(0.31, -0.27, 0.42), (-0.62, 0.19, 0.35),
                  (0.14, 0.73, -0.46), (0.85, -0.51, -0.23)]
        point_file = work / 'points.txt'
        point_file.write_text(''.join('%s %s %s\n' % p for p in points))
        result = subprocess.run([str(probe), '--molecular-probe', str(calc), str(point_file)],
                                env=env, text=True, capture_output=True, timeout=180)
        print(result.stdout, end='')
        if result.returncode:
            print(result.stderr)
            print('FAIL molecular flattened-basis density trace')
            return 1
        electrons = float(next(s.split()[1] for s in result.stdout.splitlines()
                               if s.startswith('electrons ')))
        assert abs(electrons-10) <= 1e-6
        print('PASS molecular density: sum occupied n*c^T*S*c = %.12f (tolerance 1e-6)' % electrons)
        actual = [float(s.split()[2]) for s in result.stdout.splitlines() if s.startswith('orbital ')]
        expected = [v for p in points for v in values(shells, orbitals, p)]
        assert len(actual) == len(expected)
        permuted = [(c, l, list(reversed(order)) if l == 4 else order, radial)
                    for c, l, order, radial in shells]
        wrong = [v for p in points for v in values(permuted, orbitals, p)]
        assert max(abs(a-b) for a, b in zip(expected, wrong)) > 1e-6, \
            'chosen molecular points must detect a wrong g component order'
        worst = max(abs(a-b) for a, b in zip(actual, expected))
        # Molden coefficients are printed to eight decimal places.
        assert worst < 2e-7, 'signed molecular MO error %.9g' % worst
        print('PASS occupied molecular orbitals: independent Molden values, max error %.3g' % worst)
    return 0


if __name__ == '__main__':
    sys.exit(main())
