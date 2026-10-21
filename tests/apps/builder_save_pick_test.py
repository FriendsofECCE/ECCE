#!/usr/bin/env python3
"""Builder: a real click on an atom still selects it after File > Save (#423).

    tests/apps/builder_save_pick_test.py [--build DIR] [--keep]

A new NWChem calculation is made through the Organizer's test hook; the
Builder draws water in it, saves, and the X server clicks on an atom in
Select mode (xdotool).  The same after a second Save, and after switching
to another calculation and back (the scene graph is kept per calculation).
Save renders the thumbnail with atom labels switched off and back, and once
left them switched on with empty text, after which clicks on atoms selected
nothing.

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

failures = []


def check(ok, what):
    say("%s  %s" % ("ok  " if ok else "FAIL", what))
    if not ok:
        failures.append(what)
    return ok


def newCalc(s, env, wrappers, name):
    work = os.path.join(s.state, "orgcmd")
    os.makedirs(work, exist_ok=True)
    cmd = os.path.join(work, name + ".cmd")
    open(cmd, "w").close()
    org, log = bo.organizer(s, None, env, wrappers, cmd, name)
    try:
        bo.orgCommand(cmd, log, "newproject savepick")
        out = bo.orgCommand(cmd, log, "newcalc savepick %s NWChem" % name)
    finally:
        bo.stop(org)
    check(out.startswith("ok "), "calculation %s is made: %s" % (name, out))
    return out.split()[1] if out.startswith("ok ") else ""


def realClicks(s, env, wrappers, argv, lines):
    """Runs the Builder; at each "shot realN<tag>" clicks atom N through X."""
    work = os.path.join(s.state, "savepick")
    shutil.rmtree(work, ignore_errors=True)
    os.makedirs(work)
    script = os.path.join(work, "script")
    with open(script, "w") as handle:
        handle.write("\n".join(lines) + "\n")
    log = os.path.join(work, "builder.log")
    proc = subprocess.Popen([os.path.join(wrappers, "ecce-builder")] + argv,
                            env=dict(env, ECCE_BUILDER_SCRIPT=script),
                            cwd=os.path.join(s.home, "bin"),
                            stdout=open(log, "w"), stderr=subprocess.STDOUT,
                            start_new_session=True)
    shots = [l.split()[1] for l in lines if l.startswith("shot ")]
    deadline = time.time() + 300
    try:
        while shots and time.time() < deadline and proc.poll() is None:
            ready = os.path.join(work, shots[0] + ".ready")
            if os.path.exists(ready):
                atom = re.match(r"real(\d+)", shots[0]).group(1)
                with open(log, errors="replace") as handle:
                    pos = re.findall(r"BUILDER: atompos %s at (-?\d+) (-?\d+)"
                                     % atom, handle.read())
                if pos:
                    x, y = pos[-1]
                    subprocess.run(["xdotool", "mousemove", x, y], env=env)
                    time.sleep(0.4)
                    subprocess.run(["xdotool", "click", "1"], env=env)
                    time.sleep(1.2)
                os.unlink(ready)
                open(os.path.join(work, shots[0] + ".go"), "w").close()
                shots.pop(0)
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


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--build", default=os.path.join(REPO, "build-cmake"))
    ap.add_argument("--keep", action="store_true")
    args = ap.parse_args()
    build = os.path.abspath(args.build)
    for tool in ("Xvfb", "xdotool"):
        if not shutil.which(tool):
            harness.skip("%s is not installed" % tool)
    harness.prerequisites(build, ())
    for exe in ("builder", "organizer"):
        if not os.access(os.path.join(build, exe), os.X_OK):
            harness.skip("%s is not built in %s" % (exe, build))
    import xdisplay

    s = harness.Session(build, "savepick", {}, local=True, keep=args.keep)
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
        first = newCalc(s, env, wrappers, "water")
        other = newCalc(s, env, wrappers, "other")
        if not (first and other):
            return 1
        # Each real click on atom N is followed by a shift-click on atom 1
        # through the Builder's own event path, which reports the selection:
        # "N 1" when the real click selected N, "1" when it selected nothing.
        lines = ["wait 3000", "add O Bent 0 0 0", "cmd addh", "wait 500",
                 "mode select",
                 "savemenu", "wait 1500", "atompos 2", "shot real2a",
                 "atomclick shift 1",
                 "savemenu", "wait 1500", "atompos 3", "shot real3a",
                 "atomclick shift 1",
                 "context " + other, "wait 1500", "context " + first,
                 "wait 1500", "mode select", "atompos 2", "shot real2b",
                 "atomclick shift 1", "quit"]
        text = realClicks(s, env, wrappers, ["-context", first], lines)
        sel = re.findall(r"BUILDER: selected after shift click on 1:(.*)", text)
        sel = [v.split() for v in sel]
        say("selections: %s" % sel)
        check(len(sel) == 3, "three selections reported (%d)" % len(sel))
        for i, (what, atom) in enumerate((("after Save", "2"),
                                          ("after a second Save", "3"),
                                          ("after switching back", "2"))):
            got = sel[i] if i < len(sel) else []
            check(sorted(got) == sorted([atom, "1"]),
                  "a real click selects atom %s %s (selection %s)"
                  % (atom, what, " ".join(got) or "none"))
        if failures:
            say("\n".join("      | " + l for l in text.splitlines()[-25:]))
    finally:
        if display:
            display.__exit__(None, None, None)
        s.stop()
    say("FAILED: " + "; ".join(failures) if failures else "PASSED")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
