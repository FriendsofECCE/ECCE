#!/usr/bin/env python3
"""MOPAC normal modes after Organizer File > Import sit on the stored atoms (#369).

The output is imported through the Organizer's real import (the
ECCE_TEST_CALCIMPORT hook of calcimport_test.py: JobParser, eccejobmaster,
eccejobstore, the parse scripts as the job store runs them), not through a
replay of the scripts, and the VIB and GEOMTRACE it stores are read back from
the data server.  Oracles that do not come from the parser: a vibration has
no share in the six rigid motions of the stored structure (mass-weighted),
and the umbrella mode of ammonia moves N along the C3 axis and the three H
alike.

    tests/apps/mopac_import_modes_test.py [--display N]

Same installed tree and isolation as calcimport_test.py (ECCE_TEST_HOME,
ECCE_TEST_WRAPPERS).  Exit 77 (skip) without an install or Xvfb.
"""
import glob
import math
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import apps       # noqa: E402
import fixture    # noqa: E402
import run_tests  # noqa: E402

DATA = os.path.join(HERE, "..", "parsers", "fixtures", "mopac")
MASS = {"H": 1.00794, "C": 12.0107, "N": 14.0067, "O": 15.9994}
MAX_RIGID = 0.02
FIXTURES = ("nh3_geovib", "h2o_geovib", "ch4_force", "ch4_geovib",
            os.path.join(os.pardir, os.pardir, os.pardir, os.pardir, "data",
                         "client", "examples", "general", "mopac-ammonia", "ammonia"))

BUILDER_CASES = ("nh3_geovib", "ammonia")

failures = []


def props(name):
    root = os.path.join(fixture.stateDir(), "htdocs", "Ecce", "users")
    found = glob.glob(os.path.join(root, "*", "calcimport-test", name, "Props"))
    return found[0] if found else None


def numbers(text):
    return [float(t) for t in text.split()]


def readVib(path):
    """[mode][atom] -> (x, y, z)"""
    text = open(path).read()
    natoms = int(re.search(r'\brows="(\d+)"', text).group(1))
    modes = []
    for body in re.findall(r"<matrix number=\"\d+\">(.*?)</matrix>", text, re.S):
        v = numbers(body)
        modes.append([v[3 * a:3 * a + 3] for a in range(natoms)])
    return modes


def readLastGeometry(path):
    text = open(path).read()
    steps = re.findall(r"<step number=\"\d+\">(.*?)</step>", text, re.S)
    labels = re.search(r'rowLabels="([^"]*)"', text).group(1).split()
    v = numbers(steps[-1])
    return ([lab.split("-")[1] for lab in labels],
            [v[3 * a:3 * a + 3] for a in range(len(labels))])


def rigidShare(symbols, xyz, modes):
    import numpy as np
    mass = np.array([MASS[s] for s in symbols])
    x = np.array(xyz)
    root = np.sqrt(mass)[:, None]
    centre = (x * mass[:, None]).sum(axis=0) / mass.sum()
    rigid = []
    for k in range(3):
        e = np.zeros(3)
        e[k] = 1.0
        rigid.append((root * e).ravel())
        rigid.append((root * np.cross(e, x - centre)).ravel())
    basis, _r = np.linalg.qr(np.array(rigid).T)
    worst = 0.0
    for m in modes:
        u = (root * np.array(m)).ravel()
        worst = max(worst, float(np.sum((basis.T @ u) ** 2) / np.sum(u ** 2)))
    return worst


def umbrella(symbols, xyz, modes, freqs):
    """For ammonia: the mode that moves N most along the C3 axis of the
    stored structure; returns (N along axis share, spread of the H motions
    along the axis, spread of the H motion magnitudes)."""
    import numpy as np
    x = np.array(xyz)
    hs = [i for i, s in enumerate(symbols) if s == "H"]
    n = symbols.index("N")
    axis = np.cross(x[hs[1]] - x[hs[0]], x[hs[2]] - x[hs[0]])
    axis /= np.linalg.norm(axis)
    best = max(range(len(modes)),
               key=lambda m: abs(np.dot(modes[m][n], axis)))
    d = [np.array(modes[best][i]) for i in range(len(modes[best]))]
    along = [np.dot(d[i], axis) for i in hs]
    mags = [np.linalg.norm(d[i]) for i in hs]
    return (best, freqs[best], abs(np.dot(d[n], axis)) / np.linalg.norm(d[n]),
            (max(along) - min(along)) / abs(np.mean(along)),
            (max(mags) - min(mags)) / np.mean(mags))


def newAuth():
    return fixture.authFile(
        os.path.join(fixture.stateHome(), ".ECCE", "auth.pipe"),
        user=fixture.realUser())


def builderArrows(display, log, work, mode):
    """The Builder opens the imported calculation; returns (cos of the
    strongest H arrow in the highest mode with its bond, {mode: arrows})
    as drawn, the arrows being [(symbol, xyz, direction)]."""
    url = re.search(r"ECCE_TEST_CALCIMPORT: selected (\S+)", log)
    if not url:
        return None, {}, "no selected calculation"
    nmodes = len(readVib(os.path.join(props(os.path.basename(url.group(1))),
                                      "VIB")))
    script = os.path.join(work, "vib.scene")
    lines = ["vibfocus", "vibmode %d" % (nmodes - 1), "vibvector", "vibframe"]
    if mode is not None:
        lines += ["vibmode %d" % mode, "vibvector", "vibdirs"]
    with open(script, "w") as handle:
        handle.write("\n".join(lines + [""]))
    result = apps.run(display, "builder",
                      args=("-pipe", newAuth(), "-context", url.group(1)),
                      windowTimeout=120, settle=120,
                      env={"ECCE_VIEWER_SCENE": script,
                           "ECCE_VIEWER_SCENE_OUT": work,
                           "ECCE_REALUSER": fixture.realUser()})
    out = result.log or ""
    got = re.findall(r"VIBFRAME: atom=(-?\d+) cos=(-?[\d.]+)", out)
    arrows = []
    for m in re.finditer(r"VIBDIR: \d+ (\S+) (\S+) (\S+) (\S+) (\S+) (\S+) "
                         r"(\S+)", out):
        v = [float(g) for g in m.groups()[1:]]
        arrows.append((m.group(1), v[:3], v[3:]))
    return (float(got[-1][1]) if got else None), arrows, out[-3000:]


def umbrellaMode(symbols, xyz, modes):
    """Index of the mode that moves N most along the C3 axis (the axis is
    the normal of the H plane of the stored structure)."""
    import numpy as np
    x = np.array(xyz)
    hs = [i for i, s in enumerate(symbols) if s == "H"]
    axis = np.cross(x[hs[1]] - x[hs[0]], x[hs[2]] - x[hs[0]])
    axis /= np.linalg.norm(axis)
    n = symbols.index("N")
    return max(range(len(modes)),
               key=lambda m: abs(np.dot(modes[m][n], axis)))


def umbrellaProblems(arrows):
    """The drawn umbrella mode of NH3: N moves along the C3 axis, the three
    H move alike (equal length, equal share along the axis)."""
    import numpy as np
    if len(arrows) != 4:
        return ["%d arrows drawn instead of 4" % len(arrows)]
    n = [a for a in arrows if a[0] == "N"][0]
    hs = [a for a in arrows if a[0] == "H"]
    pos = [np.array(h[1]) for h in hs]
    axis = np.cross(pos[1] - pos[0], pos[2] - pos[0])
    axis /= np.linalg.norm(axis)
    problems = []
    nd = np.array(n[2])
    along = abs(np.dot(nd, axis)) / np.linalg.norm(nd)
    mags = [np.linalg.norm(h[2]) for h in hs]
    axial = [np.dot(np.array(h[2]), axis) for h in hs]
    if along < 0.99:
        problems.append("the N arrow is %.2f of the way along the C3 axis"
                        % along)
    if (max(mags) - min(mags)) / np.mean(mags) > 0.02:
        problems.append("the H arrows differ in length: %s"
                        % ", ".join("%.3f" % m for m in mags))
    if (max(axial) - min(axial)) / abs(np.mean(axial)) > 0.02:
        problems.append("the H arrows differ along the axis: %s"
                        % ", ".join("%.3f" % a for a in axial))
    return problems


def check(display, name, log, work):
    p = props(name)
    if not re.search(r"ECCE_TEST_IMPORTREPORT: state \d+ ", log) or p is None:
        failures.append(name)
        print("FAIL  %s: the import did not finish or stored nothing" % name)
        return
    if not os.path.exists(os.path.join(p, "VIB")):
        failures.append(name)
        print("FAIL  %s: the import stored no normal modes" % name)
        return
    modes = readVib(os.path.join(p, "VIB"))
    symbols, xyz = readLastGeometry(os.path.join(p, "GEOMTRACE"))
    share = rigidShare(symbols, xyz, modes)
    problems = []
    if share > MAX_RIGID:
        problems.append("a stored mode is %.2f inside the rigid motions of "
                        "the stored structure (modes in another frame)"
                        % share)
    extra = ""
    if name in BUILDER_CASES:
        umb = umbrellaMode(symbols, xyz, modes)
        cosine, arrows, tail = builderArrows(display, log, work, umb)
        extra = ", Builder stretch arrow/bond cos %s" % cosine
        if cosine is None or cosine < 0.95:
            problems.append("the Builder's arrows do not follow the bonds "
                            "(cos %s)\n%s" % (cosine, tail))
        bad = umbrellaProblems(arrows)
        problems += ["umbrella mode %d: %s" % (umb + 1, b) for b in bad]
        if not bad:
            extra += ", umbrella mode %d symmetric" % (umb + 1)
    if problems:
        failures.append(name)
        print("FAIL  %s: %s" % (name, "; ".join(problems)))
    else:
        print("ok    %s: rigid-motion share %.1e%s" % (name, share, extra))


def main():
    def checkApp(display, name, results, verbose=False):
        os.environ["ECCE_TEST_IMPORTREPORT"] = "1"
        for case in FIXTURES:
            os.environ["ECCE_TEST_CALCIMPORT"] = os.path.abspath(
                os.path.join(DATA, case + ".out"))
            result = apps.run(display, "organizer", args=("-pipe", newAuth()),
                              windowTimeout=60, settle=20)
            work = os.path.join(fixture.stateHome(), "builder-scene")
            os.makedirs(work, exist_ok=True)
            check(display, os.path.basename(case), result.log or "", work)
        if failures:
            results.fail("mopac-import-modes", ", ".join(failures))
    run_tests.checkApp = checkApp
    sys.argv = [sys.argv[0], "--app", "organizer"] + sys.argv[1:]
    return run_tests.main()


if __name__ == "__main__":
    sys.exit(main())
