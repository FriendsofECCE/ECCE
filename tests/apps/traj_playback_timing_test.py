#!/usr/bin/env python3
"""Per-frame cost of Geometry Trace playback on a large MD system (#570).

    tests/apps/traj_playback_timing_test.py [--build build-cmake] [--frames N]
                                            [--bound MS] [--keep]

The first frames of the NaCl dissolving example's trajectory (12079 atoms,
data/client/examples/genchem) become a calculation whose GEOMTRACE holds
them; the Builder (this tree's binaries) draws the water as wireframe,
focuses the Geometry Trace panel and sweeps every frame twice with
ECCE_FRAME_TIMING set.  Prints the median per-phase times of the second
sweep and fails when the median frame takes longer than --bound ms.

Exit status 77 (skip) without Xvfb or binaries.
"""
import argparse
import os
import shutil
import statistics
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(REPO, "tests", "launch"))
sys.path.insert(0, os.path.join(REPO, "tests", "parsers"))

import builder_overlay_test as bo  # noqa: E402
import energies_groups_test as eg  # noqa: E402
import harness  # noqa: E402
from harness import say  # noqa: E402

check = bo.check
TRAJ = os.path.join(REPO, "data", "client", "examples", "genchem",
                    "gases-liquids-solids", "gromacs-nacl-dissolving",
                    "1-dynamics", "ecce-parse-traj.txt")


def frames(path, count):
    """[[(symbol, x, y, z), ...], ...]: the first `count` frames."""
    out, cur, left = [], None, 0
    with open(path) as f:
        for line in f:
            if line.startswith("ECCE-TRAJ frame"):
                if len(out) == count:
                    break
                left = int(line.split()[-1])
                cur = []
                out.append(cur)
            elif left:
                e, x, y, z = line.split()[:4]
                cur.append((e, float(x), float(y), float(z)))
                left -= 1
    return out


def writeCalc(calc, fr):
    first = fr[0]
    bonds, i = [], 0
    while i < len(first):
        if first[i][0] == "O" and i + 2 < len(first) \
                and first[i + 1][0] == "H" and first[i + 2][0] == "H":
            bonds += [(i + 1, i + 2), (i + 1, i + 3)]
            i += 3
        else:
            i += 1
    lines = ["title: nacl-traj", "type: molecule", "num_atoms: %d" % len(first),
             "atom_info: symbol cart", "atom_list: "]
    lines += ["%s %.5f %.5f %.5f" % a for a in first]
    lines += ["num_bonds: %d" % len(bonds), "bond_list: "]
    lines += ["%d %d 1.00000 2" % b for b in bonds]
    with open(os.path.join(calc, "Parameters", "chemsys.mvm"), "w") as f:
        f.write("\n".join(lines) + "\n")
    props = os.path.join(calc, "Props")
    for name in os.listdir(props):
        if name not in ("GEOMTRACE",):
            os.unlink(os.path.join(props, name))
    with open(os.path.join(props, "GEOMTRACE"), "w") as f:
        f.write('<?xml version="1.0" encoding="UTF-8" standalone="yes" ?>\n'
                '<tsvectable columnLabel="Atom" columnLabels="X Y Z" columns="3" '
                'name="GEOMTRACE" rowLabel="Geometry Step" rows="%d" '
                'units="Angstrom" vectorLabel="Coordinate" vectors="1">\n'
                % len(first))
        for n, frame in enumerate(fr):
            f.write('  <step number="%d">' % (n + 1))
            f.write(" ".join("%.4f %.4f %.4f" % a[1:] for a in frame))
            f.write("</step>\n")
        f.write("</tsvectable>\n")


def parse(path):
    rows = []
    for line in open(path):
        if not line.startswith("frame "):
            continue
        d = {}
        for tok in line.split()[2:]:
            k, v = tok.split("=")
            d[k] = float(v)
        rows.append(d)
    return rows


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--build", default=os.path.join(REPO, "build-cmake"))
    ap.add_argument("--frames", type=int, default=10)
    ap.add_argument("--bound", type=float, default=150.0)
    ap.add_argument("--keep", action="store_true")
    args = ap.parse_args()
    build = os.path.abspath(args.build)
    if not shutil.which("Xvfb"):
        harness.skip("Xvfb is not installed")
    harness.prerequisites(build, ())
    for exe in bo.GUI:
        if not os.access(os.path.join(build, exe), os.X_OK):
            harness.skip("%s is not built in %s" % (exe, build))
    import xdisplay

    work = tempfile.mkdtemp(prefix="trajtiming-")
    calc = os.path.join(work, "calc-traj")
    shutil.copytree(os.path.join(HERE, "fixtures", "calc-water-opt"), calc,
                    symlinks=True)
    writeCalc(calc, frames(TRAJ, args.frames))

    s = harness.Session(build, "trajtiming", {}, local=True, keep=args.keep)
    for exe in bo.GUI:
        harness.link(os.path.join(build, exe), os.path.join(s.home, "bin", exe))
    wrappers = os.path.join(s.state, "wrappers")
    shutil.rmtree(wrappers, ignore_errors=True)
    shutil.copytree(os.path.join(build, "wrappers"), wrappers)
    for entry in os.listdir(wrappers):
        os.chmod(os.path.join(wrappers, entry), 0o755)
    display = None
    timing = os.path.join(s.state, "frame-timing.txt")
    try:
        if not s.services(True):
            check(False, "services came up")
            return 1
        xdisplay.SCREEN = "1500x950x24"
        display = xdisplay.Display().__enter__()
        env = s.env({"DISPLAY": display.name, "GTK_THEME": "Adwaita",
                     "PATH": wrappers + os.pathsep + s.env()["PATH"]})
        url = eg.install(s, "calc-traj", work)
        scene = os.path.join(work, "scene")
        with open(scene, "w") as handle:
            handle.write("hold 20\ngtfocus\ngtsweep\ngtsweep\nhold 1\n")
        out, _shots = bo.runScript(
            s, display, env, wrappers, ["-context", url],
            ["wait 6000", "menu Render|Wireframe", "wait 60000", "quit"],
            os.path.join(s.state, "png"), "trajtiming",
            {"ECCE_PANEL_MODE": "detail", "ECCE_VIEWER_SCENE": scene,
             "ECCE_FRAME_TIMING": timing,
             "ECCE_VIEWER_SCENE_OUT": os.path.join(s.state, "trajtiming-out")})
        check("crash" not in out.lower(), "no crash in the Builder log")
        rows = parse(timing) if os.path.exists(timing) else []
        n = args.frames
        check(len(rows) >= 2 * n, "%d frames timed (want %d)" % (len(rows), 2 * n))
        second = rows[len(rows) // 2:]
        if second:
            keys = []
            for r in second:
                keys += [k for k in r if k not in keys]
            say("median ms over %d frames: %s" % (len(second), " ".join(
                "%s=%.1f" % (k, statistics.median(r.get(k, 0) for r in second))
                for k in keys)))
            med = statistics.median(r["total"] for r in second)
            check(med < args.bound, "median frame %.1f ms < %.0f ms" % (med, args.bound))
    finally:
        if display:
            display.__exit__(None, None, None)
        s.stop()
        if not args.keep:
            shutil.rmtree(work, ignore_errors=True)
    failures = bo.failures
    say("FAILED: " + "; ".join(failures) if failures else "PASSED")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
