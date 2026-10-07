#!/usr/bin/env python3
"""
No control may be cut off (clipping audit).

For every app's main window, every Builder property panel and tool in each
panel layout (with each choice of the radio boxes in a panel), and every
codereg theory/runtype dialog, on a private Xvfb of the given size, every
control is checked for:

  smaller-than-best       smaller than its GetBestSize(): truncated text,
                          cut fields, checkboxes without room for their label
  outside-parent          sticks out of an ancestor's client area
  empty-button            a button with neither label nor bitmap
  blank-bitmap            a button whose bitmap draws nothing
  text-wider-than-field   a text field narrower than the value it holds

The walk is in the apps (ewxWindowUtils::clipFindings, switched on by
ECCE_CLIP_AUDIT=<file>; Builder.runClipAudit for the panels) and in
clipwx.py for the Python dialogs.  Screens are laid out by the same toolkit
a user has, minus the window manager: a docked panel has the width the
layout gives it at this screen size.

    clip_test.py [--size 1920x1080] [--out DIR] [--report-only]
                 [--only builder|apps|dialogs] [--app NAME]

--out keeps a screenshot per window and, for windows with findings, a copy
with the findings boxed in red, plus findings.tsv.  Allowed findings are
listed in clip_allow.txt (a regular expression per line, matched against
"tag<TAB>kind<TAB>description").  Same installed tree and isolation as
run_tests.py.  Exit 77 without an install or Xvfb.
"""

import os
import re
import shutil
import subprocess
import sys
import tempfile
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
sys.path.append(os.path.join(HERE, "..", "dialogs"))
sys.path.append(os.path.join(HERE, "..", "spectrum"))

import apps       # noqa: E402
import cases      # noqa: E402
import fixture    # noqa: E402
import run_tests  # noqa: E402
import xdisplay   # noqa: E402
import smallscreen_test as small  # noqa: E402

opts = {"size": (1920, 1080), "out": None, "only": None}
windows = []      # (tag, rect, class, shot)
findings = []     # (tag, kind, description, detail, rect)


def allowed():
    path = os.path.join(HERE, "clip_allow.txt")
    out = []
    if os.path.exists(path):
        for line in open(path):
            line = line.strip()
            if line and not line.startswith("#"):
                out.append(re.compile(line))
    return out


def readReport(path, seen):
    """Parse new lines of the audit file; returns True once DONE is seen."""
    done = False
    if not os.path.exists(path):
        return done
    lines = open(path, errors="replace").read().splitlines()
    for line in lines[seen[0]:]:
        f = line.split("\t")
        if f[0] == "WINDOW" and len(f) >= 6:
            windows.append((f[1], f[2], f[4], f[5]))
        elif f[0] == "FINDING" and len(f) >= 6:
            findings.append((f[1], f[2], f[3], f[4], f[5]))
        elif f[0] == "DONE":
            done = True
    seen[0] = len(lines)
    return done


def auditEnv(report, tag=None):
    env = {"ECCE_CLIP_AUDIT": report}
    if opts["out"]:
        env["ECCE_CLIP_SHOTS"] = os.path.join(opts["out"], "shots")
    if tag:
        env["ECCE_CLIP_TAG"] = tag
    return env


def authArgs():
    authPath = fixture.authFile(
        os.path.join(fixture.stateHome(), ".ECCE", "auth.pipe"),
        user=fixture.realUser())
    return ("-pipe", authPath)


def builderPhase(display, report, seen, results):
    """Every layout, every panel, in the real Builder with a real calculation."""
    #  makecalc wants the parsers' `cases`, not this directory's.
    ours = sys.modules.pop("cases")
    import makecalc
    sys.modules["cases"] = ours
    case = "g16-h2o-optfreq"
    calc = os.path.join(fixture.stateHome(), "calc-" + case)
    makecalc.make(case, calc)
    restore = fixture.settleUpgradeNotices()
    try:
        url, error = fixture.install(case, source_dir=calc)
        if error:
            results.fail("builder", "could not install the fixture: %s" % error)
            return
        env = auditEnv(report)
        env["ECCE_CLIP_BUILDER"] = "1"
        env["ECCE_PANEL_FULLSCREEN"] = "1"
        state = {"done": False}

        def inspect(d):
            deadline = time.time() + 900
            while time.time() < deadline:
                if readReport(report, seen):
                    state["done"] = True
                    break
                time.sleep(2)
            return True

        result = apps.run(display, "builder",
                          args=authArgs() + ("-context", url),
                          windowTimeout=60, settle=5, env=env, inspect=inspect)
        if result.crashed:
            results.fail("builder", "crashed (%s) during the audit"
                         % result.signalName)
        elif not state["done"]:
            results.fail("builder", "the panel audit did not finish")
    finally:
        restore()


def appsPhase(display, report, seen, results, only):
    names = [n for n in apps.guiBinaries()
             if n not in cases.HELPERS and n != "builder" and
             (not only or n in only)]
    for name in names:
        print("  %-16s" % name, flush=True)
        apps.run(display, name, args=authArgs(), windowTimeout=40, settle=9,
                 env=auditEnv(report))
        readReport(report, seen)


def dialogsPhase(display, report, seen, results):
    root, todo = small.dialogList()
    scratch = tempfile.mkdtemp(prefix="ecce-clip-")
    import socket
    try:
        for code, script, cat, theory, runtype in todo:
            label = "dialog-%s-%s-%s-%s" % (
                code, os.path.splitext(script)[0], cat,
                theory if "runtype" not in script and "rtyp" not in script
                else runtype)
            print("    " + label, flush=True)
            restore = os.path.join(scratch, "restore.in")
            open(restore, "w").write("END_GUIValues\n")
            sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            sock.bind(("127.0.0.1", 0))
            argv = [sys.executable, os.path.join(HERE, "clipwx.py"),
                    os.path.join(root, script), restore,
                    str(sock.getsockname()[1]), "GUIValues", "Writable",
                    "DebugOff", cat, theory, runtype, "clip", "0", "C1", "10",
                    "1", "1", "5", "2", "3"]
            env = display.env()
            env.update(auditEnv(report, label))
            env["PYTHONPATH"] = root
            env["PYTHONDONTWRITEBYTECODE"] = "1"
            proc = subprocess.Popen(argv, env=env, cwd=root,
                                    stdout=subprocess.DEVNULL,
                                    stderr=subprocess.DEVNULL,
                                    start_new_session=True)
            try:
                deadline = time.time() + 25
                while time.time() < deadline and proc.poll() is None:
                    time.sleep(0.5)
                    if readReport(report, seen) or \
                            any(w[0] == label for w in windows):
                        break
            finally:
                apps._terminate(proc)
                sock.close()
    finally:
        shutil.rmtree(scratch, ignore_errors=True)


def annotate():
    """Copy each screenshot that has findings with the controls boxed."""
    out = opts["out"]
    if not out or shutil.which("convert") is None:
        return
    os.makedirs(os.path.join(out, "annotated"), exist_ok=True)
    for tag, rect, cls, shot in windows:
        mine = [f for f in findings if f[0] == tag]
        if not mine or not shot or not os.path.exists(shot):
            continue
        cmd = ["convert", shot, "-fill", "none", "-stroke", "red",
               "-strokewidth", "2"]
        for f in mine:
            try:
                x, y, w, h = [int(v) for v in f[4].split(",")]
            except ValueError:
                continue
            cmd += ["-draw", "rectangle %d,%d %d,%d" % (x, y, x + w, y + h)]
        cmd.append(os.path.join(out, "annotated", os.path.basename(shot)))
        subprocess.run(cmd, stderr=subprocess.DEVNULL)


def sweep(display, results, only):
    report = tempfile.mktemp(prefix="ecce-clip-", suffix=".tsv")
    seen = [0]
    phase = opts["only"]
    if phase in (None, "builder") and not only:
        print("  builder: every layout and panel", flush=True)
        builderPhase(display, report, seen, results)
    if phase in (None, "apps"):
        appsPhase(display, report, seen, results, only)
    if phase in (None, "dialogs") and not only:
        dialogsPhase(display, report, seen, results)
    readReport(report, seen)
    sw, sh = opts["size"]
    allow = allowed()
    bad = []
    seenKeys = set()
    for tag, kind, desc, detail, rect in findings:
        key = (tag, kind, desc)
        if key in seenKeys:
            continue
        seenKeys.add(key)
        text = "%s\t%s\t%s" % (tag, kind, desc)
        if any(a.search(text) for a in allow):
            continue
        bad.append((tag, kind, desc, detail, rect))
    if opts["out"]:
        with open(os.path.join(opts["out"], "findings.tsv"), "w") as f:
            for row in bad:
                f.write("\t".join(row) + "\n")
        shutil.copy(report, os.path.join(opts["out"], "audit.raw"))
        annotate()
    os.remove(report)
    print("\n%dx%d: %d windows audited, %d findings"
          % (sw, sh, len(windows), len(bad)))
    for tag, kind, desc, detail, rect in bad:
        results.fail("%dx%d" % (sw, sh),
                     "%s: %s: %s: %s" % (tag, kind, desc, detail))


def main():
    argv = sys.argv[1:]
    rest = []
    reportOnly = anyVersion = False
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
            os.makedirs(os.path.join(opts["out"], "shots"), exist_ok=True)
        elif a == "--only":
            i += 1
            opts["only"] = argv[i]
        elif a == "--any-version":
            anyVersion = True
        elif a == "--report-only":
            reportOnly = True
        else:
            rest.append(a)
        i += 1
    xdisplay.SCREEN = "%dx%dx24" % opts["size"]
    only = [rest[j + 1] for j in range(len(rest) - 1) if rest[j] == "--app"]

    def checkApp(display, name, results, verbose=False):
        sweep(display, results, only)
    run_tests.checkApp = checkApp
    run_tests.checkCalculation = lambda *a, **k: None
    run_tests.checkStructureFiles = lambda *a, **k: None
    sys.argv = [sys.argv[0], "--app", "organizer"] + (
        ["--any-version"] if anyVersion else [])
    code = run_tests.main()
    return 0 if reportOnly else code


if __name__ == "__main__":
    sys.exit(main())
