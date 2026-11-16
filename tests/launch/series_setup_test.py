#!/usr/bin/env python3
"""The harness's create driver stores the Calculation Editor's setup (#1034).

An xTB calculation made with theory=SE:GFN2-xTB runtype=Geometry is the
template for ecce-series on two xyz files: two Ready calculations with xTB
decks result, which a template without a stored theory cannot give.  Local
data, no launch.

    tests/launch/series_setup_test.py [--build build-cmake] [--keep]
"""

import argparse
import os
import shutil
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)

import harness  # noqa: E402
from harness import say  # noqa: E402
from xtb_test import deck, WATER, METHANE, calcDir  # noqa: E402

failures = []


def check(ok, what):
    say("  %s %s" % ("ok  " if ok else "FAIL", what))
    if not ok:
        failures.append(what)
    return ok


def xyz(path, atoms):
    with open(path, "w") as h:
        h.write("%d\nseries\n" % len(atoms))
        for a in atoms:
            h.write("%s %.6f %.6f %.6f\n" % a)


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--build", default=os.path.join(REPO, "build-cmake"))
    ap.add_argument("--keep", action="store_true")
    args = ap.parse_args()
    build = os.path.abspath(args.build)
    harness.prerequisites(build, ())

    s = harness.Session(build, "series-setup", {"xTB": shutil.which("xtb") or "/bin/true"},
                        local=True, keep=args.keep)
    work = os.path.join(s.state, "decks")
    rundir = os.path.join(s.state, "run")
    shutil.rmtree(work, ignore_errors=True)
    os.makedirs(work)
    os.makedirs(rundir, exist_ok=True)
    try:
        if not s.services(True):
            check(False, "services came up")
            return 1
        text, rc = deck(WATER, "Geometry")
        dpath = os.path.join(work, "xtb.xtbin")
        open(dpath, "w").write(text)
        txyz = os.path.join(work, "t.xyz")
        xyz(txyz, WATER)
        url, out = s.create("tpl-%d" % int(time.time()), "xtb_es", dpath, "xtb.xtbin",
                            rundir, extra=("procs=1", "code=xTB", "xyz=" + txyz,
                                           "theory=SE:GFN2-xTB", "runtype=Geometry"))
        if not check(url is not None, "template created"):
            say(out)
            return 1
        folder = os.path.dirname(calcDir(url))
        files = []
        for n, atoms in (("w", WATER), ("m", METHANE)):
            f = os.path.join(work, n + ".xyz")
            xyz(f, atoms)
            files.append(f)
        series = os.path.join(folder, "series")
        os.makedirs(series, exist_ok=True)
        shutil.copy(os.path.join(folder, ".ecce-meta"), series) \
            if os.path.exists(os.path.join(folder, ".ecce-meta")) else None
        rc, out = s.run([os.path.join(build, "ecce-series"), calcDir(url), series] + files)
        say(out)
        check(rc == 0, "ecce-series exits 0 (%d)" % rc)
        made = [d for d in os.listdir(series) if os.path.isdir(os.path.join(series, d))] \
            if os.path.isdir(series) else []
        check(len(made) == 2, "two calculations made: %s" % made)
        for d in made:
            inp = os.path.join(series, d, "Inputs")
            decks = os.listdir(inp) if os.path.isdir(inp) else []
            body = open(os.path.join(inp, decks[0])).read() if decks else ""
            check(body.startswith("xtb --gfn 2") and "--opt" in body.splitlines()[0],
                  "%s: xTB GFN2 geometry deck: %r" % (d, body.splitlines()[:1]))
            rc, st = s.driver("state", "file://" + os.path.join(series, d))
            check(st.strip().splitlines()[-1:] == ["ready"], "%s is Ready (%s)" % (d, st.strip()))
    finally:
        s.stop()
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
