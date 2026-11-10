#!/usr/bin/env python3
"""results.h5 vibrations and /trace against the programs' own output (#690, #883).

    tests/resultsh5/trace_vib_test.py <ecce-results> [<ecce-qm>]

Stored example calculations get results.h5 from `ecce-results write`; the
numbers in it are compared with the raw program output, read here with a few
lines of independent parsing (not ECCE's parsers):
  ORCA benzene opt+freq: frequencies, normal modes, IR intensities, and the
    optimisation trace (energies, coordinates);
  ORCA H2 relaxed scan: scan values and energies from ORCA's surface table;
  ECCE-QM butane torsion scan: scan values, energies, structures.
With ecce-qm: the water opt+freq run again with --results; the file ECCE-QM
writes is compared with the one written from the parsed properties, and the
job store must keep ECCE-QM's file.
"""
import gzip
import math
import os
import re
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
EX = os.path.join(ROOT, "data", "client", "examples")
BOHR = 1 / 0.52917724924        # bohr per Angstrom, as results.h5 is written
fails = 0


def ok(cond, what):
    global fails
    print(("ok:   " if cond else "FAIL: ") + what)
    if not cond:
        fails += 1


def dump(cli, h5, env):
    out = subprocess.run([cli, "dump", h5], env=env, capture_output=True, text=True, check=True).stdout
    d = {}
    for line in out.splitlines():
        w = line.split()
        if len(w) >= 2 and re.match(r"^[0-9]+$", w[1]) and len(w) == int(w[1]) + 2:
            d[w[0]] = [float(x) for x in w[2:]]
        elif w:
            d[w[0]] = line[len(w[0]):].strip()
    return d


def write(cli, stored, tmp, env, name):
    calc = os.path.join(tmp, name)
    shutil.copytree(stored, calc, ignore=shutil.ignore_patterns(".DAV"))
    out = os.path.join(tmp, name + ".h5")
    r = subprocess.run([cli, "write", calc, out], env=env, capture_output=True, text=True)
    print(r.stdout.strip() or r.stderr.strip())
    return calc, out, r.returncode == 0


def maxdiff(a, b):
    return max(abs(x - y) for x, y in zip(a, b)) if len(a) == len(b) and a else float("inf")


def orca_benzene(cli, tmp, env):
    d = os.path.join(EX, "getting-started", "orca-benzene")
    calc, h5, done = write(cli, os.path.join(d, "stored", "benzene"), tmp, env, "benzene")
    ok(done, "ORCA benzene: ecce-results write")
    if not done:
        return
    r = dump(cli, h5, env)
    text = open(os.path.join(d, "benzene.out")).read()
    nat = int(r["natoms"])
    # frequencies, IR, modes as ORCA printed them
    blk = text[text.index("VIBRATIONAL FREQUENCIES"):text.index("NORMAL MODES")]
    freq = [float(m) for m in re.findall(r"^\s*\d+:\s+(-?[\d.]+) cm\*\*-1", blk, re.M)]
    ok(maxdiff(r.get("vib/0/frequency", []), freq) <= 0.005,
       "ORCA benzene: %d frequencies equal ORCA's (max diff %.3g cm-1)"
       % (len(freq), maxdiff(r.get("vib/0/frequency", []), freq)))
    blk = text[text.index("NORMAL MODES"):text.index("IR SPECTRUM")]
    n3 = 3 * nat
    modes = [[0.0] * n3 for _ in range(n3)]
    cols = []
    for line in blk.splitlines():
        w = line.split()
        if w and all(re.match(r"^\d+$", x) for x in w):
            cols = [int(c) for c in w]
        elif cols and len(w) == len(cols) + 1 and re.match(r"^\d+$", w[0]):
            for k, c in enumerate(cols):
                modes[c][int(w[0])] = float(w[1 + k])
    flat = [x for m in modes for x in m]
    ok(maxdiff(r.get("vib/0/mode", []), flat) <= 1e-6,
       "ORCA benzene: normal modes equal ORCA's (max diff %.2g)" % maxdiff(r.get("vib/0/mode", []), flat))
    blk = text[text.index("IR SPECTRUM"):]
    ir = {int(m.group(1)): float(m.group(2)) for m in
          re.finditer(r"^\s*(\d+):\s+-?[\d.]+\s+[\d.]+\s+([\d.]+)\s", blk[:blk.index("\n\n\n")], re.M)}
    got = r.get("vib/0/ir_intensity", [])
    worst = max(abs(got[k] - v) for k, v in ir.items()) if got else float("inf")
    ok(len(ir) > 0 and worst <= 0.005,
       "ORCA benzene: %d IR intensities equal ORCA's (max diff %.3g km/mol)" % (len(ir), worst))
    # optimisation trace: one energy and one structure per cycle
    e = [float(x) for x in re.findall(r"FINAL SINGLE POINT ENERGY\s+(-?[\d.]+)", text)]
    xyz = []
    for b in re.findall(r"CARTESIAN COORDINATES \(ANGSTROEM\)\n-+\n((?:\s+[A-Z][a-z]?\s+-?[\d.]+\s+-?[\d.]+\s+-?[\d.]+\n)+)", text):
        xyz.append([float(v) * BOHR for line in b.splitlines() for v in line.split()[1:4]])
    te = r.get("trace/optimization/energy", [])
    tc = r.get("trace/optimization/coord", [])
    npt = len(te)
    ok(npt >= 2 and maxdiff(te, e[:npt]) <= 1e-9,
       "ORCA benzene: %d optimisation energies equal ORCA's (max diff %.2g Eh)" % (npt, maxdiff(te, e[:npt])))
    cd = maxdiff(tc, [v for f in xyz[:npt] for v in f])
    ok(len(tc) == npt * n3 and cd <= 2e-6,
       "ORCA benzene: optimisation structures equal ORCA's (max diff %.2g bohr)" % cd)


def orca_h2_scan(cli, tmp, env):
    d = os.path.join(EX, "computational", "h2-dissociation", "orca-h2_rhf_scan")
    stored = os.path.join(d, "stored", os.listdir(os.path.join(d, "stored"))[0])
    if not os.path.isdir(stored):
        stored = os.path.join(d, "stored", sorted(x for x in os.listdir(os.path.join(d, "stored")) if x != "stamp")[0])
    calc, h5, done = write(cli, stored, tmp, env, "h2scan")
    ok(done, "ORCA H2 scan: ecce-results write")
    if not done:
        return
    r = dump(cli, h5, env)
    text = gzip.open(os.path.join(d, "h2_rhf_scan.out.gz"), "rt").read()
    tab = text[text.index("The Calculated Surface using the 'Actual Energy'"):].splitlines()[1:]
    rows = []
    for line in tab:
        w = line.split()
        if len(w) != 2:
            break
        rows.append((float(w[0]), float(w[1])))
    v = r.get("trace/scan/value", [])
    e = r.get("trace/scan/energy", [])
    ok(len(v) == len(rows) and maxdiff(v, [a * BOHR for a, _ in rows]) <= 1e-6,
       "ORCA H2 scan: %d scan values equal ORCA's table" % len(rows))
    ok(maxdiff(e, [b for _, b in rows]) <= 1e-8,
       "ORCA H2 scan: energies equal ORCA's table (max diff %.2g Eh)" % maxdiff(e, [b for _, b in rows]))
    c = r.get("trace/scan/coord", [])
    dist = [math.dist(c[6 * i:6 * i + 3], c[6 * i + 3:6 * i + 6]) for i in range(len(c) // 6)]
    ok(maxdiff(dist, v) <= 1e-5, "ORCA H2 scan: each structure's H-H distance is its scan value (max diff %.2g bohr)"
       % maxdiff(dist, v))


def qmout_scan(path):
    text = open(path).read()
    pts = []
    for m in re.finditer(r"begin scan_geometry\s+# point (\d+) value (-?[\d.]+);.*\n((?:.*\n)*?)end scan_geometry", text):
        xyz = [float(x) * BOHR for line in m.group(3).splitlines() for x in line.split()[2:5]]
        pts.append((float(m.group(2)), xyz))
    tables = re.findall(r"begin scan_table.*\n((?:.*\n)*?)end scan_table", text)
    energies = [float(line.split()[2]) for line in tables[-1].splitlines()]
    return pts, energies


def ecceqm_butane(cli, tmp, env):
    d = os.path.join(EX, "organic", "conformations", "ecceqm-butane-scan")
    calc, h5, done = write(cli, os.path.join(d, "stored", "butane_scan"), tmp, env, "butane")
    ok(done, "ECCE-QM butane scan: ecce-results write")
    if not done:
        return
    r = dump(cli, h5, env)
    pts, energies = qmout_scan(os.path.join(d, "butane_scan.qmout"))
    v = r.get("trace/scan/value", [])
    ok(maxdiff(v, [math.radians(a) for a, _ in pts]) <= 1e-9,
       "ECCE-QM butane scan: %d scan values (radian) equal the output's" % len(pts))
    ok(maxdiff(r.get("trace/scan/energy", []), energies) <= 1e-9,
       "ECCE-QM butane scan: energies equal the output's scan table")
    cd = maxdiff(r.get("trace/scan/coord", []), [x for _, f in pts for x in f])
    ok(cd <= 1e-8, "ECCE-QM butane scan: structures equal the output's (max diff %.2g bohr)" % cd)
    nopt = len(r.get("trace/optimization/energy", []))
    nraw = len(re.findall(r"^opt_step ", open(os.path.join(d, "butane_scan.qmout")).read(), re.M))
    ok(nopt == nraw, "ECCE-QM butane scan: %d optimisation steps in the trace, %d in the output" % (nopt, nraw))


def aoKeys(d):
    keys, e = [], 0
    for at, l, n in zip(d["basis/shell_nucleus"], d["basis/shell_ang_mom"], d["basis/shell_nprim"]):
        ex = tuple(round(x, 6) for x in d["basis/exponent"][e:e + int(n)])
        e += int(n)
        keys += [(int(at), int(l), ex, c) for c in range(2 * int(l) + 1)]
    return keys


def permuted(d, want):
    """d's MO coefficients with the AOs in the order of the keys `want`."""
    have = {k: i for i, k in enumerate(aoKeys(d))}
    c = d.get("mo/0/coefficient", [])
    n = len(have)
    if set(have) != set(want):
        return []
    return [c[i * n + have[k]] for i in range(len(c) // n) for k in want]


def ecceqm_water(cli, qm, tmp, env):
    d = os.path.join(EX, "getting-started", "ecceqm-water")
    run = os.path.join(tmp, "qmrun")
    os.makedirs(run)
    shutil.copy(os.path.join(d, "water.qmin"), run)
    qenv = dict(env, ECCE_BASIS_DIR=os.path.join(ROOT, "data", "admin", "basissets"), OMP_NUM_THREADS="2")
    subprocess.run([qm, "--results", "results.h5", "-o", "water.qmout", "water.qmin"], cwd=run, env=qenv, check=True)
    own = os.path.join(run, "results.h5")
    ok(os.path.exists(own), "ECCE-QM water: ecce-qm wrote results.h5 (%d bytes)" % (os.path.getsize(own) if os.path.exists(own) else 0))
    if not os.path.exists(own):
        return
    q = dump(cli, own, env)
    ok(str(q.get("checks", "")).startswith("ok:"), "ECCE-QM water: its own checks: %s" % q.get("checks"))
    ok("hessian/0" in q, "ECCE-QM water: the Hessian is stored")
    calc, h5, done = write(cli, os.path.join(d, "stored", "water"), tmp, env, "water")
    p = dump(cli, h5, env)
    for key, tol, unit in (("vib/0/frequency", 0.05, "cm-1"), ("vib/0/ir_intensity", 0.01, "km/mol"),
                           ("mo/0/energy", 1e-5, "Eh"), ("mo/0/occupation", 0, ""),
                           ("trace/optimization/energy", 1e-6, "Eh"), ("geometry/coord", 1e-5, "bohr")):
        a, b = q.get(key, []), p.get(key, [])
        # ecce-qm stores the 3N-6 vibrations, the parsed set has 3N
        if key.startswith("vib") and a:
            b = b[-len(a):]
        diff = maxdiff(a, b)
        ok(diff <= tol, "ECCE-QM water: %s, own file against parsed (%d values): max diff %.2g %s"
           % (key, len(b), diff, unit))
    a, b = q.get("vib/0/mode", []), p.get("vib/0/mode", [])
    n3 = 9
    worst = 0.0
    if a and b and len(a) <= len(b):
        b = b[-len(a):]
        for k in range(len(a) // n3):
            x, y = a[k * n3:(k + 1) * n3], b[k * n3:(k + 1) * n3]
            nx, ny = math.sqrt(sum(t * t for t in x)), math.sqrt(sum(t * t for t in y))
            worst = max(worst, 1 - abs(sum(s * t for s, t in zip(x, y))) / (nx * ny))
    else:
        worst = 1
    ok(worst <= 1e-5, "ECCE-QM water: normal modes parallel to the parsed ones (1 - |cos| <= %.1g)" % worst)
    # MO coefficients up to each MO's sign; the shells of an atom may come in
    # another order (ecce-qm lists s shells first), so AOs are matched by
    # atom, l, exponents and component.
    a = permuted(q, aoKeys(p))
    b = p.get("mo/0/coefficient", [])
    nao = len(q.get("mo/0/energy", [])) and len(b) // len(q["mo/0/energy"])
    nocc = sum(1 for o in q.get("mo/0/occupation", []) if o > 0)
    worst = max((min(maxdiff(a[i*nao:(i+1)*nao], b[i*nao:(i+1)*nao]),
                     maxdiff(a[i*nao:(i+1)*nao], [-t for t in b[i*nao:(i+1)*nao]])) for i in range(nocc)),
                default=float("inf")) if len(a) == len(b) else float("inf")
    ok(worst <= 1e-4, "ECCE-QM water: occupied MO coefficients equal the parsed ones up to sign (max diff %.2g)" % worst)
    # the job store keeps ECCE-QM's file
    outputs = os.path.join(calc, "Outputs")
    shutil.copy(own, os.path.join(outputs, "results.h5"))
    r = subprocess.run([cli, "write", calc], env=env, capture_output=True, text=True)
    print(r.stdout.strip())
    kept = dump(cli, os.path.join(outputs, "results.h5"), env)
    ok(kept.get("provenance/writer") == "ecce-qm" and "kept" in r.stdout,
       "ECCE-QM water: the job store keeps the file ECCE-QM wrote")
    print("sizes (bytes): ecce-qm %d, from properties %d" % (os.path.getsize(own), os.path.getsize(h5)))


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 2
    cli = os.path.abspath(sys.argv[1])
    qm = os.path.abspath(sys.argv[2]) if len(sys.argv) > 2 else None
    tmp = tempfile.mkdtemp(prefix="resultsh5-trace-")
    env = dict(os.environ, ECCE_HOME=ROOT, HOME=tmp, ECCE_REALUSERHOME=tmp, ECCE_REALUSER="resultsh5")
    try:
        orca_benzene(cli, tmp, env)
        orca_h2_scan(cli, tmp, env)
        ecceqm_butane(cli, tmp, env)
        if qm and os.access(qm, os.X_OK):
            ecceqm_water(cli, qm, tmp, env)
    finally:
        shutil.rmtree(tmp, ignore_errors=True)
    print("RESULT", "FAIL" if fails else "OK")
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
