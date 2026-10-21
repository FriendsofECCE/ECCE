#!/usr/bin/env python3
"""Oracle for the native ecce-dialog: it must send what the wxPython dialog
sends and must describe the same widgets.

    tests/dialogs/native_test.py --dialog build-cmake/bin/ecce-dialog
        [--py scripts/codereg/X.py --dlg scripts/codereg/X.dlg] [-v]
        [--case TEXT] [--jobs N] [--no-layout]

Without --py/--dlg the ECCE-QM Theory pilot is checked.  For a pair, the
contexts come from data/client/cap/*.edml (every category/theory/runtype of
the codes that name the dialog, via codes.py) and are crossed with spin
multiplicity, reaction-study flag and nFrozen=-1.  In each one both programs
run on a private Xvfb with the real argv contract, the restore files are
built from the native inventory (defaults, a non-default choice, a stored
disabled flag, a stale unit, an unknown key) in GUIValues and NO_GUIValues
mode, and the UDP lines are compared byte for byte (the pid in #STARTED
aside).  The widget inventory of `ecce-dialog --inventory` is diffed against
_introspect.py's.

Needs python3 + wxPython, Xvfb, and the built ecce-dialog.  Skips (exit 77)
when one is missing.
"""
import argparse
import concurrent.futures
import json
import os
import shutil
import socket
import subprocess
import sys
import tempfile
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
CODEREG = os.path.join(os.path.dirname(os.path.dirname(HERE)), "scripts", "codereg")

import codes      # noqa: E402
import harness    # noqa: E402

FN = "ES.Theory.DFT.XCFunctionals"
UR = "ES.Theory.ECCEQM.Unrestricted"
LW, CH, MK = ("ES.Theory.ECCEQM." + n for n in ("Loewdin", "CHELPG", "MK"))
CANDIDATES = ("opt1\tfile:///proj/opt1\toptimised\n"
              "in1\tfile:///proj/in1\tinput\n")


def menu(name, value, enabled=1):
    return "%s|%s||%d|1|menu_input" % (name, value, enabled)


def tog(name, value, enabled=1):
    return "%s|%d||%d|1|toggle_input" % (name, value, enabled)


# The ECCE-QM pilot's hand-picked cases: (title, argv overrides, restore).
PILOT_CASES = []
for _category in ("DFT", "SCF"):
    for _mult in (1, 3):
        _base = {"category": _category, "spinMultiplicity": _mult}
        PILOT_CASES.append(("pilot %s mult %d defaults" % (_category, _mult),
                            _base, []))
        if _category == "DFT":
            for _f in ("SVWN", "PBE", "B3LYP", "PBE0"):
                PILOT_CASES.append(("pilot %s mult %d functional %s"
                                    % (_category, _mult, _f), _base,
                                    [menu(FN, _f)]))
        PILOT_CASES.append(("pilot %s mult %d unrestricted" % (_category, _mult),
                            _base, [tog(UR, 1)]))
        PILOT_CASES.append(("pilot %s mult %d all charges" % (_category, _mult),
                            _base, [tog(LW, 1), tog(CH, 1), tog(MK, 1)]))
        PILOT_CASES.append(("pilot %s mult %d PBE0, unrestricted, charges"
                            % (_category, _mult), _base,
                            [menu(FN, "PBE0"), tog(UR, 1), tog(LW, 1), tog(MK, 1)]))
        PILOT_CASES.append(("pilot %s mult %d stored disabled flags"
                            % (_category, _mult), _base,
                            [menu(FN, "PBE", 0), tog(UR, 1, 0), tog(CH, 1, 0)]))
PILOT_CASES.append(("pilot unknown key and a bad choice",
                    {"category": "DFT", "spinMultiplicity": 2},
                    ["ES.Theory.Nothing|7||1|1|integer_input", menu(FN, "NOPE")]))


def argv(ov, restore, port, flag):
    kw = dict(ov)
    kw.update(restoreFile=restore, port=port, restoreFlag=flag)
    return harness.buildArgv(**kw)


def run(cmd, env, cwd, restoreLines, ov, flag="GUIValues", timeout=40, shot=None):
    """Run one dialog; return (udp lines, stderr, finished).  A start-up
    failure to reach the shared Xvfb under load is retried, not reported."""
    for attempt in range(3):
        r = run1(cmd, env, cwd, restoreLines, ov, flag, timeout, shot)
        if r[2] or "initialize GTK" not in r[1]:
            break
    return r


def run1(cmd, env, cwd, restoreLines, ov, flag, timeout, shot):
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
        proc = subprocess.Popen(
            cmd + argv(ov, restore, sock.getsockname()[1], flag),
            env=env, cwd=cwd, stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
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
            if flag == "NO_GUIValues":
                done = "#CLOSING" in lines
            else:
                done = "#INITIALIZED" in lines
        if done and shot:
            time.sleep(1.5)
            subprocess.run(["import", "-window", "root", shot], env=env,
                           capture_output=True)
        proc.kill()
        try:
            err = proc.communicate(timeout=10)[1].decode(errors="replace")
        except subprocess.TimeoutExpired:
            err = "(stderr still held open after kill)"
        return lines, err, done
    finally:
        shutil.rmtree(work, ignore_errors=True)


# ---- contexts and restore cases --------------------------------------------

def contexts(py):
    """(category, theory, runtype) the codes using this dialog can open it in."""
    base = os.path.basename(py)
    found = []
    for code in codes.allCodes():
        theoryDlg = base == code.theoryDialog
        if base not in (code.theoryDialog, code.runtypeDialog):
            continue
        for category, theory, runtypes in code.theories:
            rts = (runtypes[:1] if theoryDlg else runtypes) or ["Energy"]
            for rt in rts:
                c = (category, theory, rt)
                if c not in found:
                    found.append(c)
    return found or [("SCF", "RHF", "Energy")]


VARIANTS = (("mult1", dict(spinMultiplicity=1, reactionStudyFlag=0, numFrozenOrbs=1)),
            ("mult3", dict(spinMultiplicity=3, reactionStudyFlag=0, numFrozenOrbs=1)),
            ("rxn nfrozen-1", dict(spinMultiplicity=2, reactionStudyFlag=1,
                                   numFrozenOrbs=-1)))


def nativeInventory(binary, dlg, ov, env):
    a = argv(ov, "/nonexistent", 0, "NO_GUIValues")
    r = subprocess.run([binary, "--inventory", dlg] + a, env=env,
                       capture_output=True, text=True)
    if r.returncode != 0:
        raise RuntimeError("ecce-dialog --inventory failed: " + r.stderr)
    return json.loads(r.stdout)


def line(w, value=None, enabled=None, unit=None):
    return "%s|%s|%s|%d|1|%s" % (
        w["name"], w["value"] if value is None else value,
        w["unit"] if unit is None else unit,
        w["enabled"] if enabled is None else enabled, w["type"])


def flipped(w):
    if w["kind"] in ("check", "radiopair"):
        return "0" if w["value"] == "1" else "1"
    if w["kind"] == "combo" and len(w["choices"]) > 1:
        c = w["choices"]
        return c[-1] if c[-1] != w["value"] else c[0]
    if w["kind"] == "int":
        return str(int(float(w["value"] or 0)) + 1)
    return None


def restoreCases(inv):
    ws = inv["widgets"]
    cases = [("defaults", [])]
    ch = [w for w in ws if flipped(w) is not None]
    if ch:
        cases.append(("one non-default", [line(ch[0], flipped(ch[0]))]))
        cases.append(("all non-default",
                      [line(w, flipped(w)) for w in ch]))
    if ws:
        cases.append(("stored disabled flag", [line(ws[0], enabled=0)]))
        cases.append(("stored disabled, all but first",
                      [line(w, enabled=0) for w in ws[1:]] or [line(ws[0], enabled=0)]))
    un = [w for w in ws if w["unit"] and w["kind"] in ("int", "float")]
    if un:
        cases.append(("stale unit", [line(un[0], value="7", unit="Bogus")]))
    cp = [w for w in ws if w["kind"] == "calcpick"]
    if cp:
        urls = ("file:///proj/opt1", "file:///proj/in1", "file:///proj/gone")
        cases.append(("calcpick same", [line(w, urls[0]) for w in cp]))
        cases.append(("calcpick different",
                      [line(w, urls[i % 2]) for i, w in enumerate(cp)]))
        cases.append(("calcpick first only", [line(cp[0], urls[1])]))
        cases.append(("calcpick unknown url", [line(cp[0], urls[2])]))
    cases.append(("unknown key", ["ES.Nothing|7||1|1|integer_input"]))
    return cases


# ---- inventory diff --------------------------------------------------------

KIND = {"EcceCheckBox": "check", "EcceComboBox": "combo", "EcceSpinCtrl": "int",
        "EcceFloatInput": "float", "EcceExpInput": "float",
        "EcceTextInput": "text"}


def same(a, b):
    try:
        return float(a) == float(b)
    except (TypeError, ValueError):
        return str(a) == str(b)


def inventoryDiff(py, native):
    """Differences between _introspect.py's inventory and --inventory's."""
    out = []
    nat = {w["name"]: w for w in native["widgets"]}
    seen = set()
    for rec in py["widgets"]:
        live = rec.get("live") or {}
        name = live.get("name")
        if not name or rec["widget"] not in KIND:
            continue
        seen.add(name)
        w = nat.get(name)
        if w is None:
            out.append("%s: only in the Python dialog" % name)
            continue
        if KIND[rec["widget"]] != w["kind"] and not (
                KIND[rec["widget"]] == "combo" and w["kind"] == "calcpick"):
            out.append("%s: kind %s vs %s" % (name, KIND[rec["widget"]], w["kind"]))
        kind = w["kind"]
        if kind in ("combo", "calcpick"):
            if live.get("choices") != w["choices"]:
                out.append("%s: choices %r vs %r" % (name, live.get("choices"),
                                                    w["choices"]))
            d = live.get("default")
            ch = live.get("choices") or []
            pyDef = ch[d] if isinstance(d, int) and 0 <= d < len(ch) else ""
            if kind == "calcpick":
                pyDef = ""
            if pyDef != w["default"]:
                out.append("%s: default %r vs %r" % (name, pyDef, w["default"]))
        elif kind == "check":
            if str(int(bool(live.get("default")))) != w["default"]:
                out.append("%s: default %r vs %r" % (name, live.get("default"),
                                                    w["default"]))
        elif kind in ("int", "float"):
            if not same(live.get("default"), w["default"]):
                out.append("%s: default %r vs %r" % (name, live.get("default"),
                                                    w["default"]))
            if (live.get("unit") or "") != w["unit"]:
                out.append("%s: unit %r vs %r" % (name, live.get("unit"), w["unit"]))
            for key in ("softRange", "hardRange"):
                if (live.get(key) or "") != w[key]:
                    out.append("%s: %s %r vs %r" % (name, key, live.get(key), w[key]))
        if int(live.get("export") or 0) != w["export"]:
            out.append("%s: export %r vs %r" % (name, live.get("export"), w["export"]))
        # Sizer-based widgets (combo, text) have no IsEnabled: None = unknown;
        # their enabled bit is checked through the lines they send.
        if live.get("enabled") is not None and \
                bool(live["enabled"]) != w["enabled"]:
            out.append("%s: enabled %r vs %r" % (name, live.get("enabled"),
                                                 w["enabled"]))
    for name, w in nat.items():
        if name not in seen and w["kind"] not in ("radiopair",):
            out.append("%s: only in the native dialog" % name)
    if native.get("warnings"):
        out.append("native warnings: %s" % native["warnings"])
    return out


# ---- layout (pilot only) ---------------------------------------------------

def layout(a, env, pyCmd, nativeCmd):
    """The windows must look the same: pixel-compare screenshots."""
    if not (shutil.which("import") and shutil.which("compare")):
        print("skip layout: needs ImageMagick")
        return 0
    failed = 0
    for title, ov in (("DFT open shell", {"category": "DFT", "spinMultiplicity": 3}),
                      ("SCF closed shell", {"category": "SCF", "spinMultiplicity": 1})):
        work = tempfile.mkdtemp(prefix="ecce-native-shot-")
        try:
            for tag, cmd in (("py", pyCmd), ("native", nativeCmd)):
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


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--dialog", required=True, help="the ecce-dialog binary")
    ap.add_argument("--py", default=os.path.join(CODEREG, "ecceqmtheory.py"))
    ap.add_argument("--dlg", default="")
    ap.add_argument("-v", action="store_true")
    ap.add_argument("--case", default="", help="only cases whose title has this")
    ap.add_argument("--jobs", type=int, default=4)
    ap.add_argument("--no-layout", action="store_true")
    a = ap.parse_args()
    # Never read the developer's ~/.ECCE (fonts) -- the dialogs honour this.
    os.environ.setdefault("ECCE_REALUSERHOME",
                          os.path.join(tempfile.gettempdir(), "none"))
    py = os.path.abspath(a.py)
    dlg = os.path.abspath(a.dlg or os.path.splitext(py)[0] + ".dlg")
    binary = os.path.abspath(a.dialog)
    if not (shutil.which("Xvfb") and shutil.which("python3")
            and os.path.exists(binary)):
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
    work = tempfile.mkdtemp(prefix="ecce-native-cand-")
    cand = os.path.join(work, "candidates.tsv")
    with open(cand, "w") as f:
        f.write(CANDIDATES)
    env = dict(os.environ, DISPLAY=":%d" % number, GDK_BACKEND="x11",
               PYTHONPATH=CODEREG, PYTHONDONTWRITEBYTECODE="1",
               ECCE_QST_CANDIDATES=cand,
               ECCE_REALUSERHOME=tempfile.gettempdir() + "/none")
    env.pop("WAYLAND_DISPLAY", None)
    os.environ["ECCE_QST_CANDIDATES"] = cand   # for the Python inventory run
    pyCmd = [sys.executable, py]
    nativeCmd = [binary, dlg]
    failed = 0

    def check(title, ov, restore, flag):
        pl, perr, pdone = run(pyCmd, env, CODEREG, restore, ov, flag)
        nl, nerr, ndone = run(nativeCmd, env, CODEREG, restore, ov, flag)
        ok = pdone and ndone and pl == nl
        return title, ok, pl, nl, perr, nerr

    try:
        time.sleep(1.5)
        jobs = []
        pilot = os.path.basename(py) == "ecceqmtheory.py"
        if pilot:
            for title, ov, restore in PILOT_CASES:
                jobs.append((title, ov, restore, "GUIValues"))
        ctxs = contexts(py)
        print("%d context(s): %s" % (len(ctxs), ", ".join("/".join(c) for c in ctxs)))
        diffs = []
        with harness.Display() as display:
            for category, theory, runtype in ctxs:
                for vname, vov in VARIANTS:
                    ov = dict(vov, category=category, theory=theory,
                              runType=runtype)
                    tag = "%s/%s/%s %s" % (category, theory, runtype, vname)
                    nat = nativeInventory(binary, dlg, ov, env)
                    if vname == "mult1":
                        pyInv = harness.runDialog(display, py, **ov)
                        for d in inventoryDiff(pyInv, nat):
                            diffs.append("%s: %s" % (tag, d))
                    for cname, restore in restoreCases(nat):
                        if vname == "rxn nfrozen-1" and cname not in (
                                "defaults", "one non-default"):
                            continue
                        for flag in ("GUIValues", "NO_GUIValues"):
                            if flag == "NO_GUIValues" and vname != "mult3":
                                continue
                            jobs.append(("%s %s %s" % (tag, cname, flag), ov,
                                         restore, flag))
        for d in diffs:
            if a.case in d:
                print("FAIL  inventory " + d)
                failed += 1
        print("%s  inventory diff (%d contexts)" % (
            "ok  " if not diffs else "FAIL", len(ctxs) * len(VARIANTS)))
        jobs = [j for j in jobs if a.case in j[0]]
        with concurrent.futures.ThreadPoolExecutor(a.jobs) as pool:
            for title, ok, pl, nl, perr, nerr in pool.map(
                    lambda j: check(*j), jobs):
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
        if pilot and not a.no_layout:
            failed += layout(a, env, pyCmd, nativeCmd)
    finally:
        xvfb.terminate()
        xvfb.wait()
        shutil.rmtree(work, ignore_errors=True)
        try:
            os.unlink("/tmp/.X11-unix/X%d" % number)
        except OSError:
            pass
    print("%d checks differ" % failed)
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
