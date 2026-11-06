#!/usr/bin/env python3
"""
Dipole Moment panel, Bond dipoles (#849): the Builder script command
"bonddipoles on KEY" turns the box on for stored example calculations and
logs the table rows; checks them against an independent solve (dense
least squares on the incidence matrix, here by pure-Python Gaussian
elimination) of the same stored charges, and that the arrows number the
bonds that are not 0.00 D plus the molecular arrow.

    ECCE_BONDDIP_PNG=<dir>   also copy the window pictures there

Same installed tree and isolation as dipole_poles_test.py.  Exit 77 (skip)
without an install or Xvfb.
"""
import math
import os
import re
import shutil
import subprocess
import sys
import tempfile
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import apps                   # noqa: E402
import isolate                # noqa: E402
import orca_mo_test as base   # noqa: E402
import xdisplay               # noqa: E402

REPO = os.path.dirname(os.path.dirname(HERE))
EX = os.path.join(REPO, "data", "client", "examples")
CASES = [
    ("h2o", "getting-started/ecceqm-water/stored/water", "MULLIKEN", 90),
    ("so2", "genchem/polar-bonds-and-dipole-moments/ecceqm-SO2/stored/SO2", "MULLIKEN", 90),
    ("cyclohexene", "organic/reactions/orca-cyclohexene/stored/cyclohexene", "MULLIKEN", 0),
]


def stored(calc, key):
    path = os.path.join(calc, "Props", key)
    if not os.path.exists(path):
        return None
    txt = open(path).read()
    return [float(x) for x in re.search(r"<vector[^>]*>([^<]*)<", txt).group(1).split()]


def mindipole(q, xyz, bonds):
    """Sum over bonds of d_ij (r_i - r_j) for the minimum-norm d: solve
    B B^T phi = q by elimination with the last atom of each piece pinned."""
    n = len(q)
    L = [[0.0] * n for _ in range(n)]
    for i, j in bonds:
        L[i][i] += 1; L[j][j] += 1; L[i][j] -= 1; L[j][i] -= 1
    # pieces
    comp = list(range(n))
    def find(a):
        while comp[a] != a:
            comp[a] = comp[comp[a]]
            a = comp[a]
        return a
    for i, j in bonds:
        comp[find(i)] = find(j)
    qq = list(q)
    pinned = set()
    for c in set(find(i) for i in range(n)):
        members = [i for i in range(n) if find(i) == c]
        share = sum(q[i] for i in members) / len(members)
        for i in members:
            qq[i] -= share
        pinned.add(members[-1])
    idx = [i for i in range(n) if i not in pinned]
    A = [[L[i][j] for j in idx] + [qq[i]] for i in idx]
    m = len(idx)
    for c in range(m):
        p = max(range(c, m), key=lambda r: abs(A[r][c]))
        A[c], A[p] = A[p], A[c]
        for r in range(m):
            if r != c and A[r][c] != 0.0:
                f = A[r][c] / A[c][c]
                A[r] = [x - f * y for x, y in zip(A[r], A[c])]
    phi = [0.0] * n
    for c, i in enumerate(idx):
        phi[i] = A[c][m] / A[c][c]
    tot = [0.0, 0.0, 0.0]
    for i, j in bonds:
        d = phi[i] - phi[j]
        for k in range(3):
            tot[k] += d * (xyz[i][k] - xyz[j][k])
    return tot


def runBuilder(display, url, out, png, turn, key):
    """ECCE_BUILDER_SCRIPT run with the Dipole Moment panel open; the
    window picture is taken at the script's "shot" and the log returned."""
    os.makedirs(out, exist_ok=True)
    lines = ["wait 3000", "background 1 1 1", "viewall"]
    if turn:
        lines.append("rotate %d" % turn)
    lines += ["wait 1500", "coords", "bonddipoles on " + key, "wait 800",
              "vector DIPOLE", "shot s", "quit"]
    with open(os.path.join(out, "script"), "w") as handle:
        handle.write("\n".join(lines) + "\n")
    log = os.path.join(out, "builder.log")
    env = display.env()
    env.update(ECCE_BUILDER_SCRIPT=os.path.join(out, "script"),
               ECCE_OPEN_PANEL="Dipole Moment",
               ECCE_TRANSPARENCY_FALLBACK_MS="0")
    proc = subprocess.Popen([os.path.join(apps.WRAPPERS, "ecce-builder"),
                             "-context", url], env=env,
                            stdout=open(log, "w"), stderr=subprocess.STDOUT,
                            start_new_session=True)
    ready = os.path.join(out, "s.ready")
    deadline = time.time() + 240
    try:
        while time.time() < deadline and proc.poll() is None:
            if os.path.exists(ready):
                time.sleep(1.5)
                argv = ["import", "-display", display.name, "-window", "root"]
                best = None
                for wid, _t in display.windows():
                    info = subprocess.run(["xwininfo", "-display", display.name,
                                           "-id", wid], capture_output=True,
                                          text=True).stdout
                    g = dict(re.findall(r"(Absolute upper-left [XY]|Width|"
                                        r"Height):\s+(-?\d+)", info))
                    if len(g) == 4 and (best is None or int(g["Width"]) *
                                        int(g["Height"]) > int(best["Width"]) *
                                        int(best["Height"])):
                        best = g
                if best and not os.environ.get("ECCE_BONDDIP_FULL"):
                    argv += ["-crop", "%sx%s+%s+%s" % (
                        best["Width"], best["Height"],
                        best["Absolute upper-left X"],
                        best["Absolute upper-left Y"]), "+repage"]
                subprocess.run(argv + [png], check=False)
                os.unlink(ready)
                open(os.path.join(out, "s.go"), "w").close()
            time.sleep(0.3)
        proc.wait(timeout=30)
    except subprocess.TimeoutExpired:
        pass
    finally:
        if proc.poll() is None:
            os.killpg(proc.pid, 9)
    return open(log, errors="replace").read()


def main():
    if "ECCE_TEST_WRAPPERS" not in os.environ:
        print("SKIP: set ECCE_TEST_HOME and ECCE_TEST_WRAPPERS to a private install")
        return 77
    if not os.path.exists(os.path.join(apps.WRAPPERS, "ecce-builder")):
        print("SKIP: no ecce-builder under %s" % apps.WRAPPERS)
        return 77
    if not shutil.which("Xvfb"):
        print("SKIP: no Xvfb")
        return 77
    isolate.apply(apps.INSTALL)
    os.environ["ECCE_NO_REAP"] = "1"
    os.environ["GTK_THEME"] = "Adwaita"
    pngdir = os.environ.get("ECCE_BONDDIP_PNG")
    work = tempfile.mkdtemp(prefix="bonddip-", dir=os.environ.get("ECCE_REALUSERHOME"))
    data = os.path.join(work, "data")
    os.makedirs(os.path.join(data, "users", "local"))
    os.environ["ECCE_LOCAL_DATA"] = data
    xdisplay.SCREEN = "1700x1000x24"
    failures = []

    def check(ok, what):
        print("%s %s" % ("ok   " if ok else "FAIL ", what))
        if not ok:
            failures.append(what)

    with xdisplay.Display() as display:
        subprocess.run([os.path.join(apps.INSTALL, "bin", "ecce-gateway-start")],
                       env=display.env(), timeout=180)
        for name, rel, key, turn in CASES:
            if os.environ.get("ECCE_BONDDIP_ONLY", name) != name:
                continue
            url = base.makeCalc(data, name, source=os.path.join(EX, rel))
            calc = url[len("file://"):].rstrip("/")
            out = os.path.join(work, name)
            png = os.path.join(out, name + ".png")
            log = runBuilder(display, url, out, png, turn, key)
            if pngdir and os.path.exists(png):
                os.makedirs(pngdir, exist_ok=True)
                shutil.copy(png, os.path.join(pngdir, name + ".png"))
            rows = [[c.strip() for c in m.groups()] for m in re.finditer(
                r"BUILDER: bonddipoles row \d+ \| (.*) \| (.*) \| (.*)", log)]
            summary = re.search(r"BUILDER: bonddipoles rows=(\d+) arrows=(\d+)", log)
            check(bool(rows) and summary is not None, "%s: table logged" % name)
            if not rows or not summary:
                print(log[-1500:])
                continue
            if os.environ.get("ECCE_BONDDIP_VERBOSE"):
                for m in re.finditer(r"BUILDER: vector DIPOLE \d+ .*", log):
                    print("    ", name, m.group(0))
            names = [r[0] for r in rows]
            by = {r[0]: float(r[1]) for r in rows}
            nb = names.index("Sum of bonds")
            bondrows = rows[:nb]
            xyz = [[float(x) for x in m.groups()] for m in re.finditer(
                r"BUILDER: atom \d+ \S+ (\S+) (\S+) (\S+)", log)]
            q = stored(calc, key)
            bonds = []
            for r in bondrows:
                m = re.match(r"[A-Za-z]+(\d+) -> [A-Za-z]+(\d+)$", r[0])
                bonds.append((int(m.group(1)) - 1, int(m.group(2)) - 1))
            want = mindipole(q, xyz, bonds)
            wantD = 4.80320 * math.sqrt(sum(x * x for x in want))
            if abs(by["Sum of bonds"] - wantD) >= 0.002:
                print("DEBUG", rows, want, flush=True)
            check(abs(by["Sum of bonds"] - wantD) < 0.002,
                  "%s: sum of bonds %.3f D (independent solve %.3f D)"
                  % (name, by["Sum of bonds"], wantD))
            pc = 4.80320 * math.sqrt(sum(sum(qi * c[k] for qi, c in zip(q, xyz)) ** 2
                                         for k in range(3)))
            # Neutral molecules: bonds add up to the point-charge dipole.
            if abs(sum(q)) < 1e-3:
                check(abs(by["Point charges"] - by["Sum of bonds"]) < 0.002,
                      "%s: bonds add up to the point charges (%.3f D)"
                      % (name, by["Point charges"]))
            mu = stored(calc, "DIPOLE")
            check(abs(by["Computed"] - math.sqrt(sum(x * x for x in mu))) < 0.002,
                  "%s: computed row is the stored dipole" % name)
            check(any(n.startswith("Remainder") for n in names),
                  "%s: remainder is its own row" % name)
            visible = sum(1 for r in bondrows if float(r[1]) >= 0.005)
            check(int(summary.group(2)) == visible + 1,
                  "%s: %d bond arrows + the molecular arrow (%s)"
                  % (name, visible, summary.group(2)))
    if not failures:
        shutil.rmtree(work, ignore_errors=True)
    else:
        print("work dir kept:", work)
    print("FAILED: %d" % len(failures) if failures else "all ok")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
