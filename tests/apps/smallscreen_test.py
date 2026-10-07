#!/usr/bin/env python3
"""
Every window must fit a small screen (#189).

FastX shows X in a browser tab and laptops have short panels, so the screen
ECCE sees is often 1024x600 or less.  A window taller than that has its
bottom button row off-screen with no way to reach it.

On a private Xvfb of the given size this opens each installed GUI app's main
window and the Theory/Runtype Details dialog of every code (scripts/codereg,
run as the apps run them), measures every top-level window, and fails for any
whose frame extends past the screen.  There is no window manager, so what is
measured is the size the toolkit asked for: a window that fits here still has
room for a title bar only if the app capped itself with room to spare, which
is why the shared cap leaves a margin.

    smallscreen_test.py [--size 1024x600] [--out DIR] [--report-only]
                        [--app NAME] [--dialogs-only|--apps-only]

--out keeps a screenshot of the whole screen per window.  --report-only
prints the offenders and exits 0, for measuring before a fix.

Same installed tree, isolation and services as run_tests.py (ECCE_TEST_HOME,
ECCE_TEST_WRAPPERS).  Exit 77 (skip) without an install or Xvfb.
"""

import os
import re
import shutil
import socket
import subprocess
import sys
import tempfile
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
sys.path.append(os.path.join(HERE, "..", "dialogs"))

import apps       # noqa: E402
import cases      # noqa: E402
import fixture    # noqa: E402
import run_tests  # noqa: E402
import xdisplay   # noqa: E402

# Codes whose dialogs get no new work (CLAUDE.md memory: retired codes).
RETIRED = {"Gaussian-03", "Gaussian-98", "GAMESS-UK", "Amica"}
# Every (category, theory) of these; the others get the first of each
# category, since their dialogs differ little between theories.
EXHAUSTIVE = {"NWChem", "ORCA"}

CHILD = re.compile(
    r'^\s+(0x[0-9a-f]+) (?:"(.*)"|\(has no name\)): \(.*?\)\s+'
    r'(\d+)x(\d+)\+(-?\d+)\+(-?\d+)\s+\+(-?\d+)\+(-?\d+)')

#  A window manager adds a title bar (and a panel takes room) that Xvfb does
#  not have; this is what a window must leave free, so a pass here is a pass
#  on a desktop.
TITLE_ALLOWANCE = 36

offenders = []
checked = []
opts = {"out": None, "size": (1024, 600)}


def topLevels(display):
    """[(title, width, height, x, y)] of the named top-level windows."""
    try:
        text = subprocess.run(
            ["xwininfo", "-display", display.name, "-root", "-children"],
            stdout=subprocess.PIPE, stderr=subprocess.DEVNULL,
            timeout=20).stdout.decode("utf-8", "replace")
    except subprocess.TimeoutExpired:
        return []
    found = []
    for line in text.splitlines():
        m = CHILD.match(line)
        if not m or m.group(2) is None:
            continue
        w, h = int(m.group(3)), int(m.group(4))
        if w > 20 and h > 20:
            found.append((m.group(2), w, h, int(m.group(7)),
                          int(m.group(8))))
    return found


def shoot(display, label):
    out = opts["out"]
    if not out or shutil.which("import") is None:
        return
    safe = re.sub(r"[^A-Za-z0-9._-]+", "_", label)
    subprocess.run(["import", "-display", display.name, "-window", "root",
                    os.path.join(out, safe + ".png")],
                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
                   timeout=60)


def measure(display, label, before):
    """Record every window opened since `before` that leaves the screen."""
    sw, sh = opts["size"]
    wins = [w for w in topLevels(display) if w not in before]
    checked.append(label)
    shoot(display, label)
    if not wins:
        print("      %s: no window to measure" % label)
        return
    for title, w, h, x, y in wins:
        right, bottom = x + w, y + h + TITLE_ALLOWANCE
        if x < 0 or y < 0 or right > sw or bottom > sh:
            offenders.append(
                "%s: \"%s\" is %dx%d at +%d+%d, so with a title bar it ends at "
                "%dx%d on a %dx%d screen (%s)"
                % (label, title, w, h, x, y, right, bottom, sw, sh,
                   ("bottom edge %dpx off-screen" % (bottom - sh)
                    if bottom > sh else "") +
                   (" right edge %dpx off-screen" % (right - sw)
                    if right > sw else "")))


def appCheck(display, name):
    before = set(topLevels(display))

    def inspect(d):
        measure(d, "app-" + name, before)
        return True

    #  Without credentials every app stops at the login dialog.
    authPath = fixture.authFile(
        os.path.join(fixture.stateHome(), ".ECCE", "auth.pipe"),
        user=fixture.realUser())
    result = apps.run(display, name, args=("-pipe", authPath),
                      windowTimeout=40, settle=6, inspect=inspect)
    if result.crashed:
        print("      %s crashed (%s); the apps test reports that"
              % (name, result.signalName))


def dialogList():
    import codes
    root = os.path.join(apps.INSTALL, "scripts", "codereg")
    todo = []
    for code in codes.checkableCodes():
        if code.name in RETIRED:
            continue
        cats = []
        for cat, theory, runtypes in code.theories:
            if code.name not in EXHAUSTIVE and cat in cats:
                continue
            cats.append(cat)
            todo.append((code.name, code.theoryDialog, cat, theory,
                         runtypes[0] if runtypes else "Energy"))
            if code.runtypeDialog and runtypes:
                todo.append((code.name, code.runtypeDialog, cat, theory,
                             runtypes[0]))
    seen = set()
    unique = []
    for item in todo:
        key = item[1:] if "runtype" in item[1] or "rtyp" in item[1] else item[1:4]
        if key in seen:
            continue
        seen.add(key)
        unique.append(item)
    return root, unique


def dialogCheck(display, root, item, scratch):
    code, script, cat, theory, runtype = item
    label = "dialog-%s-%s-%s-%s" % (code, os.path.splitext(script)[0], cat,
                                    theory if "runtype" not in script
                                    and "rtyp" not in script else runtype)
    restore = os.path.join(scratch, "restore.in")
    open(restore, "w").write("END_GUIValues\n")
    #  The dialog sends every setting to this port; nothing may refuse it.
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind(("127.0.0.1", 0))
    argv = [sys.executable, os.path.join(root, script), restore,
            str(sock.getsockname()[1]), "GUIValues",
            "Writable", "DebugOff", cat, theory, runtype, "smallscreen",
            "0", "C1", "10", "1", "1", "5", "2", "3"]
    env = display.env()
    env["PYTHONPATH"] = root
    env["PYTHONDONTWRITEBYTECODE"] = "1"
    before = set(topLevels(display))
    proc = subprocess.Popen(argv, env=env, cwd=root, stdout=subprocess.DEVNULL,
                            stderr=subprocess.DEVNULL, start_new_session=True)
    try:
        deadline = time.time() + 30
        while time.time() < deadline and proc.poll() is None:
            if [w for w in topLevels(display) if w not in before]:
                break
            time.sleep(0.3)
        time.sleep(1.5)
        measure(display, label, before)
    finally:
        apps._terminate(proc)
        sock.close()


def sweep(display, results, only=None):
    names = [n for n in apps.guiBinaries()
             if n not in cases.HELPERS and (not only or n in only)]
    if not opts["dialogs_only"]:
        for name in names:
            print("  %-16s" % name, flush=True)
            appCheck(display, name)
            results.checks += 1
    if not opts["apps_only"] and not only:
        root, todo = dialogList()
        scratch = tempfile.mkdtemp(prefix="ecce-smallscreen-")
        try:
            for item in todo:
                dialogCheck(display, root, item, scratch)
                results.checks += 1
        finally:
            shutil.rmtree(scratch, ignore_errors=True)
    sw, sh = opts["size"]
    print("\n%dx%d: %d windows checked, %d extend past the screen"
          % (sw, sh, len(checked), len(offenders)))
    for line in offenders:
        results.fail("%dx%d" % (sw, sh), line)


def main():
    argv = sys.argv[1:]
    rest = []
    opts["dialogs_only"] = opts["apps_only"] = False
    reportOnly = False
    i = 0
    while i < len(argv):
        a = argv[i]
        if a == "--size":
            i += 1
            w, h = argv[i].lower().split("x")
            opts["size"] = (int(w), int(h))
        elif a == "--out":
            i += 1
            opts["out"] = os.path.abspath(argv[i])
            os.makedirs(opts["out"], exist_ok=True)
        elif a == "--report-only":
            reportOnly = True
        elif a == "--dialogs-only":
            opts["dialogs_only"] = True
        elif a == "--apps-only":
            opts["apps_only"] = True
        else:
            rest.append(a)
        i += 1
    xdisplay.SCREEN = "%dx%dx24" % opts["size"]
    only = [rest[j + 1] for j in range(len(rest) - 1) if rest[j] == "--app"]

    def checkApp(display, name, results, verbose=False):
        sweep(display, results, only)
    run_tests.checkApp = checkApp
    #  run_tests.main() runs one check per selected app; one is enough.
    sys.argv = [sys.argv[0], "--app", "organizer"]
    code = run_tests.main()
    return 0 if reportOnly else code


if __name__ == "__main__":
    sys.exit(main())
