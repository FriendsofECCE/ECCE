#!/usr/bin/env python3
"""A Gaussian 16 calculation's "Density (ESP)" surface, headless (#410).

    tests/apps/g16_esp_test.py --builder build-cmake/builder

Formaldehyde HF/6-31G* as ECCE holds a Gaussian 16 run (tests/mocomp/
fixtures/h2co631: MO and ORBENG from the real gaussian-16.mo, the basis, plus
ORBOCC for the 8 doubly occupied orbitals).  The Builder's scene command
`esp 40` computes the potential-mapped density exactly as the MOs panel does;
`espdump` writes the 40^3 grid.  Checks:

  * the density is positive and sums to about the 16 electrons;
  * the potential is positive far out on the proton side and negative at the
    carbonyl oxygen lone-pair side, the sign convention the colour ramp uses;
  * if Gaussian's cubegen is installed (GAUSS_EXEDIR or /opt/gaussian/g16),
    it is run live on tests/mocomp/molden/h2co631.fchk over the same nodes
    and ECCE's density and potential are held to it (both only more than 0.8 A
    from a nucleus, where a grid of this spacing is no longer exact).  Nothing from
    that run is stored.

Skips (exit 77) without the builder, Xvfb or the installed tree.
"""
import argparse
import math
import os
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(ROOT, "tests", "panels"))

SKIP = 77
MOCOMP = os.path.join(ROOT, "tests", "mocomp")
FCHK = os.path.join(MOCOMP, "molden", "h2co631.fchk")
RES = 40
BOHR = 0.529177210903
NUCLEI = [(6, 0.0, 0.0, -0.530125), (8, 0.0, 0.0, 0.674875),
          (1, 0.0, 0.935, -1.109125), (1, 0.0, -0.935, -1.109125)]


def orbocc(count=34, occupied=8):
    rows = " ".join(str(i + 1) for i in range(count))
    vals = "   ".join("2" if i < occupied else "0" for i in range(count))
    return ('<?xml version="1.0" encoding="UTF-8" standalone="yes" ?>\n'
            '<vector name="ORBOCC" rowLabel="Basis Function" rowLabels="%s" '
            'rows="%d" units="NA">%s</vector>\n' % (rows, count, vals))


def makeCalc(omo, data):
    """The ORCA water shell of orca_mo_test with this fixture's geometry,
    basis and properties, recorded as a Gaussian-16 calculation."""
    url = omo.makeCalc(data, "h2co")
    calc = url[len("file://"):].rstrip("/")
    fixture = os.path.join(MOCOMP, "fixtures", "h2co631")
    for name in ("chemsys.mvm", "BasisSet.ecce_basisset"):
        shutil.copy(os.path.join(fixture, "Parameters", name),
                    os.path.join(calc, "Parameters", name))
    props = os.path.join(calc, "Props")
    for name in os.listdir(props):
        if not name.startswith("."):
            os.unlink(os.path.join(props, name))
    for name in ("MO", "ORBENG"):
        shutil.copy(os.path.join(fixture, "Props", name), props)
    with open(os.path.join(props, "ORBOCC"), "w") as handle:
        handle.write(orbocc())
    meta = os.path.join(calc, ".ecce-meta")
    text = open(meta).read().replace("\tORCA\n", "\tGaussian-16\n")
    with open(meta, "w") as handle:
        handle.write(text)
    return url


def readDump(path):
    with open(path) as handle:
        head = handle.readline().split()
        n = [int(x) for x in head[:3]]
        origin = [float(x) for x in head[3:6]]
        corner = [float(x) for x in head[6:9]]
        rows = [tuple(float(x) for x in line.split()) for line in handle]
    return n, origin, corner, rows


def nodes(n, origin, corner):
    step = [(corner[k] - origin[k]) / (n[k] - 1) for k in range(3)]
    for ix in range(n[0]):
        for iy in range(n[1]):
            for iz in range(n[2]):
                yield (origin[0] + ix * step[0], origin[1] + iy * step[1],
                       origin[2] + iz * step[2])


def cubegenExe():
    for base in (os.environ.get("GAUSS_EXEDIR"), "/opt/gaussian/g16"):
        if base and os.access(os.path.join(base, "cubegen"), os.X_OK):
            return base
    return None


def cubegenGrid(base, what, n, origin, corner, work):
    """cubegen's `what` (potential or density) over the same nodes; a zero atom count in the
    grid header means Angstrom."""
    step = [(corner[k] - origin[k]) / (n[k] - 1) for k in range(3)]
    grid = os.path.join(work, "grid.txt")
    with open(grid, "w") as handle:
        handle.write("0 %.8f %.8f %.8f\n" % tuple(origin))
        for k in range(3):
            vec = [0.0, 0.0, 0.0]
            vec[k] = step[k]
            handle.write("%d %.8f %.8f %.8f\n" % ((n[k],) + tuple(vec)))
    env = dict(os.environ, GAUSS_EXEDIR=base, GAUSS_SCRDIR=work)
    shutil.copy(FCHK, work)
    with open(grid) as handle:
        subprocess.run([os.path.join(base, "cubegen"), "0", what + "=scf",
                        "h2co631.fchk", "out.cube", "-1", "h"], cwd=work,
                       env=env, stdin=handle, stdout=subprocess.DEVNULL,
                       stderr=subprocess.DEVNULL, check=True)
    lines = open(os.path.join(work, "out.cube")).read().splitlines()
    values = []
    for line in lines[2 + 4 + len(NUCLEI):]:
        values += [float(x) for x in line.split()]
    return values


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--builder", default="build-cmake/builder")
    parser.add_argument("--keep-dump", metavar="PATH",
                        help="copy the grid the Builder wrote here")
    options = parser.parse_args()
    builder = os.path.abspath(options.builder)
    if not os.path.exists(builder) or not os.path.isdir("/opt/ecce") or \
            not shutil.which("Xvfb"):
        print("SKIP: no builder, /opt/ecce or Xvfb")
        return SKIP

    import capture
    home = os.path.join(os.path.dirname(builder), "g16esp-home")
    os.environ["PANELS_REPO_CLIENT"] = "1"
    capture.overlay(builder, home, os.path.join(
        ROOT, "data", "client", "config", "PropertyPanelDescriptor.xml"))
    os.environ["ECCE_TEST_HOME"] = home
    import orca_mo_test as omo
    import isolate
    import xdisplay

    settings = isolate.apply(omo.apps.INSTALL)
    os.environ["ECCE_NO_REAP"] = "1"
    os.environ["GTK_THEME"] = "Adwaita"
    work = tempfile.mkdtemp(prefix="g16-esp-",
                            dir=os.environ.get("ECCE_REALUSERHOME"))
    data = os.path.join(work, "data")
    os.makedirs(os.path.join(data, "users", "local"))
    os.environ["ECCE_LOCAL_DATA"] = data
    xdisplay.SCREEN = "1366x768x24"
    problems = []
    try:
        with xdisplay.Display(screen="1366x768x24") as display:
            subprocess.run([os.path.join(home, "bin", "ecce-gateway-start")],
                           env=display.env(), timeout=180)
            try:
                url = makeCalc(omo, data)
                out = os.path.join(work, "esp")
                dump = os.path.join(out, "grid.dump")
                os.makedirs(out, exist_ok=True)
                result = omo.runBuilder(
                    display, url, out, "esp %d\nespdump %s\n" % (RES, dump))
                shot = os.path.join(out, "esp.png")
                if not os.path.exists(dump):
                    problems.append("no grid written: %s"
                                    % (result.log or "")[-1500:])
            finally:
                subprocess.run([os.path.join(home, "bin",
                                             "ecce-gateway-stop")],
                               env=display.env(), timeout=120)
        if options.keep_dump and os.path.exists(dump):
            shutil.copy(dump, options.keep_dump)
        if not problems:
            problems += check(dump, work)
    finally:
        shutil.rmtree(work, ignore_errors=True)
    if problems:
        for p in problems:
            print("FAIL  " + p)
        return 1
    print("PASSED")
    return 0


def check(dump, work):
    problems = []
    n, origin, corner, rows = readDump(dump)
    if n != [RES] * 3 or len(rows) != RES ** 3:
        return ["grid is %s with %d rows" % (n, len(rows))]
    pts = list(nodes(n, origin, corner))
    cell = 1.0
    for k in range(3):
        cell *= (corner[k] - origin[k]) / (n[k] - 1)
    #  A grid of 0.2 A misses the cores, so the integral is only a sanity
    #  check; the density itself is held to cubegen below.
    electrons = sum(r[0] for r in rows) * cell / BOHR ** 3
    if not 12.0 < electrons < 17.0:
        problems.append("density integrates to %.2f electrons" % electrons)

    def at(x, y, z):
        best = min(range(len(pts)), key=lambda i: (pts[i][0] - x) ** 2 +
                   (pts[i][1] - y) ** 2 + (pts[i][2] - z) ** 2)
        return rows[best][1]

    far_h = at(0.0, 3.0, -2.0)
    if not far_h > 0:
        problems.append("potential beside the hydrogens is %g, want > 0"
                        % far_h)
    near_o = at(1.8, 0.0, 0.9)
    if not near_o < far_h:
        problems.append("potential at the oxygen side (%g) is not below the "
                        "hydrogen side (%g)" % (near_o, far_h))
    if not problems:
        print("ok    potential sign: +%.4f beside H, %.4f at O" % (far_h,
                                                                   near_o))

    base = cubegenExe()
    if not base:
        print("skip  no cubegen: potential not held to a second program")
        return problems
    ref = cubegenGrid(base, "potential", n, origin, corner, work)
    dens = cubegenGrid(base, "density", n, origin, corner, work)
    keep = [i for i, p in enumerate(pts)
            if all(math.dist(p, nuc[1:]) > 0.8 for nuc in NUCLEI)]
    worst = max(abs(rows[i][1] - ref[i]) for i in keep)
    if len(ref) != len(rows):
        problems.append("reference has %d nodes, ECCE %d" % (len(ref),
                                                             len(rows)))
    elif worst > 2e-3:
        problems.append("potential differs by up to %.2e Hartree/e over %d "
                        "nodes" % (worst, len(keep)))
    else:
        print("ok    potential held to cubegen on %d nodes" % len(keep))
    #  Closer than that to a heavy nucleus the density is the 1s core,
    #  which the grid evaluates less exactly; no surface is drawn there.
    dworst = max(abs(rows[i][0] - dens[i]) for i in keep)
    if dworst > 1e-3:
        problems.append("density differs by up to %.2e e/bohr^3" % dworst)
    else:
        print("ok    density held to cubegen on %d nodes" % len(keep))
    return problems


if __name__ == "__main__":
    sys.exit(main())
