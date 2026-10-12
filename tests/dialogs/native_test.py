#!/usr/bin/env python3
"""Golden comparison: the native ecce-dialog and the wxPython dialog send the
same settings for the same input.

Each case is a restore file (stored GUIValues) plus the editor's argv fields
(category, multiplicity).  Both programs run on a private Xvfb with the real
argv contract, and what each sends over UDP up to #INITIALIZED is compared,
line for line (the pid in #STARTED aside).

    tests/dialogs/native_test.py --dialog build-cmake/bin/ecce-dialog [-v]

Needs python3 + wxPython, Xvfb, and the built ecce-dialog.  Skips (exit 77)
when one is missing.
"""
import argparse
import os
import shutil
import socket
import subprocess
import sys
import tempfile
import time

HERE = os.path.dirname(os.path.abspath(__file__))
CODEREG = os.path.join(os.path.dirname(os.path.dirname(HERE)), "scripts", "codereg")

# (title, argv overrides, restore lines)
FN = "ES.Theory.DFT.XCFunctionals"
UR = "ES.Theory.ECCEQM.Unrestricted"
LW, CH, MK = ("ES.Theory.ECCEQM." + n for n in ("Loewdin", "CHELPG", "MK"))


def menu(name, value, enabled=1):
    return "%s|%s||%d|1|menu_input" % (name, value, enabled)


def tog(name, value, enabled=1):
    return "%s|%d||%d|1|toggle_input" % (name, value, enabled)


CASES = []
for category in ("DFT", "SCF"):
    for mult in (1, 3):
        base = {"category": category, "spinMultiplicity": mult}
        CASES.append(("%s mult %d defaults" % (category, mult), base, []))
        for f in ("SVWN", "PBE", "B3LYP", "PBE0"):
            if category == "DFT":
                CASES.append(("%s mult %d functional %s" % (category, mult, f),
                              base, [menu(FN, f)]))
        CASES.append(("%s mult %d unrestricted" % (category, mult), base,
                      [tog(UR, 1)]))
        CASES.append(("%s mult %d all charges" % (category, mult), base,
                      [tog(LW, 1), tog(CH, 1), tog(MK, 1)]))
        CASES.append(("%s mult %d loewdin+MK, PBE0, unrestricted" % (category, mult),
                      base, [menu(FN, "PBE0"), tog(UR, 1), tog(LW, 1),
                             tog(MK, 1)]))
        CASES.append(("%s mult %d stored disabled flags" % (category, mult),
                      base, [menu(FN, "PBE", 0), tog(UR, 1, 0), tog(CH, 1, 0)]))
CASES.append(("unknown key and a bad choice", {"category": "DFT",
              "spinMultiplicity": 2},
              ["ES.Theory.Nothing|7||1|1|integer_input", menu(FN, "NOPE")]))


def argv(overrides, restore, port):
    v = dict(category="SCF", theory="HF", runType="Energy", spinMultiplicity=1)
    v.update(overrides)
    return [restore, str(port), "GUIValues", "Writable", "DebugOff",
            v["category"], v["theory"], v["runType"], "t", "0", "C1", "10",
            str(v["spinMultiplicity"]), "1", "5", "2", "3"]


def run(cmd, env, cwd, restoreLines, overrides, timeout=40, shot=None):
    work = tempfile.mkdtemp(prefix="ecce-native-")
    try:
        restore = os.path.join(work, "restore.in")
        with open(restore, "w") as f:
            for l in restoreLines:
                f.write(l + "\n")
            f.write("END_GUIValues\n")
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.bind(("127.0.0.1", 0))
        sock.settimeout(0.2)
        proc = subprocess.Popen(cmd + argv(overrides, restore,
                                           sock.getsockname()[1]),
                                env=env, cwd=cwd, stdout=subprocess.DEVNULL,
                                stderr=subprocess.PIPE)
        lines, done, deadline = [], False, time.time() + timeout
        while time.time() < deadline and not done:
            try:
                data = sock.recv(65536).decode()
            except socket.timeout:
                if proc.poll() is not None:
                    break
                continue
            for l in data.splitlines():
                if l.startswith("#STARTED"):
                    l = "#STARTED"
                if l:
                    lines.append(l)
            done = any(l == "#INITIALIZED" for l in lines)
        if done and shot:
            time.sleep(1.5)
            subprocess.run(["import", "-window", "root", shot], env=env,
                           capture_output=True)
        proc.kill()
        err = proc.communicate()[1].decode(errors="replace")
        return lines, err, done
    finally:
        shutil.rmtree(work, ignore_errors=True)


def layout(a, env):
    """The windows must look the same: pixel-compare screenshots."""
    if not (shutil.which("import") and shutil.which("compare")):
        print("skip layout: needs ImageMagick")
        return 0
    py = [sys.executable, os.path.join(CODEREG, "ecceqmtheory.py")]
    native = [os.path.abspath(a.dialog), os.path.join(CODEREG, "ecceqmtheory.dlg")]
    failed = 0
    for title, ov in (("DFT open shell", {"category": "DFT", "spinMultiplicity": 3}),
                      ("SCF closed shell", {"category": "SCF", "spinMultiplicity": 1})):
        work = tempfile.mkdtemp(prefix="ecce-native-shot-")
        try:
            for tag, cmd in (("py", py), ("native", native)):
                run(cmd, env, CODEREG, [], ov, shot=os.path.join(work, tag + ".png"))
            r = subprocess.run(["compare", "-metric", "AE",
                                os.path.join(work, "py.png"),
                                os.path.join(work, "native.png"), "null:"],
                               capture_output=True, text=True)
            ok = r.stderr.strip().split()[0:1] == ["0"]
            failed += not ok
            print("%s  layout %s (%s pixels differ)" % (
                "ok  " if ok else "FAIL", title, r.stderr.strip()))
        finally:
            shutil.rmtree(work, ignore_errors=True)
    return failed

    return 1 if failed else 0


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--dialog", required=True, help="the ecce-dialog binary")
    ap.add_argument("-v", action="store_true")
    ap.add_argument("--case", default="", help="only cases whose title has this")
    a = ap.parse_args()
    if not (shutil.which("Xvfb") and shutil.which("python3")
            and os.path.exists(a.dialog)):
        print("skip: needs Xvfb, python3 and the built ecce-dialog")
        return 77
    try:
        subprocess.run([sys.executable, "-c", "import wx"], check=True,
                       capture_output=True)
    except Exception:
        print("skip: python3 has no wx")
        return 77

    number = 90
    while os.path.exists("/tmp/.X11-unix/X%d" % number):
        number += 1
    xvfb = subprocess.Popen(["Xvfb", ":%d" % number, "-screen", "0",
                             "1280x1024x24"], stdout=subprocess.DEVNULL,
                            stderr=subprocess.DEVNULL)
    env = dict(os.environ, DISPLAY=":%d" % number, GDK_BACKEND="x11",
               PYTHONPATH=CODEREG, PYTHONDONTWRITEBYTECODE="1",
               ECCE_REALUSERHOME=tempfile.gettempdir() + "/none")
    env.pop("WAYLAND_DISPLAY", None)
    failed = 0
    try:
        time.sleep(1.5)
        py = [sys.executable, os.path.join(CODEREG, "ecceqmtheory.py")]
        native = [os.path.abspath(a.dialog),
                  os.path.join(CODEREG, "ecceqmtheory.dlg")]
        for title, overrides, restore in CASES:
            if a.case not in title:
                continue
            pl, perr, pdone = run(py, env, CODEREG, restore, overrides)
            nl, nerr, ndone = run(native, env, CODEREG, restore, overrides)
            ok = pdone and ndone and pl == nl
            if not ok:
                failed += 1
            print("%s  %s" % ("ok  " if ok else "FAIL", title))
            if a.v or not ok:
                for tag, l in (("python", pl), ("native", nl)):
                    print("    " + tag)
                    for x in l:
                        print("      " + x)
                if not ok:
                    print("    python stderr: " + perr.strip()[-300:])
                    print("    native stderr: " + nerr.strip()[-300:])
        failed += layout(a, env)
    finally:
        xvfb.terminate()
        xvfb.wait()
        try:
            os.unlink("/tmp/.X11-unix/X%d" % number)
        except OSError:
            pass
    print("%d checks differ" % failed)
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
