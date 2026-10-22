#!/usr/bin/env python3
"""Builder: bond orders, deleting atoms with nubs, the select-mode drag box.

    tests/apps/builder_edit_bugs_test.py [--build DIR] [--keep] [--shots DIR]
Headless on benzene.car through ECCE_BUILDER_SCRIPT, with the events the GUI
sends: the bond kind (covalent or metal-pi; no orders exist) chosen as the
Bond tool, then Bond mode on two picked atoms for a new bond between two
molecules ("Align molecules?" answered Yes, the default).  Deleting two
bonded atoms together with their
bond (as a drag box selects them) must take their nubs along.  A real drag
in Select mode through the X server (xdotool): a drag from empty space draws
a box (screen shot taken mid-drag) and selects the atoms inside it without
moving anything, a drag from a selected atom moves the selection, a drag
from an unselected atom moves nothing.

Exit status 77 (skip) without Xvfb, xdotool or the built Builder.
"""
import argparse
import os
import re
import shutil
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(REPO, "tests", "launch"))

import builder_overlay_test as bo  # noqa: E402
import harness  # noqa: E402
from harness import say  # noqa: E402

STRUCTURE = os.path.join(REPO, "tests", "fragreaders", "data", "benzene.car")
failures = []


def check(ok, what):
    say("%s  %s" % ("ok  " if ok else "FAIL", what))
    if not ok:
        failures.append(what)
    return ok


def drag(env, x0, y0, x1, y1, shot=None):
    subprocess.run(["xdotool", "mousemove", str(x0), str(y0)], env=env)
    time.sleep(0.3)
    subprocess.run(["xdotool", "mousedown", "1"], env=env)
    for i in range(1, 11):
        subprocess.run(["xdotool", "mousemove",
                        str(x0 + (x1 - x0) * i // 10),
                        str(y0 + (y1 - y0) * i // 10)], env=env)
        time.sleep(0.05)
    time.sleep(0.3)
    if shot:
        subprocess.run(["import", "-window", "root", shot], env=env)
    subprocess.run(["xdotool", "mouseup", "1"], env=env)
    time.sleep(1.0)


def runBuilder(s, env, wrappers, name, lines, shots=None, extra=None,
               structure=STRUCTURE):
    """Runs the Builder on benzene; at "shot dragN" drags across the view
    between the last two viewpos points, at "shot picN" saves the screen."""
    work = os.path.join(s.state, name)
    shutil.rmtree(work, ignore_errors=True)
    os.makedirs(work)
    script = os.path.join(work, "script")
    with open(script, "w") as handle:
        handle.write("\n".join(lines) + "\n")
    log = os.path.join(work, "builder.log")
    runEnv = dict(env, ECCE_BUILDER_SCRIPT=script, **(extra or {}))
    proc = subprocess.Popen([os.path.join(wrappers, "ecce-builder")] +
                            ([structure] if structure else []), env=runEnv,
                            cwd=os.path.join(s.home, "bin"),
                            stdout=open(log, "w"), stderr=subprocess.STDOUT,
                            start_new_session=True)
    pending = [l.split()[1] for l in lines if l.startswith("shot ")]
    deadline = time.time() + 300
    try:
        while pending and time.time() < deadline and proc.poll() is None:
            ready = os.path.join(work, pending[0] + ".ready")
            if os.path.exists(ready):
                with open(log, errors="replace") as handle:
                    text = handle.read()
                if pending[0].startswith("drag"):
                    pts = re.findall(r"BUILDER: (?:viewpos|atompos) \S+ at "
                                     r"(-?\d+) (-?\d+)", text)[-2:]
                    if len(pts) == 2:
                        shot = None
                        if pending[0].startswith("dragshot"):
                            shot = os.path.join(shots or work,
                                                pending[0] + ".png")
                        drag(env, *[int(v) for p in pts for v in p],
                             shot=shot)
                elif shots:
                    subprocess.run(["import", "-window", "root",
                                    os.path.join(shots, pending[0] + ".png")],
                                   env=env)
                os.unlink(ready)
                open(os.path.join(work, pending[0] + ".go"), "w").close()
                pending.pop(0)
            time.sleep(0.3)
        while proc.poll() is None and time.time() < deadline:
            with open(log, errors="replace") as handle:
                if "ECCE_BUILDER_SCRIPT: quit" in handle.read():
                    break
            time.sleep(0.5)
        time.sleep(2)
    finally:
        bo.stop(proc)
    with open(log, errors="replace") as handle:
        return handle.read()


def orders(text):
    """Each 'bondorders' dump: {(sym1, sym2, i, j): (order, drawn)}."""
    out, cur = [], None
    for line in text.splitlines():
        m = re.search(r"BUILDER: bond (\d+)-(\d+) (\w+)-(\w+) order=(\S+) "
                      r"drawn=(\d+)", line)
        if m:
            if cur is None:
                cur = {}
                out.append(cur)
            cur[tuple(sorted((m.group(3), m.group(4)))) +
                (int(m.group(1)), int(m.group(2)))] = (float(m.group(5)),
                                                       int(m.group(6)))
        elif cur is not None:
            cur = None
    return out


def bondOf(dump, a, b):
    for key, val in dump.items():
        if key[:2] == tuple(sorted((a, b))):
            return key, val
    return None, None


def orderCase(s, env, wrappers, shots):
    # No bond order is settable; a bond is covalent (display SINGLE_BOND 1)
    # or metal-pi.  Clean stores its own bookkeeping orders, which are still
    # drawn single.
    lines = ["wait 3000", "bondorders",
             "bondkind metal", "bondkind covalent",
             "select -1", "add C Tetrahedral 8 0 0", "select -1",
             "add O Bent 12 0 0",
             "selectelem O", "addelem C", "click",
             "bondorders", "clean", "wait 2000", "bondorders",
             "wait 1000", "shot picorders", "quit"]
    text = runBuilder(s, env, wrappers, "orders", lines, shots,
                      {"ECCE_TEST_DIALOG_CLOSE": "1",
                       "ECCE_TEST_DIALOG_ANSWER": "yes"})
    cur = re.findall(r"BUILDER: bondkind current=(\S+)", text)
    check(cur[:2] == ["0.5", "1"],
          "the Bond tool offers covalent and metal-pi only (%s)" % cur)
    d = orders(text)
    if not check(len(d) == 3, "three bond dumps (%d)" % len(d)):
        say("\n".join("      | " + l for l in text.splitlines()[-30:]))
        return
    _, co = bondOf(d[1], "C", "O")
    check(co is not None and co[0] == 1.0 and co[1] == 1,
          "a new bond between two molecules is covalent single (%s)" % (co,))
    c1c6 = [k for k in d[2] if k[2:] in ((0, 5), (5, 0))]
    check(bool(c1c6) and d[2][c1c6[0]][1] == 1,
          "benzene after Clean: the bond is drawn single (%s)"
          % (d[2].get(c1c6[0]) if c1c6 else None,))
    check(all(v[1] == 1 for v in d[2].values()),
          "every bond is drawn as a single stick")
    check(any(v[0] == 1.5 for v in d[2].values()),
          "Clean kept aromatic bookkeeping orders")


def deleteCase(s, env, wrappers):
    lines = ["wait 3000", "select -1", "add Si Tetrahedral 8 0 0",
             "mode atom", "element Si", "selectnub Si", "click", "info",
             "selectelem Si", "addelem Si", "selectbonds", "selection",
             "key delete", "info", "quit"]
    text = runBuilder(s, env, wrappers, "delete", lines)
    n = [(int(a), int(b)) for a, b in
         re.findall(r"BUILDER: atoms=(\d+) lattice=\w+ nubs=(\d+)", text)]
    sel = re.findall(r"BUILDER: selection atoms=(\d+) bonds=(\d+)", text)
    if not check(len(n) == 2 and sel,
                 "the script ran (%s, %s)" % (n, sel)):
        say("\n".join("      | " + l for l in text.splitlines()[-30:]))
        return
    check(n[0] == (20, 6), "Si2 with six nubs next to benzene (%s)"
          % (n[0],))
    check(sel[0] == ("2", "1"), "both Si and their bond selected (%s)"
          % (sel[0],))
    check(n[1] == (12, 0),
          "deleting them takes their nubs along (atoms, nubs: %s)"
          % (n[1],))


def coordDumps(text):
    """Each 'coords' dump: the list of (x, y, z) per atom."""
    out, cur = [], None
    for line in text.splitlines():
        m = re.search(r"BUILDER: atom (\d+) \w+ (\S+) (\S+) (\S+)$", line)
        if m:
            if cur is None:
                cur = []
                out.append(cur)
            cur.append(tuple(float(m.group(i)) for i in (2, 3, 4)))
        else:
            cur = None
    return out


def selections(text):
    return [int(a) for a in re.findall(r"BUILDER: selection atoms=(\d+)",
                                       text)]


def moved(before, after):
    return [i for i, (a, b) in enumerate(zip(before, after))
            if max(abs(u - v) for u, v in zip(a, b)) > 1e-4]


def dragCases(s, env, wrappers, shots):
    # Empty space: a box, selects the 12 atoms, moves nothing.
    lines = ["wait 3000", "mode select", "coords", "selection",
             "viewpos 0.02 0.02", "viewpos 0.98 0.98", "shot dragshotbox",
             "wait 500", "selection", "coords", "quit"]
    text = runBuilder(s, env, wrappers, "dragbox", lines, shots)
    sel, co = selections(text), coordDumps(text)
    if not check(len(sel) == 2 and len(co) == 2,
                 "the box script ran (%s, %d dumps)" % (sel, len(co))):
        say("\n".join("      | " + l for l in text.splitlines()[-30:]))
    else:
        check(sel[0] == 0 and sel[1] == 12,
              "a drag from empty space selects the atoms in the box (%s)"
              % (sel,))
        check(not moved(co[0], co[1]), "and moves nothing (%s)"
              % (moved(co[0], co[1]),))

    # A selected atom: the selection moves (only that atom is selected).
    lines = ["wait 3000", "mode select", "atomclick left 1", "coords",
             "atompos 1", "viewpos 0.9 0.2", "shot dragmove", "wait 500",
             "selection", "coords", "quit"]
    text = runBuilder(s, env, wrappers, "dragmove", lines, shots)
    sel, co = selections(text), coordDumps(text)
    if not check(len(sel) == 1 and len(co) == 2,
                 "the move script ran (%s, %d dumps)" % (sel, len(co))):
        say("\n".join("      | " + l for l in text.splitlines()[-30:]))
    else:
        check(moved(co[0], co[1]) == [0],
              "a drag from a selected atom moves it, and only it (%s)"
              % (moved(co[0], co[1]),))

    # An unselected atom: nothing moves, the selection is not changed.
    lines = ["wait 3000", "mode select", "atomclick left 1", "coords",
             "atompos 4", "viewpos 0.9 0.2", "shot dragnone", "wait 500",
             "selection", "coords", "quit"]
    text = runBuilder(s, env, wrappers, "dragnone", lines, shots)
    sel, co = selections(text), coordDumps(text)
    if not check(len(sel) == 1 and len(co) == 2,
                 "the unselected-atom script ran (%s, %d dumps)"
                 % (sel, len(co))):
        say("\n".join("      | " + l for l in text.splitlines()[-30:]))
    else:
        check(not moved(co[0], co[1]),
              "a drag from an unselected atom moves nothing (%s)"
              % (moved(co[0], co[1]),))
        check(sel[0] == 1, "and keeps the selection (%s)" % (sel,))


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--build", default=os.path.join(REPO, "build-cmake"))
    ap.add_argument("--keep", action="store_true")
    ap.add_argument("--shots", help="save screen shots here")
    args = ap.parse_args()
    build = os.path.abspath(args.build)
    for tool in ("Xvfb", "xdotool"):
        if not shutil.which(tool):
            harness.skip("%s is not installed" % tool)
    harness.prerequisites(build, ())
    if not os.access(os.path.join(build, "builder"), os.X_OK):
        harness.skip("builder is not built in %s" % build)
    import xdisplay

    s = harness.Session(build, "editbugs", {}, local=True, keep=args.keep)
    for exe in bo.GUI:
        harness.link(os.path.join(build, exe), os.path.join(s.home, "bin", exe))
    wrappers = os.path.join(s.state, "wrappers")
    shutil.rmtree(wrappers, ignore_errors=True)
    shutil.copytree(os.path.join(build, "wrappers"), wrappers)
    for entry in os.listdir(wrappers):
        os.chmod(os.path.join(wrappers, entry), 0o755)
    say("binary: %s" % os.path.realpath(os.path.join(s.home, "bin", "builder")))

    display = None
    try:
        if not s.services(True):
            check(False, "services came up")
            return 1
        xdisplay.SCREEN = "1366x768x24"
        display = xdisplay.Display().__enter__()
        env = s.env({"DISPLAY": display.name, "GTK_THEME": "Adwaita",
                     "PATH": wrappers + os.pathsep + s.env()["PATH"]})
        shots = os.path.abspath(args.shots) if args.shots else None
        if shots:
            os.makedirs(shots, exist_ok=True)
        orderCase(s, env, wrappers, shots)
        deleteCase(s, env, wrappers)
        dragCases(s, env, wrappers, shots)
    finally:
        if display:
            display.__exit__(None, None, None)
        s.stop()
    say("FAILED: " + "; ".join(failures) if failures else "PASSED")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
